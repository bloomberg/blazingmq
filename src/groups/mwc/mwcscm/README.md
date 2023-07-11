MWCSCM
======
> The `MWCSCM` package provides versioning information for library components
> in **mwc**.


Description
-----------
The 'mwcscm' package provides versioning information that is incorporated into
every release of the 'mwc' Package Group Library.  This versioning information
is in turn available to all clients of 'mwc' both at run time and by inspection
of appropriate .o files.


Component Synopsis
------------------
Component           | Provides ...
--------------------|-----------------------------------------------------------
`mwcscm_version`    | source control management (versioning) information.
`mwcscm_versiontag` | versioning information for the 'mwc' package group.


Version scheme and manipulation
-------------------------------
The version is composed of 3 parts (a major, a minor and a patch) represented as
an aggregated single integer.  Each part of the version belongs to the 0..99
range.  Main's version is always `999999`, nightlies versions are `99mmdd`
where 'mm' and 'dd' are the two digits numerical representation of respectively
the month and day at which time this nightly was built.

The following snippet of code can be used to retrieve the version:

```c++
#include <mwcscm_version.h>

// Let's assume the current version is {Major: 1, Minor: 23, Path: 45}
int         version    = mwcscm::Version::versionAsInt(); // 12345
const char *versionStr = mwcscm::Version::version();      // BLP_LIB_MWC_1.23.45
```

The following snippet of code can be used to build a version for comparison
(for example to enable functionality based on the library version):

```c++
#include <mwcscm_version.h>
#include <mwcscm_versiontag.h>

if (mwcscm::Version::versionAsInt() < MWC_MAKE_EXT_VERSION(1, 2, 0)) {
    // Using an older version, can't enable the feature
}
```
