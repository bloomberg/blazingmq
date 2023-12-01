#!/usr/bin/env python3
"""Integration tests for BlazingMQ Prometheus plugin.

Test plan:
 - Test Prometheus plugin in 'push' mode:
    - Run Prometheus (in docker);
    - Run broker with local cluster and enabled Prometheus plugin in sandbox (temp folder);
    - Put several messages into different queues;
    - Request metrics from Prometheus and compare them with expected metric values.
 - Test Prometheus plugin in 'pull' mode:
    - Run Prometheus (in docker);
    - Run broker with local cluster and enabled Prometheus plugin in sandbox (temp folder);
    - Put several messages into different queues;
    - Request metrics from Prometheus and compare them with expected metric values.

Prerequisites:
1. bmqbroker, bmqtool and plugins library should be built;
2. Python3 should be installed;
3. Docker should be installed, user launching the test script must be included into the group 'docker'.

Usage: ./prometheus_prometheusstatconsumer_test.py [-h] -p PATH
options:
  -h, --help            show this help message and exit
  -p PATH, --path PATH  path to BlasingMQ build folder
  -m {all,pull,push}, --mode {all,pull,push}
                        prometheus mode
  --no-docker           don't run Prometheus in docker, assume it is running on localhost
 """

import argparse
import http.client
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.parse
from pathlib import Path


QUEUE_METRICS = ['queue_heartbeat', 'queue_producers_count', 'queue_consumers_count', 'queue_put_msgs', 'queue_put_bytes',
                 'queue_push_msgs', 'queue_push_bytes', 'queue_ack_msgs']
QUEUE_PRIMARY_NODE_METRICS = ['queue_gc_msgs',
                              'queue_cfg_msgs', 'queue_content_msgs']
CLUSTER_METRICS = ['cluster_healthiness']
BROKER_METRICS = ['brkr_summary_queues_count', 'brkr_summary_clients_count']

# Must be in sync with docker/docker-compose.yml
PROMETHEUS_HOST = 'localhost:9090'


def parse_arguments():
    parser = argparse.ArgumentParser(
        description='Integration tests for Prometheus plugin')
    parser.add_argument('-p', '--path', type=str, required=True,
                        help="path to BlasingMQ build folder")
    parser.add_argument('-m', '--mode', type=str,
                        choices=['all', 'pull', 'push'], default='all', help="prometheus mode")
    parser.add_argument('--no-docker', action='store_true',
                        help="don't run Prometheus in docker, assume it is running on localhost")

    return parser.parse_args()


