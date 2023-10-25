// Copyright 2022-2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// mwcst_tableutil.cpp -*-C++-*-
#include <mwcst_tableutil.h>

#include <mwcscm_version.h>
#include <mwcst_tableinfoprovider.h>
#include <mwcst_utable.h>

#include <mwcst_value.h>

#include <mwcu_memoutstream.h>

#include <bdlb_stringrefutil.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlsb_fixedmemoutstreambuf.h>

#include <bsl_iomanip.h>
#include <bsls_alignedbuffer.h>

namespace BloombergLP {
namespace mwcu {

// ---------------
// class TableUtil
// ---------------

// CLASS METHODS
int TableUtil::printTable(bsl::ostream& stream, const TableInfoProvider& info)
{
    bdlma::LocalSequentialAllocator<5 * 1024> seqAlloc;

    int numRows         = info.numRows();
    int numHeaderLevels = info.numHeaderLevels();

    if (numHeaderLevels < 1) {
        stream << "Bad numHeaderLevels: " << numHeaderLevels << bsl::endl;

        return -1;
    }

    // Figure out width of each column
    bsl::vector<bsl::vector<int> > columnWidths(numHeaderLevels, &seqAlloc);

    columnWidths[0].resize(info.numColumns(0), 0);
    for (size_t i = 0; i < columnWidths[0].size(); ++i) {
        columnWidths[0][i] = info.getHeaderSize(0, static_cast<int>(i));

        for (int row = 0; row < numRows; ++row) {
            columnWidths[0][i] = bsl::max(
                columnWidths[0][i],
                info.getValueSize(row, static_cast<int>(i)));
        }
    }

    // Figure out widths of additional header cells
    for (size_t levelIdx = 1; levelIdx < columnWidths.size(); ++levelIdx) {
        int               level            = static_cast<int>(levelIdx);
        bsl::vector<int>& levelHeaders     = columnWidths[levelIdx];
        bsl::vector<int>& lastLevelHeaders = columnWidths[levelIdx - 1];

        levelHeaders.resize(info.numColumns(level), 0);

        int lastUpdatedHeader = -1;
        int numSubHeaders     = 0;
        for (size_t col = 0; col < lastLevelHeaders.size(); ++col) {
            int headerIdx = info.getParentHeader(level - 1,
                                                 static_cast<int>(col));
            levelHeaders[headerIdx] += lastLevelHeaders[col];

            if (lastUpdatedHeader == headerIdx) {
                // Add 2 characters for the "| " between columns
                levelHeaders[headerIdx] += 2;
            }
            else if (lastUpdatedHeader != -1) {
                // This is the first subheader of a new header cell.
                // Make sure the last header isn't wider than its subheaders.
                // Widen them if so.
                int headerSize = info.getHeaderSize(level, lastUpdatedHeader);
                if (headerSize > levelHeaders[lastUpdatedHeader]) {
                    int excess = headerSize - levelHeaders[lastUpdatedHeader];
                    int excessPerSubHeader = excess / numSubHeaders;
                    int mod                = excess % numSubHeaders;

                    levelHeaders[lastUpdatedHeader] = headerSize;
                    for (size_t hIdx = 0;
                         static_cast<int>(hIdx) < numSubHeaders;
                         ++hIdx) {
                        // Add 'excessPerSubHeader' to each subHeader, and an
                        // extra '1' to 'mod' of them
                        lastLevelHeaders[col - hIdx - 1] +=
                            excessPerSubHeader +
                            (static_cast<int>(hIdx) < mod ? 1 : 0);
                    }
                }

                numSubHeaders = 0;
            }

            lastUpdatedHeader = headerIdx;
            numSubHeaders++;
        }
    }

    if (info.hasTitle()) {
        // Determine the total width
        int totalWidth = 0;
        for (size_t colIdx = 0; colIdx < columnWidths[0].size(); ++colIdx) {
            if (colIdx > 0) {
                totalWidth += 2;  // To account for the +-
            }
            totalWidth += columnWidths[0][colIdx];
        }

        // Print the title
        stream << "\n" << bsl::setw(totalWidth) << bsl::setfill(' ');
        info.printTitle(stream);

        // Print separating line
        stream << "\n" << bsl::setw(totalWidth) << bsl::setfill('-') << "";
    }

    stream << bsl::setfill(' ');

    // Print the header rows
    for (int level = static_cast<int>(columnWidths.size()) - 1; level >= 0;
         --level) {
        stream << bsl::endl;

        for (size_t i = 0; i < columnWidths[level].size(); ++i) {
            if (i > 0) {
                stream << "| ";
            }

            stream << bsl::setw(columnWidths[level][i]);
            info.printHeader(stream,
                             level,
                             static_cast<int>(i),
                             columnWidths[level][i]);
        }
    }

    // Print separating line
    stream << bsl::endl;
    stream << bsl::setfill('-');
    for (size_t colIdx = 0; colIdx < columnWidths[0].size(); ++colIdx) {
        if (colIdx > 0) {
            stream << bsl::setw(1) << "+-";
        }

        stream << bsl::setw(columnWidths[0][colIdx]) << "";
    }

    stream << bsl::setfill(' ');

    // Print values
    stream << bsl::endl;
    for (int row = 0; row < numRows; ++row) {
        for (size_t col = 0; col < columnWidths[0].size(); ++col) {
            if (col > 0) {
                stream << bsl::setw(1) << "| ";
            }

            stream << bsl::setw(columnWidths[0][col]);
            info.printValue(stream,
                            row,
                            static_cast<int>(col),
                            columnWidths[0][col]);
        }
        stream << bsl::endl;
    }

    return 0;
}

int TableUtil::outputToVector(bsl::vector<bsl::vector<bsl::string> >* dest,
                              const TableInfoProvider&                info)
{
    bsls::AlignedBuffer<1024>   buf;
    bdlsb::FixedMemOutStreamBuf sb(buf.buffer(), sizeof(buf));
    bsl::ostream                stream(&sb);

    bsl::vector<bsl::vector<bsl::string> >& out = *dest;

    int numRows    = info.numRows();
    int numColumns = info.numColumns(0);

    out.resize(numRows + 1);

    // Output header
    out[0].resize(numColumns);
    for (int col = 0; col < numColumns; ++col) {
        stream.seekp(0);
        stream.clear();
        info.printHeader(stream, 0, col, 0);
        stream.flush();

        out[0][col].assign(sb.data(), sb.length());
    }

    // Output rows
    for (int row = 0; row < numRows; ++row) {
        out[row + 1].resize(numColumns);
        for (int col = 0; col < numColumns; ++col) {
            stream.seekp(0);
            stream.clear();
            info.printValue(stream, row, col, 0);
            stream.flush();

            out[row + 1][col].assign(sb.data(), sb.length());
        }
    }

    return 0;
}

void TableUtil::printCsv(bsl::ostream& stream, const Table& table)
{
    int numRows    = table.numRows();
    int numColumns = table.numColumns();

    for (int col = 0; col < numColumns; ++col) {
        if (col != 0) {
            stream << ',';
        }
        stream << table.columnName(col);
    }
    stream << '\n';

    // Output rows
    mwct::Value value;
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numColumns; ++col) {
            if (col != 0) {
                stream << ',';
            }
            table.value(&value, row, col);
            stream << value;
        }
        stream << '\n';
    }
}

void TableUtil::printCsv(bsl::ostream& stream, const TableInfoProvider& info)
{
    int numRows    = info.numRows();
    int numColumns = info.numColumns(0);

    // Output header
    for (int col = 0; col < numColumns; ++col) {
        if (0 < col) {
            stream << ',';
        }
        info.printHeader(stream, 0, col, 0);
    }
    stream << '\n';

    // Output rows

    bdlma::LocalSequentialAllocator<128> bsa;
    mwcu::MemOutStream                   ostream(&bsa);

    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numColumns; ++col) {
            if (0 < col) {
                stream << ',';
            }

            // Print each value to 'ostream' first, so that we may remove the
            // level padding added by the info provider.

            ostream.reset();
            info.printValue(ostream, row, col, 0);

            stream << bdlb::StringRefUtil::trim(ostream.str());
        }
        stream << '\n';
    }
}

}  // close package namespace
}  // close enterprise namespace
