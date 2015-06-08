require 'mkmf'

$CFLAGS << ' -Wno-declaration-after-statement'

create_makefile('fast_method_source/fast_method_source')
