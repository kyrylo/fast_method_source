#include "fast_method_source.h"

static VALUE rb_eSourceNotFoundError;

static unsigned
read_lines_after(const unsigned method_location,
                 const char *filename, char **filebuf[])
{
    read_order order = {1, 0};
    return read_lines(order, method_location, filename, filebuf);
}

static unsigned
read_lines_before(const unsigned method_location,
                  const char *filename, char **filebuf[])
{
    read_order order = {0, 1};
    return read_lines(order, method_location, filename, filebuf);
}

static unsigned
read_lines(read_order order, const unsigned method_location,
           const char *filename, char **filebuf[])
{
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        rb_raise(rb_eIOError, "No such file or directory - %s", filename);
    }

    ssize_t cl_len;

    char *current_line = NULL;
    char **current_linebuf = NULL;
    size_t current_linebuf_size = 0;
    unsigned rl_n = 0;
    unsigned line_count = 0;

    while ((cl_len = getline(&current_line, &current_linebuf_size, fp)) != -1) {
        line_count++;
        if (order.forward) {
            if (line_count < method_location) {
                continue;
            }
        } else if (order.backward) {
            if (line_count > method_location) {
                break;
            }
        }

        if ((rl_n != 0) && (rl_n % (MAXLINES-1) == 0)) {
            reallocate_filebuf(filebuf, rl_n);
        }

        current_linebuf = &(*filebuf)[rl_n];

        if (cl_len >= MAXLINELEN) {
            reallocate_linebuf(current_linebuf, cl_len);
        }

        strncpy(*current_linebuf, current_line, cl_len);
        (*current_linebuf)[cl_len] = '\0';
        rl_n++;
    }

    free(current_line);
    fclose(fp);

    return rl_n;
}

static void
reallocate_linebuf(char **linebuf, const unsigned cl_len)
{
    char *tmp_line;

    if ((tmp_line = realloc(*linebuf, cl_len + 1)) == NULL) {
        rb_raise(rb_eNoMemError, "failed to allocate memory");
    }

    *linebuf = tmp_line;
}

static void
reallocate_filebuf(char **lines[], unsigned rl_len)
{
    unsigned new_size = rl_len + MAXLINES + 1;
    char **temp_lines = realloc(*lines, sizeof(*temp_lines) * new_size);

    if (temp_lines == NULL) {
        rb_raise(rb_eNoMemError, "failed to allocate memory");
    } else {
        *lines = temp_lines;

        for (int i = 0; i < MAXLINES; i++) {
            if (((*lines)[rl_len + i] = malloc(sizeof(char) * MAXLINELEN)) == NULL) {
                rb_raise(rb_eNoMemError, "failed to allocate memory");
            }
        }
    }
}

static NODE *
parse_with_silenced_stderr(VALUE rb_str)
{
    int old_stderr;
    FILE *null_fd;

    old_stderr = DUP(STDERR_FILENO);
    fflush(stderr);
    null_fd = fopen(null_filename, "w");
    DUP2(fileno(null_fd), STDERR_FILENO);

    volatile VALUE vparser = rb_parser_new();
    NODE *node = rb_parser_compile_string(vparser, "-", rb_str, 1);
    rb_str_free(rb_str);

    fflush(stderr);
    fclose(null_fd);

    DUP2(old_stderr, STDERR_FILENO);
    close(old_stderr);

    return node;
}

static NODE *
parse_expr(VALUE rb_str) {
    return parse_with_silenced_stderr(rb_str);
}

static void
filter_interp(char *line)
{
    int brackets = 0;

    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '#') {
            if (line[i + 1] == '{') {
                brackets++;

                line[i] = SAFE_CHAR;

                if (line[i - 1] == '\\')
                    line[i - 1] = SAFE_CHAR;
            }
        }

        if (line[i] == '}') {
            brackets--;

            if (brackets == 0) {
                line[i] = SAFE_CHAR;
            }
        } else if (brackets > 0) {
            line[i] = SAFE_CHAR;
        }
    }
}

static int
contains_end_kw(const char *line)
{
    char *match;
    char prev_ch;

    if ((match = strstr(line, "end")) != NULL) {
        prev_ch = (match - 1)[0];
        return prev_ch == ' ' || prev_ch == '\0' || prev_ch == ';';
    } else {
        return 0;
    }
}

static int
is_accessor(const char *line)
{
    return strstr(line, "attr_reader") != NULL ||
        strstr(line, "attr_writer") != NULL ||
        strstr(line, "attr_accessor") != NULL;
}

