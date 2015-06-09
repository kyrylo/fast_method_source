require_relative 'bench_helper'

method_list = SystemNavigation.new.all_methods.select do |method|
  if method.source_location
    begin
      FastMethodSource.comment_for(method)
    rescue FastMethodSource::SourceNotFoundError, IOError
    end
  end
end

puts "Sample methods: #{method_list.count}"

Benchmark.bmbm do |bm|
  bm.report('FastMethodSource#comment') do
    method_list.each do |method|
      FastMethodSource.comment_for(method)
    end
  end

  bm.report('MethodSource#comment') do
    method_list.each do |method|
      method.comment
    end
  end
end
