require 'mkmf'

$CFLAGS << ' -std=c99 -Wno-declaration-after-statement'

have_func('rb_sym2str', 'ruby.h')

create_makefile('fast_method_source/fast_method_source')
