require 'mkmf'

$CFLAGS << ' -std=c99 -Wno-declaration-after-statement'

create_makefile('fast_method_source/fast_method_source')
