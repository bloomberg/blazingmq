from blazingmq.dev.it.logging import *


def test_clip():
    assert clip("foo", 6) == "foo   "
    assert clip("foobarbaz", 6) == "*arbaz"
