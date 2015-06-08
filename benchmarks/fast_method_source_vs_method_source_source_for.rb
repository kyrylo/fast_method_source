require_relative 'bench_helper'

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
