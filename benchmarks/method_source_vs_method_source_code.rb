# coding: utf-8
require 'benchmark'

def require_dep(dep_name)
  require dep_name
rescue LoadError => e
  name = e.message.scan(/(?<=-- ).+/).first
  puts "'#{name}' is not installed. To fix: gem install #{name}"
  exit
end

module Kernel
  def suppress_warnings
    original_verbosity = $VERBOSE
    $VERBOSE = nil
    result = yield
    $VERBOSE = original_verbosity
    return result
  end
end

require_dep 'require_all'
require_dep 'method_source'
require_dep 'system_navigation'

stdlib_files = File.join(ENV['RUBY_ROOT'], "lib/ruby/2*/**/*.rb")
IGNORE_LIST = [
  /tkmacpkg.rb/,
  /macpkg.rb/,
  /debug.rb/,
  /multi-tk.rb/,
  /tk.rb/,
  /tkwinpkg.rb/,
  /winpkg.rb/,
  /ICONS.rb/i,
  /tkextlib/,
  /profile.rb/,
  /formatter_test_case.rb/,
  /test_case.rb/,
  /xmlparser/,
  /xmlscanner/,
  /cgi_runner.rb/,
  /multi-irb.rb/,
  /subirb.rb/,
  /ws-for-case-2.rb/,
  /test_utilities.rb/,
  /irb\/frame.rb/,
  /psych\/parser.rb/,
  /rake\/contrib\/sys.rb/,
  /rake\/gempackagetask.rb/,
  /rake\/rdoctask.rb/,
  /ruby182_test_unit_fix.rb/,
  /rake\/runtest.rb/
]

files_to_require = Dir.glob(stdlib_files).reject do |p|
  IGNORE_LIST.any? { |pattern| pattern =~ p }
end

if $DEBUG
  print "Ignoring these files: "
  p Dir.glob(stdlib_files) - files_to_require

  puts "Requiring the rest of stdlib..."

  require_all files_to_require
else
  suppress_warnings { require_all files_to_require }
end

require_relative '../lib/fast_method_source'

puts "Counting the number of sample methods..."

method_list = SystemNavigation.new.all_methods.select do |method|
  if method.source_location
    begin
      FastMethodSource.source_for(method)
    rescue
    end
  end
end

puts "Sample methods: #{method_list.count}"

Benchmark.bmbm do |bm|
  bm.report('method_source') do
    method_list.each do |method|
      method.source
    end
  end

  bm.report('fast_method_source') do
    method_list.each do |method|
      FastMethodSource.source_for(method)
    end
  end
end
