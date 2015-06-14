// For getline(), fileno() and friends
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
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

#ifndef HAVE_RB_SYM2STR
# define rb_sym2str(obj) rb_id2str(SYM2ID(obj))
#endif

#ifdef __APPLE__
void *
memrchr(const void *s, int c, size_t n)
{
    const unsigned char *cp;

    if (n != 0) {
        cp = (unsigned char *)s + n;
        do {
            if (*(--cp) == (unsigned char)c)
                return((void *)cp);
        } while (--n != 0);
    }
    return NULL;
}
#else
void *memrchr(const void *s, int c, size_t n);
#endif

#define EXPRESSION_SIZE 80 * 50

#define SAFE_CHAR 'z'

typedef struct {
    int source  : 1;
    int comment : 1;
} finder;

struct method_data {
    unsigned method_location;
    const char *filename;
    VALUE method_name;
};

static VALUE read_lines(finder finder, struct method_data *data);
static VALUE read_lines_before(struct method_data *data);
static VALUE read_lines_after(struct method_data *data);
static VALUE find_method_comment(struct method_data *data);
static VALUE find_method_source(struct method_data *data);
static VALUE find_comment_expression(struct method_data *data);
static NODE *parse_expr(VALUE rb_str);
static NODE *parse_with_silenced_stderr(VALUE rb_str);
static void filter_interp(char *line);
static int contains_skippables(const char *line);
static int contains_end_kw(const char *line);
static int is_comment(const char *line, const size_t line_len);
static int is_static_definition(const char *line);
static int is_accessor(const char *line);
static int is_dangling_literal_end(const char *line);
static int is_dangling_literal_begin(const char *line);
static void raise_if_nil(VALUE val, VALUE method_name);
static void realloc_expression(char **expression, size_t len);
static void method_data_init(VALUE self, struct method_data *data);
static VALUE mMethodExtensions_source(VALUE self);

static VALUE rb_eSourceNotFoundError;

static VALUE
find_method_source(struct method_data *data)
{
    return read_lines_after(data);
}

static VALUE
find_method_comment(struct method_data *data)
{
    return read_lines_before(data);
}

static VALUE
read_lines_after(struct method_data *data)
{
    finder finder = {1, 0};
    return read_lines(finder, data);
}

static VALUE
read_lines_before(struct method_data *data)
{
    finder finder = {0, 1};
    return read_lines(finder, data);
}

static VALUE
find_comment_expression(struct method_data *data)
{
    struct stat filestat;
    int fd;
    char *map;

    VALUE comment = Qnil;

    if ((fd = open(data->filename, O_RDONLY)) == -1) {
        rb_raise(rb_eIOError, "failed to read - %s", data->filename);
    }

    if (fstat(fd, &filestat) == -1) {
        rb_raise(rb_eIOError, "filestat failed for %s", data->filename);
    }

    map = mmap(NULL, filestat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        rb_raise(rb_eIOError, "mmap failed for %s", data->filename);
    }

    char *rest = map;
    unsigned line_count = 0;

    while (line_count++ != data->method_location - 1) {
        rest = (char *) memchr(rest, '\n', filestat.st_size) + 1;
    }

    size_t end = filestat.st_size - strlen(rest);
    map[end] = '\0';

    size_t offset = end;
    char *prev = NULL;

    while (line_count-- != 0) {
        rest = memrchr(map, '\n', offset - 1);
        if (rest == NULL) {
            rest = memchr(map, '\n', offset);
        } else {
            rest++;
        }

        offset = end - strlen(rest);

        if (is_comment(rest, offset)) {
            prev = rest;
        } else {
            if (prev == NULL) {
                comment = rb_str_new("", 0);
            } else {
                comment = rb_str_new2(prev);
            }

            break;
        }
    }

    if (munmap (map, filestat.st_size) == -1) {
        rb_raise(rb_eIOError, "munmap failed for %s", data->filename);
    }
    close(fd);

    return comment;
}

