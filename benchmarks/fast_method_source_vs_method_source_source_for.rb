require_relative 'bench_helper'

method_list = SystemNavigation.new.all_methods.select do |method|
  if method.source_location
    begin
      FastMethodSource.source_for(method)
    rescue
    end
  end
end

puts "Sample methods: #{method_list.count}"

Benchmark.bmbm do |bm|
  bm.report('FastMethodSource#source') do
    method_list.each do |method|
      FastMethodSource.source_for(method)
    end
  end

  bm.report('MethodSource#source') do
    method_list.each do |method|
      method.source
    end
  end
end