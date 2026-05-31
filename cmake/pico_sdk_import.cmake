# This is a small local import shim. Set PICO_SDK_PATH to an installed Pico SDK,
# or place this project beside pico-sdk and configure with -DPICO_SDK_PATH=...
if (DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

if (NOT PICO_SDK_PATH)
    get_filename_component(_project_parent "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
    get_filename_component(_project_parent_parent "${_project_parent}/.." ABSOLUTE)
    if (EXISTS "${_project_parent_parent}/pico-sdk/external/pico_sdk_import.cmake")
        set(PICO_SDK_PATH "${_project_parent_parent}/pico-sdk")
    endif()
endif()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH is not set. Install pico-sdk, then set PICO_SDK_PATH or pass -DPICO_SDK_PATH=/path/to/pico-sdk.")
endif()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" ABSOLUTE)
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
