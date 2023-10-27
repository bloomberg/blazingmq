# Prometheus plugin
# ------------

if (NOT BMQ_TARGET_PROMETHEUS_NEEDED )
  return()
endif()

find_package( PkgConfig REQUIRED )

bmq_add_plugin( prometheus )
