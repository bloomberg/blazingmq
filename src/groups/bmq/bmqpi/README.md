@dir bmqpi

@brief The `BMQPI` (BlazingMQ Public Interfaces) package provides class
interfaces intended to serve as extension points for the BlazingMQ SDK.

The `bmqpi` package provides pure abstract interfaces, which are intended for
clients to extend in their own applications and libraries. These extension
points facilitate integration with other aspects of a runtime environment (e.g.
authentication, host health-checking), which may vary significantly from
organization to organization.
