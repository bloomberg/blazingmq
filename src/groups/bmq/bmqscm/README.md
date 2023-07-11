BMQSCM
======
> The `BMQSCM` (BlazingMQ Source Control Management) package provides
> versioning information for library components in **bmq**.


Description
-----------
The 'bmqscm' package provides versioning information that is incorporated into
every release of the 'bmq' Package Group Library.  This versioning information
is in turn available to all clients of 'bmq' both at run time and by inspection
of appropriate .o files.


Component Synopsis
------------------
Component           | Provides ...
--------------------|-----------------------------------------------------------
`bmqscm_version`    | source control management (versioning) information.
`bmqscm_versiontag` | versioning information for the 'bmq' package group.


Version scheme and manipulation
-------------------------------
The version is composed of 3 parts (a major, a minor and a patch) represented as
an aggregated single integer.  Each part of the version belongs to the [0..99]
range.  Main's version is always `999999`, nightlies versions are `99mmdd`
where 'mm' and 'dd' are the two digits numerical representation of respectively
the month and day at which time this nightly was built.

The following snippet of code can be used to retrieve the version:

```c++
#include <bmqscm_version.h>

// Let's assume the current version is {Major: 1, Minor: 23, Path: 45}
int         version    = bmqscm::Version::versionAsInt(); // 12345
const char *versionStr = bmqscm::Version::version();      // BLP_LIB_BMQ_1.23.45
```

The following snippet of code can be used to build a version for comparison (for example to enable functionality based on the library version):

```c++
#include <bmqscm_version.h>
#include <bmqscm_versiontag.h>

if (bmqscm::Version::versionAsInt() < BMQ_MAKE_EXT_VERSION(1, 2, 0)) {
    // Using an older version, can't enable the feature
}
```
