# coding: utf-8
require 'benchmark'
require 'require_all'

module Kernel
  def suppress_warnings
    original_verbosity = $VERBOSE
    $VERBOSE = nil
    result = yield
    $VERBOSE = original_verbosity
    return result
  end
end

stdlib_files = File.join(ENV['RUBY_ROOT'], "lib/ruby/#{RUBY_VERSION}/**/*.rb")
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
  /irb\/frame.rb/
]

files_to_require =  Dir.glob(stdlib_files).reject do |p|
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


require_relative '../../system_navigation/lib/system_navigation'
require_relative '../lib/fast_method_source'

puts "Counting the number of sample methods..."

method_list = SystemNavigation.new.all_methods.select do |method|
  if method.source_location
    begin
      method.source
    rescue
    end
  end
end

puts "Sample methods: #{method_list.count}"

Benchmark.bmbm do |bm|
  bm.report('method_source') do
    method_list.map do |method|
      method.source
    end
  end

  bm.report('method_source_code') do
    method_list.map do |method|
      FastMethodSource.source_for(method)
    end
  end
end
