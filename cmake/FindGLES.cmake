#
# Try to find GLES library and include path.
# Once done this will define
#
# GLES_FOUND
# GLES_INCLUDE_PATH
# GLES_LIBRARY
#

if(ARM AND RPI)
	# Try RPi location
	find_path(GLES_INCLUDE_DIR NAMES GLES/gl.h PATHS "/opt/vc/include/")
	find_library(GLES_LIBRARY NAMES brcmGLESv2 PATHS "/opt/vc/lib/")
else()
	find_path(GLES_INCLUDE_DIR GLES/gl.h)
	find_library(GLES_LIBRARY NAMES GLESv1_CM)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLES DEFAULT_MSG GLES_LIBRARY GLES_INCLUDE_DIR)
