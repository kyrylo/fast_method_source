require 'rake/testtask'
require 'rake/extensiontask'
require 'rake/clean'

Rake::ExtensionTask.new do |ext|
  ext.name    = 'fast_method_source'
  ext.ext_dir = 'ext/fast_method_source'
  ext.lib_dir = 'lib/fast_method_source'
end

task :test do
  Rake::TestTask.new do |t|
    t.test_files = Dir.glob('test/**/test_*.rb')
  end
end

task default: :test
