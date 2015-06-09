require_relative 'bench_helper'

method_list = SystemNavigation.new.all_methods.select do |method|
  if method.source_location
    begin
      FastMethodSource.comment_and_source_for(method)
    rescue FastMethodSource::SourceNotFoundError, IOError
    end
  end
end

puts "Sample methods: #{method_list.count}"

Benchmark.bmbm do |bm|
  bm.report('FastMethodSource#comment_and_source_for') do
    method_list.each do |method|
      FastMethodSource.comment_and_source_for(method)
    end
  end
end
