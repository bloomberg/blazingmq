"""
blazingmq.dev.it.fixtures

PURPOSE: Provide various fixtures.

Provide a fixture class that creates and populates a directory structure with
scripts for running a cluster consisting of four nodes in two data centers and
two proxies.
"""

# pyright: reportOptionalMemberAccess=false
# pylint: disable=redefined-outer-name
# mypy: disable-error-code="union-attr"

import contextlib
import functools
import itertools
import logging
import os
import re
import shutil
import tempfile
from enum import IntEnum
from pathlib import Path
from typing import Callable, Generator, Iterator, List, Optional, Tuple

import blazingmq.dev.configurator as cfg
import blazingmq.dev.it.process.bmqproc
import blazingmq.dev.it.testconstants as tc
import blazingmq.util.logging as bul
import psutil
import pytest
from blazingmq.dev.it.cluster import Cluster
from blazingmq.dev.it.tweaks import tweak  # pylint: disable=unused-import
from blazingmq.dev.it.tweaks import TWEAK_ATTRIBUTE, Tweak
from blazingmq.dev.it.util import internal_use
from blazingmq.dev.paths import paths
from blazingmq.dev.pytest import PYTEST_LOG_SPEC_VAR
from blazingmq.dev.reserveport import reserve_port
from blazingmq.schemas import mqbcfg, mqbconf

order = pytest.mark.order

logger = logging.LoggerAdapter(logging.getLogger(__name__), {"bmqprocess": "pytest"})
osinfo_logger = logging.LoggerAdapter(
    logging.getLogger(__name__ + ".osinfo"), {"bmqprocess": "pytest"}
)
test_logger = logging.LoggerAdapter(
    logging.getLogger("blazingmq.test"), {"bmqprocess": "pytest"}
)

BROKER_CATEGORY = "blazingmq.tsk.bmqbrkr"
TOOL_CATEGORY = "blazingmq.tsk.bmqtool"


def start_cluster(start=True, wait_leader=True, wait_ready=False):
    def decorator(func):
        setattr(func, "_start_cluster", start)
        setattr(func, "_wait_leader", wait_leader)
        setattr(func, "_wait_ready", wait_ready)
        return func

    return decorator


def _prop(request, property_name):
    try:
        return getattr(request, property_name)
    except AttributeError:
        return None


def get_cluster_param(request, opt, deflt=None):
    for location in (
        _prop(request, "function"),
        _prop(request, "instance"),
        _prop(request, "cls"),
    ):
        if location and hasattr(location, opt):
            return getattr(location, opt)
    return deflt


def get_actual_log_level(config, *setting_names):
    """Return the actual logging level."""

    for setting_name in setting_names:
        log_level = config.getoption(setting_name)
        if log_level is None:
            log_level = config.getini(setting_name)
        if log_level:
            break
    else:
        return

    if isinstance(log_level, str):
        log_level = log_level.upper()
    try:
        return int(getattr(logging, log_level, log_level))
    except ValueError as error:
        # Python logging does not recognize this as a logging level
        raise pytest.UsageError(
            "'{}' is not recognized as a logging level name for "
            "'{}'. Please consider passing the "
            "logging level num instead.".format(log_level, setting_name)
        ) from error


def get_option_ini(config, *names):
    for name in names:
        ret = config.getoption(name) or config.getini(name)
        if ret is not None:
            return ret
    return None


