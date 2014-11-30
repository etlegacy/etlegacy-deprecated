if(WIN32)
	FIND_PATH( JANSSON_INCLUDE_PATH jansson.h
		DOC "The directory where jansson.h resides"
	)
	FIND_LIBRARY( JANSSON_LIBRARY
		NAMES jansson
		PATHS
		DOC "The Jansson library"
	)
else(WIN32)
	FIND_PATH( JANSSON_INCLUDE_PATH jansson.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		DOC "The directory where jansson.h resides"
	)
	FIND_LIBRARY( JANSSON_LIBRARY
		NAMES jansson
		PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		DOC "The Jansson library"
	)
endif(WIN32)

if(JANSSON_INCLUDE_PATH)
	set( JANSSON_FOUND 1 CACHE STRING "Set to 1 if Jansson is found, 0 otherwise")
else(JNSSON_INCLUDE_PATH)
	set( JANSSON_FOUND 0 CACHE STRING "Set to 1 if Jansson is found, 0 otherwise")
endif(JANSSON_INCLUDE_PATH)

MARK_AS_ADVANCED( JANSSON_FOUND JANSSON_LIBRARY JANSSON_INCLUDE_PATH )
