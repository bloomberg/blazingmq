from blazingmq.dev.it.tweaks import tweak, TWEAK_ATTRIBUTE
from blazingmq.dev.workspace import Workspace


def apply_tweaks(workspace, tweaks, stage):
    for (tweak_callable, tweak_stage) in tweaks:
        if tweak_stage == stage:
            tweak_callable(workspace)


@tweak.broker.app_config.logs_observer_max_size(42)
def test_broker_tweak(request):
    tweaks = getattr(getattr(request, "function"), TWEAK_ATTRIBUTE)
    assert len(tweaks) == 1
    workspace = Workspace()
    apply_tweaks(workspace, tweaks, 0)
    assert workspace.proto.broker.app_config.logs_observer_max_size == 42


@tweak.domain.max_consumers(42)
def test_domain_tweak(request):
    tweaks = getattr(getattr(request, "function"), TWEAK_ATTRIBUTE)
    assert len(tweaks) == 1
    workspace = Workspace()
    apply_tweaks(workspace, tweaks, 0)
    assert workspace.proto.domain.max_consumers == 42


@tweak.cluster.queue_operations.open_timeout_ms(42)
def test_cluster_tweak(request):
    tweaks = getattr(getattr(request, "function"), TWEAK_ATTRIBUTE)
    assert len(tweaks) == 1
    workspace = Workspace()
    apply_tweaks(workspace, tweaks, 0)
    assert workspace.proto.cluster.queue_operations.open_timeout_ms == 42
