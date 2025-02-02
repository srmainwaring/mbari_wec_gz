cmake_minimum_required(VERSION 3.5)
project(buoy_gazebo)

# if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#   add_compile_options(-Wall -Wextra -Wpedantic)
# endif()

# Option to enable profiler
option(ENABLE_PROFILER "Enable Ignition Profiler" FALSE)

if(ENABLE_PROFILER)
  add_definitions("-DIGN_PROFILER_ENABLE=1")
else()
  add_definitions("-DIGN_PROFILER_ENABLE=0")
endif()

find_package(ament_cmake REQUIRED)
find_package(buoy_description REQUIRED)
find_package(buoy_interfaces REQUIRED)
find_package(rclcpp REQUIRED)

find_package(ignition-cmake2 REQUIRED)
find_package(ignition-plugin1 REQUIRED COMPONENTS register)
set(GZ_PLUGIN_VER ${ignition-plugin1_VERSION_MAJOR})
find_package(ignition-common4 REQUIRED COMPONENTS profiler)
set(GZ_COMMON_VER ${ignition-common4_VERSION_MAJOR})
find_package(ignition-gazebo6 REQUIRED)
set(GZ_SIM_VER ${ignition-gazebo6_VERSION_MAJOR})


# Plugins
#
# gz_add_plugin (<plugin_name>
#              SOURCES <sources>
#              [PUBLIC_LINK_LIBS <libraries...>]
#              [PRIVATE_LINK_LIBS <libraries...>])
#              [INCLUDE_DIRS <paths_to_directories...>
#              [ROS]
#
# Add a system plugin
#
# <plugin_name> Required. Name of the library
#
# [SOURCES]: Specify the source files for the plugin.
#
#
# [PUBLIC_LINK_LIBS]: Specify a list of libraries to be publicly linked.
#
# [PRIVATE_LINK_LIBS]: Specify a list of libraries to be privately linked.
#
# [INCLUDE_DIRS]: Specify a list of path to directories to be included,
#
# [ROS]: Signal that this plugin is ROS-enabled

function(gz_add_plugin plugin_name)
  set(options ROS)
  set(oneValueArgs)
  set(multiValueArgs
    SOURCES
    PUBLIC_LINK_LIBS
    PRIVATE_LINK_LIBS
    INCLUDE_DIRS
  )

  cmake_parse_arguments(gz_add_plugin "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_library(${plugin_name} SHARED
    ${gz_add_plugin_SOURCES}
  )
  target_link_libraries(${plugin_name}
    PUBLIC
      ignition-gazebo${GZ_SIM_VER}::ignition-gazebo${GZ_SIM_VER}
      ignition-plugin${GZ_PLUGIN_VER}::ignition-plugin${GZ_PLUGIN_VER}
      ignition-common${GZ_COMMON_VER}::profiler
      ${gz_add_plugin_PUBLIC_LINK_LIBS}
    PRIVATE
      ${gz_add_plugin_PRIVATE_LINK_LIBS}
  )
  if(gz_add_plugin_ROS)
    ament_target_dependencies(${plugin_name} PUBLIC rclcpp buoy_interfaces)
  endif()
  target_include_directories(${plugin_name}
      PUBLIC ${gz_add_plugin_INCLUDE_DIRS})
  # target_compile_features(${plugin_name} PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17
  install(
    TARGETS ${plugin_name}
    DESTINATION lib)
endfunction()

add_subdirectory(src)


# Resources
install(DIRECTORY
  worlds
  launch
  gazebo
  rviz
  DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  # GTest
  find_package(ament_cmake_gtest REQUIRED)
  ament_find_gtest()

  # launch_testing
  find_package(launch_testing_ament_cmake REQUIRED)

  # Helper function to generate gtest
  function(gz_add_gtest TEST_NAME)
    set(
      options
        ROS
        LAUNCH_TEST
        GAZEBO
    )
    set(oneValueArgs)
    set(multiValueArgs
      EXTRA_ROS_PKGS)

    cmake_parse_arguments(gz_add_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    ament_add_gtest_executable(${TEST_NAME}
      test/${TEST_NAME}.cpp
    )
    if(gz_add_test_GAZEBO)
      target_link_libraries(${TEST_NAME}
        ignition-gazebo${IGN_GAZEBO_VER}::ignition-gazebo${IGN_GAZEBO_VER}
      )
    endif()
    if(gz_add_test_ROS)
      set(ROS_PKGS rclcpp ${gz_add_test_EXTRA_ROS_PKGS})
      foreach(PKG ${ROS_PKGS})
        find_package(${PKG} REQUIRED)
      endforeach()
      ament_target_dependencies(${TEST_NAME} ${ROS_PKGS})
    endif()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${TEST_NAME}
      Stopwatch
    )
    install(
      TARGETS ${TEST_NAME}
      DESTINATION lib/${PROJECT_NAME}
    )
    if(gz_add_test_LAUNCH_TEST)
      add_launch_test(test/launch/${TEST_NAME}.launch.py
        TIMEOUT 300
      )
    else()
      ament_add_gtest_test(${TEST_NAME})
    endif()
  endfunction()

  # Add gtests
  gz_add_gtest(test_stopwatch ROS)

endif()

ament_environment_hooks("${CMAKE_CURRENT_SOURCE_DIR}/hooks/${PROJECT_NAME}.dsv.in")

ament_package()
