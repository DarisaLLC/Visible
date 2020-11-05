#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OpenImageIO::OpenImageIO_Util" for configuration "Release"
set_property(TARGET OpenImageIO::OpenImageIO_Util APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(OpenImageIO::OpenImageIO_Util PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libOpenImageIO_Util.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS OpenImageIO::OpenImageIO_Util )
list(APPEND _IMPORT_CHECK_FILES_FOR_OpenImageIO::OpenImageIO_Util "${_IMPORT_PREFIX}/lib/libOpenImageIO_Util.a" )

# Import target "OpenImageIO::OpenImageIO" for configuration "Release"
set_property(TARGET OpenImageIO::OpenImageIO APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(OpenImageIO::OpenImageIO PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libOpenImageIO.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS OpenImageIO::OpenImageIO )
list(APPEND _IMPORT_CHECK_FILES_FOR_OpenImageIO::OpenImageIO "${_IMPORT_PREFIX}/lib/libOpenImageIO.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
