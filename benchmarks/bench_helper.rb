# coding: utf-8
require 'benchmark'
require 'rbconfig'

def require_dep(dep_name)
  require dep_name
rescue LoadError => e
  name = e.message.scan(/(?<=-- ).+/).first
  raise e unless name
  name.gsub!('/', '-')
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
require_dep 'sys/cpu'

stdlib_files = File.join(RbConfig::CONFIG['rubylibdir'], "**/*.rb")
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
  /psych\/parser.rb/,
  /rake\/contrib\/sys.rb/,
  /rake\/gempackagetask.rb/,
  /rake\/rdoctask.rb/,
  /ruby182_test_unit_fix.rb/,
  /rake\/runtest.rb/,
  /tcl/,
  /minitest\/spec.rb/,
  /rubygems\/exceptions.rb/,
  /\/irb\//,
  /\/rake\//,
  /package\/old.rb/,
  /rubygems\/rdoc.rb/
]

files_to_require = Dir.glob(stdlib_files).reject do |p|
  IGNORE_LIST.any? { |pattern| pattern =~ p }
end

if $DEBUG
  print "Ignoring these files: "
  p Dir.glob(stdlib_files) - files_to_require

  puts "Requiring the rest of stdlib..."

  require_all files_to_require

  puts "Requiring fast_method_source from source..."
  require_relative '../lib/fast_method_source'
else
  suppress_warnings { require_all files_to_require }
end

puts "Processor: #{Sys::CPU.processors.first.model_name}"
puts "Platform: #{RUBY_ENGINE} #{RUBY_VERSION}p#{RUBY_PATCHLEVEL} (#{RUBY_RELEASE_DATE} revision #{RUBY_REVISION}) [#{RUBY_PLATFORM}]"
puts "Counting the number of sample methods..."
