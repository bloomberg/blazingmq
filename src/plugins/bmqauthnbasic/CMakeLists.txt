# Basic Authenticator plugin
# --------------------------

if (NOT BMQ_TARGET_AUTHNBASIC_NEEDED )
  return()
endif()

find_package( PkgConfig REQUIRED )

bmq_add_plugin( bmqauthnbasic )
