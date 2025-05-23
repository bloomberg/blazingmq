# Fail Authenticator plugin
# -------------------------

if (NOT BMQ_TARGET_AUTHNFAIL_NEEDED )
  return()
endif()

find_package( PkgConfig REQUIRED )

bmq_add_plugin( bmqauthnfail )

if(APPLE)
  set_target_properties(bmqauthnfail PROPERTIES
    LINK_FLAGS "-undefined dynamic_lookup"
  )
elseif(UNIX)
  set_target_properties(bmqauthnfail PROPERTIES
    LINK_FLAGS "-Wl,--allow-shlib-undefined"
  )
endif()
