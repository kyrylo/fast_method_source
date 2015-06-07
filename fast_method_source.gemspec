Gem::Specification.new do |s|
  s.name         = 'fast_method_source'
  s.version      = File.read('VERSION')
  s.date         = Time.now.strftime('%Y-%m-%d')
  s.summary      = 'Retrieves the source code for a method (and its comment)'
  s.description  = 'Retrieves the source code for a method (and its comment) and does it faster than any other existing solution'
  s.author       = 'Kyrylo Silin'
  s.email        = 'silin@kyrylo.org'
  s.homepage     = 'https://github.com/kyrylo/fast_method_source'
  s.licenses     = 'Zlib'

  s.require_path = 'lib'
  s.files        = %w[
    ext/fast_method_source/extconf.rb
    ext/fast_method_source/fast_method_source.c
    ext/fast_method_source/fast_method_source.h
    ext/fast_method_source/node.h
    lib/fast_method_source.rb
    lib/fast_method_source/core_ext.rb
  ]
  s.extensions = ['ext/fast_method_source/extconf.rb']
  s.platform = Gem::Platform::RUBY

  s.add_development_dependency 'bundler', '~> 1.9'
  s.add_development_dependency 'rake', '~> 10.4'
  s.add_development_dependency 'rake-compiler', '~> 0.9'
  s.add_development_dependency 'minitest', '~> 5.7'
end
