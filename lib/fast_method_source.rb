require_relative 'fast_method_source/fast_method_source'

module FastMethodSource
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
end
