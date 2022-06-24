
cmake_minimum_required (VERSION 3.2.1)
project (Visible VERSION 2.0
         LANGUAGES CXX C)

cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0074 NEW)

set( CMAKE_VERBOSE_MAKEFILE ON )
set (PROJ_NAME VISI)    # short name, caps
set (APP_NAME Visible)


string (TOLOWER ${PROJ_NAME} PROJ_NAME_LOWER)  # short name lower case
string (TOUPPER ${PROJ_NAME} PROJ_NAME_UPPER)  # short name upper case
set (PROJECT_VERSION_RELEASE_TYPE "" CACHE STRING
    "Build type, for example: dev, beta2, RC1 (empty string for normal release)")
set (${PROJECT_NAME}_VERSION_RELEASE_TYPE ${PROJECT_VERSION_RELEASE_TYPE})
set (PROJECT_AUTHORS "Contributors to the OpenImageIO project")
option (${PROJECT_NAME}_SUPPORTED_RELEASE
       "Set ON for supported release branch, OFF for master" ON)
if (${PROJECT_NAME}_SUPPORTED_RELEASE)
    set (${PROJECT_NAME}_DEV_RELEASE OFF)
else ()
    set (${PROJECT_NAME}_DEV_RELEASE ON)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
endif ()

# If the user hasn't configured cmake with an explicit
# -DCMAKE_INSTALL_PREFIX=..., then set it to safely install into ./dist, to
# help prevent the user from accidentally writing over /usr/local or whatever.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
      AND PROJECT_IS_TOP_LEVEL)
    set (CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/dist" CACHE PATH
         "Installation location" FORCE)
endif()


