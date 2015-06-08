require_relative 'helper'

class TestFastMethodSource < Minitest::Test
  def test_source_for_class
    method = SampleClass.instance_method(:sample_method)
    assert_match(/:sample_method/, FastMethodSource.source_for(method))
  end

  def test_source_for_module
    method = SampleModule.instance_method(:sample_method)
    assert_match(/:sample_method/, FastMethodSource.source_for(method))
  end

  def test_source_for_proc
    method = proc { |*args|
      args.first + args.last
    }

    assert_match(/args\.first \+ args\.last/, FastMethodSource.source_for(method))
  end

  def test_source_for_lambda
    method = proc { |*args|
      args.first + args.last
    }

    assert_match(/args\.first \+ args\.last/, FastMethodSource.source_for(method))
  end

  def test_source_for_rss_image_favicon
    require 'rss'

    method = RSS::ImageFaviconModel::ImageFavicon.instance_method(:dc_date_elements)
    assert_match(/klass.install_have_children_element/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_erb_scan
    require 'erb'

    method = ERB::Compiler::SimpleScanner2.instance_method(:scan)
    assert_match(/def scan\n\s+stag_reg/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_irb_change_workspace
    require 'irb'

    method = IRB::ExtendCommandBundle.instance_method(:irb_change_workspace)
    assert_match(/def \#{cmd_name}\(\*opts, &b\)/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_rss_time_w3cdtf
    require 'rss'

    method = Time.instance_method(:w3cdtf)
    assert_match(/def w3cdtf\n      if usec.zero\?\n/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_kernel_require
    method = Kernel.instance_method(:require)
    assert_match(/def require path\n    RUBYGEMS_ACTIVATION_MONITOR.enter\n\n/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_irb_init_config
    require 'irb'

    method = IRB.singleton_class.instance_method(:init_config)
    assert_match(/def IRB.init_config\(ap_path\)\n    # class instance variables\n/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_rss_def_classed_element
    require 'rss'

    method = RSS::Maker::Base.singleton_class.instance_method(:def_classed_element)
    assert_match(/module_eval\(<<-EOC, __FILE__, __LINE__ \+ 1\)\n/,
                 FastMethodSource.source_for(method))
    assert_match(/\#{name}.\#{attribute_name}\n/,
                             FastMethodSource.source_for(method))
    assert_match(/@\#{name}.\#{attribute_name} = new_value\n/,
                 FastMethodSource.source_for(method))
    assert_match(/\s+end\n\s+end\n\z/, FastMethodSource.source_for(method))
  end

  def test_source_for_rss_square_brackets
    require 'rss'

    method = REXML::Elements.instance_method(:[])
    assert_match(/def \[\]\( index, name=nil\)\n/, FastMethodSource.source_for(method))
  end

  def test_source_for_core_ext
    require 'fast_method_source/core_ext'

    method = proc { |*args|
      args.first + args.last
    }

    assert_raises(NameError) { Pry }
    assert_raises(NameError) { MethodSource }
    assert_match(/args\.first \+ args\.last/, method.source)
  end

  def test_source_for_mkmf_configuration
    require 'mkmf'

    method = MakeMakefile.instance_method(:configuration)
    assert_match(/CONFIG\["hdrdir"\] \|\|\= \$hdrdir/,
                 FastMethodSource.source_for(method))
  end
end