static VALUE
read_lines(finder finder, struct method_data *data)
{
    FILE *fp;
    if ((fp = fopen(data->filename, "r")) == NULL) {
        rb_raise(rb_eIOError, "No such file or directory - %s", data->filename);
    }

    ssize_t cl_len;
    VALUE rb_expr;

    size_t bufsize, parse_bufsize, future_bufsize, future_parse_bufsize,
        current_linebuf_size, line_count;

    bufsize = parse_bufsize = EXPRESSION_SIZE;
    line_count = current_linebuf_size = 0;

    int should_parse, dangling_literal, found_expression;
    should_parse = dangling_literal = found_expression = 0;

    char *expression = ALLOC_N(char, bufsize);
    expression[0] = '\0';

    char *parse_expression = ALLOC_N(char, bufsize);
    parse_expression[0] = '\0';

    char *current_line = NULL;

    if (finder.comment) {
        return find_comment_expression(data);
    } else if (finder.source) {
        while ((cl_len = getline(&current_line, &current_linebuf_size, fp)) != -1) {
            line_count++;

            if (finder.source && line_count >= data->method_location) {
                future_bufsize = strlen(expression) + cl_len;
                if (future_bufsize >= bufsize) {
                    bufsize = future_bufsize + EXPRESSION_SIZE + 1;
                    realloc_expression(&expression, bufsize);
                }
                strncat(expression, current_line, cl_len);

                if (current_line[0] == '\n' ||
                    is_comment(current_line, cl_len))
                    continue;

                if (line_count == data->method_location) {
                    if (is_static_definition(current_line) || is_accessor(current_line)) {
                        should_parse = 1;
                    }
                }

                if (is_dangling_literal_end(current_line)) {
                    dangling_literal = 0;
                } else if (is_dangling_literal_begin(current_line)) {
                    dangling_literal = 1;
                } else if (dangling_literal) {
                    continue;
                }

                filter_interp(current_line);
                future_parse_bufsize = strlen(parse_expression) + cl_len;
                if (future_parse_bufsize >= parse_bufsize) {
                    parse_bufsize = future_parse_bufsize + EXPRESSION_SIZE + 1;
                    realloc_expression(&parse_expression, parse_bufsize);
                }
                strncat(parse_expression, current_line, cl_len);

                if (contains_skippables(current_line))
                    continue;

                if (should_parse || contains_end_kw(current_line)) {
                    if (parse_expr(rb_str_new2(parse_expression)) != NULL) {
                        found_expression = 1;
                        break;
                    }
                }
            }
        }
    }

    if (found_expression) {
        rb_expr = rb_str_new2(expression);
    } else {
        rb_expr = Qnil;
    }

    xfree(parse_expression);
    xfree(expression);
    free(current_line);
    fclose(fp);

    return (rb_expr);
}

static void
realloc_expression(char **expression, size_t len)
{
    char *tmp_comment = REALLOC_N(*expression, char, len);
    *expression = tmp_comment;
}

static NODE *
parse_with_silenced_stderr(VALUE rb_str)
{
    int old_stderr;
    FILE *null_fd;
    VALUE last_exception = rb_errinfo();

    old_stderr = DUP(STDERR_FILENO);
    fflush(stderr);
    null_fd = fopen(null_filename, "w");
    DUP2(fileno(null_fd), STDERR_FILENO);

    volatile VALUE vparser = rb_parser_new();
    NODE *node = rb_parser_compile_string(vparser, "-", rb_str, 1);
    rb_set_errinfo(last_exception);

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
contains_skippables(const char *line)
{

    static unsigned short len = 57;
    static const char *skippables[57] = {
        " if ", " else ", " then ", " case ", " for ", " in ", " loop ",
        " unless ", "||", "||=", "&&", " and ", " or ", " do ", " not ",
        " while ", " = ", " == ", " === ", " > ", " < ", " >= ", " <= ",
        " << ", " += ", "\"", "'", ":", ",", ".", " % ", "!", "?",
        " until ", "(&", "|", "(", ")", "~", " * ", " - ", " + ", " / ",
        " {", " raise ", " fail ", " begin ", " rescue ", " ensure ",
        ".new", "@", " lambda", " proc", " ->", "[", "]", "$"};

    for (unsigned short i = 0; i < len; i++) {
        if (strstr(line, skippables[i]) != NULL)
            return 1;
    }

    return 0;
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
        return line[i] == '#' && line[i + 1] != '{';
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

static void
raise_if_nil(VALUE val, VALUE method_name)
{
    if (NIL_P(val)) {
        if (SYMBOL_P(method_name)) {
            rb_raise(rb_eSourceNotFoundError, "could not locate source for %s",
                     RSTRING_PTR(rb_sym2str(method_name)));
        } else {
            rb_raise(rb_eSourceNotFoundError, "could not locate source for %s",
                     RSTRING_PTR(method_name));
        }
    }
}

static void
method_data_init(VALUE self, struct method_data *data)
{
    VALUE source_location = rb_funcall(self, rb_intern("source_location"), 0);
    VALUE name = rb_funcall(self, rb_intern("name"), 0);

    raise_if_nil(source_location, name);

    VALUE rb_filename = RARRAY_AREF(source_location, 0);
    VALUE rb_method_location = RARRAY_AREF(source_location, 1);

    raise_if_nil(rb_filename, name);
    raise_if_nil(rb_method_location, name);

    data->filename = RSTRING_PTR(rb_filename);
    data->method_location = FIX2INT(rb_method_location);
    data->method_name = name;
}

static VALUE
mMethodExtensions_source(VALUE self)
{
    struct method_data data;
    method_data_init(self, &data);

    VALUE source = find_method_source(&data);
    raise_if_nil(source, data.method_name);

    return source;
}

static VALUE
mMethodExtensions_comment(VALUE self)
{
    struct method_data data;
    method_data_init(self, &data);

    VALUE comment = find_method_comment(&data);
    raise_if_nil(comment, data.method_name);

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