def task_log_params(normalized_levels):
    top_default_level, *category_levels = normalized_levels

    proc_default_level = None
    broker_default_level = None
    broker_category_levels = []

    broker_threshold = min(logging.INFO, top_default_level)
    tool_threshold = min(logging.INFO, top_default_level)

    for category_level in category_levels:
        category, level = category_level

        if level >= logging.INFO:
            # I.e. we want less verbose logging.  We cannot tell
            # the tasks to log less verbosely than INFO, otherwise
            # the various bmqit functions that scan the logs will
            # not work anymore.  This is not a problem, because the
            # unwanted log records will be filtered out by the
            # pytest loggers.
            continue

        if category == "blazingmq.tsk":
            proc_default_level = level
            broker_default_level = broker_default_level or level
            broker_threshold = min(broker_threshold, level)
            tool_threshold = min(tool_threshold, level)
            continue

        if category == BROKER_CATEGORY:
            broker_default_level = level
            broker_threshold = min(broker_threshold, level)
            continue

        if category.startswith(BROKER_CATEGORY + "."):
            broker_threshold = min(broker_threshold, level)
            broker_category_levels.append(
                (category[1 + len(BROKER_CATEGORY) :].upper(), level)
            )
            continue

        if category.split(".")[0] == TOOL_CATEGORY:
            tool_threshold = min(tool_threshold, level)
            continue

    # Prepend a catch-all pair to requested category levels, using the
    # most specific level, or INFO.

    broker_category_levels = [
        (
            "*",
            broker_default_level
            or proc_default_level
            or top_default_level
            or logging.INFO,
        )
    ] + broker_category_levels

    logger.debug(
        "(broker_threshold, broker_category_levels, tool_threshold) = "
        f"{(broker_threshold, broker_category_levels, tool_threshold)}"
    )

    return (broker_threshold, broker_category_levels, tool_threshold)


