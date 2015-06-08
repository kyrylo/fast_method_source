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

```
Sample methods: 19299
Rehearsal ------------------------------------------------------
method_source       90.900000   0.360000  91.260000 (101.708412)
method_source_code  19.850000   1.460000  21.310000 ( 23.213132)
------------------------------------------- total: 112.570000sec

                         user     system      total        real
method_source       87.020000   0.280000  87.300000 ( 97.182961)
method_source_code  18.840000   1.240000  20.080000 ( 21.821779)
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

The library supports *only* CRuby.

* CRuby 2.3.0dev and higher

Licence
-------

The project uses Zlib License. See the LICENCE.txt file for more information.

[repo]: https://github.com/kyrylo/fast_method_source/ "Home page"
[ms]: https://github.com/banister/method_source
