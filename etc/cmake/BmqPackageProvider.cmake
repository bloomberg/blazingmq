include_guard()

# Configure aliases for different package providers. We only support vcpkg as an alternative to the default (pkg-config).
# Since vcpkg uses real CMake targets in namespaces, we also have to provide an alias to the BdeBuildSystem compatible
# pkg-config name.
macro(setup_package_provider)
    if(DEFINED VCPKG_TOOLCHAIN)
        find_package(bsl CONFIG PATH_SUFFIXES share/bde REQUIRED)
        find_package(bal CONFIG PATH_SUFFIXES share/bde REQUIRED)
        find_package(bdl CONFIG PATH_SUFFIXES share/bde REQUIRED)
        find_package(nts REQUIRED)
        find_package(ntc REQUIRED)

        # vcpkg exports most other targets under a different name than what BdeBuildSystem uses to find
        # targets. Because of this, if we detect that we're using vcpkg to provide targets, we alias them to the
        # pkg-config style names BdeBuildSystem is trying to use.
        find_package(benchmark CONFIG REQUIRED)
        find_package(ZLIB REQUIRED)

        add_library(benchmark ALIAS benchmark::benchmark)
        add_library(zlib ALIAS ZLIB::ZLIB)
    endif()
endmacro()
