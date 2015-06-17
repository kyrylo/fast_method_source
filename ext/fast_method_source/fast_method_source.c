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
static VALUE find_source_expression(struct method_data *data);
static NODE *parse_expr(VALUE rb_str);
static NODE *parse_with_silenced_stderr(VALUE rb_str);
static void restore_ch(char *line, char ch, int last_ch_idx);
static void close_map(int fd, char *map, size_t map_size, const char *filename);
static void nulify_ch(char *line, char *ch, int last_ch_idx);
static int contains_end_kw(const char *line);
static int is_comment(const char *line, const size_t line_len);
static int is_definition_end(const char *line);
static int is_static_definition_start(const char *line);
static size_t count_prefix_spaces(const char *line);
static void raise_if_nil(VALUE val, VALUE method_name);
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
        rest = memrchr(map, '\n', offset - 2);

        if (rest == NULL) {
            rest = memrchr(map, '\n', offset);
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
find_source_expression(struct method_data *data)
{
    struct stat filestat;
    int fd;
    char *map;

    VALUE source = Qnil;

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

    char *expr_start = map;
    char *expr_end, *next_expr, *line;
    unsigned line_count = 0;

    while (line_count++ != data->method_location - 1) {
        expr_start = (char *) memchr(expr_start, '\n', filestat.st_size) + 1;
    }

    size_t last_ch_idx;
    char ch, next_ch;
    next_expr = expr_end = expr_start;
    size_t leftover_size = filestat.st_size - strlen(expr_end);
    size_t offset = filestat.st_size - leftover_size;
    size_t next_offset;
    size_t prefix_len = 0;
    int inside_static_def = 0;
    int should_parse = 0;
    VALUE rb_expr;

    while (offset != 0) {
        next_expr = (char *) memchr(expr_end, '\n', offset) + 1;
        next_offset = strlen(next_expr);
        line = expr_end;

        last_ch_idx = offset - next_offset - 1;

        nulify_ch(line, &ch, last_ch_idx);

        expr_end = next_expr;
        offset = next_offset;

        if (is_static_definition_start(line) && !inside_static_def) {
            inside_static_def = 1;
            prefix_len = count_prefix_spaces(line);

            if (contains_end_kw(line)) {
                if (parse_expr(rb_str_new2(line)) != NULL) {
                    restore_ch(line, ch, last_ch_idx);
                    rb_expr = rb_str_new2(expr_start);

                    close_map(fd, map, filestat.st_size, data->filename);
                    return rb_expr;
                }
            }
        }

        if (line[0] == '\n' || is_comment(line, strlen(line))) {
            line[last_ch_idx] = ch;
            continue;
        }

        if (inside_static_def) {
            if (is_definition_end(line) &&
                count_prefix_spaces(line) == prefix_len)
            {
                restore_ch(line, ch, last_ch_idx);
                rb_expr = rb_str_new2(expr_start);

                close_map(fd, map, filestat.st_size, data->filename);
                return rb_expr;
            } else {
                line[last_ch_idx] = ch;
                continue;
            }
        } else {
            should_parse = 1;
        }

        if (should_parse) {
            line[last_ch_idx] = ch;
            nulify_ch(line, &next_ch, last_ch_idx + 1);

            if (parse_expr(rb_str_new2(expr_start)) != NULL) {
                rb_expr = rb_str_new2(expr_start);
                close_map(fd, map, filestat.st_size, data->filename);
                return rb_expr;
            } else {
                line[last_ch_idx+1] = next_ch;
            }
        }

        line[last_ch_idx] = ch;
    }
    close_map(fd, map, filestat.st_size, data->filename);

    return source;
}

static VALUE
read_lines(finder finder, struct method_data *data)
{
    if (finder.comment) {
        return find_comment_expression(data);
    } else if (finder.source) {
        return find_source_expression(data);
    }

    return Qnil;
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
restore_ch(char *line, char ch, int last_ch_idx)
{
    line[last_ch_idx] = ch;
    line[last_ch_idx + 1] = '\0';
}

static void
close_map(int fd, char *map, size_t map_size, const char *filename)
{
    close(fd);
    if (munmap(map, map_size) == -1) {
        rb_raise(rb_eIOError, "munmap failed for %s", filename);
    }
}

static void
nulify_ch(char *line, char *ch, int last_ch_idx)
{
    *ch = line[last_ch_idx];
    line[last_ch_idx] = '\0';
}

static size_t
count_prefix_spaces(const char *line)
{
    int i = 0;
    size_t spaces = 0;

    do {
        if (line[i] == ' ') {
            spaces++;
        } else {
            break;
        }
    } while (line[i++] != '\n');

    return spaces;
}

static int
is_definition_end(const char *line)
{
    int i = 0;

    do {
        if (line[i] == ' ') {
            continue;
        } else if (strncmp((line + i), "end", 3) == 0) {
            return 1;
        } else {
            return 0;
        }
    } while (line[i++] != '\0');

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
is_static_definition_start(const char *line)
{
    int i = 0;

    do {
        if (line[i] == ' ') {
            continue;
        } else if (strncmp((line + i), "def ", 4) == 0) {
            return 1;
        } else {
            return 0;
        }
    } while (line[i++] != '\0');

    return 0;
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
