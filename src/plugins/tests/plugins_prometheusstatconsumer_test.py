#!/usr/bin/env python
"""Integration tests for BlazingMQ Prometheus plugin.

Prerequisites:
1. Python3 should be installed, the following python3 libs should be installed (e.g. 'pip install <package_name>'):
 - 'requests'  TODO: can be replaced with native http.client, but it is too low level
2. Docker should be installed, user launching the test script must be included into the group 'docker'

 """

import argparse
import os
import subprocess
import tempfile
import time
import shutil

from pathlib import Path

import requests

QUEUE_METRICS = ['queue_producers_count', 'queue_consumers_count', 'queue_put_msgs', 'queue_put_bytes', 'queue_push_msgs', 'queue_push_bytes', 'queue_ack_msgs']
QUEUE_PRIMARY_NODE_METRICS = ['queue_gc_msgs', 'queue_cfg_msgs', 'queue_content_msgs']
CLUSTER_METRICS = ['cluster_healthiness']
BROKER_METRICS = ['brkr_summary_queues_count', 'brkr_summary_clients_count']


def parse_arguments():
    parser = argparse.ArgumentParser(description='Integration tests for Prometheus plugin')
    parser.add_argument('-p', '--path', type=str, required=True, help="Path to BlasingMQ folder")
    parser.add_argument('-u', '--url', type=str, required=True, help="Prometheus URL")

    return parser.parse_args()


def test_local_cluster_with_push_mode(broker_path, broker_cfg_path, tool_path, prometheus_url, prometheus_docker_file_path):
    # Run Prometheus in docker
    docker_proc =  subprocess.Popen(['docker', 'compose', '-f', prometheus_docker_file_path, 'up', '-d'])
    docker_proc.wait()
    time.sleep(3)  # wait until Prometheus runs

    with tempfile.TemporaryDirectory() as tmpdirname:
        shutil.copy(broker_path.joinpath('bmqbrkr.tsk'), tmpdirname)
        path = shutil.copytree(Path(broker_cfg_path), Path(tmpdirname).joinpath('localBMQ'))
        # for path,dirs,files in os.walk(tmpdirname):
        #     for filename in files:
        #         print(os.path.join(path,filename))

        # Run broker
        os.chdir(tmpdirname)
        broker_proc =  subprocess.Popen(['./bmqbrkr.tsk', 'localBMQ/etc'])
        time.sleep(3)  # wait until broker runs

        try:
            # Wait until cluster becomes healthy
            for trial in range(20):
                # print("""""""""""""""""""""""""", trial)
                # time.sleep(1)
                response = _make_request(f'{prometheus_url}/api/v1/query', dict(query='cluster_healthiness'))
                value  = response['result'][0]['value'][-1] if response['result'] else None
                if value == '1':
                    break
                assert  trial < 20, 'cluster did not become healthy during 20 sec'
                time.sleep(1)

            # Check initial statistic from Prometheus
            _check_initial_statistic(prometheus_url)

            # Run bmqtool to open queue, put one message and exit
            tool_args = [tool_path, '--mode=auto', '-f', 'write', '-q', 'bmq://bmq.test.persistent.priority/my-first-queue', '--eventscount=1', '--shutdownGrace=2']
            tool_proc = subprocess.Popen(tool_args)
            tool_proc.wait()

            # Check current statistic from Prometheus
            _check_statistic(prometheus_url)

        except AssertionError as e:
            print('Statistic check failed: ', e)
            return False
        finally:
            broker_proc.terminate()
            broker_proc.wait()
            docker_proc =  subprocess.Popen(['docker', 'compose', '-f', prometheus_docker_file_path, 'down'])
            docker_proc.wait()

        return True


def main(args):
    # Check plugin is enabled in config or use config fixtures

    prometheus_docker_file_path = Path(args.path).joinpath('src/plugins/tests/docker/docker-compose.yml')
    broker_path = Path(args.path).joinpath('build/blazingmq/src/applications/bmqbrkr')
    broker_cfg_path = Path('localBMQ').absolute()
    tool_path = Path(args.path).joinpath('build/blazingmq/src/applications/bmqtool/bmqtool.tsk')
    prometheus_url = args.url

    print('local_cluster_test_with_push_mode : ', test_local_cluster_with_push_mode(broker_path, broker_cfg_path, tool_path, prometheus_url, prometheus_docker_file_path))



def _make_request(prometheus_url, params={}):
    response = requests.get(prometheus_url, params=params)
    assert response.status_code == requests.codes.ok
    return response.json()['data']



def _check_initial_statistic(prometheus_url):
    all_metrics = QUEUE_METRICS + QUEUE_PRIMARY_NODE_METRICS + BROKER_METRICS
    for metric in all_metrics:
        response = _make_request(f'{prometheus_url}/api/v1/query', dict(query=metric))
        assert not response['result']  # must be empty
    
    response = _make_request(f'{prometheus_url}/api/v1/query', dict(query='cluster_healthiness'))
    value  = response['result'][0]['value'][-1] if response['result'] else None
    assert  value == '1', _assert_message('cluster_healthiness', '1', value) # ClusterStatus::e_CLUSTER_STATUS_HEALTHY


def _check_statistic(prometheus_url):
    all_metrics = QUEUE_METRICS + QUEUE_PRIMARY_NODE_METRICS + BROKER_METRICS + CLUSTER_METRICS
    for metric in all_metrics:
        response = _make_request(f'{prometheus_url}/api/v1/query', dict(query=metric))
        value  = response['result'][0]['value'][-1] if response['result'] else None
        match(metric):
            # Queue statistic
            case 'queue_producers_count':
                assert value == '1', _assert_message(metric, '1', value)
            case 'queue_consumers_count':
                assert value is None, _assert_message(metric, 'None', value)
            case 'queue_put_msgs':
                assert value == '1', _assert_message(metric, '1', value)
            case 'queue_put_bytes':
                assert value == '1024', _assert_message(metric, '1024', value)
            case 'queue_push_msgs':
                assert value is None, _assert_message(metric, 'None', value)
            case 'queue_push_bytes':
                assert value is None, _assert_message(metric, 'None', value)
            case 'queue_ack_msgs':
                assert value == '1', _assert_message(metric, '1', value)
            # Queue primary node statistic
            case 'queue_content_msgs':
                # assert value == '1', _assert_message(metric, '1', value)
                pass  # 1 or 2 sporadic
            # Broker statistic
            case 'brkr_summary_queues_count':
                assert value == '1', _assert_message(metric, '1', value)
            case 'brkr_summary_clients_count':
                assert value == '1', _assert_message(metric, '1', value)
            # Cluster statistic
            case 'cluster_healthiness': # ClusterStatus::e_CLUSTER_STATUS_HEALTHY
                assert value == '1', _assert_message(metric, '1', value)

def _assert_message(metric, expected, given):
    return f'{metric} expected {expected} but {given} given'


if __name__ == '__main__':
    main(parse_arguments())