def cluster_fixture(request, configure) -> Generator:
    # Get a temporary directory and also add a unique suffix to the dir name to
    # further avoid name collision on a machine.

    with contextlib.ExitStack() as on_exit:
        log_file_path = None
        log_file_handler = None
        log_dir = get_option_ini(request.config, "bmq_log_dir")

        if log_dir:
            log_dir = Path(log_dir)
            log_dir.mkdir(parents=True, exist_ok=True)
            log_file_path = re.sub(r"^[^:]+::", "", request.node.nodeid)
            log_file_path = re.sub(r"/", "-", log_file_path)
            log_file_path = (log_dir / (log_file_path + ".log")).resolve()
            log_file_handler = logging.FileHandler(
                log_file_path, mode="w", encoding="UTF-8"
            )
            log_file_format = get_option_ini(
                request.config, "log_file_format", "log_format"
            )
            log_file_date_format = get_option_ini(
                request.config, "log_file_date_format", "log_date_format"
            )
            log_file_formatter = logging.Formatter(
                log_file_format, datefmt=log_file_date_format
            )
            log_file_handler.setFormatter(log_file_formatter)
            logging.getLogger().addHandler(log_file_handler)
            log_file_level = get_actual_log_level(request.config, "log_file_level")
            if log_file_level is not None:
                log_file_handler.setLevel(log_file_level)
                logging.getLogger().setLevel(log_file_level)

            failures = 0

            def remove_log_file_handler():
                logging.getLogger().removeHandler(log_file_handler)
                log_file_handler.close()
                if failures == request.session.testsfailed and not get_option_ini(
                    request.config, "bmq_keep_logs"
                ):
                    try:
                        log_file_path.unlink()
                    except:
                        pass

            on_exit.callback(remove_log_file_handler)

        work_dir = Path(tempfile.mkdtemp())
        logger.info("work_dir = %s", work_dir)
        statvfs = os.statvfs(work_dir)
        osinfo_logger.info(
            "memory in use: {:,} disk free space: {:,} processes: {:,}".format(
                psutil.Process().memory_info().rss,
                statvfs.f_frsize * statvfs.f_bavail,
                len(psutil.pids()),
            )
        )

        def remove_work_dir():
            logger.debug("removing work directory %s", work_dir)
            shutil.rmtree(work_dir)

        if get_option_ini(request.config, "bmq_keep_work_dirs"):
            logger.debug(
                "--bmq-keep-work-dirs specified, will not delete directory %s", work_dir
            )
        else:
            on_exit.callback(remove_work_dir)

        broker_threshold, broker_category_levels, tool_threshold = task_log_params(
            bul.normalize_log_levels(
                request.config.getoption(PYTEST_LOG_SPEC_VAR)
                or request.config.getini(PYTEST_LOG_SPEC_VAR)
            )
        )

        tool_extra_args = []
        tool_extra_args.append(f"--verbosity={logging.getLevelName(tool_threshold)}")

        def check_sequential_tests():
            if int(os.environ.get("PYTEST_XDIST_WORKER_COUNT", "1")) > 1:
                message = "fixed port allocation is incompatible with parallelism"
                logger.error(message)
                raise RuntimeError(message)

        with contextlib.ExitStack() as scope:
            if env_ports := os.environ.get("BMQIT_PORTS"):
                check_sequential_tests()
                logger.info("using ports %s", env_ports)

                def port_pool_allocator():
                    for port in env_ports.split(","):
                        yield int(port)

                    message = "out of ports"
                    logger.error(message)
                    raise RuntimeError(message)

                port_allocator = port_pool_allocator()
            elif env_port_base := os.environ.get("BMQIT_PORT_BASE"):
                check_sequential_tests()
                logger.info("using ports sequentially allocated from %s", env_port_base)
                port_allocator = itertools.count(int(env_port_base))
            else:
                logger.info("allocating ephemeral ports")

                def ephemeral_port_allocator():
                    while True:
                        yield scope.enter_context(reserve_port()).port

                port_allocator = ephemeral_port_allocator()

            extra_cluster_kw_args = {}

            if log_dir:
                extra_cluster_kw_args["copy_cores"] = log_dir

            configurator = cfg.Configurator()
            log_config = configurator.proto.broker.task_config.log_controller
            log_config.logging_verbosity = logging.getLevelName(broker_threshold)
            log_config.console_severity_threshold = logging.getLevelName(
                broker_threshold
            )
            log_config.console_format = blazingmq.dev.it.process.bmqproc.PROC_LOG_FORMAT
            log_config.categories = [
                f"{cat[0]}:{logging.getLevelName(cat[1])}:white".replace(
                    "WARNING", "WARN"
                ).replace("CRITICAL", "FATAL")
                for cat in broker_category_levels
            ]

            def apply_tweaks(stage: int):
                for request_location in "cls", "function", "instance":
                    if request_context := getattr(request, request_location, None):
                        tweaks: List[Tuple[Tweak, bool]] = getattr(request_context, TWEAK_ATTRIBUTE, None)  # type: ignore
                        if tweaks:
                            for (tweak_callable, tweak_stage) in tweaks:
                                if tweak_stage == stage:
                                    tweak_callable(configurator)

            apply_tweaks(0)
            configure(configurator, port_allocator)
            apply_tweaks(1)

            for broker in configurator.brokers.values():
                broker.deploy(cfg.LocalSite(work_dir / broker.name))

            main_cluster: Optional[cfg.Cluster] = None

            for cluster_config in configurator.clusters.values():
                if cluster_config.name == "itCluster":
                    assert main_cluster is None
                    main_cluster = cluster_config

            assert main_cluster is not None

            with Cluster(
                main_cluster,
                configurator,
                work_dir,
                tool_extra_args=tool_extra_args,
                **extra_cluster_kw_args,
            ) as cluster:
                failures = (
                    0 + request.session.testsfailed
                )  # it doesn’t work without the ’0 +’, why?

                try:
                    with internal_use(cluster):
                        logger.debug("starting cluster")

                        if get_cluster_param(request, "_start_cluster", True):
                            with internal_use(cluster):
                                cluster.start(
                                    wait_leader=get_cluster_param(
                                        request, "_wait_leader", True
                                    ),
                                    wait_ready=get_cluster_param(
                                        request, "_wait_ready", False
                                    ),
                                )

                        if request.instance is not None and hasattr(
                            request.instance, "setup_cluster"
                        ):
                            request.instance.setup_cluster(cluster)

                except Exception as initial_exception:
                    logger.warning(
                        "stopping cluster after exception %s in during setup",
                        initial_exception,
                    )

                    try:
                        cluster.stop()
                    finally:
                        raise initial_exception from None

                if request.config.getoption("bmq_break_before_test"):
                    yield from break_before_test(request, cluster)
                else:
                    yield cluster

                logger.debug("teardown")

                with internal_use(cluster):
                    logger.debug("stopping cluster")

                    try:
                        cluster.stop()
                    except:
                        if not get_option_ini(
                            request.config, "bmq_tolerate_dirty_shutdown"
                        ):
                            raise

                logger.debug("teardown complete")


@contextlib.contextmanager
def cluster_context(request, config):
    yield from cluster_fixture(request, config)


