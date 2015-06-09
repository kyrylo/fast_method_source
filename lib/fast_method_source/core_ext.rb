class Proc
  unless self.method_defined?(:name)
    protected

    def name
      self.inspect
    end
  end
end

[Method, UnboundMethod, Proc].each do |klass|
  klass.include FastMethodSource::MethodExtensions

  klass.class_eval do
    def comment_and_source
      self.comment + self.source
    end
  end
end
