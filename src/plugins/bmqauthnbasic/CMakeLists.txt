# Basic Authenticator plugin
# -------------------------

if (NOT BMQ_TARGET_AUTHNBASIC_NEEDED )
  return()
endif()

find_package( PkgConfig REQUIRED )

bmq_add_plugin( bmqauthnbasic )

if(APPLE)
  set_target_properties(bmqauthnbasic PROPERTIES
    LINK_FLAGS "-undefined dynamic_lookup"
  )
elseif(UNIX)
  set_target_properties(bmqauthnbasic PROPERTIES
    LINK_FLAGS "-Wl,--allow-shlib-undefined"
  )
endif()
