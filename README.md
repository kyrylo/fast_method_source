<a name="Back_to_top">
# Fast Method Source

[![Build Status](https://travis-ci.org/kyrylo/fast_method_source.svg?branch=master)](https://travis-ci.org/kyrylo/fast_method_source)

* Repository: [https://github.com/kyrylo/fast_method_source/][repo]
* API description: [https://github.com/kyrylo/fast_method_source/blob/master/docs/API.md][apimd]

Description
-----------

Fast Method Source is a Ruby library for querying methods, procs and lambdas for
their source code and comments.

```ruby
require 'fast_method_source'
require 'fast_method_source/core_ext' # Adds #source to UnboundMethod
require 'set'

puts Set.instance_method(:merge).source
```

Output.

```
  def merge(enum)
    if enum.instance_of?(self.class)
      @hash.update(enum.instance_variable_get(:@hash))
    else
      do_with_enum(enum) { |o| add(o) }
    end

    self
  end
```

Fast Method Source provides functionality similar to [method_source][ms].
However, it is significantly faster. It can show the source for 19K methods in
roughly a second, while method_source takes about 30 seconds. The benchmarks
below show the difference expressed in numbers.

#### #source
##### ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]

The speed boost of method_source after the rehearsal that can be observed in
this benchmark comes from the fact that it uses memoization. Fast Method Source
does not support it at the moment.

```
Processor: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
Platform: ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]
Counting the number of sample methods...
Sample methods: 19280
Rehearsal -----------------------------------------------------------
FastMethodSource#source   0.760000   0.240000   1.000000 (  1.104455)
MethodSource#source      53.240000   0.250000  53.490000 ( 59.304424)
------------------------------------------------- total: 54.490000sec

                              user     system      total        real
FastMethodSource#source   0.610000   0.190000   0.800000 (  0.890971)
MethodSource#source      51.770000   0.230000  52.000000 ( 57.640986)
```

#### #comment
##### ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]

```
Processor: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
Platform: ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]
Counting the number of sample methods...
Sample methods: 19437
Rehearsal ------------------------------------------------------------
FastMethodSource#comment   0.180000   0.170000   0.350000 (  0.395569)
MethodSource#comment      19.860000   0.230000  20.090000 ( 36.373289)
-------------------------------------------------- total: 20.440000sec

                               user     system      total        real
FastMethodSource#comment   0.270000   0.190000   0.460000 (  0.520414)
MethodSource#comment      19.360000   0.020000  19.380000 ( 21.726528)
```

#### #comment_and_source
##### ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]

This is a convenience method, and method_source doesn't have it.

```
Processor: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
Platform: ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]
Counting the number of sample methods...
Sample methods: 19435
       user     system      total        real
FastMethodSource#comment_and_source_for  0.810000   0.420000   1.230000 (  1.374435)
```

API
---

Read here: [https://github.com/kyrylo/fast_method_source/blob/master/docs/API.md][apimd]

Installation
------------

All you need is to install the gem.

    gem install fast_method_source


Limitations
-----------

### Rubies

* CRuby 2.1.x
* CRuby 2.2.x
* CRuby 2.3.0dev and higher

Rubies below 2.2.2 were not tested, so in theory if it quacks like Ruby 2, it
may work.

### Operation Systems

* GNU/Linux
* Mac OS (hopefully)

Licence
-------

The project uses Zlib License. See the LICENCE.txt file for more information.

[repo]: https://github.com/kyrylo/fast_method_source/ "Home page"
[ms]: https://github.com/banister/method_source
[apimd]: https://github.com/kyrylo/fast_method_source/blob/master/docs/API.md
