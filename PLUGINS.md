# BlazingMQ plugins

## Prometheus plugin

### Overview
This plugin gathers BlazingMQ broker statistic and sends it to [Prometheus](https://prometheus.io/) monitoring system. Both `push` and `pool` modes of interaction with Prometheus are supported.

### Configuration
By default, plugin is disabled. To enable and configure it, edit `bmqbrkcfg.json` file as follows:
1. Enable plugin and provide path to plugin library
```json
"appConfig": {
    ...

    "plugins": {
        "libraries": ["<path-to-prometheus-plugin-library-folder>"],
        "enabled": ["PrometheusStatConsumer"]
    }
}
```
2. Provide plugin configuration
```json
"appConfig": {
    "stats": {
        "plugins": [
            ...
            {
                "name": "PrometheusStatConsumer",
                "publishInterval": 10,
                "prometheusSpecific": {
                    "host": "localhost",
                    "port": 9091,
                    "mode": "E_PUSH"    
                }
            }
        ],
        ...
    }
}
```
where
- `name`: plugin name, must be "PrometheusStatConsumer";
- `publishInterval` 
  - in `push` mode: it is the time period (in seconds) to send statistic to Prometheus Push Gateway;
  - in `pull` mode: it is time (in seconds) to update statistic;
- `host`
  - in `push` mode: Prometheus Push Gateway URL;
  - in `pull` mode: Prometheus exposer (local http server) URL that should be accessable by Prometheus to pull the statistic, usually Host IP address;
- `port`
  - in `push` mode: Prometheus Push Gateway port, usually 9091;
  - in `pull` mode: Prometheus exposer port that should be accessable by Prometheus to pull the statistic;
- `mode`: interaction with Prometheus mode: `E_PUSH` or `E_PULL`;

### Build and Run plugin in demo environment
To build Prometheus plugin, pass '--plugins prometheus' argument to the build script, e.g.
```bash
bin/build-ubuntu.sh --plugins prometheus
```
To run plugin in demo environment, perform the following steps:
1. Set plugin configuration:
  - For `push` mode:
  ```
    "host": "localhost",
    "port": 9091,
    "mode": "E_PUSH"
  ```
  - For `pull` mode:
  ```
    "host": "localhost",
    "port": 8080,
    "mode": "E_PULL"
  ```
2. Run BlazingMQ broker, it will automatically load and configure the plugin;
3. Run Prometheus and Grafana services in Docker:
```bash
docker compose -f docker/plugins/prometheus/docker-compose.yml up
```
4. In browser open link `http://localhost:9090/` with Prometheus UI to analyze available metrics;
5. [Optional] In browser open link `http://localhost:3000/` with Grafana UI to analyze available metrics:
  - Select `Prometrheus` data source;
  - Set `http://prometheus:9090` as Prometheus server URL;

### Integration tests
Test plan:
1. Test Prometheus plugin in 'push' mode:
  - Run Prometheus (in docker);
  - Run broker with local cluster and enabled Prometheus plugin in sandbox (temp folder);
  - Put several messages into different queues;
  - Request metrics from Prometheus and compare them with expected metric values.
2. Test Prometheus plugin in 'pull' mode:
  - Run Prometheus (in docker);
  - Run broker with local cluster and enabled Prometheus plugin in sandbox (temp folder);
  - Put several messages into different queues;
  - Request metrics from Prometheus and compare them with expected metric values.

Prerequisites:
1. bmqbroker, bmqtool and prometheus plugin library should be built;
2. Python3 should be installed;
3. Docker should be installed, user launching the test script must be included into the group 'docker'.
```bash
Usage: ./src/plugins/prometheus/tests/prometheus_prometheusstatconsumer_test.py [-h] -p PATH
options:
  -h, --help            show this help message and exit
  -p PATH, --path PATH  path to BlazingMQ build folder, e.g. './build/blazingmq'
```

### Available BlazingMQ metrics
#### System metrics
|Metric Name|Description|
|-----------|-----------|
|brkr_system_cpu_all|All CPU usage|
|brkr_system_cpu_sys|System CPU usage|
|brkr_system_cpu_usr|User CPU usage|
|brkr_system_mem_res|Resident memory|
|brkr_system_mem_virt|Virtual memory|
|brkr_system_os_pagefaults_minor|Minor page faults|
|brkr_system_os_pagefaults_major|Major page faults|
|brkr_system_os_swaps|Swaps number|
|brkr_system_os_ctxswitch_voluntary|Voluntary context switches|
|brkr_system_os_ctxswitch_voluntary|Involuntary context switches|

#### Network metrics
|Metric Name|Description|
|-----------|-----------|
|brkr_system_net_local_in_bytes|Local network input bytes|
|brkr_system_net_local_out_bytes|Local network output bytes|
|brkr_system_net_remote_in_bytes|Remote network input bytes|
|brkr_system_net_remote_out_bytes|Remote network output bytes|

#### Broker metrics
|Metric Name|Description|
|-----------|-----------|
|brkr_summary_queues_count|Summary queues count|
|brkr_summary_clients_count|Summary clients count|

#### Cluster metrics
|Metric Name|Description|
|-----------|-----------|
|cluster_healthiness|Cluster healthiness|

#### Cluster partitions metrics
|Metric Name|Description|
|-----------|-----------|
|cluster_<partition name>_rollover_time|Partition rollover time|
|cluster_<partition name>_journal_outstanding_bytes|Partition journal outstanding bytes|
|cluster_<partition name>_data_outstanding_bytes|Partition data outstanding bytes|

#### Domain metrics
|Metric Name|Description|
|-----------|-----------|
|domain_cfg_msgs|Messages number|
|domain_cfg_bytes|Bytes number|

#### Queue metrics
|Metric Name|Description|
|-----------|-----------|
|queue_producers_count|Producers number|
|queue_consumers_count|Consumers number|
|queue_put_msgs|PUT messages number|
|queue_put_bytes|PUT messages bytes|
|queue_push_msgs|PUSH messages number|
|queue_push_bytes|PUSH messages bytes|
|queue_ack_msgs|ACK messages number|
|queue_ack_time_avg|ACK average time|
|queue_ack_time_max|ACK max time|
|queue_nack_msgs|NACK messages number|
|queue_confirm_msgs|CONFIRM messages number|
|queue_confirm_time_avg|CONFIRM average time|
|queue_confirm_time_max|CONFIRM max time|
|queue_heartbeat|Queue heartbeat, always zero|

For primary primary node only
|Metric Name|Description|
|-----------|-----------|
|queue_gc_msgs|GC messages number|
|queue_cfg_msgs|Config messages number|
|queue_cfg_bytes|Config messages bytes|
|queue_content_msgs|Content messages number|
|queue_content_bytes|Content messages bytes|
|queue_queue_time_avg|Queue time average|
|queue_queue_time_max|Queue time max|
|queue_reject_msgs|Rejected messages number|
|queue_nack_noquorum_msgs|NACK noquorum messages number|