def test_local_cluster(plugin_path, broker_path, tool_path, prometheus_host, prometheus_docker_file_path, mode):
    current_dir = os.getcwd()

    # Run Prometheus in docker
    if prometheus_docker_file_path:
        docker_proc = subprocess.Popen(
            ['docker', 'compose', '-f', prometheus_docker_file_path, 'up', '-d'])
        docker_proc.wait()

    # Create sandbox in temp folder
    with tempfile.TemporaryDirectory() as tmpdirname:
        # Copy broker and plugins lib into sandbox
        shutil.copy(plugin_path, tmpdirname)
        shutil.copy(broker_path, tmpdirname)
        # Copy broker config folder into sandbox
        broker_cfg_path = broker_path.parent.joinpath('etc')
        local_cfg_path = shutil.copytree(
            Path(broker_cfg_path), Path(tmpdirname).joinpath('localBMQ/etc'))
        # Create required folders
        os.makedirs(Path(tmpdirname).joinpath('localBMQ/logs'))
        os.makedirs(Path(tmpdirname).joinpath(
            'localBMQ/storage/local/archive'))

        # Edit broker config for given mode
        local_cfg_file = Path(local_cfg_path.joinpath('bmqbrkrcfg.json'))
        with local_cfg_file.open() as f:
            local_cfg = json.load(f)
        local_cfg['taskConfig']['logController']['consoleSeverityThreshold'] = 'ERROR'
        local_cfg['appConfig']['plugins'] = dict(
            libraries=['./'], enabled=['PrometheusStatConsumer'])
        plugins_cfg = local_cfg['appConfig']['stats']['plugins'] = [
            dict(name='PrometheusStatConsumer', publishInterval=1)]

        prometheus_cfg = plugins_cfg[0]['prometheusSpecific'] = dict(
            host='localhost')
        if mode == 'push':
            prometheus_cfg['port'] = 9091
            prometheus_cfg['mode'] = 'E_PUSH'
        elif mode == 'pull':
            prometheus_cfg['port'] = 8080
            prometheus_cfg['mode'] = 'E_PULL'
        else:
            assert False, f'Unexpected mode: {mode}'
        with local_cfg_file.open('w') as f:
            json.dump(local_cfg, f)

        # Run broker
        os.chdir(tmpdirname)
        broker_proc = subprocess.Popen(['./bmqbrkr.tsk', 'localBMQ/etc'])

        try:
            # Wait until broker runs and cluster becomes healthy
            for attempt in range(20):
                response = _make_request(
                    prometheus_host, '/api/v1/query', dict(query='cluster_healthiness'))
                value = response['result'][0]['value'][-1] if response['result'] else None
                if value == '1':
                    break
                assert attempt < 19, 'cluster did not become healthy during 20 sec'
                time.sleep(1)

            # Check initial statistic from Prometheus
            _check_initial_statistic(prometheus_host)

            # Run bmqtool to open queue, put two messages and exit
            tool_args = [tool_path, '--mode=auto', '-f', 'write', '-q', 'bmq://bmq.test.persistent.priority/first-queue',
                         '--eventscount=2', '--shutdownGrace=2', '--verbosity=warning']
            tool_proc = subprocess.Popen(tool_args)
            tool_proc.wait()
            # Run bmqtool to open another queue, put one message and exit
            tool_args = [tool_path, '--mode=auto', '-f', 'write', '-q', 'bmq://bmq.test.persistent.priority/second-queue',
                         '--eventscount=1', '--shutdownGrace=2', '--verbosity=warning']
            tool_proc = subprocess.Popen(tool_args)
            tool_proc.wait()

            # Check current statistic from Prometheus
            _check_statistic(prometheus_host)

        except AssertionError as error:
            print('ERROR: Prometheus metrics check failed: ', error)
            return False
        finally:
            os.chdir(current_dir)
            broker_proc.terminate()
            broker_proc.wait()
            if prometheus_docker_file_path:
                docker_proc = subprocess.Popen(
                    ['docker', 'compose', '-f', prometheus_docker_file_path, 'down'])
                docker_proc.wait()

        return True


def main(args):
    build_dir = Path(args.path).absolute()
    test_folder = Path(__file__).absolute().parent
    prometheus_docker_file_path = None if args.no_docker else test_folder.joinpath(
        'docker/docker-compose.yml')
    broker_path = build_dir.joinpath('src/applications/bmqbrkr/bmqbrkr.tsk')
    tool_path = build_dir.joinpath('src/applications/bmqtool/bmqtool.tsk')
    plugin_path = build_dir.joinpath('src/plugins/libbmqprometheus.so')

    modes = ['push', 'pull'] if args.mode == 'all' else [args.mode]
    results = dict()
    for mode in modes:
        results[f'local_cluster_test_with_{mode}_mode'] = test_local_cluster(
            plugin_path, broker_path, tool_path, PROMETHEUS_HOST, prometheus_docker_file_path, mode)

    print('\n\n\n========================================')
    print(f'{len(results)} integration tests executed with following results:')
    for test, result in results.items():
        print(f'{test} : {"passed" if result else "failed"}')

    return not all(results.values())


