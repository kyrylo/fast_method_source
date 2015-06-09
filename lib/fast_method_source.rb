require_relative 'fast_method_source/fast_method_source'

module FastMethodSource
  # The VERSION file must be in the root directory of the library.
  VERSION_FILE = File.expand_path('../../VERSION', __FILE__)

  VERSION = File.exist?(VERSION_FILE) ?
              File.read(VERSION_FILE).chomp : '(could not find VERSION file)'

  # The root path of Pry Theme source code.
  ROOT = File.expand_path(File.dirname(__FILE__))

  class Method
    include MethodExtensions

    def initialize(method)
      @method = method
    end

    def source_location
      @method.source_location
    end

    def name
      if @method.respond_to?(:name)
        @method.name
      else
        @method.inspect
      end
    end
  end

  def self.source_for(method)
    FastMethodSource::Method.new(method).source
  end

  def self.comment_for(method)
    FastMethodSource::Method.new(method).comment
  end

  def self.comment_and_source_for(method)
    self.comment_for(method) + self.source_for(method)
  end
end
