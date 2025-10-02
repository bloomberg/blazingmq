---
layout: default
title: Plugins
parent: Features
nav_order: 9
---

# Plugins
{: .no_toc }

* toc
{:toc}

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
           "snapshotInterval": 1,
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

1. Common plugins configuration

   - `snapshotInterval`: represents how often stats are computed by broker
     internally, in seconds (typically every 1s);

2. Prometheus plugin configuration

   - `name`: plugin name, must be "PrometheusStatConsumer";
   - `publishInterval`: Specified as a number of seconds. Must be a multiple of the `snapshotInterval`.
     - in `push` mode: it is the time period (in seconds) to send statistic to Prometheus Push Gateway;
     - in `pull` mode: it is time (in seconds) to update statistic;
   - `host`
     - in `push` mode: Prometheus Push Gateway URL;
     - in `pull` mode: Prometheus exposer (local http server) URL that should be accessible by Prometheus to pull the statistic, usually Host IP address;
   - `port`
     - in `push` mode: Prometheus Push Gateway port, usually 9091;
     - in `pull` mode: Prometheus exposer port that should be accessible by Prometheus to pull the statistic;
   - `mode`: interaction with Prometheus mode: `E_PUSH` or `E_PULL`;

### Build and Run plugin in demo environment
To build Prometheus plugin, pass '--plugins prometheus' argument to the build script, e.g.
```bash
bin/build-ubuntu.sh --plugins prometheus
```
To run plugin in demo environment, perform the following steps:

