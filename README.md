# Fast Method Source

* Repository: [https://github.com/kyrylo/fast_method_source/][repo]

Description
-----------

Fast Method Source is an extension for quering methods for their source code and
comments. Additionally, it supports Procs and lambdas.

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

Fast Method Source provides similar functionality to [method_source][ms], but
with a number of major key differences.

### Speed improvements

The main goal of Fast Method Source is to be as fast as possible. In result, the
library is much faster than method_source. The benchmark showed that it's
about 5-10x faster than its competitor. There's also room for future speed
improvements.

#### ruby 2.2.2p95 (2015-04-13 revision 50295) [x86_64-linux]

```
Your processor is: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
Counting the number of sample methods...
Sample methods: 19437
Rehearsal ------------------------------------------------------
fast_method_source  19.200000   1.150000  20.350000 ( 22.144561)
method_source       95.400000   0.550000  95.950000 (114.498600)
------------------------------------------- total: 116.300000sec

                         user     system      total        real
fast_method_source  18.010000   1.100000  19.110000 ( 20.718103)
method_source       89.930000   0.270000  90.200000 (100.268377)
```

### Correctness of output

Fast Method Source is capable of displaying source code even for dynamically
defined methods.

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

API
---

The library provides two methods `#source` and `#comment`. There are two ways to
use Fast Method Source. One way is to monkey-patch default Ruby classes and use
these methods directly.

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

The other way is by using these methods accessing the library directly.

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

Although Fast Method Source is faster than any of its competitors, it's still
very slow. On average, a mature Rails 4 application has at least 45K methods. In
order to query all those methods for their source code, you would need to wait
for a while and perhaps to drink a cup of tea.

The goal of the project is to be able to query 50K methods in less than 15
seconds. Whether it's possible or not is to be determined.

Licence
-------

The project uses Zlib License. See the LICENCE.txt file for more information.

[repo]: https://github.com/kyrylo/fast_method_source/ "Home page"
[ms]: https://github.com/banister/method_source
