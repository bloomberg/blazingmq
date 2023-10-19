# plugins
# ------------

if (NOT BMQ_TARGET_PLUGINS_NEEDED)
  return()
endif()

find_package( PkgConfig REQUIRED )

bmq_add_plugin( plugins )
