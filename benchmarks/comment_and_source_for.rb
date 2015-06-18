require_relative 'bench_helper'

method_list = SystemNavigation.new.all_rb_methods.select do |method|
  begin
    FastMethodSource.comment_and_source_for(method)
  rescue FastMethodSource::SourceNotFoundError, IOError
  end
end

puts "Sample methods: #{method_list.count}"

Benchmark.bm do |bm|
  bm.report('FastMethodSource#comment_and_source_for') do
    method_list.each do |method|
      FastMethodSource.comment_and_source_for(method)
    end
  end
end