def break_before_test(request, cluster):
    capmanager = request.config.pluginmanager.getplugin("capturemanager")
    with capmanager.global_and_fixture_disabled():
        print("\nENTERING PYTHON DEBUGGER")
        print(f"work dir    : {cluster.work_dir}")
        broker_path = str(paths.build_dir / "src/applications/bmqbrkr/bmqbrkr.tsk")
        print(f"broker      : {broker_path}")
        if cluster.last_known_leader is not None:
            print(f"debug leader: gdb {broker_path} {cluster.last_known_leader.pid}")
        if not cluster.is_single_node:
            print("nodes:")
            for broker in cluster.nodes():
                print(f"  {broker.name}:  {broker.pid}", end="")
                if broker is cluster.last_known_leader:
                    print(f" (leader)", end="")
                print()
            print("proxies:")
            for proxy in cluster.proxies():
                print(f"  {proxy.name}: {proxy.pid}")
        breakpoint()
        yield cluster


###############################################################################
# cluster modes


class Mode(IntEnum):
    LEGACY = 0
    CSL = 1
    FSM = 2

    def tweak(self, cluster: mqbcfg.ClusterDefinition):
        cluster.cluster_attributes.is_cslmode_enabled = self >= Mode.CSL
        cluster.cluster_attributes.is_fsmworkflow = self == Mode.FSM

    @property
    def suffix(self) -> str:
        return ["", "_csl", "_fsm"][self]

    @property
    def marks(self):
        return [[], [pytest.mark.csl_mode], [pytest.mark.fsm_mode]][self]


class ProxyConnection:
    pass


class ForwardProxyConnection(ProxyConnection):
    suffix = ""


class ReverseProxyConnection:
    suffix = "_rev"


WorkspaceConfigurator = Callable[..., None]


###############################################################################
# single node cluster


def add_test_domains(cluster: cfg.Cluster):
    for (domain_factory, domain_name, *args) in (
        (cluster.priority_domain, tc.DOMAIN_PRIORITY),
        (cluster.priority_domain, tc.DOMAIN_PRIORITY_SC),
        (cluster.fanout_domain, tc.DOMAIN_FANOUT, tc.TEST_APPIDS),
        (cluster.fanout_domain, tc.DOMAIN_FANOUT_SC, tc.TEST_APPIDS),
        (cluster.broadcast_domain, tc.DOMAIN_BROADCAST),
    ):
        domain: cfg.Domain = domain_factory(domain_name, *args)
        assert domain.definition.parameters is not None
        if domain_name.endswith(".sc"):
            domain.definition.parameters.consistency = mqbconf.Consistency(
                strong=mqbconf.QueueConsistencyStrong()
            )
        else:
            domain.definition.parameters.consistency = mqbconf.Consistency(
                eventual=mqbconf.QueueConsistencyEventual()
            )


def single_node_cluster_config(
    configurator: cfg.Configurator, port_allocator: Iterator[int], mode: Mode
):
    mode.tweak(configurator.proto.cluster)

    broker = configurator.broker(
        name="single",
        tcp_host="localhost",
        tcp_port=next(port_allocator),
        data_center="single_node",
    )

    cluster = configurator.cluster(name="itCluster", nodes=[broker])
    add_test_domains(cluster)


single_node_cluster_params = [
    pytest.param(
        functools.partial(single_node_cluster_config, mode=mode),
        id=f"single_node{mode.suffix}",
        marks=[
            pytest.mark.integrationtest,
            pytest.mark.quick_integrationtest,
            pytest.mark.pr_integrationtest,
            pytest.mark.single,
            *mode.marks,
        ],
    )
    for mode in Mode.__members__.values()
]


@pytest.fixture(params=single_node_cluster_params)
def single_node(request):
    yield from cluster_fixture(request, request.param)


###############################################################################
# multi_node cluster