1. Set plugin configuration:
   - For `push` mode:
   ```json
       {
         "host": "localhost",
         "port": 9091,
         "mode": "E_PUSH"
       }
   ```
   - For `pull` mode:
   ```json
       {
         "host": "localhost",
         "port": 8080,
         "mode": "E_PULL"
       }
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

### Labels
The following labels may be used by each metric, whenever applicable. Refer to each individual metric documentation for restrictions which may apply.

|Label Name|Description|
|----------|-----------|
|Instance|instance id of the broker (e.g. A), or 'default'|
|Cluster|name of the cluster (e.g. bmqc01/A)|
|Domain|name of the domain (e.g. my.domain)|
|Tier|value of the tier (e.g. pd)|
|Queue|name of the queue (e.g. my_queue)|
|Role|role of the broker with regard to this queue (possible values are 'PRIMARY', 'PROXY', 'REPLICA')|
|RemoteHost|name of the 'upstream' node or '_none_'|
|AppId|application ID (e.g. my_app), applicable only in fanout mode, if feature is configured, see 'appIdTagDomains' setting|

### Available BlazingMQ metrics
This section lists all the metrics reported to Prometheus by BlazingMQ brokers. Note that any metric whose value is 0 is not published. The only exception to this rule is queue.heartbeat which is always published.

In this section, report interval refers to how often stats are being published in `push` mode (typically every 30s) and snapshot represents how often stats are computed internally (typically every 1s).

Note that all metrics published to Prometheus are also dumped by BlazingMQ broker on the local machine in a file, which is located at this location:

    <broker_path>/localBMQ/logs/stat.*

These stat files can come in handy when Prometheus is unavailable or when metrics during certain time interval are missing from Prometheus for some reason.

### System metrics
System metrics represent BlazingMQ broker's overall operating system metrics. Every broker reports them, with 'Instance' label which is always set.

#### CPU

|Metric Name|Description|
|-----------|-----------|
|brkr_system_cpu_all|Average snapshot value over report interval window for the total system CPU usage.|
|brkr_system_cpu_sys|Average snapshot value over report interval window for the total CPU usage.|
|brkr_system_cpu_usr|Average snapshot value over report interval window for the total user CPU usage.|

#### Operating System

|Metric Name|Description|
|-----------|-----------|
|brkr_system_mem_res|Maximum snapshot value over report interval window for the resident memory usage.|
|brkr_system_mem_virt|Maximum snapshot value over report interval window for the virtual memory usage.|

#### Memory

|Metric Name|Description|
|-----------|-----------|
|brkr_system_os_pagefaults_minor|Total number of times over the report interval window, a page fault serviced without any I/O activity. In this case, I/O activity is avoided by reclaiming a page frame from the list of pages awaiting reallocation.|
|brkr_system_os_pagefaults_major|Total number of times over the report interval window, a page fault serviced that required I/O activity.|
|brkr_system_os_swaps|Total number of times the process was swapped out of main memory over the report interval window.|
|brkr_system_os_ctxswitch_voluntary|Total number of times, over the report interval window, a context switch resulted because the process voluntarily gave up the processor before its time slice was completed.|
|brkr_system_os_ctxswitch_voluntary|Total number of times, over the report interval window, a context switch resulted because a higher priority process ran, or the current process exceeded its time slice.|

#### Network metrics

|Metric Name|Description|
|-----------|-----------|
|brkr_system_net_local_in_bytes|Total number of 'bytes' received by the broker from local TCP connections over the report interval window.|
|brkr_system_net_local_out_bytes|Total number of 'bytes' sent by the broker to local TCP connections over the report interval window.|
|brkr_system_net_remote_in_bytes|Total number of 'bytes' received by the broker from remote TCP connections over the report interval window.|
|brkr_system_net_remote_out_bytes|Total number of 'bytes' sent by the broker to remote TCP connections over the report interval window.|

### Broker metrics
Broker summary metrics represent high level aggregated view of the activity on that broker. Every broker reports them, with 'Instance' label.

|Metric Name|Description|
|-----------|-----------|
|brkr_summary_queues_count|Maximum snapshot'd value over the report interval window for the number of opened queues (including inactive not yet gc’ed queues).|
|brkr_summary_clients_count|Maximum snapshot'd value over the report interval window for the number of clients connected to the broker.|

### Cluster metrics
Every broker reports this metric for each of the clusters it has created, with the following tags: 'Instance', 'Cluster' and 'Role'. If the 'Role' is 'PROXY', the 'RemoteHost' tag is also set (but can have the value '_none_').

|Metric Name|Description|
|-----------|-----------|
|cluster_healthiness|Represents whether this cluster is healthy, as perceived from this host. If the cluster was considered healthy during the entire report interval, a value of 1 is reported; on the other hand, if the cluster was unhealthy for at least one snapshot during the report interval, a value of 2 is reported. Cluster being considered healthy means different things depending on if the host is a proxy to the cluster or a member of the cluster. If it is a proxy, having an active upstream node is sufficient for it to be healthy. For a member, it means being aware of an active leader, all partitions assigned to an active primary, etc.|
|cluster_partition_cfg_journal_bytes|The configured partition’s journal size, used to compute percentage of resource used. This metric is reported only by the leader node of the cluster, with the bmqCluster and instanceName tags set, and with the host tag set to 'dummy'.|
|cluster_partition_cfg_data_bytes|The configured partition’s data size, used to compute percentage of resource used. This metric is reported only by the leader node of the cluster, with the bmqCluster and instanceName tags set, and with the host tag set to 'dummy'.|

Metrics in the following section are reported only by the leader node of the cluster.

|cluster_partition_cfg_journal_bytes|Configured maximum size bytes of the journal file.|
|cluster_partition_cfg_data_bytes|Configured maximum size bytes of the data file.|
|cluster_csl_cfg_bytes|Configured maximum size bytes of the CSL file.|
|cluster_csl_offset_bytes|The last observed offset bytes in the newest log of the CSL.|
|cluster_csl_write_bytes|The amount of bytes written to the CSL file during the report interval.|
|cluster_csl_replication_time_ns_avg|Time in nanoseconds it took for replication of a new entry in CSL file. Average observed during the report interval.|
|cluster_csl_replication_time_ns_max|Time in nanoseconds it took for replication of a new entry in CSL file. Maximum observed during the report interval.|

### Cluster partitions metrics
The following metrics are reported for each partition, only by the primary of the partition, with the 'Cluster' and 'Instance' tags set. Note that in the following metrics name \<X\> represents the partition id (0 based integer).

|Metric Name|Description|
|-----------|-----------|
|cluster_\<partition name\>_rollover_time|The time (in nanoseconds) it took for the rollover operation of the partition. Note that if more than one rollover happened for the partition during the report interval, then the maximum time is reported. This metric can be used to see how long, but also how often, rollover happens for a partition.|
|cluster_\<partition name\>_replication_time_ns_avg|The average observed time in nanoseconds it took to store a message record at primary and replicate it to a majority of nodes in the cluster.|
|cluster_\<partition name\>_replication_time_ns_max|The maximum observed time in nanoseconds it took to store a message record at primary and replicate it to a majority of nodes in the cluster.|
|cluster_\<partition name\>_journal_offset_bytes|The latest observed offset bytes in the journal file of the partition.|
|cluster_\<partition name\>_journal_outstanding_bytes|The maximum observed outstanding bytes in the journal file of the partition over the report interval. Note that this value is internally updated at every sync point being generated, and therefore is not an absolute exact value, but a rather close estimate.|
|cluster_\<partition name\>_journal_utilization_max|The maximum observed utilization percents of the journal file of the partition.|
|cluster_\<partition name\>_data_offset_bytes|The latest observed offset bytes in the data file of the partition.|
|cluster_\<partition name\>_data_outstanding_bytes|The maximum observed outstanding bytes in the data file of the partition over the report interval. Note that this value is internally updated at every sync point being generated, and therefore is not an absolute exact value, but a rather close estimate.|
|cluster_\<partition name\>_data_utilization_max|The maximum observed utilization percents of the data file of the partition.|
|cluster_\<partition name\>_sequence_number|The latest observed sequence number of the partition.|

### Domain metrics
Domain metrics represent high level metrics related to a domain. Only the leader node of the cluster reports them, with the 'Cluster', 'Domain', 'Tier' and 'Instance' tags set.

|Metric Name|Description|
|-----------|-----------|
|domain_cfg_msgs|The configured domain messages capacity, used to compute percentage of resource used.|
|domain_cfg_bytes|The configured domain bytes capacity, used to compute percentage of resource used.|
|domain_queue_count|The maximum snapshot'd value over the report interval window for the number of opened queues (including inactive not yet gc’ed queues) on that domain.|

### Queue metrics
Queue metrics represent detailed, per queue, metrics. For each of them, the following tags are set: 'Cluster', 'Domain', 'Tier', 'Queue', 'Role' (which value can only be one of 'PRIMARY', 'REPLICA' or 'PROXY') and instanceName. 'AppId' tag could be applied on 'queue.confirm_time_max' and 'queue.queue_time_max' metrics in fanout mode if this feature is configured (see 'appIdTagDomains' setting).

|Metric Name|Description|
|-----------|-----------|
|queue_producers_count|The maximum snapshot'd value over the report interval window for the number of aggregated producers downstream of that broker. When looking at the primary reported stats, this gives the global view.|
|queue_consumers_count|The maximum snapshot'd value over the report interval window for the number of aggregated consumers downstream of that broker. When looking at the primary reported stats, this gives the global view.|
|queue_put_msgs|Total number of 'put' messages for the queue received by this broker from its downstream clients over the report interval window.|
|queue_put_bytes|Total cumulated bytes of all 'put' messages (only application payload, excluding bmq protocol overhead) for the queue received by this broker from its downstream clients over the report interval window.|
|queue_push_msgs|Total number of 'push' messages for the queue received by this broker from its upstream connections over the report interval window.|
|queue_push_bytes|Total cumulated bytes of all 'push' messages (only application payload, excluding bmq protocol overhead) for the queue received by this broker from its upstream connections over the report interval window.|
|queue_ack_msgs|Total number of 'ack' messages (both positive acknowledgments as well as negative ones) for the queue received by this broker from its upstream connections over the report interval window.|
|queue_ack_time_avg|Represent the average time elapsed between when a put message was received by the downstream and when its corresponding ack was received for the messages posted during the report interval window. Note that this metric is only reported at the first hop. This represents the clients perceived 'post' time.|
|queue_ack_time_max|Represent the maximum time elapsed between when a put message was received by the downstream and when its corresponding ack was received for the messages posted during the report interval window. Note that this metric is only reported at the first hop. This represents the clients perceived 'post' time.|
|queue_nack_msgs|Total number of failed 'nack’ (i.e. failed 'ack') generated by this broker, over the report interval window.|
|queue_confirm_msgs|Total number of 'confirm' messages for the queue received by this broker from its downstream clients over the report interval window.|
|queue_confirm_time_avg|This metric is only reported by the first hop, and represent the average time elapsed between when a message is pushed down to the client, and when the client confirms it, for the messages pushed during the report interval window.|
|queue_confirm_time_max|This metric is only reported by the first hop, and represent the maximum time elapsed between when a message is pushed down to the client, and when the client confirms it, for the messages pushed during the report interval window.|
|queue_heartbeat|This metric is always reported for every queue, so that there is guarantee to always (i.e. at any point in time) be a time series containing all the tags that can be leveraged in Prometheus|

Metrics in the following section are reported only by the node primary of the queue.

|Metric Name|Description|
|-----------|-----------|
|queue_gc_msgs|The number of messages which have been garbage collected from the queue (i.e. purged due to TTL expiration) over the report interval window.|
|queue_cfg_msgs|The configured queue messages capacity, used to compute percentage of resource used.|
|queue_cfg_bytes|The configured queue bytes capacity, used to compute percentage of resource used.|
|queue_content_msgs|The maximum snapshot'd value over the report interval window for the number of messages pending in the queue.|
|queue_msgs_utilization_max|Queue messages utilization max: queue_content_msgs max/ queue_cfg_msgs * 100%.|
|queue_content_bytes|The maximum snapshot'd value over the report interval window for the cumulated bytes of all messages pending in the queue.|
|queue_bytes_utilization_max|Queue bytes utilization max: queue_content_bytes max/ queue_cfg_bytes * 100%.|
|queue_queue_time_avg|Represent the average time elapsed between when the message was received by the primary, and when it was pushed downstream to the next hop consumer for the messages pushed during the report interval window. This basically represents how long a message stayed in the queue due to no (usable) consumer available. Note that if the primary has switched between the put and the push, then the reported time only has seconds precision.|
|queue_queue_time_max|Represent the maximum time elapsed between when the message was received by the primary, and when it was pushed downstream to the next hop consumer for the messages pushed during the report interval window. This basically represents how long a message stayed in the queue due to no (usable) consumer available. Note that if the primary has switched between the put and the push, then the reported time only has seconds precision.|
|queue_reject_msgs|Total number of rejected messages for the queue (where the number delivery attempts reaches 0) over the report interval window.|
|queue_nack_noquorum_msgs|Total number of messages NACKed due to primary timing out waiting for quorum replication receipts.|

---
