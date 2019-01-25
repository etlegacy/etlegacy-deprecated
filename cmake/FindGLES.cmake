#
# Try to find GLES library and include path.
# Once done this will define
#
# GLES_FOUND
# GLES_INCLUDE_PATH
# GLES_LIBRARY
#

find_path(GLES_INCLUDE_DIR GLES/gl.h)
find_library(GLES_LIBRARY NAMES GLESv1_CM)

if(ARM AND NOT GLES_LIBRARY)
	# Try RPi location
	find_path(GLES_INCLUDE_DIR /opt/vc/include/GLES/gl.h)
	find_library(GLES_LIBRARY NAMES /opt/vc/lib/GLESv1_CM.so)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLES DEFAULT_MSG GLES_LIBRARY GLES_INCLUDE_DIR)
