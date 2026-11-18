# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_multipletopics_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED multipletopics_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(multipletopics_FOUND FALSE)
  elseif(NOT multipletopics_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(multipletopics_FOUND FALSE)
  endif()
  return()
endif()
set(_multipletopics_CONFIG_INCLUDED TRUE)

# output package information
if(NOT multipletopics_FIND_QUIETLY)
  message(STATUS "Found multipletopics: 0.0.1 (${multipletopics_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'multipletopics' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT multipletopics_DEPRECATED_QUIET)
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(multipletopics_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "")
foreach(_extra ${_extras})
  include("${multipletopics_DIR}/${_extra}")
endforeach()
