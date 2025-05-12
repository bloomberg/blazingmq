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

// mqbplug_pluginmanager.cpp                                          -*-C++-*-
#include <mqbplug_pluginmanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqbplug_pluginlibrary.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_stringrefutil.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bdls_processutil.h>
#include <bsl_algorithm.h>
#include <bsl_functional.h>
#include <bsl_unordered_map.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>
#include <bsls_platform.h>

// POSIX
#include <dlfcn.h>

namespace BloombergLP {
namespace mqbplug {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBPLUG.PLUGINMANAGER");

const char k_PLUGIN_ENTRY_POINT[] = "instantiatePluginLibrary";

typedef void (*PluginEntryFnPtr)(bslma::ManagedPtr<mqbplug::PluginLibrary>*,
                                 bslma::Allocator*);

void dlcloseDeleter(void* dlopenHandle, BSLA_UNUSED void*)
{
    BSLA_MAYBE_UNUSED int rc = dlclose(dlopenHandle);
    BSLS_ASSERT_SAFE(rc == 0);
}

bool findPluginPredicate(const bsl::string&         target,
                         const mqbplug::PluginInfo& pluginInfo)
{
    return bdlb::StringRefUtil::areEqualCaseless(target, pluginInfo.name());
}

bool findPluginWithDuplicates(const bsl::pair<bsl::string, int>& plugin)
{
    return plugin.second != 1;
}

}  // anonymous namespace

PluginManager::PluginManager(bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_pluginHandles(d_allocator_p)
, d_pluginLibraries(d_allocator_p)
, d_enabledPlugins(d_allocator_p)
{
    // NOTHING
}

void PluginManager::enableRequiredPlugins(
    const bslstl::StringRef&                pluginPath,
    const bslma::ManagedPtr<PluginLibrary>& pluginLibrary,
    RequiredPluginsRecord*                  requiredPlugins,
    bsl::vector<bsl::string>*               pluginsProvided,
    bsl::ostream&                           errorDescription,
    int*                                    rc)
{
    enum RcEnum { rc_SUCCESS = 0, rc_ACTIVATION_FAILED = -1 };

    // If an error was encountered during a prior 'loadPluginLibrary()' call,
    // then 'rc' will be nonzero, and no further plugins should be loaded.
    if ((*rc) != rc_SUCCESS) {
        return;  // RETURN
    }

    // Indicates whether the 'PluginLibrary::activate()'
    // method has yet been called: It should be called only once, if and only
    // if an enabled plugin is provided by the library.
    bool activated = false;

    // Load any required plugins provided by the library into the
    // 'd_enabledPlugins' map.
    //
    RequiredPluginsRecord::iterator requirementIt = requiredPlugins->begin();
    for (; requirementIt != requiredPlugins->end(); ++requirementIt) {
        // Bound function used with STL algorithms to find plugins provided by
        // 'pluginLibrary' matching the name required for this 'requirementIt'.
        bsl::function<bool(const PluginInfo&)> findPluginFn =
            bdlf::BindUtil::bind(findPluginPredicate,
                                 requirementIt->first,
                                 bdlf::PlaceHolders::_1);

        // Search for a plugin from 'pluginLibrary' matching 'requirementIt'.
        bsl::vector<PluginInfo>::const_iterator pluginInfoIt = bsl::find_if(
            pluginLibrary->plugins().cbegin(),
            pluginLibrary->plugins().cend(),
            findPluginFn);
        if (pluginInfoIt != pluginLibrary->plugins().cend()) {
            // Found a plugin matching 'requirementIt'.

            // Count how many plugins provided by this library share this name.
            // If this evaluates to more than '1', it will be an error.
            int numLibInstances = 1 + bsl::count_if(
                                          pluginInfoIt + 1,
                                          pluginLibrary->plugins().cend(),
                                          findPluginFn);

            // Update 'requiredPlugins' with how many matches
            // were found.
            int numTotalInstances = (*requiredPlugins)[pluginInfoIt->name()] +=
                numLibInstances;

            // Multiple plugins with the same name (even across multiple
            // plugins) is considered a fatal error. Check first whether this
            // individual 'PluginLibrary' provides multiple such plugins, and
            // then whether a previously loaded 'PluginLibrary' has already
            // satisfied this plugin requirement.
            if (numLibInstances > 1) {
                errorDescription
                    << "Plugin library ambiguously provides multiple plugins "
                       "matching a name required by the broker configuration; "
                       "this will terminate the broker during startup [path: '"
                    << pluginPath << "', pluginName: '" << pluginInfoIt->name()
                    << "', count: " << numLibInstances << "]";
                break;  // BREAK
            }
            else if (numTotalInstances > 1) {
                BALL_LOG_ERROR
                    << "PluginLibrary provides a plugin with the same name as "
                       "a plugin already provided by another library [path: '"
                    << pluginPath << "', pluginName: '" << pluginInfoIt->name()
                    << "']";
                break;  // BREAK
            }

            // From the 'PluginLibrary's loaded so far, 'pluginInfoIt' is the
            // unique 'PluginInfo' satisfying 'requirementIt'. If the
            // 'PluginLibrary::activate()' method is yet to be called, do so
            // now.
            if (!activated && numTotalInstances == 1) {
                if (int status = pluginLibrary->activate()) {
                    errorDescription
                        << "Failed to activate 'PluginLibrary'; plugins from "
                        << "this library will not be loaded [path: '"
                        << pluginPath << "', error: " << status << "]'";
                    // Don't even consider plugins from a library that could
                    // not be activated.
                    (*rc) = rc_ACTIVATION_FAILED;
                    break;  // BREAK
                }
                activated = true;
            }

            // Enable the plugin.
            pluginsProvided->emplace_back(requirementIt->first);
            d_enabledPlugins.emplace(pluginInfoIt->type(), &(*pluginInfoIt));

            BALL_LOG_INFO << "Enabled plugin '" << requirementIt->first
                          << "' from library '" << pluginPath << "'";
        }
    }
}

void PluginManager::loadPluginLibrary(const char*            path,
                                      RequiredPluginsRecord* requiredPlugins,
                                      bsl::ostream&          errorDescription,
                                      int*                   rc)
{
    BALL_LOG_INFO << "Loading '" << path << "' as a candidate plugin object";

    // Load the plugin-library into memory.
#if defined(BSLS_PLATFORM_OS_SOLARIS)
    // Solaris provides a non-standard 'RTLD_NODELETE' dlopen-flag, which
    // instructs the OS not to delete objects during dlclose(). This avoids
    // segfaults which can occur if an object from bmqbrkr.tsk holds a
    // reference to objects loaded by a plugin. In particular, this can
    // happen when the ball::LoggerManager d'tor tries to access a static
    // "category-holder" instantiated by BALL_LOG_SET_*_CATEGORY() calls.
    const int dlopenFlags = RTLD_NOW | RTLD_NODELETE;
#else
    const int dlopenFlags = RTLD_NOW;
#endif
    void* objectHandle = dlopen(path, dlopenFlags);
    if (!objectHandle) {
        BALL_LOG_WARN << "Error while opening shared object [path: '" << path
                      << "', dlerror: '" << dlerror() << "']";
        return;  // RETURN
    }
    bslma::ManagedPtr<void> objectHandleMp(objectHandle,
                                           (void*)0,
                                           dlcloseDeleter);

    // Clear 'dlerror' before calling 'dlsym()'. See docs for details:
    //   https://linux.die.net/man/3/dlerror
    dlerror();

    // Find the entry-point for instantiating the 'PluginLibrary' object.
    PluginEntryFnPtr entryPoint = reinterpret_cast<PluginEntryFnPtr>(
        dlsym(objectHandle, k_PLUGIN_ENTRY_POINT));
    char* dlerrorText = dlerror();
    if (dlerrorText != NULL) {
        BALL_LOG_WARN << "Error while loading plugin entry point [symbol: '"
                      << k_PLUGIN_ENTRY_POINT << "', pluginPath: '" << path
                      << "', dlerror: '" << dlerrorText << "']";
        return;  // RETURN
    }

    // Load the 'PluginLibrary' object into a 'ManagedPtr'.
    bslma::ManagedPtr<mqbplug::PluginLibrary> pluginLibrary;
    entryPoint(&pluginLibrary, d_allocator_p);
    if (!pluginLibrary.get()) {
        BALL_LOG_WARN << "Failed to instantiate 'PluginLibrary' object from "
                      << "plugin entry-point [symbol: '"
                      << k_PLUGIN_ENTRY_POINT << "', pluginPath: '" << path
                      << "']";
        return;  // RETURN
    }

    // Load all plugins satisfying the requirements from 'requiredPlugins' into
    // 'd_enabledPlugins'. Populate 'pluginsProvided' with the list of all such
    // enabled plugins. Update 'errorDescription' and 'rc' to reflect any fatal
    // errors that occurred during loading (e.g., failure while activating the
    // 'pluginLibrary' object).
    bsl::vector<bsl::string> pluginsProvided(d_allocator_p);
    enableRequiredPlugins(path,
                          pluginLibrary,
                          requiredPlugins,
                          &pluginsProvided,
                          errorDescription,
                          rc);

    // Log a summary of all plugins provided by the library, even if 'rc' is in
    // an error state.
    {
        bsl::stringstream summaryBuilder(d_allocator_p);
        summaryBuilder << "Successfully loaded plugin library (" << path
                       << "): \n\n";

        bsl::vector<PluginInfo>::const_iterator it =
            pluginLibrary->plugins().cbegin();
        for (; it != pluginLibrary->plugins().cend(); ++it) {
            bool enabled = bsl::find(pluginsProvided.cbegin(),
                                     pluginsProvided.cend(),
                                     it->name()) != pluginsProvided.cend();

            summaryBuilder << "[" << (enabled ? "*" : " ") << "] "
                           << it->name() << " (version: " << it->version()
                           << ")\n    " << it->description() << "\n";
        }
        BALL_LOG_INFO << summaryBuilder.str();
    }

    // Add library to internal tracking if we loaded any plugins from it.
    // Otherwise, allow the plugin handle to destruct and dlclose().
    if (pluginsProvided.size() > 0) {
        // Keep the library alive in 'd_pluginLibraries' for the lifetime of
        // the 'PluginManager' object.
        d_pluginLibraries.emplace_back(
            bslmf::MovableRefUtil::move(pluginLibrary));

        // Move the object-handle into 'd_pluginHandles', to be 'dlclose'd at
        // the end of the 'PluginManager' lifetime.
        d_pluginHandles.emplace_back(
            bslmf::MovableRefUtil::move(objectHandleMp));
    }
}

int PluginManager::start(const mqbcfg::Plugins& pluginsConfig,
                         bsl::ostream&          errorDescription)
{
    enum RcEnum {
        // Value for the various RC error categories.
        rc_SUCCESS                     = 0,
        rc_INVALID_PLUGIN_DIRECTORY    = -1,
        rc_NO_UNIQUE_PLUGIN_FOUND      = -2,
        rc_LOAD_PLUGIN_LIBRARY_FAILURE = -3
    };
    int rc = rc_SUCCESS;

    // Initialize working list of required plugins.
    RequiredPluginsRecord requiredPlugins(d_allocator_p);
    {
        bsl::vector<bsl::string>::const_iterator it =
            pluginsConfig.enabled().cbegin();
        for (; it != pluginsConfig.enabled().cend(); ++it) {
            requiredPlugins[*it] = 0;
        }
    }

    // If no plugin is enabled at all, log a warning.
    if (requiredPlugins.empty()) {
        BMQTSK_ALARMLOG_ALARM("PLUGIN_MISSING")
            << "No plugin is enabled at all. Please double check if this is "
            << "intended." << BMQTSK_ALARMLOG_END;
        return rc_SUCCESS;  // RETURN
    }

    // For each directory provided via broker configuration, attempt to load
    // all shared-object files as plugin-library candidates. For every library
    // loaded, fill 'd_enabledPlugins' with the 'PluginInfo' for any plugins
    // whose names belong to 'requiredPlugins' and remove the name from
    // 'requiredPlugins'.

    const bsl::vector<bsl::string>* libraries = &pluginsConfig.libraries();

    bsl::vector<bsl::string>::const_iterator libraryIt = libraries->cbegin();
    for (; libraryIt != libraries->cend(); ++libraryIt) {
        bsl::string pattern(*libraryIt);
        if (int status = bdls::PathUtil::appendIfValid(&pattern, "*.so")) {
            errorDescription
                << "Plugin library directory was invalid [path: " << pattern
                << ", error: " << status << "]";
            rc = rc_INVALID_PLUGIN_DIRECTORY;
            break;  // BREAK
        }
        else if (!bdls::FilesystemUtil::visitPaths(
                     pattern,
                     bdlf::BindUtil::bind(&PluginManager::loadPluginLibrary,
                                          this,
                                          bdlf::PlaceHolders::_1,
                                          &requiredPlugins,
                                          bsl::ref(errorDescription),
                                          &rc))) {
            BALL_LOG_INFO << "Found no shared-object files in plugin library "
                             "path [path: '"
                          << *libraryIt << "']";
            continue;  // CONTINUE
        }
        else if (rc) {
            rc = rc_LOAD_PLUGIN_LIBRARY_FAILURE;
            break;  // BREAK
        }
    }

    // If any required plugins could not be located in the plugin-libraries
    // from the paths provided, exit with error during broker startup.
    RequiredPluginsRecord::const_iterator reqPluginIt = bsl::find_if(
        requiredPlugins.cbegin(),
        requiredPlugins.cend(),
        findPluginWithDuplicates);
    if (reqPluginIt != requiredPlugins.cend()) {
        bool              hasDuplicatePlugins = false;
        bsl::stringstream libraryList(d_allocator_p);

        libraryList << "'" << reqPluginIt->first << "' ("
                    << reqPluginIt->second << ")";
        if (reqPluginIt->second > 1) {
            hasDuplicatePlugins = true;
        }
        for (++reqPluginIt; reqPluginIt != requiredPlugins.cend();
             ++reqPluginIt) {
            if (reqPluginIt->second != 1) {
                libraryList << ", '" << reqPluginIt->first << "', ("
                            << reqPluginIt->second << ")";
            }
            if (reqPluginIt->second > 1) {
                hasDuplicatePlugins = true;
            }
        }

        bmqu::MemOutStream errorDesc;
        errorDesc << "Could not find unique candidates for all required "
                     "plugins [missingOrDuplicate: ["
                  << libraryList.str() << "]]";

        if (hasDuplicatePlugins) {
            errorDescription << errorDesc.str();
            rc = rc_NO_UNIQUE_PLUGIN_FOUND;
        }
        else {
            // If there are missing plugins but no duplicates, we will log an
            // error but we can still start the broker.

            BMQTSK_ALARMLOG_ALARM("PLUGIN_MISSING")
                << errorDesc.str() << BMQTSK_ALARMLOG_END;
        }
    }
    return rc;
}

void PluginManager::stop()
{
    // Destroy all references to 'PluginInfo' objects.
    d_enabledPlugins.clear();

    // Deactivate all plugin-libraries.
    bsl::vector<bslma::ManagedPtr<PluginLibrary> >::const_iterator it =
        d_pluginLibraries.cbegin();
    for (; it != d_pluginLibraries.cend(); ++it) {
        (*it)->deactivate();
    }

    // Destroy all 'PluginLibrary' objects, which closes handles returned by
    // 'dlopen()' in the process.
    d_pluginLibraries.clear();
    d_pluginHandles.clear();
}

void PluginManager::get(
    mqbplug::PluginType::Enum                    type,
    bsl::unordered_set<mqbplug::PluginFactory*>* result) const
{
    bsl::multimap<PluginType::Enum, const PluginInfo*>::const_iterator it =
        d_enabledPlugins.cbegin();
    for (; it != d_enabledPlugins.cend(); ++it) {
        if (it->first == type) {
            result->insert(it->second->factory().get());
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
