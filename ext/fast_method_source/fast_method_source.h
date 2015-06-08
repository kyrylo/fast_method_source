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

#define MAXLINES 1000
#define MAXLINELEN 300

static int read_lines(const char *filename, char **file[], const int start_line);
static void reallocate_lines(char **lines[], int occupied_lines);
static VALUE find_expression(char **lines[], const int end_line);
static VALUE mMethodExtensions_source(VALUE self);
static NODE *parse_expr(VALUE rb_str);
static NODE *with_silenced_stderr(VALUE rb_str);
static void filter_interp(char *line);
static char **allocate_memory_for_file(void);
static void free_memory_for_file(char **file[], const int occupied_lines);
static int contains_end_kw(const char *line);
static int is_comment(const char *line);
static int is_static_definition(const char *line);
static int is_accessor(const char *line);

#define SAFE_CHAR 'z'
