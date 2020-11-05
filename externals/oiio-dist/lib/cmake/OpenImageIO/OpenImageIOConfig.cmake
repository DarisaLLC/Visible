
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# add here all the find_dependency() whenever switching to config based dependencies
# e.g. if switching to Boost::Boost instead of using ${Boost_LIBRARY_DIRS} the add:
# find_dependency(Boost 107400)

set_and_check (OpenImageIO_INCLUDE_DIR "/Volumes/medvedev/Users/arman/XcodeProjects/dev/externals/oiio/dist/macosx/include")
set_and_check (OpenImageIO_INCLUDES    "/Volumes/medvedev/Users/arman/XcodeProjects/dev/externals/oiio/dist/macosx/include")
set_and_check (OpenImageIO_LIB_DIR     "/Volumes/medvedev/Users/arman/XcodeProjects/dev/externals/oiio/dist/macosx/lib")
set (OpenImageIO_PLUGIN_SEARCH_PATH         "")

#...logic to determine installedPrefix from the own location...
#set (OpenImageIO_CONFIG_DIR  "${installedPrefix}/")

include ("${CMAKE_CURRENT_LIST_DIR}/OpenImageIOTargets.cmake")

check_required_components ("OpenImageIO")