def multi_node_cluster_config(
    configurator: cfg.Configurator,
    port_allocator: Iterator[int],
    mode: Mode,
    reverse_proxy: bool = False,
) -> None:
    mode.tweak(configurator.proto.cluster)

    cluster = configurator.cluster(
        name="itCluster",
        nodes=[
            configurator.broker(
                name=f"{data_center}{broker}",
                tcp_host="localhost",
                tcp_port=next(port_allocator),
                data_center=data_center,
            )
            for data_center in ("east", "west")
            for broker in ("1", "2")
        ],
    )

    add_test_domains(cluster)

    for data_center in ("east", "west"):
        configurator.broker(
            name=f"{data_center}p",
            tcp_host="localhost",
            tcp_port=next(port_allocator),
            data_center=data_center,
        ).proxy(cluster, reverse=reverse_proxy)


multi_node_cluster_params = [
    pytest.param(
        functools.partial(multi_node_cluster_config, mode=mode),
        id=f"multi_node{mode.suffix}",
        marks=[
            pytest.mark.integrationtest,
            pytest.mark.pr_integrationtest,
            pytest.mark.multi,
            *mode.marks,
        ],
    )
    for mode in Mode.__members__.values()
]


@pytest.fixture(params=multi_node_cluster_params)
def multi_node(request):
    yield from cluster_fixture(request, request.param)


###############################################################################
# single_node + multi_node cluster


@pytest.fixture(params=single_node_cluster_params + multi_node_cluster_params)
def cluster(request):
    yield from cluster_fixture(request, request.param)


# -----------------------------------------------------------------------------
# virtual_cluster_config


def virtual_cluster_config(
    configurator: cfg.Configurator,
    port_allocator: Iterator[int],
    mode: Mode,
    reverse_proxy: bool = False,
) -> None:
    mode.tweak(configurator.proto.cluster)

    final_cluster = configurator.cluster(
        name="itCluster",
        nodes=[
            configurator.broker(
                name=f"{data_center}{broker}",
                tcp_host="localhost",
                tcp_port=next(port_allocator),
                data_center=data_center,
            )
            for data_center in ("east", "west")
            for broker in ("1", "2")
        ],
    )
    add_test_domains(final_cluster)

    cluster = configurator.virtual_cluster(
        name="itVirtualCluster",
        nodes=[
            configurator.broker(
                name=f"{data_center}v",
                tcp_host="localhost",
                tcp_port=next(port_allocator),
                data_center=data_center,
            )
            for data_center in ("east", "west")
        ],
    )
    cluster.proxy(final_cluster)

    for data_center in ("east", "west"):
        configurator.broker(
            name=f"{data_center}p",
            tcp_host="localhost",
            tcp_port=next(port_allocator),
            data_center=data_center,
        ).proxy(cluster, reverse=reverse_proxy)


virtual_cluster_params = [
    pytest.param(
        functools.partial(virtual_cluster_config, mode=mode),
        id=f"virtual_cluster{mode.suffix}",
        marks=[
            pytest.mark.integrationtest,
            pytest.mark.pr_integrationtest,
            pytest.mark.multi,
            *mode.marks,
        ],
    )
    for mode in Mode.__members__.values()
]


@pytest.fixture(params=virtual_cluster_params)
def virtual_cluster(request):
    yield from cluster_fixture(request, request.param)


###############################################################################
# cluster based on an arbitrary config


@pytest.fixture
def cluster_config(request, config):
    yield from cluster_fixture(request, get_cluster_param(request, "_cluster_config"))


###############################################################################
# cluster fixture for all the combinations of three setups:
#    - connect via a virtual cluster
#    - cluster reverse connects to proxies
#    - use CSL mode


cartesian_product_cluster_params = [
    pytest.param(
        functools.partial(config, mode=mode, reverse_proxy=rp_suffix != ""),
        id=f"{topology}{mode.suffix}{rp_suffix}",
        marks=[
            pytest.mark.integrationtest,
            pytest.mark.pr_integrationtest,
            pytest.mark.multi,
            *mode.marks,
        ],
    )
    for topology, config in (
        ("multi_node", multi_node_cluster_config),
        ("virtual", virtual_cluster_config),
    )
    for mode in Mode.__members__.values()
    for rp_suffix in ("", "_rp")
]


@pytest.fixture(params=cartesian_product_cluster_params)
def cartesian_product_cluster(request):
    yield from cluster_fixture(request, request.param)
