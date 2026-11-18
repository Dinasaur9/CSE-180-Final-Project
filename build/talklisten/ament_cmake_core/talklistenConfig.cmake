# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_talklisten_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED talklisten_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(talklisten_FOUND FALSE)
  elseif(NOT talklisten_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(talklisten_FOUND FALSE)
  endif()
  return()
endif()
set(_talklisten_CONFIG_INCLUDED TRUE)

# output package information
if(NOT talklisten_FIND_QUIETLY)
  message(STATUS "Found talklisten: 0.0.2 (${talklisten_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'talklisten' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT talklisten_DEPRECATED_QUIET)
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(talklisten_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "")
foreach(_extra ${_extras})
  include("${talklisten_DIR}/${_extra}")
endforeach()
