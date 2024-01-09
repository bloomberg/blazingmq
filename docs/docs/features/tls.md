# TLS in BlazingMQ

BlazingMQ supports authenticating brokers using TLS.


## Generating Test Certs

```sh
mkdir -p certs && cd certs
../bin/gen-tls-certs.sh ca ca-cert blazingmq
../bin/gen-tls-certs.sh server ca-cert broker_bmqc00_ bmqc00
```