def _make_request(host, url, params={}):
    try:
        conn = http.client.HTTPConnection(host)
        url = f'{url}?{urllib.parse.urlencode(params)}' if params else url
        conn.request('GET', url)
        response = conn.getresponse()
        assert response.status == 200
        return json.loads(response.read().decode())['data']
    finally:
        conn.close()


def _check_initial_statistic(prometheus_host):
    all_metrics = QUEUE_METRICS + QUEUE_PRIMARY_NODE_METRICS + BROKER_METRICS
    for metric in all_metrics:
        response = _make_request(
            prometheus_host, '/api/v1/query', dict(query=metric))
        assert not response['result'], _assert_message(
            metric, 'None', response['result'])  # must be empty


def _check_statistic(prometheus_host):
    all_metrics = QUEUE_METRICS + QUEUE_PRIMARY_NODE_METRICS + \
        BROKER_METRICS + CLUSTER_METRICS
    for metric in all_metrics:
        response = _make_request(
            prometheus_host, '/api/v1/query', dict(query=metric))
        value = response['result'][0]['value'][-1] if response['result'] else None
        # Queue statistic
        if metric == 'queue_heartbeat':
            # For first queue
            assert value == '0', _assert_message(metric, '0', value)
            labels = response['result'][0]['metric']
            assert labels['Queue'] == 'first-queue', _assert_message(
                metric, 'first-queue', labels['Queue'])
            # For second queue
            value = response['result'][1]['value'][-1]
            assert value == '0', _assert_message(metric, '0', value)
            labels = response['result'][1]['metric']
            assert labels['Queue'] == 'second-queue', _assert_message(
                metric, 'second-queue', labels['Queue'])
        elif metric == 'queue_producers_count':
            assert value == '1', _assert_message(metric, '1', value)
        elif metric == 'queue_consumers_count':
            assert value is None, _assert_message(metric, 'None', value)
        elif metric == 'queue_put_msgs':
            # For first queue
            assert value == '2', _assert_message(metric, '2', value)
            labels = response['result'][0]['metric']
            assert labels['Queue'] == 'first-queue', _assert_message(
                metric, 'first-queue', labels['Queue'])
            # For second queue
            value = response['result'][1]['value'][-1]
            assert value == '1', _assert_message(metric, '1', value)
            labels = response['result'][1]['metric']
            assert labels['Queue'] == 'second-queue', _assert_message(
                metric, 'second-queue', labels['Queue'])
        elif metric == 'queue_put_bytes':
            # For first queue
            assert value == '2048', _assert_message(metric, '2048', value)
            # For second queue
            value = response['result'][1]['value'][-1]
            assert value == '1024', _assert_message(metric, '1024', value)
        elif metric == 'queue_push_msgs':
            assert value is None, _assert_message(metric, 'None', value)
        elif metric == 'queue_push_bytes':
            assert value is None, _assert_message(metric, 'None', value)
        elif metric == 'queue_ack_msgs':
            # For first queue
            assert value == '2', _assert_message(metric, '2', value)
            # For second queue
            value = response['result'][1]['value'][-1]
            assert value == '1', _assert_message(metric, '1', value)
        # Queue primary node statistic
        elif metric == 'queue_content_msgs':
            # For first queue
            assert value == '2', _assert_message(metric, '2', value)
            # For second queue
            value = response['result'][1]['value'][-1]
            assert value == '1', _assert_message(metric, '1', value)
        # Broker statistic
        elif metric == 'brkr_summary_queues_count':
            assert value == '2', _assert_message(metric, '2', value)
        elif metric == 'brkr_summary_clients_count':
            assert value == '1', _assert_message(metric, '1', value)
        # Cluster statistic
        elif metric == 'cluster_healthiness':  # ClusterStatus::e_CLUSTER_STATUS_HEALTHY
            assert value == '1', _assert_message(metric, '1', value)


def _assert_message(metric, expected, given):
    return f'{metric} expected {expected} but {given} given'


if __name__ == '__main__':
    sys.exit(main(parse_arguments()))