static int
is_comment(const char *line, const size_t line_len)
{
    for (size_t i = 0; i < line_len; i++) {
        if (line[i] == ' ')
            continue;

        if (line[i] == '#' && line[i + 1] != '{') {
            for (size_t j = i - 1; j != 0; j--) {
                if (line[j] != ' ')
                    return 0;
            }

            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}

static int
is_static_definition(const char *line)
{
    return strstr(line, " def ") != NULL || strncmp(line, "def ", 4);
}

static int
is_dangling_literal_end(const char *line)
{
    return strstr(line, "}") != NULL && strstr(line, "{") == NULL;
}

static int
is_dangling_literal_begin(const char *line)
{
    return strstr(line, "%{") != NULL && strstr(line, "}") == NULL;
}

static VALUE
find_source(char **filebuf[], const unsigned relevant_lines_n)
{
    VALUE rb_expr;

    const unsigned expr_size = relevant_lines_n * MAXLINELEN;
    char *expr = malloc(expr_size);
    expr[0] = '\0';
    char *parseable_expr = malloc(expr_size);
    parseable_expr[0] = '\0';

    int l = 0;
    while ((*filebuf)[l][0] == '\n') {
        l++;
        continue;
    }
    char *first_line = (*filebuf)[l];

    char *current_line = NULL;
    int should_parse = 0;
    int dangling_literal = 0;
    size_t current_line_len;

    if (is_static_definition(first_line) || is_accessor(first_line)) {
        should_parse = 1;
    }

    for (unsigned i = l; i < relevant_lines_n; i++) {
        current_line = (*filebuf)[i];
        current_line_len = strlen(current_line);

        strncat(expr, current_line, current_line_len);

        if (is_comment(current_line, current_line_len))
            continue;

        if (is_dangling_literal_end(current_line)) {
            dangling_literal = 0;
        } else if (is_dangling_literal_begin(current_line)) {
            dangling_literal = 1;
        } else if (dangling_literal) {
            continue;
        }

        filter_interp(current_line);
        strncat(parseable_expr, current_line, current_line_len);

        if (should_parse || contains_end_kw(current_line)) {
            if (parse_expr(rb_str_new2(parseable_expr)) != NULL) {
                rb_expr = rb_str_new2(expr);
                free(expr);
                free(parseable_expr);
                return rb_expr;
            }
        }
    }

    free(expr);
    free(parseable_expr);
    free_memory_for_file(filebuf, relevant_lines_n);

    return Qnil;
}

static void
strnprep(char *s, const char *t, size_t len)
{
    size_t i;

    memmove(s + len, s, strlen(s) + 1);

    for (i = 0; i < len; ++i) {
        s[i] = t[i];
    }
}

static VALUE
find_comment(char **filebuf[], const unsigned method_location,
             const unsigned relevant_lines_n)
{
    char comment[COMMENT_SIZE];
    comment[0] = '\0';

    int i = method_location - 2;

    while ((*filebuf)[i][0] == '\n') {
        i--;
        continue;
    }

    if (!is_comment((*filebuf)[i], strlen((*filebuf)[i]))) {
        return rb_str_new("", 0);
    } else {
        while (is_comment((*filebuf)[i],  strlen((*filebuf)[i]))) {
            strnprep(comment, (*filebuf)[i],  strlen((*filebuf)[i]));
            i--;
        }

        return rb_str_new2(comment);
    }

    free_memory_for_file(filebuf, relevant_lines_n);
    return Qnil;
}

static char **
allocate_memory_for_file(void)
{
    char **file;

    if ((file = malloc(sizeof(*file) * MAXLINES)) == NULL) {
        rb_raise(rb_eNoMemError, "failed to allocate memory");
    }

    for (int i = 0; i < MAXLINES; i++) {
        if ((file[i] = malloc(sizeof(char) * MAXLINELEN)) == NULL) {
            rb_raise(rb_eNoMemError, "failed to allocate memory");
        };
    }

    return file;
}

static void
free_memory_for_file(char **file[], const unsigned relevant_lines_n)
{
    int lines_to_free = relevant_lines_n >= MAXLINES ? relevant_lines_n : MAXLINES;

    for (int i = 0; i < lines_to_free; i++) {
        free((*file)[i]);
    }

    free(*file);
}

static VALUE
mMethodExtensions_source(VALUE self)
{
    VALUE source_location = rb_funcall(self, rb_intern("source_location"), 0);
    VALUE name = rb_funcall(self, rb_intern("name"), 0);

    if (NIL_P(source_location)) {
        rb_raise(rb_eSourceNotFoundError, "could not locate source for %s!",
                 RSTRING_PTR(rb_sym2str(name)));
    }

    const char *filename = RSTRING_PTR(RARRAY_AREF(source_location, 0));
    const unsigned method_location = FIX2INT(RARRAY_AREF(source_location, 1));

    char **filebuf = allocate_memory_for_file();
    const unsigned relevant_lines_n = read_lines_after(method_location,
                                                       filename, &filebuf);
    VALUE source = find_source(&filebuf, relevant_lines_n);

    if (NIL_P(source)) {
        rb_raise(rb_eSourceNotFoundError, "could not locate source for %s!",
                 RSTRING_PTR(rb_sym2str(name)));
    }
    free_memory_for_file(&filebuf, relevant_lines_n);

    return source;
}

static VALUE
mMethodExtensions_comment(VALUE self)
{
    VALUE source_location = rb_funcall(self, rb_intern("source_location"), 0);
    VALUE name = rb_funcall(self, rb_intern("name"), 0);

    if (NIL_P(source_location)) {
        rb_raise(rb_eSourceNotFoundError, "could not locate comment for %s!",
                 RSTRING_PTR(rb_sym2str(name)));
    }

    const char *filename = RSTRING_PTR(RARRAY_AREF(source_location, 0));
    const unsigned method_location = FIX2INT(RARRAY_AREF(source_location, 1));

    char **filebuf = allocate_memory_for_file();
    const unsigned relevant_lines_n = read_lines_before(method_location,
                                                        filename, &filebuf);
    VALUE comment = find_comment(&filebuf, method_location, relevant_lines_n);

    if (NIL_P(comment)) {
        rb_raise(rb_eSourceNotFoundError, "could not locate comment for %s!",
                 RSTRING_PTR(rb_sym2str(name)));
    }
    free_memory_for_file(&filebuf, relevant_lines_n);

    return comment;
}


void Init_fast_method_source(void)
{
    VALUE rb_mFastMethodSource = rb_define_module_under(rb_cObject, "FastMethodSource");

    rb_eSourceNotFoundError = rb_define_class_under(rb_mFastMethodSource,"SourceNotFoundError", rb_eStandardError);
    VALUE rb_mMethodExtensions = rb_define_module_under(rb_mFastMethodSource, "MethodExtensions");

    rb_define_method(rb_mMethodExtensions, "source", mMethodExtensions_source, 0);
    rb_define_method(rb_mMethodExtensions, "comment", mMethodExtensions_comment, 0);
}
