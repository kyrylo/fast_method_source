# Fast Method Source

* Repository: [https://github.com/kyrylo/fast_method_source/][repo]

Description
-----------

Fast Method Source is a Ruby library for quering methods, procs and lambdas for
their source code and comments.

```ruby
require 'fast_method_source'
require 'fast_method_source/core_ext'
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

Installation
------------

All you need is to install the gem.

    gem install fast_method_source

Synopsis
--------

Fast Method Source provides similar functionality to [method_source][ms], which
is being used by the Pry REPL, but with a number of major key differences.

### Speed improvements

The main goal of Fast Method Source is to be as fast as possible. In result, the
library is much faster than method_source. What's acceptable for Pry, when you
query only one method, is not acceptable for other use cases such as querying
all the methods defined in your Ruby process. The benchmark showed that Fast
Method Source is about 5-10x faster than its competitor.

I'd be remiss if I didn't mention that there's also room for further speed
improvements. The benchmarks below represent comparison of both libraries.

#### #comment_and_source
##### ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]

This is a utility method and method_source doesn't feature it.

```
Processor: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
Platform: ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]
Counting the number of sample methods...
Sample methods: 19438
       user     system      total        real
FastMethodSource#comment_and_source_for 26.050000   1.420000  27.470000 ( 30.528066)
```

#### #source
##### ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]

```
Processor: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
Platform: ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]
Counting the number of sample methods...
Sample methods: 19596
Rehearsal ------------------------------------------------------------
FastMethodSource#comment   4.730000   0.300000   5.030000 (  5.590186)
MethodSource#comment      73.660000   0.340000  74.000000 ( 89.475399)
-------------------------------------------------- total: 79.030000sec

                               user     system      total        real
FastMethodSource#comment   4.450000   0.300000   4.750000 (  5.278761)
MethodSource#comment      69.880000   0.110000  69.990000 ( 77.663383)
```

#### #comment
##### ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]

```
Processor: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
Platform: ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]
Counting the number of sample methods...
Sample methods: 19438
Rehearsal -----------------------------------------------------------
FastMethodSource#source  20.610000   1.290000  21.900000 ( 24.341358)
MethodSource#source      92.730000   0.470000  93.200000 (111.384362)
------------------------------------------------ total: 115.100000sec

                              user     system      total        real
FastMethodSource#source  20.990000   1.150000  22.140000 ( 24.612721)
MethodSource#source      86.390000   0.290000  86.680000 ( 96.241494)
```

### Correctness of output

Fast Method Source is capable of displaying source code even for dynamically
defined methods (there are some crazy methods in stdlib).

```ruby
require 'fast_method_source'
require 'rss'

puts RSS::Maker::ChannelBase.instance_method(:dc_description=).source
```

Output.

```
            def #{name}=(#{new_value_variable_name})
              #{local_variable_name} =
                #{plural_name}.first || #{plural_name}.new_#{new_name}
              #{additional_setup_code}
              #{local_variable_name}.#{attribute} = #{new_value_variable_name}
            end
```

### RAM consumption

Memory consumption is considerably higher for Fast Method Source (for the
benchmarks it used about 800-1500 MB of RES RAM). In fact, since the library is
very young, it *may* have memory leaks.

API
---

### General description

The library provides two methods `#source` and `#comment`. There are two ways to
use Fast Method Source. One way is to monkey-patch relevant core Ruby classes
and use the methods directly.

```ruby
require 'fast_method_source'
require 'fast_method_source/core_ext'

# With UnboundMethod
Set.instance_method(:merge).source
#=> "  def merge(enum)\n..."

# With Method
Set.method(:[]).source
#=> "  def self.[](*ary)\n..."

# With Proc (or lambda)
myproc = proc { |arg|
  arg + 1
}
myproc.source
#=> "myproc = proc { |arg|\n..."
```

The other way is by using these methods defined on the library's class directly.

```ruby
require 'fast_method_source'

# With UnboundMethod
FastMethodSource.source_for(Set.instance_method(:merge))
#=> "  def merge(enum)\n..."

# With Method
FastMethodSource.source_for(Set.method(:[]))
#=> "  def self.[](*ary)\n..."

# With Proc (or lambda)
myproc = proc { |arg|
  arg + 1
}
FastMethodSource.source_for(myproc)
#=> "myproc = proc { |arg|\n..."
```

### Methods

#### FastMethodSource#source_for(method)

Returns the source code of the given _method_ as a String.
Raises `FastMethodSource::SourceNotFoundError` if:

* _method_ is defined outside of a file (for example, in a REPL)
* _method_ doesn't have a source location (typically C methods)

Raises `IOError` if:

* the file location of _method_ is inaccessible

```ruby
FastMethodSource.source_for(Set.instance_method(:merge))
#=> "  def merge(enum)\n..."
FastMethodSource.source_for(Array.instance_method(:pop))
#=> FastMethodSource::SourceNotFoundError
```

#### FastMethodSource#comment_for(method)

Returns the comment of the given _method_ as a String. The rest is identical to
`FastMethodSource#source_for(method)`.

#### FastMethodSource#comment_and_source_for(method)

Returns the comment and the source code of the given _method_ as a String (the
order is the same as the method's name). The rest is identical to
`FastMethodSource#source_for(method)`.

Limitations
-----------

### Rubies

* CRuby 2.2.2
* CRuby 2.3.0dev and higher

Rubies below 2.2.2 were not tested, so in theory if it quacks like Ruby 2, it
may work.

### OS'es

* GNU/Linux
* Mac OS (hopefully)

Roadmap
-------

### Further speed improvements

Although Fast Method Source is faster than any of its competitors, it's still
very slow. On average, a mature Rails 4 application has at least 45K methods. In
order to query all those methods for their source code, you would need to wait
for a while and perhaps to drink a cup of tea.

The goal of the project is to be able to query 50K methods in less than 15
seconds. Whether it's possible or not is to be determined.

### Decrease memory consumption

I'm not happy about the current rates. To be investigated.

Licence
-------

The project uses Zlib License. See the LICENCE.txt file for more information.

[repo]: https://github.com/kyrylo/fast_method_source/ "Home page"
[ms]: https://github.com/banister/method_source
