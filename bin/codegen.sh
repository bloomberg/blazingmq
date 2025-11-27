#!/bin/bash
# This is a codegen script used to generate implementations of the schema format used in BlazingMQ.
#
# This script is not able to be run by non-Bloomberg developers as it relies on internal tooling.

COPYRIGHT="// Copyright 2025 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the \"License\");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an \"AS IS\" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
"

codegen() {
  local dir=$1
  local xsd=$2
  local pkg=$3
  local component=$4

  # Do the codegen
  (cd "${dir}" &&
    "${CODEGEN}" \
      -m msg \
      --noAggregateConversion \
      --noExternalization \
      --noIdent \
      --package "${pkg}" \
      --msgComponent "${component}" \
      "${xsd}")

  local codegen_output=("${dir}/${pkg}_${component}.h" "${dir}/${pkg}_${component}.cpp")
  for file in "${codegen_output[@]}"; do
    # Fixup the copyright notice
    {
      echo "${COPYRIGHT}"
      head -n -7 "${file}"
    } >"${file}.tmp"
    mv "${file}.tmp" "${file}"
    # Format the codegen
    clang-format -i "${file}"
  done
}

main() {
  set -eux

  if [[ -z "${CODEGEN+}" ]]; then
    echo "This script is not able to be run by non-Bloomberg developers as it relies on \
internal tooling. Please open an issue if you are an open-source contributor \
in need of code generation. If you are a Bloomberg developer, please rerun this \
script with the CODEGEN enviornment variable set to the internal codegen tool." >&2
    exit 1
  fi

  case "$1" in
    m_bmqtool)
      codegen ./src/applications/bmqtool bmqtoolcmd.xsd m_bmqtool messages
      ;;
    bmqp_ctrlmsg)
      codegen ./src/groups/bmq/bmqp bmqp_ctrlmsg.xsd bmqp_ctrlmsg messages
      ;;
    bmqstm)
      codegen ./src/groups/bmq/bmqstm bmqstm.xsd bmqstm values
      ;;
    mqbcfg)
      codegen ./src/groups/mqb/mqbcfg mqbcfg.xsd mqbcfg messages
      ;;
    mqbcmd)
      codegen ./src/groups/mqb/mqbcmd mqbcmd.xsd messages
      ;;
    mqbconf)
      codegen ./src/groups/mqb/mqbconfm mqbconf.xsd messages
      ;;
    *)
      echo "Unrecognized/missing codegen target" >&2
      exit 1
      ;;
  esac
}

main "$@"
