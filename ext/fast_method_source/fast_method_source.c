// For getline(), fileno() and friends
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <ruby.h>

#include "node.h"

#ifdef _WIN32
#include <io.h>
static const char *null_filename = "NUL";
#define DUP(fd) _dup(fd)
#define DUP2(fd, newfd) _dup2(fd, newfd)
#else
#include <unistd.h>
static const char *null_filename = "/dev/null";
#define DUP(fd) dup(fd)
#define DUP2(fd, newfd) dup2(fd, newfd)
#endif

#define MAXLINES 600
#define MAXLINELEN 90
#define COMMENT_SIZE 1000
#define SAFE_CHAR 'z'

typedef struct {
    int forward  : 1;
    int backward : 1;
} read_order;

struct filebuf {
    unsigned method_location;
    const char *filename;
    VALUE method_name;
    char **lines;
    unsigned relevant_lines;
};

struct retval {
    VALUE val;
    VALUE method_name;
};

static void read_lines(read_order order, struct filebuf *filebuf);
static void read_lines_before(struct filebuf *filebuf);
static void read_lines_after(struct filebuf *filebuf);
static void reallocate_filebuf(char **lines[], unsigned rl_len);
static void reallocate_linebuf(char **linebuf, const unsigned cl_len);
static VALUE find_source(struct filebuf *filebuf);
static VALUE find_comment(struct filebuf *filebuf);
static VALUE mMethodExtensions_source(VALUE self);
static NODE *parse_expr(VALUE rb_str);
static NODE *parse_with_silenced_stderr(VALUE rb_str);
static void filter_interp(char *line);
static char **allocate_memory_for_file(void);
static void free_memory_for_file(char **file[], const unsigned relevant_lines_n);
static int contains_end_kw(const char *line);
static int is_comment(const char *line, const size_t line_len);
static int is_static_definition(const char *line);
static int is_accessor(const char *line);
static int is_dangling_literal_end(const char *line);
static int is_dangling_literal_begin(const char *line);
static void strnprep(char *s, const char *t, size_t len);
static void raise_if_nil(VALUE val, VALUE method_name);
static VALUE raise_if_nil_safe(VALUE args);
static void realloc_comment(char **comment, unsigned len);
static void filebuf_init(VALUE self, struct filebuf *filebuf);
static void free_filebuf(VALUE retval, struct filebuf *filebuf);

static VALUE rb_eSourceNotFoundError;

static void
read_lines_after(struct filebuf *filebuf)
{
    read_order order = {1, 0};
    read_lines(order, filebuf);
}

static void
read_lines_before(struct filebuf *filebuf)
{
    read_order order = {0, 1};
    read_lines(order, filebuf);
}

static void
read_lines(read_order order, struct filebuf *filebuf)
{
    FILE *fp;

    if ((fp = fopen(filebuf->filename, "r")) == NULL) {
        rb_raise(rb_eIOError, "No such file or directory - %s", filebuf->filename);
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
            if (line_count < filebuf->method_location) {
                continue;
            }
        } else if (order.backward) {
            if (line_count > filebuf->method_location) {
                break;
            }
        }

        if ((rl_n != 0) && (rl_n % (MAXLINES-1) == 0)) {
            reallocate_filebuf(&filebuf->lines, rl_n);
        }

        current_linebuf = &(filebuf->lines)[rl_n];

        if (cl_len >= MAXLINELEN) {
            reallocate_linebuf(current_linebuf, cl_len);
        }

        strncpy(*current_linebuf, current_line, cl_len);
        (*current_linebuf)[cl_len] = '\0';
        rl_n++;
    }

    free(current_line);
    fclose(fp);
    filebuf->relevant_lines = rl_n;
}

static void
reallocate_linebuf(char **linebuf, const unsigned cl_len)
{
    char *tmp_line = REALLOC_N(*linebuf, char, cl_len + 1);
    *linebuf = tmp_line;
}

static void
reallocate_filebuf(char **lines[], unsigned rl_len)
{
    unsigned new_size = rl_len + MAXLINES + 1;
    char **temp_lines = REALLOC_N(*lines, char *, new_size);

    *lines = temp_lines;

    for (int i = 0; i < MAXLINES; i++) {
        (*lines)[rl_len + i] = ALLOC_N(char, MAXLINELEN);
    }
}

