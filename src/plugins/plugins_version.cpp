// plugins_version.cpp                                              -*-C++-*-
#include <plugins_version.h>

namespace BloombergLP {

#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

#define PLUGINS_VERSION_STRING                                              \
                      "BLP_LIB_PLUGINS_" STRINGIFY(PLUGINS_VERSION_MAJOR) \
                                       "." STRINGIFY(PLUGINS_VERSION_MINOR) \
                                       "." STRINGIFY(PLUGINS_VERSION_PATCH)

const char *plugins::Version::s_ident =
                                         "$Id: " PLUGINS_VERSION_STRING " $";
const char *plugins::Version::s_what  = "@(#)"  PLUGINS_VERSION_STRING;

const char *plugins::Version::PLUGINS_S_VERSION = PLUGINS_VERSION_STRING;
const char *plugins::Version::s_dependencies      = "";
const char *plugins::Version::s_buildInfo         = "";
const char *plugins::Version::s_timestamp         = "";
const char *plugins::Version::s_sourceControlInfo = "";

}  // close enterprise namespace

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2023
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
