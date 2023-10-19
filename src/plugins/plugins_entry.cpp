// plugins_entry.cpp                                                -*-C++-*-

// PLUGINS
#include <plugins_version.h>
#include <plugins_pluginlibrary.h>

// MQB
#include <mqbplug_pluginlibrary.h>

// BDE
#include <ball_log.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>

using namespace BloombergLP;

extern "C" {
void
instantiatePluginLibrary(bslma::ManagedPtr<mqbplug::PluginLibrary> *library,
                         bslma::Allocator                          *allocator);
}  // close extern "C"

void
instantiatePluginLibrary(bslma::ManagedPtr<mqbplug::PluginLibrary> *library,
                         bslma::Allocator                          *allocator)
{
    BALL_LOG_SET_CATEGORY("PLUGINS.ENTRY");

    BALL_LOG_INFO << "Instantiating 'libplugins.so' plugin library "
                     "(version: " << plugins::Version::version() << ")";

    *library =
        bslma::ManagedPtrUtil::allocateManaged<plugins::PluginLibrary>(
                                                                    allocator);
}
