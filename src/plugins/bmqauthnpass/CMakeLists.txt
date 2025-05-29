# Pass Authenticator plugin
# -------------------------

if (NOT BMQ_TARGET_AUTHNPASS_NEEDED )
  return()
endif()

find_package( PkgConfig REQUIRED )

bmq_add_plugin( bmqauthnpass )

if(APPLE)
  set_target_properties(bmqauthnpass PROPERTIES
    LINK_FLAGS "-undefined dynamic_lookup"
  )
elseif(UNIX)
  set_target_properties(bmqauthnpass PROPERTIES
    LINK_FLAGS "-Wl,--allow-shlib-undefined"
  )
endif()
