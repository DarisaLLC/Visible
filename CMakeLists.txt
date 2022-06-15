cmake_minimum_required (VERSION 3.1.3...3.5.1 FATAL_ERROR)

set( CMAKE_VERBOSE_MAKEFILE ON )

project( Visible OBJC C CXX)
include(ExternalProject)
set( APP_NAME "${PROJECT_NAME}App" )

# set the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#-----------------------------------------------------------------------------
# Load some macros.
include(proj/cmake/macros.cmake)
#-----------------------------------------------------------------------------

# Set 'ON' to enable 'TRACE' macro.
option(LSPDLOG_ENABLE_TRACE_LOGGING "Enable trace logging." ON)
  
# Set 'ON' to enable 'LOG' macro.
option(LSPDLOG_ENABLE_DATA_LOGGING  "Enable data logging."  ON)

get_filename_component( APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )
get_filename_component( CINDER_PATH "${APP_DIR}/externals/cinder" ABSOLUTE )
get_filename_component( DEPS_PATH "${APP_DIR}/build/_deps" ABSOLUTE )

add_subdirectory("${APP_DIR}/externals/lspdlog" lspdlog)
include_directories(${LSPDLOG_INCLUDE_DIRS}) #<-- necessary
message(" lspdlog  include: ${LSPDLOG_INCLUDE_DIRS}")

add_subdirectory("${APP_DIR}/dependencies") 

#set( IMGUI_DIR "${DEPS_PATH}/imgui-src")
set( IMGUI_DIR "${APP_DIR}/local_cinder_imgui/lib/imgui" )
set( IMGUI_CINDER_DIR "${APP_DIR}/local_cinder_imgui" )
set( NFD_DIR "${APP_DIR}/src/nfd")
set( IMGUI_CUSTOM_DIR "${APP_DIR}/include/imGuiCustom")
#set( OIIO_DIST "${APP_DIR}/externals/oiio-dist")
set( OIIO_DIST "/usr/local")



file(GLOB SOURCE_LIST CONFIGURE_DEPENDS
        "${APP_DIR}/src/*.h"
        "${APP_DIR}/src/*.hpp"
        "${APP_DIR}/src/*.cpp"
		"${APP_DIR}/src/*.c"
        "${APP_DIR}/src/*.m")


find_package(Boost REQUIRED REQUIRED COMPONENTS filesystem system thread program_options)

message(STATUS "    config: ${Boost_DIR}")
message(STATUS "    version: ${Boost_VERSION}")
message(STATUS " boost  include: ${Boost_INCLUDE_DIR}")
message(STATUS " boost   lib path: ${Boost_LIBRARY_DIR}")
message(STATUS " boost   libraries: ${Boost_LIBRARIES}")


set(NFD
${NFD_DIR}/nfd_cocoa.m
${NFD_DIR}/nfd_common.c
)

set_source_files_properties("${NFD_DIR}/nfd_cocoa.m" PROPERTIES LANGUAGE OBJC)
set_source_files_properties("${NFD_DIR}/nfd_common.c" PROPERTIES LANGUAGE CXX)

set(CINDER_IMGUI
${IMGUI_CINDER_DIR}/src/CinderImGui.cpp
${IMGUI_CINDER_DIR}/include/CinderImGui.h
${IMGUI_CINDER_DIR}/include/CinderImGuiConfig.h
${IMGUI_CINDER_DIR}/include/CinderImGuiExports.h
)


set(IMPLOT
${APP_DIR}/externals/implot/implot.cpp
${APP_DIR}/externals/implot/implot_items.cpp
)

set(IMGUI
${IMGUI_DIR}/imgui.cpp
${IMGUI_DIR}/imgui_draw.cpp
${IMGUI_DIR}/imgui_demo.cpp
#${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
${IMGUI_DIR}/imgui_impl_opengl3.cpp
${IMGUI_DIR}/imgui_tables.cpp
${IMGUI_DIR}/imgui_widgets.cpp
)

set(IMGUI_CUSTOM
${IMGUI_CUSTOM_DIR}/imgui_panel.cpp
${IMGUI_CUSTOM_DIR}/imgui_utils.cpp
${IMGUI_CUSTOM_DIR}/imguivariouscontrols.cpp
${IMGUI_CUSTOM_DIR}/imgui_visible_widgets.cpp
${IMGUI_CUSTOM_DIR}/imgui_plot.cpp
)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
set(OpenCV_DIR /usr/local/Cellar/opencv/4.5.5_2/lib/cmake/opencv4)
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

 string(APPEND CMAKE_CXX_FLAGS " -DSPDLOG_FMT_EXTERNAL ")

ci_make_app(
	APP_NAME ${APP_NAME}
	INCLUDES ${APP_DIR}/include
	SOURCES ${SOURCE_LIST} 
	${NFD}
	${IMPLOT}
	${IMGUI}
	${IMGUI_CUSTOM}
	${CINDER_IMGUI}	
	CINDER_PATH ${CINDER_PATH}
	INCLUDES ${OpenCV_INCLUDE_DIRS} ${APP_DIR}/include
	/usr/local/opt/fmt
#	${LSPDLOG_INCLUDE_DIRS}
	/usr/local/opt/ilmbase/include/
	${OIIO_DIST}/include/OpenImageIO
	${APP_DIR}/externals/Eigen
	${APP_DIR}/externals/implot	
	${APP_DIR}/externals/cereal-1.1.1/include
	${APP_DIR}/local_cinder_imgui/include
	${APP_DIR}/local_cinder_imgui/lib/imgui

	LIBRARIES ${OpenCV_LIBS} 
	${Boost_LIBRARIES}
	${OIIO_DIST}/lib/libOpenImageIO.dylib
	${OIIO_DIST}/lib/libOpenImageIO_Util.dylib
	/usr/local/lib/libfmt.a
	/usr/local/lib/libtiff.a
	/usr/local/lib/libpng.a
	/usr/local/lib/libIlmImf-2_5.a	
	/usr/local/lib/libIlmImfUtil-2_5.a
	/usr/local/lib/libIlmThread-2_5.a
	/usr/local/lib/libImath-2_5.a
#	/usr/local/lib/libheif.a
	/usr/local/lib/libIex-2_5.a	
	/usr/local/lib/libIexMath-2_5.a
	/usr/local/lib/libjpeg.a
	/usr/local/lib/libde265.a
	/usr/local/lib/libopenjp2.a
	/usr/local/lib/libHalf-2_5.a
	/usr/local/lib/libgif.a
	/Volumes/medvedev/Users/arman/cmakevisible/aom/build/libaom.a
	/Volumes/medvedev/Users/arman/cmakevisible/Visible/build/lspdlog/internal/spdlog/src/spdlog-build/libspdlog.a
)

add_dependencies(${APP_NAME} spdlog)

get_target_property( OUTPUT_DIR ${APP_NAME} RUNTIME_OUTPUT_DIRECTORY )

if( APPLE )
	set_target_properties( ${APP_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${APP_DIR}/proj/Info.plist
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
