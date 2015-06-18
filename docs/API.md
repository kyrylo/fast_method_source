API
===

Overview
--

The library provides the following methods:

* `#comment`
* `#source`
* `#comment_and_source`

There are two ways to use Fast Method Source. One way is to monkey-patch
relevant core Ruby classes and use the methods directly.

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

Method Information
--

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
