# project (clap_test)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (HEADERS
  clap_test_scan.h
)

set (SOURCES
  clap_test_scan.cpp
)

add_executable (${PROJECT_NAME}
  ${HEADERS}
  ${SOURCES}
)


# Add some debugger flags.
if (CONFIG_DEBUG)
   set (CONFIG_DEBUG_OPTIONS -g -fsanitize=address -fno-omit-frame-pointer)
   target_compile_options (${PROJECT_NAME} PRIVATE -DCONFIG_DEBUG)
   target_compile_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
   target_link_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
endif ()

set_target_properties (${PROJECT_NAME} PROPERTIES C_STANDARD 99)
set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

target_include_directories (${PROJECT_NAME} PRIVATE clap/include)

target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core)

