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

typedef struct {
    int forward  : 1;
    int backward : 1;
} read_order;

static unsigned read_lines(read_order order, const unsigned method_location,
                           const char *filename, char **filebuf[]);
static unsigned read_lines_before(const unsigned method_location,
                                  const char *filename, char **filebuf[]);
static unsigned read_lines_after(const unsigned method_location,
                                 const char *filename, char **filebuf[]);
static void reallocate_filebuf(char **lines[], unsigned rl_len);
static void reallocate_linebuf(char **linebuf, const unsigned cl_len);
static VALUE find_source(char **filebuf[], const unsigned relevant_lines_n);
static VALUE find_comment(char **filebuf[], const unsigned relevant_lines_n,
                          const unsigned method_location);
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
static void check_if_nil(VALUE val, VALUE name);


#define SAFE_CHAR 'z'