if ("${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message (FATAL_ERROR "Not allowed to run in-source build!")
endif ()


message (STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")
message (STATUS "CMake ${CMAKE_VERSION}")
message (STATUS "CMake system           = ${CMAKE_SYSTEM}")
message (STATUS "CMake system name      = ${CMAKE_SYSTEM_NAME}")
message (STATUS "Project source dir     = ${PROJECT_SOURCE_DIR}")
message (STATUS "Project build dir      = ${CMAKE_BINARY_DIR}")
message (STATUS "Project install prefix = ${CMAKE_INSTALL_PREFIX}")
message (STATUS "Configuration types    = ${CMAKE_CONFIGURATION_TYPES}")
message (STATUS "Build type             = ${CMAKE_BUILD_TYPE}")

option (VERBOSE "Print lots of messages while compiling" ON)
set(OpenCV_ROOT /usr/local/Cellar/opencv/4.5.5_2)

list (APPEND CMAKE_MODULE_PATH
      "${PROJECT_SOURCE_DIR}/cmake/modules"
      "${PROJECT_SOURCE_DIR}/cmake")
	  
include (GNUInstallDirs)
	  
# Utilities
include (colors)
include (check_is_enabled)
include (checked_find_package)
	  
# All the C++ and compiler related options and adjustments
include (compiler)

# Dependency finding utilities and all dependency-related options
include (externalpackages)


#-----------------------------------------------------------------------------
# Load some macros.
include("${PROJECT_SOURCE_DIR}/cmake/macros.cmake")
#-----------------------------------------------------------------------------

get_filename_component( APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )
get_filename_component( CINDER_PATH "${APP_DIR}/externals/cinder" ABSOLUTE )
get_filename_component( DEPS_PATH "${APP_DIR}/build/_deps" ABSOLUTE )


add_subdirectory("${PROJECT_SOURCE_DIR}/externals/spdlog" spdlog)


message(" lspdlog  include: ${SPDLOG_INCLUDE_DIR} lib: ${SPDLOG_LIBRARY_PATH}")

add_subdirectory("${APP_DIR}/dependencies") 

set( IMGUI_DIR "${DEPS_PATH}/imgui-src")
set( IMGUI_FILE_DIR "${DEPS_PATH}/imgui_filebrowser-src")
set( IMGUI_CUSTOM_DIR "${APP_DIR}/src/imGuiCustom")
#set( OIIO_DIST "${APP_DIR}/externals/oiio-dist")
set( OIIO_DIST "/usr/local")


file(GLOB SOURCE_LIST CONFIGURE_DEPENDS
        "${APP_DIR}/src/*.h"
        "${APP_DIR}/src/*.hpp"
        "${APP_DIR}/src/*.cpp"
		"${APP_DIR}/src/*.c"
        "${APP_DIR}/src/*.m")


set(IMGUI_FILEBROWSER
${IMGUI_FILE_DIR}/ImGuiFileDialog.cpp
${IMGUI_FILE_DIR}/ImGuiFileDialog.h
${IMGUI_FILE_DIR}/ImGuiFileDialogConfig.h
)

set(IMPLOT
${APP_DIR}/externals/implot/implot.cpp
${APP_DIR}/externals/implot/implot_items.cpp
)

set(IMGUI
${IMGUI_DIR}/imgui.cpp
${IMGUI_DIR}/imgui_draw.cpp
${IMGUI_DIR}/imgui_demo.cpp
${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
#${IMGUI_DIR}/imgui_impl_opengl3.cpp
${IMGUI_DIR}/imgui_tables.cpp
${IMGUI_DIR}/imgui_widgets.cpp
)

set(IMGUI_CUSTOM
${IMGUI_CUSTOM_DIR}/imgui_panel.cpp
${IMGUI_CUSTOM_DIR}/imgui_utils.cpp
${IMGUI_CUSTOM_DIR}/imguivariouscontrols.cpp
${IMGUI_CUSTOM_DIR}/imgui_visible_widgets.cpp
${IMGUI_CUSTOM_DIR}/imgui_plot.cpp
${IMGUI_CUSTOM_DIR}/CinderImGui.cpp
)



include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

 string(APPEND CMAKE_CXX_FLAGS " -DSPDLOG_FMT_EXTERNAL ")

ci_make_app(
	APP_NAME ${APP_NAME}

	INCLUDES ${APP_DIR}/include ${OpenCV_INCLUDE_DIR} ${APP_DIR}/include
	${IMGUI_DIR}
	/usr/local/opt/fmt
	${IMGUI_FILE_DIR}
	/usr/local/opt/ilmbase/include/
	${OIIO_DIST}/include/OpenImageIO
	${APP_DIR}/externals/Eigen
	${APP_DIR}/externals/implot	
	${APP_DIR}/externals/cereal-1.1.1/include
		
	SOURCES ${SOURCE_LIST} 
	${IMGUI_FILEBROWSER}
	${IMPLOT}
	${IMGUI}
	${IMGUI_CUSTOM}
	${CINDER_IMGUI}	
	CINDER_PATH ${CINDER_PATH}



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
	${LSPDLOG_LIBRARY_PATH}	

)

add_dependencies(${APP_NAME} spdlog)


get_target_property( OUTPUT_DIR ${APP_NAME} RUNTIME_OUTPUT_DIRECTORY )

if( APPLE )
	set(CMAKE_OSX_ARCHITECTURES "x86_64")
	message(" ========. ${CMAKE_OSX_ARCHITECTURES} ============= ")
	set(
        MACOSX_BUNDLE_GUI_IDENTIFIER
        "net.darisallc.Visible" )
		
 	set(
        MACOSX_BUNDLE_ICON_FILE
        "Visible.icns" )

 	set(
        MACOSX_BUNDLE_BUNDLE_NAME
        "VisibleApp" )
		
	set_target_properties( ${APP_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${APP_DIR}/proj/Info.plist
	)

	file(COPY ${APP_DIR}/resources DESTINATION ${MACOSX_BUNDLE_BUNDLE_NAME}.app/Contents/)	
	add_custom_command(TARGET ${APP_NAME} POST_BUILD COMMAND cp ${APP_DIR}/Resources/${MACOSX_BUNDLE_ICON_FILE} ${OUTPUT_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE})
	add_custom_command(TARGET ${APP_NAME} PRE_BUILD COMMAND cp ${APP_DIR}/Resources/imgui.ini ${OUTPUT_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}.app/Contents/Resources/imgui.ini)	
add_custom_command(TARGET ${APP_NAME} PRE_BUILD COMMAND cp ${APP_DIR}/Resources/visible_scope.png ${OUTPUT_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}.app/Contents/Resources/visible_scope.png)	

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
