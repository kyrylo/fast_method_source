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
end
