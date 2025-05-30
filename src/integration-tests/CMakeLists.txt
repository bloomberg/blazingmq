# integration tests
# -----------------

if(NOT BMQ_TARGET_IT_NEEDED)
  return()
endif()

# Find Python 3 installed on the system.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  message(STATUS "Setting up Python virtual environment")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")

  # Create a virtual environment using the system Python.
  set(ENV{VIRTUAL_ENV} "${CMAKE_CURRENT_BINARY_DIR}/venv")
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}"
      -m venv
      "${CMAKE_CURRENT_BINARY_DIR}/venv"
  )

  # Forget about the system Python version; look for Python 3 installed within
  # the virtual environment now.
  set(Python3_FIND_VIRTUALENV FIRST)
  unset(Python3_EXECUTABLE)
  find_package(Python3 COMPONENTS Interpreter)

  # Upgrade virtual environment pip and install required dependencies.
  message(STATUS "Upgrade virtual environment pip")
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}"
      -m pip
      install
      --quiet
      --upgrade
      pip
  )
  message(STATUS "Install dependencies into virtual environment")
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}"
        -m pip
        install
        --quiet
        -r "${CMAKE_CURRENT_SOURCE_DIR}/../python/requirements.txt"
  )

  # Use pytest-cmake from pypi to integrate pytest integration tests as CTest
  # targets.
  message(STATUS "Install pytest-cmake into virtual environment")
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}"
      -m pip
      install
      --quiet
      pytest-cmake
  )
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/venv/share/Pytest/cmake")
  set(Pytest_ROOT "${CMAKE_CURRENT_BINARY_DIR}/venv/bin")
  find_package(Pytest)

  # If everything went well, we can use `pytest_discover_tests` to construct
  # CTest targets for each integration test.
  if(Pytest_FOUND)
    message(STATUS "Adding integration tests to CTest")

    file(GLOB_RECURSE integration_test_files "*.py")
    pytest_discover_tests(
      all.it
      ENVIRONMENT
        "BLAZINGMQ_BUILD_DIR=${CMAKE_BINARY_DIR}"
      PYTHON_PATH_PREPEND
        "${CMAKE_SOURCE_DIR}/src/python/"
      STRIP_PARAM_BRACKETS
      TRIM_FROM_NAME
        "^test_"
      DEPENDS
        bmqbrkr bmqtool ${integration_test_files}
      PROPERTIES
        LABELS "integration;all"
    )
  endif()

  list(POP_BACK CMAKE_MESSAGE_INDENT)
endif()
