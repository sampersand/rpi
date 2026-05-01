#!/usr/bin/env ruby

$*.clear
$* << __dir__ + '/h' if $*.empty?

$exe = $*.shift or fail

puts "running tests for: #{$exe}"

require "minitest/autorun"

class TestMath < Minitest::Test
  def assert_output(expected, args, chomp: true, compact: false)
    env = {}
    env['COMPACT'] = '1' if compact
    IO.popen(env, [$exe, *args]) do |io|
      out = io.read
      out.chomp! if chomp
      assert_equal expected, out
    end
  rescue
    binding.irb
  end

  def test_single_args
    assert_output '<div>', %w[ div ]
    assert_output '</foo>', %w[ /foo ]
  end

  def test_multiple_args
    assert_output "<div>\n<foo>", %w[ div foo ]
    assert_output "<div>\n<br/>\n<div>", %w[ div br/ div ]
  end

  def test_inner_text
    assert_output '<div>foo</div>', %w[ div -t foo ]
    assert_output "<div>foo</div>\n<div>bar</div>", %w[ div -t foo -tbar ]

    assert_output "<div>foo</div>\n<div>#{File.read(__FILE__)}</div>", %W[ div -t foo -f #{__FILE__} ]
  end

  def test_attributes
    assert_output '<html lang=en>', %w[ html -a lang=en ]
    assert_output '<span t id=foo style="font-style: italic">', %w[ span -at -aid=foo -astyle="font-style:\ italic" ]
    assert_output <<~EOS.chomp, %w[ div -ax -t lol -t hi -a that=good -t here -A -t what ]
    <div x>lol</div>
    <div x>hi</div>
    <div x that=good>here</div>
    <div>what</div>
    EOS
  end

  def test_nested
    assert_output <<~EOS.chomp, %w[ div [ a b c ] ]
    <div>
    \t<a>
    \t<b>
    \t<c>
    </div>
    EOS

    assert_output <<~EOS.chomp, %w[ div -abar [ a b c ] ]
    <div bar>
    \t<a>
    \t<b>
    \t<c>
    </div>
    EOS

    assert_output <<~EOS.chomp, %w[ div [ -T hello -T there -T world ] ]
    <div>
    \thello
    \tthere
    \tworld
    </div>
    EOS

    assert_output <<~EOS.chomp, %w[ div [ -T hello strong -t there -T world ] ]
    <div>
    \thello
    \t<strong>there</strong>
    \tworld
    </div>
    EOS
  end

  def test_compact
    assert_output <<~EOS.chomp, %w[ div [ x y z ] ], compact: true
    <div><x><y><z></div>
    EOS
    assert_output <<~EOS.chomp, %w[ div [ a b c ] ], compact: true
    <div><a> <b><c></div>
    EOS

    assert_output <<~EOS.chomp, %w[ div [ -T hello -T there -T world ] ], compact: true
    <div>hello there world</div>
    EOS

    assert_output <<~EOS.chomp, %w[ div [ -T hello strong -t there -T world ] ], compact: true
    <div>hello <strong>there</strong> world</div>
    EOS
    assert_output <<~EOS.chomp, %W[ div [ -T hello strong -t there -iT ,\sworld! ] ], compact: true
    <div>hello <strong>there</strong>, world!</div>
    EOS

    assert_output <<~EOS.chomp, %w[ div [ -Thello -T w strong -itthere -t what -iT world q -i [ r ] ] span [ -T a -T b ] ], compact: true
    <div>hello w<strong>there</strong> <strong>what</strong>world<q><r></q></div><span>a b</span>
    EOS

    assert_output <<~EOS.chomp, %w[ div [ -Thello -T w strong -itthere -t what -iT world q -i [ r ] ] span [ -T a -T b ] ], compact: false
    <div>
    \thello
    \tw<strong>there</strong>
    \t<strong>what</strong>world<q>
    \t\t<r>
    \t</q>
    </div>
    <span>
    \ta
    \tb
    </span>
    EOS
  end
end
