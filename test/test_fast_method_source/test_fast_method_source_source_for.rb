require_relative '../helper'

class TestFastMethodSource < Minitest::Test
  def test_source_for_class
    method = SampleClass.instance_method(:sample_method)
    expected = "  def sample_method\n    :sample_method\n  end\n"
    assert_equal expected, FastMethodSource.source_for(method)
  end

  def test_source_for_module
    method = SampleModule.instance_method(:sample_method)
    expected = "  def sample_method\n    :sample_method\n  end\n"
    assert_equal expected, FastMethodSource.source_for(method)
  end

  def test_source_for_proc
    method = proc { |*args|
      args.first + args.last
    }

    expected = "    method = proc { |*args|\n      args.first + args.last\n    }\n"
    assert_equal expected, FastMethodSource.source_for(method)
  end

  def test_source_for_proc_instance
    method = Proc.new { |*args|
      args.first + args.last
    }

    expected = "    method = Proc.new { |*args|\n      args.first + args.last\n    }\n"
    assert_equal expected, FastMethodSource.source_for(method)
  end

  def test_source_for_lambda
    method = lambda { |*args|
      args.first + args.last
    }

    expected = "    method = lambda { |*args|\n      args.first + args.last\n    }\n"
    assert_equal expected, FastMethodSource.source_for(method)
  end

  def test_source_for_lambda_arrow
    method = -> *args {
      args.first + args.last
    }

    expected = "    method = -> *args {\n      args.first + args.last\n    }\n"
    assert_equal expected, FastMethodSource.source_for(method)
  end

  def test_source_for_rss_image_favicon
    require 'rss'

    method = RSS::ImageFaviconModel::ImageFavicon.instance_method(:dc_date_elements)
    expected = %r{klass.install_have_children_element\(name, DC_URI, \".+\n\s+full_name, full_plural_name\)}
    assert_match expected, FastMethodSource.source_for(method)
  end

  def test_source_for_erb_scan
    require 'erb'

    method = ERB::Compiler::SimpleScanner2.instance_method(:scan)
    expected = /def scan\n.+yield\(scanner.+end\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_irb_change_workspace
    require 'irb'

    method = IRB::ExtendCommandBundle.instance_method(:irb_change_workspace)
    expected = /def \#{cmd_name}\(\*opts, &b\).+\*opts, &b\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_rss_time_w3cdtf
    require 'rss'

    method = Time.instance_method(:w3cdtf)
    expected = /def w3cdtf\n      if usec.zero\?\n.+end\n\z/m
    assert_match(expected,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_kernel_require
    method = Kernel.instance_method(:require)
    expected = /def require path\n    RUBYGEMS_ACTIVATION_MONITOR.enter\n.+raise load_error\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_irb_init_config
    require 'irb'

    method = IRB.singleton_class.instance_method(:init_config)
    expected = /def IRB.init_config\(ap_path\)\n    # class instance variables\n.+@CONF\[:DEBUG_LEVEL\] = 0\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_rss_def_classed_element
    require 'rss'

    method = RSS::Maker::Base.singleton_class.instance_method(:def_classed_element)
    expected = /def def_classed_element\(name, class_name=nil, attribute_name=nil\)\n.+attr_reader name\n\s+end\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_rss_square_brackets
    require 'rss'

    method = REXML::Elements.instance_method(:[])
    expected = /def \[\]\( index, name=nil\)\n.+return nil\n.+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_core_ext
    require 'fast_method_source/core_ext'

    method = proc { |*args|
      args.first + args.last
    }

    assert_raises(NameError) { Pry }
    assert_raises(NameError) { MethodSource }

    expected = "    method = proc { |*args|\n      args.first + args.last\n    }\n"
    assert_equal expected, method.source
  end

  def test_source_for_mkmf_configuration
    require 'mkmf'

    method = MakeMakefile.instance_method(:configuration)
    expected = /def configuration\(srcdir\)\n.+mk\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_rdoc_setup_parser
    require 'rdoc'

    method = RDoc::Markdown.instance_method(:setup_parser)
    expected = /def setup_parser\(str, debug=false\)\n.+setup_foreign_grammar\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_rdoc_escape
    require 'rdoc'

    method = RDoc::Generator::POT::POEntry.instance_method(:escape)
    expected = /def escape string\n.+end\n    end\n  end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_rss_to_feed
    require 'rss'

    method = RSS::Maker::DublinCoreModel::DublinCoreDatesBase.instance_method(:to_feed)
    expected = /def to_feed\(\*args\)\n.+end\n\s+end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_rdoc_find_attr_comment
    require 'rdoc'

    method = RDoc::Parser::C.instance_method(:find_attr_comment)
    expected = /def find_attr_comment var_name, attr_name, read.+RDoc::Comment.new comment, @top_level\n  end\n\z/m
    assert_match(expected, FastMethodSource.source_for(method))
  end

  def test_source_for_fileutils_cd
    require 'fileutils'

    method = FileUtils::LowMethods.instance_method(:cd)

    # Reports wrong source_location (lineno 1981 instead of 1980).
    # https://github.com/ruby/ruby/blob/a9721a259665149b1b9ff0beabcf5f8dc0136120/lib/fileutils.rb#L1681
    expected = /:commands, :options, :have_option\?, :options_of, :collect_method\]\n\n/
    assert_match expected, FastMethodSource.source_for(method)
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

  def test_source_for_accessor
    method = SampleClass.instance_method(:accessor)
    assert_match(/attr_accessor :accessor\n\z/, FastMethodSource.source_for(method))
  end

  def test_source_for_multiline_reader
    method = SampleClass.instance_method(:reader2)
    assert_match(/attr_reader :reader1,\n\s*:reader2\n\z/,
                 FastMethodSource.source_for(method))
  end

  def test_source_for_rss_author
    require 'rss'

    # source_location returns bad line number (complete nonsense) and
    # method_source raises an exception when tries to parse this. In case of
    # Fast Method Source, it returns the closest method. Let's just make sure
    # here that it returns at least something.
    method = RSS::Atom::Feed::Entry.instance_method(:author=)
    assert_equal false, FastMethodSource.source_for(method).empty?
  end
end
