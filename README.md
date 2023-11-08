<p align="center">
  <a href="https://bloomberg.github.io/blazingmq">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="docs/assets/images/blazingmq_logo_label_dark.svg">
      <img src="docs/assets/images/blazingmq_logo_label.svg" width="70%">
    </picture>
  </a>
</p>

---
[![OS Linux](https://img.shields.io/badge/OS-Linux-blue)](#)
[![OS AIX](https://img.shields.io/badge/OS-AIX-blue)](#)
[![OS Solaris](https://img.shields.io/badge/OS-Solaris-blue)](#)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue)](LICENSE)
[![C++](https://img.shields.io/badge/C++-blue)](#)
[![CMake](https://img.shields.io/badge/CMake-blue)](#)
[![MessageQueue](https://img.shields.io/badge/MessageQueue-blue)](#)
[![Documentation](https://img.shields.io/badge/Documentation-blue)](https://bloomberg.github.io/blazingmq)

# BlazingMQ - A Modern, High-Performance Message Queue

[BlazingMQ](https://bloomberg.github.io/blazingmq) is an open source
distributed message queueing framework, which focuses on efficiency,
reliability, and a rich feature set for modern-day workflows.

At its core, BlazingMQ provides durable, fault-tolerant, highly performant, and
highly available queues, along with features like various message routing
strategies (e.g., work queues, priority, fan-out, broadcast, etc.),
compression, strong consistency, poison pill detection, etc.

Message queues generally provide a loosely-coupled, asynchronous communication
channel ("queue") between application services (producers and consumers) that
send messages to one another. You can think about it like a mailbox for
communication between application programs, where 'producer' drops a message in
a mailbox and 'consumer' picks it up at its own leisure. Messages placed into
the queue are stored until the recipient retrieves and processes them. In other
words, producer and consumer applications can temporally and spatially isolate
themselves from each other by using a message queue to facilitate
communication.

BlazingMQ's back-end (message brokers) has been implemented in C++, and client
libraries are available in C++, Java, and Python.

BlazingMQ is an actively developed project and has been battle-tested in
production at Bloomberg for 8+ years.

This repository contains BlazingMQ message broker, BlazingMQ C++ client library
and a BlazingMQ command line tool, while BlazingMQ Java client library can be
found in [this](https://github.com/bloomberg/blazingmq-sdk-java) repository.

---

## Menu

- [Documentation](#documentation)
- [Quick Start](#quick-start)
- [Building](#building)
- [Installation](#installation)
- [Contributions](#contributions)
- [License](#license)
- [Code of Conduct](#code-of-conduct)
- [Security Vulnerability Reporting](#security-vulnerability-reporting)

---

## Documentation

Comprehensive documentation about BlazingMQ can be found
[here](https://bloomberg.github.io/blazingmq).

---

## Quick Start

[This](https://bloomberg.github.io/blazingmq/docs/getting_started/blazingmq_in_action/)
article guides readers to build, install, and experiment with BlazingMQ locally
in a Docker container.

In the
[companion](https://bloomberg.github.io/blazingmq/docs/getting_started/more_fun_with_blazingmq)
article, readers can learn about some intermediate and advanced features of
BlazingMQ and see them in action.

---

## Building

[bin/build-ubuntu.sh](bin/build-ubuntu.sh) and
[bin/build-darwin.sh](bin/build-darwin.sh) build BlazingMQ and its dependencies,
respectively, on Ubuntu 22.04.2 LTS and Darwin 22.6.0. They can serve as a basis
to build BlazingMQ on other systems.

### With vcpkg

There is also support for building BlazingMQ with [vpckg](https://vcpkg.io/en/).

Before attempting to build, you will have to acquire `flex`, `bison`, and `bde-tools` for your system, as vcpkg cannot fetch them. Both `flex` and `bison` can likely be installed through your system's package manager. Clone [`bde-tools`](https://github.com/bloomberg/bde-tools.git), we'll assume `blazingmq/thirdparty/bde-tools` for this guide.

Once the prerequisite tools are installed, you should be able to build BlazingMQ with the following:

```sh
export VCPKG_ROOT=path/to/vcpkg
cmake --preset [preset-name] -DCMAKE_PREFIX_PATH=thirdparty/bde-tools
cmake --build cmake.bld
```

For a list of presets, please look at the `*-vcpkg` configurations in [`CMakePresets.json`](./CMakePresets.json).

---

## Installation

[This](https://bloomberg.github.io/blazingmq/docs/installation/deployment/)
article describes the steps for installing a BlazingMQ cluster in a set of Docker
containers, along with a recommended set of configurations.

---

## Contributions

We welcome your contributions to help us improve and extend this project!

We welcome issue reports [here](../../issues); be sure to choose the proper
issue template for your issue, so that we can be sure you're providing us with
the necessary information.

Before sending a [Pull Request](../../pulls), please make sure you have read
our [Contribution
Guidelines](https://github.com/bloomberg/.github/blob/main/CONTRIBUTING.md).

---

## License

BlazingMQ is Apache 2.0 licensed, as found in the [LICENSE](LICENSE) file.

---

## Code of Conduct

This project has adopted a [Code of
Conduct](https://github.com/bloomberg/.github/blob/main/CODE_OF_CONDUCT.md).
If you have any concerns about the Code, or behavior which you have experienced
in the project, please contact us at opensource@bloomberg.net.

---

## Security Vulnerability Reporting

If you believe you have identified a security vulnerability in this project,
please send an email to the project team at opensource@bloomberg.net, detailing
the suspected issue and any methods you've found to reproduce it.

Please do NOT open an issue in the GitHub repository, as we'd prefer to keep
vulnerability reports private until we've had an opportunity to review and
address them.

---
