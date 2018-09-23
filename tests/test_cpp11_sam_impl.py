import string
import random
import _sam_impl


def ascii_without(size, factor):
    text = ''.join(random.choices(string.ascii_lowercase, k=size))
    text = text.replace(factor, ''.join(random.choices(string.ascii_lowercase, k=len(factor))))
    return text


def to_ints(text):
    return [ord(c) for c in text]


def test_occur():
    factor = 'bcd'

    # Not occur.
    opt = _sam_impl.SamStateOpt()
    opt.online(to_ints(ascii_without(256 * 1024, factor)))
    opt.finalize()

    assert opt.occur_count(to_ints(factor)) == -1
    del opt

    # Do occur.
    expected_occur = 10
    opt = _sam_impl.SamStateOpt()
    for _ in range(expected_occur):
        opt.online(to_ints(ascii_without(256 * 102, factor)))
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