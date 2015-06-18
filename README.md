<a name="Back_to_top">
# Fast Method Source

[![Build Status](https://travis-ci.org/kyrylo/fast_method_source.svg?branch=master)](https://travis-ci.org/kyrylo/fast_method_source)

* Repository: [https://github.com/kyrylo/fast_method_source/][repo]

Table of Contents
-----------------

* <a href="#description">Description</a>
* <a href="#installation">Installation</a>
* <a href="#synopsis">Synopsis</a>
    * <a href="#speed-improvements">Speed Improvements</a>
        * <a href="#source">#source</a>
        * <a href="#comment">#comment</a>
        * <a href="#source-and-comment">#source_and_comment</a>
    * <a href="#correctness-of-output">Correctness Of Output</a>
    * <a href="#ram-consumption">RAM Consumption</a>
* <a href="#api">API</a>
    * <a href="#method-information">Method Information</a>
        * <a href="#fastmethodsourcesource_formethod">FastMethodSource#source_for(method)</a>
        * <a href="#fastmethodsourcecomment_formethod">FastMethodSource#comment_for(method)</a>
        * <a href="#fastmethodsourcecomment_and_source_formethod">FastMethodSource#comment_and_source_for(method)</a>
* <a href="#limitations">Limitations</a>
  * <a href="#rubies">Rubies</a>
  * <a href="#operation-systems">Operation Systems</a>
* <a href="#roadmap">Roadmap</a>
  * <a href="#optional-memoization">Optional Memoization</a>
* <a href="#licence">Licence</a>

Description
-----------

Fast Method Source is a Ruby library for quering methods, procs and lambdas for
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

Installation
------------

All you need is to install the gem.

    gem install fast_method_source

Synopsis
--------

Fast Method Source provides functionality similar to [method_source][ms], which
powers [Pry](https://github.com/pry/pry)'s `show-source`, `show-comment` and
`find-method` commands, but with a number of key differences that are listed
below.

### Speed Improvements

The main goal of Fast Method Source is to retrieve information as fast as
possible. In result, the library is extremely fast and is capable of quering
thousands of methods in a couple of seconds. The benchmarks included in this
repository show that Fast Method Source outpaces method_source in every aspect
by far. The results from those benchmarks below show the comparison of these
libraries.

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

### Correctness Of Output

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

That said, you need to be cautious, because sometimes it's not working as
expected. Feel free to file issues.

### RAM Consumption

The [`comment_and_source`](/benchmarks/comment_and_source_for.rb) benchmark
shows that the library uses about 10 MB of RES RAM for computing information for
19K methods. I measure with `htop`, manually. If you know a better way to
measure RAM, please drop a letter here: `silin@kyrylo.org`.

API
---

The library provides the following methods: `#comment`, `#source` and
`#comment_and_source`. There are two ways to use Fast Method Source.

One way is to monkey-patch relevant core Ruby classes and use the methods
directly.

```ruby
require 'fast_method_source'
require 'fast_method_source/core_ext' # <= monkey-patches

# Monkey-patch of UnboundMethod
Set.instance_method(:merge).source #=> "  def merge(enum)\n..."

# Monkey-patch of Method
Set.method(:[]).source #=> "  def self.[](*ary)\n..."

# Monkey-patch of Proc (or lambda)
myproc = proc { |arg|
  arg + 1
}
myproc.source #=> "myproc = proc { |arg|\n..."
```

The other way is by using these methods defined on the library's class directly.

```ruby
require 'fast_method_source'

# With UnboundMethod
FastMethodSource.source_for(Set.instance_method(:merge)) #=> "  def merge(enum)\n..."

# With Method
FastMethodSource.source_for(Set.method(:[])) #=> "  def self.[](*ary)\n..."

# With Proc (or lambda)
myproc = proc { |arg|
  arg + 1
}
FastMethodSource.source_for(myproc) #=> "myproc = proc { |arg|\n..."
```

### Method Information

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

* CRuby 2.1.x
* CRuby 2.2.x
* CRuby 2.3.0dev and higher

Rubies below 2.2.2 were not tested, so in theory if it quacks like Ruby 2, it
may work.

### Operation Systems

* GNU/Linux
* Mac OS (hopefully)

Roadmap
-------

### Optional Memoization

I'm not sure about this, if it's really needed, but it will speed up further
queries greatly (at the cost of RAM). At this moment I think if I add it, it
should be optional and configurable like this:

```ruby
FastMethodSource.memoization = true # or `false`
```

So far I'm happy about the speed of the library.

Licence
-------

The project uses Zlib License. See the LICENCE.txt file for more information.

[repo]: https://github.com/kyrylo/fast_method_source/ "Home page"
[ms]: https://github.com/banister/method_source
