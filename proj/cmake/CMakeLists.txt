cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( Visible )
set( APP_NAME "${PROJECT_NAME}App" )
set(CMAKE_CXX_STANDARD 14)

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cinder" ABSOLUTE )

get_filename_component( APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )
get_filename_component( BLOCK_DIR "${APP_DIR}/blocks" ABSOLUTE )

file(GLOB SOURCE_LIST CONFIGURE_DEPENDS
        "${APP_DIR}/src/*.h"
        "${APP_DIR}/src/*.hpp"
        "${APP_DIR}/src/*.cpp")

if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
set(OpenCV_DIR /usr/local/Cellar/opencv/4.5.5_2)
find_package(OpenCV REQUIRED)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
	APP_NAME ${APP_NAME}
	INCLUDES ${APP_DIR}/include
	SOURCES ${SOURCE_LIST} 
	CINDER_PATH ${CINDER_PATH}
	INCLUDES ${OpenCV_INCLUDE_DIRS} ${APP_DIR}/include
	${APP_DIR}/externals/oiio-dist/include
	${APP_DIR}/externals/Eigen
	${APP_DIR}/externals/implot	
	${APP_DIR}/externals/cereal-1.1.1/include
	${APP_DIR}/local_cinder_imgui/include
	${APP_DIR}/local_cinder_imgui/lib/imgui
	/usr/local/include	
	LIBRARIES ${OpenCV_LIBS}
	BLOCKS ${BLOCK_DIR}/Cinder-Boost
	
)

get_target_property( OUTPUT_DIR ${APP_NAME} RUNTIME_OUTPUT_DIRECTORY )

if( APPLE )
	set_target_properties( ${APP_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${APP_DIR}/proj/cmake/Info.plist
	)

    add_custom_target( run
        COMMAND open ${OUTPUT_DIR}/${APP_NAME}.app
        DEPENDS ${OUTPUT_DIR}/${APP_NAME}.app/Contents/MacOS/${APP_NAME}
        WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    )
elseif( UNIX )
    add_custom_target( run
        COMMAND ${OUTPUT_DIR}/${APP_NAME}
        DEPENDS ${OUTPUT_DIR}/${APP_NAME}
        WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    )
endif()
