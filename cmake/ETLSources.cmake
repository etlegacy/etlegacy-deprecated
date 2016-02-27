#-----------------------------------------------------------------
# Sources
#-----------------------------------------------------------------

FILE(GLOB COMMON_SRC
	"src/qcommon/*.cpp"
	"src/qcommon/*.h"
)

FILE(GLOB COMMON_SRC_REMOVE
	"src/qcommon/dl_main_curl.cpp"
	"src/qcommon/dl_main_stubs.cpp"
	"src/qcommon/i18n_*"
	"src/qcommon/json.cpp"
	"src/qcommon/json_stubs.cpp"
)

LIST(REMOVE_ITEM COMMON_SRC ${COMMON_SRC_REMOVE})

# Platform specific code for server and client
if(UNIX)
	if(APPLE)
		LIST(APPEND PLATFORM_SRC "src/sys/sys_osx.m")
		SET_SOURCE_FILES_PROPERTIES("src/sys/sys_osx.m" PROPERTIES LANGUAGE C)
	endif(APPLE)

	LIST(APPEND PLATFORM_SRC "src/sys/sys_unix.cpp")
	LIST(APPEND PLATFORM_SRC "src/sys/con_tty.cpp")
elseif(WIN32)
	LIST(APPEND PLATFORM_SRC "src/sys/sys_win32.cpp")
	LIST(APPEND PLATFORM_SRC "src/sys/sys_win32_con.cpp")
	LIST(APPEND PLATFORM_SRC "src/sys/con_win32.cpp")
	LIST(APPEND PLATFORM_SRC "src/sys/win_resource.rc")
endif()

FILE(GLOB SERVER_SRC
	"src/server/*.cpp"
	"src/server/*.h"
	"src/null/*.cpp"
	"src/null/*.h"
	"src/botlib/be*.cpp"
	"src/botlib/be*.h"
	"src/botlib/l_*.cpp"
	"src/botlib/l_*.h"
	"src/sys/sys_main.cpp"
	"src/sys/con_log.cpp"
	"src/qcommon/update.cpp"
	"src/qcommon/download.cpp"
)

FILE(GLOB CLIENT_SRC
	"src/server/*.cpp"
	"src/server/*.h"
	"src/client/*.cpp"
	"src/client/*.h"
	"src/botlib/be*.cpp"
	"src/botlib/be*.h"
	"src/botlib/l_*.cpp"
	"src/botlib/l_*.h"
	"src/sys/sys_main.cpp"
	"src/sys/con_log.cpp"
	"src/sdl/*.cpp"
	"src/sdl/*.h"
	"src/qcommon/update.cpp"
	"src/qcommon/download.cpp"
)

# These files are shared with the CGAME from the UI library
FILE(GLOB UI_SHARED
	"src/ui/ui_shared.cpp"
	"src/ui/ui_parse.cpp"
	"src/ui/ui_script.cpp"
	"src/ui/ui_menu.cpp"
	"src/ui/ui_menuitem.cpp"
)

FILE(GLOB CGAME_SRC
	"src/cgame/*.cpp"
	"src/cgame/*.h"
	"src/qcommon/q_math.cpp"
	"src/qcommon/q_shared.cpp"
	"src/qcommon/q_unicode.cpp"
	"src/game/bg_*.cpp"
)

LIST(APPEND CGAME_SRC ${UI_SHARED})

FILE(GLOB QAGAME_SRC
	"src/game/*.cpp"
	"src/game/*.h"
	"src/qcommon/crypto/sha-1/sha1.cpp"
	"src/qcommon/q_math.cpp"
	"src/qcommon/q_shared.cpp"
)

FILE(GLOB UI_SRC
	"src/ui/*.cpp"
	"src/ui/*.h"
	"src/qcommon/q_math.cpp"
	"src/qcommon/q_shared.cpp"
	"src/qcommon/q_unicode.cpp"
	"src/game/bg_classes.cpp"
	"src/game/bg_misc.cpp"
)

FILE(GLOB CLIENT_FILES
	"src/client/*.cpp"
)

FILE(GLOB SERVER_FILES
	"src/server/*.cpp"
)

FILE(GLOB SYSTEM_FILES
	"src/sys/sys_main.cpp"
	"src/sys/con_log.cpp"
)

FILE(GLOB SDL_FILES
	"src/sdl/*.cpp"
)

FILE(GLOB BOTLIB_FILES
	"src/botlib/be*.cpp"
	"src/botlib/l_*.cpp"
)

FILE(GLOB RENDERER_COMMON
	"src/renderercommon/*.cpp"
	"src/renderercommon/*.h"
	#Library build requires the following
	"src/sys/sys_local.h"
	"src/qcommon/q_shared.h"
	"src/qcommon/puff.h"
)

FILE(GLOB RENDERER_COMMON_DYNAMIC
	"src/qcommon/q_shared.cpp"
	"src/qcommon/q_math.cpp"
	"src/qcommon/puff.cpp"
	"src/qcommon/md4.cpp"
)

FILE(GLOB RENDERER1_FILES
	"src/renderer/*.cpp"
	"src/renderer/*.h"
)

FILE(GLOB RENDERERGLES_FILES
	"src/rendererGLES/*.cpp"
	"src/rendererGLES/*.h"
	"src/sdl/eglport.cpp"
)

FILE(GLOB RENDERER2_FILES
	"src/renderer2/*.cpp"
	"src/renderer2/*.h"
)

SET(GLSL_PATH "src/renderer2/glsl")

FILE(GLOB RENDERER2_SHADERS
	"${GLSL_PATH}/*.glsl"
	"${GLSL_PATH}/*/*.glsl"
)

FILE(GLOB RENDERER2_SHADERDEFS
	"src/renderer2/gldef/*.gldef"
)

FILE(GLOB IRC_CLIENT_FILES
	"src/qcommon/htable.cpp"
	"src/qcommon/htable.h"
)
