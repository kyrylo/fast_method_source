require_relative '../helper'

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

  def test_source_for_proc_instance
    method = Proc.new { |*args|
      args.first + args.last
    }

    assert_match(/args\.first \+ args\.last/, FastMethodSource.source_for(method))
  end

  def test_source_for_lambda
    method = lambda { |*args|
      args.first + args.last
    }

    assert_match(/args\.first \+ args\.last/, FastMethodSource.source_for(method))
  end

  def test_source_for_lambda_arrow
    method = -> *args {
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

  def test_source_for_rdoc_setup_parser
    require 'rdoc'

    method = RDoc::Markdown.instance_method(:setup_parser)
    assert_match(/def setup_parser\(str, debug=false\)\n/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_rdoc_escape
    require 'rdoc'

    method = RDoc::Generator::POT::POEntry.instance_method(:escape)
    assert_match(/def escape string\n/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_rss_to_feed
    require 'rss'

    method = RSS::Maker::DublinCoreModel::DublinCoreDatesBase.instance_method(:to_feed)
    assert_match(/@\#{plural}.each do |\#{name}|\n/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_rdoc_find_attr_comment
    require 'rdoc'

    method = RDoc::Parser::C.instance_method(:find_attr_comment)
    assert_match(/@\#{plural}.each do |\#{name}|\n/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_fileutils_cd
    require 'fileutils'

    method = FileUtils::LowMethods.instance_method(:cd)

    # Reports wrong source_location (lineno 1981 instead of 1980).
    # https://github.com/ruby/ruby/blob/a9721a259665149b1b9ff0beabcf5f8dc0136120/lib/fileutils.rb#L1681
    assert_raises(FastMethodSource::SourceNotFoundError) {
      FastMethodSource.source_for(method)
    }
  end

  def test_source_for_process_utime
    method = Process::Tms.instance_method(:utime=)

    # Process::Tms.instance_method(:utime=).source_location
    # #=> ["<main>", nil]
    assert_raises(FastMethodSource::SourceNotFoundError) {
      FastMethodSource.source_for(method)
    }
  end

  def test_source_for_etc_name
    method = Process::Tms.instance_method(:utime=)

    # Etc::Group.instance_method(:name=).source_location
    # #=> ["/opt/rubies/druby-2.2.2/lib/ruby/2.2.0/x86_64-linux/etc.so", nil]
    assert_raises(FastMethodSource::SourceNotFoundError) {
      FastMethodSource.source_for(method)
    }
  end

  def test_source_for_c_method
    method = Array.instance_method(:pop)

    assert_raises(FastMethodSource::SourceNotFoundError) {
      FastMethodSource.source_for(method)
    }
  end
end
