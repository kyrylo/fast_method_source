#include "fast_method_source.h"

static VALUE rb_eSourceNotFoundError;

static int
read_lines(const char *filename, char **file[], const int start_line)
{
    FILE *fp;
    ssize_t read;
    char *line = NULL;
    size_t len = 0;
    size_t line_len = 0;
    int line_count = 1;
    int occupied_lines = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        rb_raise(rb_eIOError, "No such file or directory - %s", filename);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if (line_count < start_line) {
            line_count++;
            continue;
        }

        if ((occupied_lines != 0) && (occupied_lines % (MAXLINES-1) == 0)) {
            reallocate_lines(file, occupied_lines);
        }

        line_len = strlen(line);

        if (line_len >= MAXLINELEN) {
            char *tmp;

            if ((tmp = realloc((*file)[occupied_lines], line_len + 1)) == NULL) {
                rb_raise(rb_eNoMemError, "failed to allocate memory");
            }

            (*file)[occupied_lines] = tmp;
        }

        strncpy((*file)[occupied_lines], line, read);
        (*file)[occupied_lines][read] = '\0';
        occupied_lines++;
    }

    free(line);
    fclose(fp);

    return occupied_lines;
}

static void
reallocate_lines(char **lines[], int occupied_lines)
{
    int new_size = occupied_lines + MAXLINES + 1;
    char **temp_lines = realloc(*lines, sizeof(*temp_lines) * new_size);

    if (temp_lines == NULL) {
        rb_raise(rb_eNoMemError, "failed to allocate memory");
    } else {
        *lines = temp_lines;

        for (int i = 0; i < MAXLINES; i++) {
            if (((*lines)[occupied_lines + i] = malloc(sizeof(char) * MAXLINELEN)) == NULL) {
                rb_raise(rb_eNoMemError, "failed to allocate memory");
            }
        }
    }
}

static NODE *
with_silenced_stderr(VALUE rb_str)
{
    int old_stderr;
    FILE *null_fd;

    old_stderr = DUP(STDERR_FILENO);
    fflush(stderr);
    null_fd = fopen(null_filename, "w");
    DUP2(fileno(null_fd), STDERR_FILENO);

    volatile VALUE vparser = rb_parser_new();
    NODE *node = rb_parser_compile_string(vparser, "-", rb_str, 1);

    fflush(stderr);
    fclose(null_fd);

    DUP2(old_stderr, STDERR_FILENO);
    close(old_stderr);

    return node;
}

static NODE *
parse_expr(VALUE rb_str) {
    return with_silenced_stderr(rb_str);
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
is_comment(const char *line)
{
    size_t line_len = strlen(line);

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

static VALUE
find_expression(char **file[], const int occupied_lines)
{
    int expr_size = occupied_lines * MAXLINELEN;

    char *expr = malloc(expr_size);
    char *parseable_expr = malloc(expr_size);
    VALUE rb_expr;

    int l = 0;
    while ((*file)[l][0] == '\n') {
        l++;
        continue;
    }
    char *first_line = (*file)[l];

    char *line = NULL;
    int should_parse;
    int dangling = 0;

    expr[0] = '\0';
    parseable_expr[0] = '\0';

    if (is_static_definition(first_line)) {
        should_parse = 1;
    } else if (is_accessor(first_line)) {
        should_parse = 1;
    } else {
        should_parse = 0;
    }

    for (int i = l; i < occupied_lines; i++) {
        line = (*file)[i];

        strcat(expr, line);

        if (is_comment(line))
            continue;

        if (strstr(line, "}") != NULL && strstr(line, "{") == NULL) {
            dangling = 0;
        }

        if (dangling)
            continue;

        if (strstr(line, "%{") != NULL && strstr(line, "}") == NULL) {
            dangling = 1;
        }

        filter_interp(line);
        strcat(parseable_expr, line);

        if (should_parse || contains_end_kw(line)) {
            if (parse_expr(rb_str_new2(parseable_expr)) != NULL) {
                rb_expr = rb_str_new2(expr);
                free(expr);
                free(parseable_expr);
                return rb_expr;
            }
        }
    }

    printf("%s", parseable_expr);
    free(expr);
    free(parseable_expr);
    free_memory_for_file(file, occupied_lines);
    rb_raise(rb_eSyntaxError, "failed to parse expression (probably a bug)");

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
free_memory_for_file(char **file[], const int occupied_lines)
{
    int lines_to_free = occupied_lines >= MAXLINES ? occupied_lines : MAXLINES;

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
        rb_raise(rb_eSourceNotFoundError, "Could not locate source for %s!",
                 RSTRING_PTR(rb_sym2str(name)));
    }

    char **file = allocate_memory_for_file();

    const char *filename = RSTRING_PTR(RARRAY_AREF(source_location, 0));
    const int start_line = FIX2INT(RARRAY_AREF(source_location, 1)) - 1;

    const int occupied_lines = read_lines(filename, &file, start_line);

    VALUE expression = find_expression(&file, occupied_lines);

    free_memory_for_file(&file, occupied_lines);

    return expression;
}

void Init_fast_method_source(void)
{
    VALUE rb_mFastMethodSource = rb_define_module_under(rb_cObject, "FastMethodSource");

    rb_eSourceNotFoundError = rb_define_class_under(rb_mFastMethodSource,"SourceNotFoundError", rb_eStandardError);
    VALUE rb_mMethodExtensions = rb_define_module_under(rb_mFastMethodSource, "MethodExtensions");

    rb_define_method(rb_mMethodExtensions, "source", mMethodExtensions_source, 0);
}
