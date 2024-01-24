from blazingmq.dev.it.tweaks import tweak, TWEAK_ATTRIBUTE
from blazingmq.dev.configurator import Configurator


def apply_tweaks(configurator, tweaks, stage):
    for (tweak_callable, tweak_stage) in tweaks:
        if tweak_stage == stage:
            tweak_callable(configurator)


@tweak.broker.app_config.logs_observer_max_size(42)
def test_broker_tweak(request):
    tweaks = getattr(getattr(request, "function"), TWEAK_ATTRIBUTE)
    assert len(tweaks) == 1
    configurator = Configurator()
    apply_tweaks(configurator, tweaks, 0)
    assert configurator.proto.broker.app_config.logs_observer_max_size == 42


@tweak.domain.max_consumers(42)
def test_domain_tweak(request):
    tweaks = getattr(getattr(request, "function"), TWEAK_ATTRIBUTE)
    assert len(tweaks) == 1
    configurator = Configurator()
    apply_tweaks(configurator, tweaks, 0)
    assert configurator.proto.domain.max_consumers == 42


@tweak.cluster.queue_operations.open_timeout_ms(42)
def test_cluster_tweak(request):
    tweaks = getattr(getattr(request, "function"), TWEAK_ATTRIBUTE)
    assert len(tweaks) == 1
    configurator = Configurator()
    apply_tweaks(configurator, tweaks, 0)
    assert configurator.proto.cluster.queue_operations.open_timeout_ms == 42