static void
realloc_comment(char **comment, unsigned len)
{
    char *tmp_comment = REALLOC_N(*comment, char, len);
    *comment = tmp_comment;
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

    rb_set_errinfo(Qnil);

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
find_source(struct filebuf *filebuf)
{
    VALUE rb_expr;

    const unsigned expr_size = filebuf->relevant_lines * MAXLINELEN;
    char *expr = ALLOC_N(char, expr_size);
    expr[0] = '\0';
    char *parseable_expr = ALLOC_N(char, expr_size);
    parseable_expr[0] = '\0';

    int l = 0;
    while (filebuf->lines[l][0] == '\n') {
        l++;
        continue;
    }
    char *first_line = (filebuf->lines)[l];

    char *current_line = NULL;
    int should_parse = 0;
    int dangling_literal = 0;
    size_t current_line_len;

    if (is_static_definition(first_line) || is_accessor(first_line)) {
        should_parse = 1;
    }

    for (unsigned i = l; i < filebuf->relevant_lines; i++) {
        current_line = filebuf->lines[i];
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
                xfree(expr);
                xfree(parseable_expr);
                return rb_expr;
            }
        }
    }

    xfree(expr);
    xfree(parseable_expr);

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
find_comment(struct filebuf *filebuf)
{
    size_t comment_len;
    size_t current_line_len;
    size_t future_bufsize;
    char *current_line = NULL;
    VALUE rb_comment;

    unsigned long bufsize = COMMENT_SIZE;
    char *comment = ALLOC_N(char, COMMENT_SIZE);
    comment[0] = '\0';

    int i = filebuf->method_location - 2;

    while (filebuf->lines[i][0] == '\n') {
        i--;
        continue;
    }

    if (!is_comment(filebuf->lines[i], strlen(filebuf->lines[i]))) {
        return rb_str_new("", 0);
    } else {
        while ((current_line = filebuf->lines[i]) &&
               is_comment(current_line,  (current_line_len = strlen(current_line)))) {
            comment_len = strlen(comment);
            future_bufsize = comment_len + current_line_len;

            if (future_bufsize >= bufsize) {
                bufsize = future_bufsize + COMMENT_SIZE + 1;
                realloc_comment(&comment, bufsize);
            }
            strnprep(comment, current_line, current_line_len);
            i--;
        }

        rb_comment = rb_str_new2(comment);
        xfree(comment);
        return rb_comment;
    }

    xfree(comment);
    free_memory_for_file(&filebuf->lines, filebuf->relevant_lines);
    return Qnil;
}

static char **
allocate_memory_for_file(void)
{
    char **file = ALLOC_N(char *, MAXLINES);

    for (int i = 0; i < MAXLINES; i++) {
        file[i] = ALLOC_N(char, MAXLINELEN);
    }

    return file;
}

static void
free_memory_for_file(char **file[], const unsigned relevant_lines_n)
{
    int lines_to_free = relevant_lines_n >= MAXLINES ? relevant_lines_n : MAXLINES;

    for (int i = 0; i < lines_to_free; i++) {
        xfree((*file)[i]);
    }

    xfree(*file);
}

static VALUE
raise_if_nil_safe(VALUE args)
{
    struct retval *retval = (struct retval *) args;
    raise_if_nil(retval->val, retval->method_name);

    return Qnil;
}

static void
raise_if_nil(VALUE val, VALUE method_name)
{
    if (NIL_P(val)) {
        rb_raise(rb_eSourceNotFoundError, "could not locate source for %s",
                 RSTRING_PTR(rb_sym2str(method_name)));
    }
}

static void
filebuf_init(VALUE self, struct filebuf *filebuf)
{
    VALUE source_location = rb_funcall(self, rb_intern("source_location"), 0);
    VALUE name = rb_funcall(self, rb_intern("name"), 0);

    raise_if_nil(source_location, name);

    VALUE rb_filename = RARRAY_AREF(source_location, 0);
    VALUE rb_method_location = RARRAY_AREF(source_location, 1);

    raise_if_nil(rb_filename, name);
    raise_if_nil(rb_method_location, name);

    filebuf->filename = RSTRING_PTR(rb_filename);
    filebuf->method_location = FIX2INT(rb_method_location);
    filebuf->method_name = name;
    filebuf->lines = allocate_memory_for_file();
}

static void
free_filebuf(VALUE val, struct filebuf *filebuf)
{
    int failed;
    struct retval retval = {val, filebuf->method_name};

    rb_protect(raise_if_nil_safe, (VALUE) &retval, &failed);
    free_memory_for_file(&filebuf->lines, filebuf->relevant_lines);

    if (failed) {
        rb_jump_tag(failed);
    }
}

static VALUE
mMethodExtensions_source(VALUE self)
{
    struct filebuf filebuf;

    filebuf_init(self, &filebuf);
    read_lines_after(&filebuf);

    VALUE source = find_source(&filebuf);

    free_filebuf(source, &filebuf);

    return source;
}

static VALUE
mMethodExtensions_comment(VALUE self)
{
    struct filebuf filebuf;

    filebuf_init(self, &filebuf);
    read_lines_before(&filebuf);

    VALUE comment = find_comment(&filebuf);

    free_filebuf(comment, &filebuf);

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
