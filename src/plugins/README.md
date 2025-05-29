# Plugins
This package contains some out of the box plugins.

## Authenticator
- bmqauthnfail: an testing authenticator plugin that fails no matter what input is given.
- bmqauthnpass: an testing authenticator plugin that passes no matter what input is given.

## StatConsumer
- bmqprometheus: a statconsumer plugin that gathers BlazingMQ broker statistic and sends it to Prometheus monitoring system. Both push and pool modes of interaction with Prometheus are supported.
