cmake_minimum_required (VERSION 3.1.3...3.5.1 FATAL_ERROR)

# ExternalProject CMake module
include(ExternalProject)


get_filename_component( EXT_SPDLOG "${CMAKE_CURRENT_SOURCE_DIR}/../..//externals/spdlog" ABSOLUTE )
message( " spdlog is ${EXT_SPDLOG}")


# Download spdlog
ExternalProject_Add(spdlog
  PREFIX spdlog
  #--Download step--------------
  SOURCE_DIR=${EXT_SPDLOG}
  
  #--Configure step-------------
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
	-DCMAKE_BUILD_TYPE=Release
    -DSPDLOG_BUILD_SHARED=OFF
	-DSPDLOG_FMT_EXTERNAL=ON
	-DSPDLOG_BUILD_EXAMPLE=ON
  #--Build step-----------------
  BUILD_COMMAND make
  #--Install step---------------
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON)

set(SPDLOG_INCLUDE_DIRS
    ${SPDLOG_INCLUDE_DIRS}
    ${EXT_SPDLOG}/include PARENT_SCOPE)
	
set(SPDLOG_LIBRARY_PATH
  ${SPDLOG_LIBRARY_PATH}
  ${EXT_SPDLOG}/spdlog/build/libspdlog.a)
  
	
