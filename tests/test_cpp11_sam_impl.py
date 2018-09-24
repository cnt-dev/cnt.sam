import string
import random
import numpy
from cnt.sam import _sam_impl


def ascii_without(size, factor):
    other = ''.join(set(string.ascii_lowercase) - set(factor))
    fill = ''.join(random.choices(other, k=len(factor)))

    text = ''.join(random.choices(string.ascii_lowercase, k=size))
    text = text.replace(factor, fill)
    return other + text + other


def to_ints(text):
    return [ord(c) for c in text]


def test_occur():
    factor = 'bcd'

    # Not occur.
    opt = _sam_impl.SamStateOpt()
    opt.online(to_ints(ascii_without(256 * 100, factor)))
    opt.finalize()

    assert opt.occur_count(to_ints(factor)) == -1
    del opt

    # Do occur.
    expected_occur = 10
    opt = _sam_impl.SamStateOpt()
    for _ in range(expected_occur):
        opt.online(to_ints(ascii_without(256 * 10, factor)))
        opt.online(to_ints(factor))
    opt.finalize()

    assert opt.occur_count(to_ints(factor)) == expected_occur


def test_occur_with_limit():
    factor = 'bcd'

    # Do occur with limit.
    expected_occur = 100
    opt = _sam_impl.SamStateOpt()
    for _ in range(expected_occur):
        opt.online(to_ints(ascii_without(256 * 102, factor)), maxlen_limit=3)
        opt.online(to_ints(factor), maxlen_limit=3)
    opt.finalize()

    assert opt.occur_count(to_ints(factor)) == expected_occur


def test_occur_degree():
    opt = _sam_impl.SamStateOpt()
    opt.online(to_ints('ab' * 10**5))
    opt.online(to_ints('xyz'))
    opt.finalize()

    numpy.testing.assert_almost_equal(opt.occur_degree(to_ints('ab')), 2.0, decimal=3)
    numpy.testing.assert_almost_equal(opt.occur_degree(to_ints('xyz'), cap=100.0), 100.0)


def test_out_count():
    opt = _sam_impl.SamStateOpt()
    opt.online(to_ints('ababacacad'))
    opt.finalize()

    assert opt.out_count(to_ints('a')) == 3
    assert opt.out_count(to_ints('b')) == 1
    assert opt.out_count(to_ints('c')) == 1
    assert opt.out_count(to_ints('d')) == 0


def test_out_degree():
    opt = _sam_impl.SamStateOpt()
    opt.online(to_ints('abc' * (3 * 10**5)))
    opt.online(to_ints('abe' * (2 * 10**5)))
    opt.finalize()

    numpy.testing.assert_almost_equal(opt.out_degree(to_ints('ab'), ord('c')), 0.6)
