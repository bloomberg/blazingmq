from blazingmq.dev.it.process.client import _bool_lower, _build_command


def test_bool_lower():
    assert _bool_lower(True) == "true"
    assert _bool_lower(False) == "false"


def test_build_command():
    assert (
        _build_command("open", {"async": _bool_lower}, {"async_": True})
        == "open async=true"
    )
    assert (
        _build_command(
            "configure",
            {"maxUnconfirmedMessages": None},
            {"max_unconfirmed_messages": 42},
        )
        == "configure maxUnconfirmedMessages=42"
    )
    try:
        _build_command("configure", {}, {"foo": 42})
        assert False
    except:
        pass
