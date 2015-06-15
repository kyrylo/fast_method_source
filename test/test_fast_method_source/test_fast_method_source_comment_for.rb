require_relative '../helper'

class TestFastMethodSource < Minitest::Test
  def test_comment_for_class
    method = SampleClass.instance_method(:sample_method)
    assert_match(/# Sample method/, FastMethodSource.comment_for(method))
  end

  def test_comment_for_module
    method = SampleModule.instance_method(:sample_method)
    assert_match(/# Sample method/, FastMethodSource.comment_for(method))
  end

  def test_comment_for_proc
    # Heavy rain
    method = proc { |*args|
      args.first + args.last
    }

    assert_match(/# Heavy rain/, FastMethodSource.comment_for(method))
  end

  def test_comment_for_proc_instance
    # Heavy rain
    method = Proc.new { |*args|
      args.first + args.last
    }

    assert_match(/# Heavy rain/, FastMethodSource.comment_for(method))
  end

  def test_comment_for_lambda
    # Life is strange
    method = lambda { |*args|
      args.first + args.last
    }

    assert_match(/# Life is strange/, FastMethodSource.comment_for(method))
  end

  def test_comment_for_lambda_arrow
    # Life is strange
    method = -> *args {
      args.first + args.last
    }

    assert_match(/# Life is strange/, FastMethodSource.comment_for(method))
  end

  def test_comment_for_set_merge
    require 'set'

    method = Set.instance_method(:merge)
    expected = %r{
      \s*
      # Merges the elements of the given enumerable object to the set and\n
      \s*
      # returns self.\n
    }x
    assert_match(expected, FastMethodSource.comment_for(method))
  end

  def test_comment_for_padding
    # Life is strange


    method = -> *args {
      args.first + args.last
    }

    assert_equal "", FastMethodSource.comment_for(method)
  end

  def test_comment_for_no_comment
    method = proc {
      args.first + args.last
    }

    assert_equal '', FastMethodSource.comment_for(method)
  end

  def test_comment_for_kernel_require
    method = Kernel.instance_method(:require)
    assert_match(/##\n  # When RubyGems is required, Kernel#require is replaced/,
                 FastMethodSource.comment_for(method))
  end
end
