#!/usr/bin/env bash

# This script can be used to build BlazingMQ, and all of its transitive
# dependencies (up to and including the standard library) using:
#   - Clang
#   - LLVM libc++ standard library
#   - A CMake toolchain file specific for instrumented build
# It is currently used to build instrumented BlazingMQ binaries for CI for all
# Clang sanitizers (i.e. Address/Leak, Memory, Thread, UndefinedBehavior).
#
# It performs the following:
# 1) Install clang compiler.
# 2) Download llvm-project required for libc++ instrumentation.
# 3) Download external dependencies required for instrumentation.
# 4) Build libc++ with the instrumentation specified by <LLVM Sanitizer Name>.
# 5) Build sanitizer-instrumented dependencies including BDE, NTF, GoogleTest,
#    Google Benchmark and zlib.
# 6) Build sanitizer-instrumented BlazingMQ unit tests.
# 7) Generate scripts to run unit tests:
#      ./cmake.bld/Linux/run-unittests.sh
#    This script is used as-is by CI to run unit tests with sanitizer.

set -eux



# :: Required arguments :::::::::::::::::::::::::::::::::::::::::::::::::::::::
if [ -z ${1} ]; then
    echo 'Error: Missing sanitizer name.' >&2
    echo '  (Usage: build_sanitizer.sh <sanitizer-name>)' >&2
    exit 1
fi


SANITIZER_NAME="${1}"



DIR_ROOT="${PWD}"
DIR_SCRIPTS="${DIR_ROOT}/.github/workflows/sanitizers"
DIR_EXTERNAL="${DIR_ROOT}/deps"
DIR_SRCS_EXT="${DIR_EXTERNAL}/srcs"
DIR_BUILD_EXT="${DIR_EXTERNAL}/cmake.bld"

DIR_SRC_BMQ="${DIR_ROOT}"
DIR_BUILD_BMQ="${DIR_SRC_BMQ}/cmake.bld/Linux"


# Parse sanitizers config
cfgquery() {
    jq "${1}" "${DIR_SCRIPTS}/sanitizers.json" --raw-output
}

# :: Create testing scripts :::::::::::::::::::::::::::::::::::::::::::::::::::
envcfgquery() {
    # Parses the '<build-name>.environment' object from 'sanitizers.json',
    # and outputs a string of whitespace-separated 'VAR=VAL' pairs intended to
    # be used to set the environment for a command.
    #   e.g. 'asan' -> 'ASAN_OPTIONS="foo=bar:baz=baf" LSAN_OPTIONS="abc=fgh"'
    #
    echo $(cfgquery "                        \
            .${1}.environment |              \
            to_entries |                     \
            map(\"\(.key)=\\\"\(.value |     \
                to_entries |                 \
                map(\"\(.key)=\(.value)\") | \
                join(\":\"))\\\"\") |        \
            join(\" \")") |
        sed "s|%%SRC%%|$(realpath ${DIR_SRC_BMQ})|g" |
        sed "s|%%ROOT%%|$(realpath ${DIR_ROOT})|g"
}

mkscript() {
    local cmd=${1}
    local outfile=${2}

    echo '#!/usr/bin/env bash' > ${outfile}
    echo "${cmd}" >> ${outfile}
    chmod +x ${outfile}
}

SANITIZER_ENV="BMQ_BUILD=$(realpath ${DIR_BUILD_BMQ}) "
SANITIZER_ENV+="BMQ_REPO=${DIR_SRC_BMQ} "
SANITIZER_ENV+="$(envcfgquery ${SANITIZER_NAME})"

# 'run-env.sh' runs a command with environment required of the sanitizer.
mkscript "${SANITIZER_ENV} \${@}" "${DIR_BUILD_BMQ}/blazingmq/run-env.sh"

# 'run-unittests.sh' runs all instrumented unit-tests.
CMD="cd $(realpath ${DIR_SRC_BMQ}) && "
CMD+="${DIR_BUILD_BMQ}/blazingmq/run-env.sh ctest -E mwcsys_executil.t --output-on-failure"
mkscript "${CMD}" "${DIR_BUILD_BMQ}/blazingmq/run-unittests.sh"

cat "${DIR_BUILD_BMQ}/blazingmq/run-env.sh"
cat "${DIR_BUILD_BMQ}/blazingmq/run-unittests.sh"
