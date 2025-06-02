// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqtsk_consoleobserver.cpp                                         -*-C++-*-
#include <bmqtsk_consoleobserver.h>

#include <bmqscm_version.h>

#include <bmqu_memoutstream.h>
#include <bmqu_stringutil.h>

// BDE
#include <ball_loggermanager.h>
#include <ball_record.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_cstddef.h>
#include <bsl_cstdio.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslmt_mutexassert.h>
#include <bsls_assert.h>

// SYSTEM
#include <unistd.h>  // for tty detection

namespace BloombergLP {
namespace bmqtsk {

namespace {
const char k_COLOR_RESET[] = "\033[0m";
// Code to reset coloring to normal

/// Struct representing color information
const struct Color {
    const char* d_name_p;  // Name of the color
    int         d_fgCode;  // Code for the foreground color
    int         d_bgCode;  // Code for the background color
} k_COLORS[] = {
    {"black", 30, 40},
    {"red", 31, 41},
    {"green", 32, 42},
    {"yellow", 33, 43},
    {"blue", 34, 44},
    {"magenta", 35, 45},
    {"cyan", 36, 46},
    {"gray", 37, 47},
    {"white", 97, 107},
    {0, 0, 0}  // Last
};

const char k_COLOR_WARN[] = "bold-yellow";
// Color to use for any WARNING severity log

const char k_COLOR_ERROR[] = "bold-red";
// Color to use for any ERROR severity log
}  // close unnamed namespace

// ---------------------
// class ConsoleObserver
// ---------------------

void ConsoleObserver::initializeColorMap()
{
    if (!isatty(fileno(stdout))) {
        // Not writing to a TTY, disable all coloring
        return;  // RETURN
    }

    bmqu::MemOutStream osCode;
    bmqu::MemOutStream osName;
    bsl::string        name;

    // Colors with no background
    for (size_t i = 0; k_COLORS[i].d_name_p != 0; ++i) {
        const Color& color = k_COLORS[i];

        // Reset os
        osName.reset();
        osCode.reset();

        // Standard
        osCode << "\033[0;" << color.d_fgCode << "m" << bsl::ends;
        osName << color.d_name_p;
        name.assign(osName.str().data(), osName.str().length());
        d_colorMap[name] = osCode.str();

        // Reset os
        osName.reset();
        osCode.reset();

        // Bold
        osCode << "\033[1;" << color.d_fgCode << "m" << bsl::ends;
        osName << "bold-" << color.d_name_p;
        name.assign(osName.str().data(), osName.str().length());
        d_colorMap[name] = osCode.str();
    }

    // Colors with background
    for (size_t i = 0; k_COLORS[i].d_name_p != 0; ++i) {
        const Color& bgColor = k_COLORS[i];
        for (size_t j = 0; k_COLORS[j].d_name_p != 0; ++j) {
            const Color& fgColor = k_COLORS[j];

            // Reset os
            osName.reset();
            osCode.reset();

            // Standard
            osCode << "\033[0;" << fgColor.d_fgCode << ";" << bgColor.d_bgCode
                   << "m" << bsl::ends;
            osName << fgColor.d_name_p << "-on-" << bgColor.d_name_p;
            name.assign(osName.str().data(), osName.str().length());
            d_colorMap[name] = osCode.str();

            // Reset os
            osName.reset();
            osCode.reset();

            // Bold
            osCode << "\033[1;" << fgColor.d_fgCode << ";" << bgColor.d_bgCode
                   << "m" << bsl::ends;
            osName << "bold-" << fgColor.d_name_p << "-on-"
                   << bgColor.d_name_p;
            name.assign(osName.str().data(), osName.str().length());
            d_colorMap[name] = osCode.str();
        }
    }
}

const char*
ConsoleObserver::getColorCodeForRecord(const ball::Record& record) const
{
    // PRECONDITIONS
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex was LOCKED

    // Errors and Warnings have a special color, regardless of the category
    if (record.fixedFields().severity() == ball::Severity::e_WARN) {
        ColorMap::const_iterator it = d_colorMap.find(k_COLOR_WARN);
        BSLS_ASSERT_SAFE(it != d_colorMap.end());
        return it->second.c_str();  // RETURN
    }
    if (record.fixedFields().severity() == ball::Severity::e_ERROR) {
        ColorMap::const_iterator it = d_colorMap.find(k_COLOR_ERROR);
        BSLS_ASSERT_SAFE(it != d_colorMap.end());
        return it->second.c_str();  // RETURN
    }

    // Pick the color assigned to the matching category, if any.  We must
    // linearly iterate over all registered categories, and can't do a map
    // lookup because of the '*' matching.  Note that we stop at first match;
    // which may not necessarily be best match (for example, if "A*" was
    // registered before "A.B", then the color associated to "A*" will be used
    // for an "A.B" category).
    for (CategoryColorVec::const_iterator it = d_categoryColorVec.begin();
         it != d_categoryColorVec.end();
         ++it) {
        if (bmqu::StringUtil::match(record.fixedFields().category(),
                                    it->first)) {
            return it->second;  // RETURN
        }
    }

    return 0;
}

ConsoleObserver::ConsoleObserver(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_formatter(allocator)
, d_severityThreshold(ball::Severity::e_INFO)
, d_colorMap(allocator)
, d_categoryColorVec(allocator)
{
    d_formatter.disablePublishInLocalTime();  // Always use UTC time
    initializeColorMap();
}

ConsoleObserver::~ConsoleObserver()
{
    // NOTHING (required because of virtual class)
}

ConsoleObserver&
ConsoleObserver::setCategoryColor(const bslstl::StringRef& category,
                                  const bslstl::StringRef& color)
{
    if (!isatty(fileno(stdout))) {
        // Not writing to a TTY, disable all coloring
        return *this;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    if (color.length() == 0) {
        // Clear category color entry, if found
        CategoryColorVec::iterator it;
        for (it = d_categoryColorVec.begin(); it != d_categoryColorVec.end();
             ++it) {
            if (it->first == category) {
                d_categoryColorVec.erase(it);
                break;  // BREAK
            }
        }
        return *this;  // RETURN
    }

    // Lookup the color
    ColorMap::const_iterator colorIt = d_colorMap.find(color);
    if (colorIt == d_colorMap.end()) {
        if (ball::LoggerManager::isInitialized()) {
            guard.release()->unlock();  // UNLOCK
            BALL_LOG_ERROR << "Color '" << color << "' not found";
        }
        else {
            // The ball logger may not have been initialized yet, depending on
            // the initialization sequence performed by the user of this
            // component.
            bsl::cerr << "bmqtsk::ConsoleObserver: " << "Color '" << color
                      << "' not found\n"
                      << bsl::flush;
        }

        return *this;  // RETURN
    }

    // Insert the new color registration entry for that category, or replace an
    // existing one if that same category is already registered.
    CategoryColorVec::iterator categoryIt;
    for (categoryIt = d_categoryColorVec.begin();
         categoryIt != d_categoryColorVec.end();
         ++categoryIt) {
        if (categoryIt->first == category) {
            // Replace color information
            categoryIt->second = colorIt->second.c_str();
            break;  // BREAK
        }
    }

    // Not found, append new entry
    if (categoryIt == d_categoryColorVec.end()) {
        d_categoryColorVec.emplace_back(
            bsl::make_pair(category, colorIt->second.c_str()));
    }

    return *this;
}

void ConsoleObserver::publish(const ball::Record& record,
                              BSLA_UNUSED const ball::Context& context)
{
    // Check if the record's severity is higher than the severity threshold
    if (record.fixedFields().severity() > d_severityThreshold) {
        return;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    if (d_colorMap.empty()) {
        d_formatter(bsl::cout, record);
        return;  // RETURN
    }

    // Format record with color information
    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bmqu::MemOutStream                    os(&localAllocator);
    d_formatter(os, record);

    // Add color information
    const char* color = getColorCodeForRecord(record);
    if (color) {
        bsl::cout << color << os.str() << k_COLOR_RESET;
    }
    else {
        bsl::cout << k_COLOR_RESET << os.str();
    }
}

}  // close package namespace
}  // close enterprise namespace
