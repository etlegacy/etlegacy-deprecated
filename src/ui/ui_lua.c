#include "ui_local.h"

#ifdef BUNDLED_LUA
#    include "lua.h"
#    include "lauxlib.h"
#    include "lualib.h"
#else
#    include <lua.h>
#    include <lauxlib.h>
#    include <lualib.h>
#endif

static lua_State *L;
static char *uifile = "lua/ui/UICore.lua";

static const luaL_Reg ui_custom[] = {
	{ "_G", luaopen_base },
	//{ LUA_LOADLIBNAME, luaopen_package },
	//{ LUA_COLIBNAME, luaopen_coroutine }, //We might want this?
	{ LUA_TABLIBNAME, luaopen_table },
	//{ LUA_IOLIBNAME, luaopen_io },
	//{ LUA_OSLIBNAME, luaopen_os },
	{ LUA_STRLIBNAME, luaopen_string },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ LUA_UTF8LIBNAME, luaopen_utf8 },
	{ LUA_DBLIBNAME, luaopen_debug },
	{ NULL, NULL }
};

static void UI_LoadDefaultLibs(lua_State *L) {
	const luaL_Reg *lib;
	/* "require" functions from 'loadedlibs' and set results to global table */
	for (lib = ui_custom; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}
}

static int _et_register_shader(lua_State *L)
{
	qhandle_t tmp = trap_R_RegisterShaderNoMip(luaL_checkstring(L, 1));
	lua_pushnumber(L, tmp);
	return 1;
}

static int _et_draw_shader(lua_State *L)
{
	qhandle_t tmp;
	float x, y, w, h;

	x = luaL_checknumber(L, 1);
	y = luaL_checknumber(L, 2);
	w = luaL_checknumber(L, 3);
	h = luaL_checknumber(L, 4);

	tmp = luaL_checknumber(L, 5);

	UI_AdjustFrom640(&x, &y, &w, &h);

	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, tmp);
	return 0;
}

static int _et_Com_Print(lua_State *L)
{
	char text[1024];

	Q_strncpyz(text, luaL_checkstring(L, 1), sizeof(text));
	Com_Printf(text);
	return 0;
}

static int derptest(lua_State *L, int d1, lua_KContext d2) {
	(void)d1;  (void)d2;  /* only to match 'lua_Kfunction' prototype */
	return lua_gettop(L) - 1;
}

static int _et_dofile(lua_State *L) {
	int flen = 0;
	int res = 0;
	fileHandle_t f;
	char *code;

	const char *fname = luaL_optstring(L, 1, NULL);
	lua_settop(L, 1);


	flen = trap_FS_FOpenFile(va("lua/%s.lua", fname), &f, FS_READ);
	if (flen < 0)
	{
		Com_Printf("Lua API: can not open file %s\n", fname);
		return qfalse;
	}

	code = malloc(flen + 1);

	if (code == NULL)
	{
		Com_Printf("Lua API: memory allocation error for %s data\n", fname);
	}

	trap_FS_Read(code, flen, f);
	*(code + flen) = '\0';
	trap_FS_FCloseFile(f);

	if (luaL_loadstring(L, code) != LUA_OK)
		return lua_error(L);
	lua_callk(L, 0, LUA_MULTRET, 0, derptest);
	return derptest(L, 0, 0);
}

static int _lua_atpanic(lua_State *L)
{
	Com_Error(ERR_FATAL, "Lua failed\n");
	return 0;
}

static const luaL_Reg uilib[] =
{
	{ "registerFont", _et_Com_Print },
	{ "getShader", _et_register_shader },
	{ "drawRect", _et_Com_Print },
	{ "drawShader", _et_draw_shader },
	{ "drawText", _et_Com_Print },
	{ "drawTextWithCursor", _et_Com_Print },
	{ "textWidth", _et_Com_Print },
	{ "textHeight", _et_Com_Print },
	{ "getText", _et_Com_Print },
	{ "getVersion", _et_Com_Print },
	{ "setVisible", _et_Com_Print },
	{ "print", _et_Com_Print },
	{ NULL, NULL },
};

int luaadd(int x, int y)
{
	int sum;

	/* the function name */
	lua_getglobal(L, "add");

	/* the first argument */
	lua_pushnumber(L, x);

	/* the second argument */
	lua_pushnumber(L, y);

	/* call the function with 2 arguments, return 1 result */
	lua_call(L, 2, 1);

	/* get the result */
	sum = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	return sum;
}

static qboolean UI_LuaExec(const char *func, int nargs, int nresults)
{
	switch (lua_pcall(L, nargs, nresults, 0))
	{
	case LUA_ERRRUN:
		Com_Printf("Lua API: %s error running lua script: %s\n", func, lua_tostring(L, -1));
		lua_pop(L, 1);
		return qfalse;
	case LUA_ERRMEM:
		Com_Printf("Lua API: memory allocation error #2 ( %s )\n", uifile);
		return qfalse;
		return qfalse;
	case LUA_ERRERR:
		Com_Printf("Lua API: traceback error ( %s )\n", uifile);
		return qfalse;
	default:
		return qtrue;
	}
	return qtrue;
}

static qboolean UI_LuaExecuteGlobal(const char *func, int nargs, int nresults)
{
	lua_getglobal(L, func);
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return qfalse;
	}
	return UI_LuaExec(func, nargs, nresults);
}

void UI_LuaUpdateFrameValues()
{
	lua_getglobal(L, "etl");

	// Set the cursor info
	lua_pushnumber(L, uiInfo.uiDC.cursorx);
	lua_setfield(L, -2, "cursorX");
	lua_pushnumber(L, uiInfo.uiDC.cursory);
	lua_setfield(L, -2, "cursorY");

	// pop back up
	lua_pop(L, 1);
}


qboolean UI_RunLuaFrame(void)
{
	if (L)
	{
		UI_LuaUpdateFrameValues();
		return UI_LuaExecuteGlobal("RunUi", 0, 0);
	}

	return qfalse;
}

static void UI_LuaSetUpGlobals()
{
	// remove some stuff from the global array (so we can replace it later)
	lua_getglobal(L, "_G");
	lua_pushnil(L);
	lua_setfield(L, -2, "dofile");
	lua_pushnil(L);
	lua_setfield(L, -2, "print");
	lua_pop(L, 1);

	lua_pushcfunction(L, _et_dofile);
	lua_setglobal(L, "dofile");

	lua_pushcfunction(L, _et_dofile);
	lua_setglobal(L, "require");

	lua_pushcfunction(L, _et_Com_Print);
	lua_setglobal(L, "print");
}


qboolean UI_InitLua(void)
{
	int flen = 0;
	int res = 0;
	fileHandle_t f;
	char *code;

	L = NULL;

	flen = trap_FS_FOpenFile(uifile, &f, FS_READ);
	if (flen < 0)
	{
		Com_Printf("Lua API: can not open file %s\n", uifile);
		return qfalse;
	}

	code = malloc(flen + 1);

	if (code == NULL)
	{
		Com_Printf("Lua API: memory allocation error for %s data\n", uifile);
	}

	trap_FS_Read(code, flen, f);
	*(code + flen) = '\0';
	trap_FS_FCloseFile(f);

	L = luaL_newstate();

	lua_atpanic(L, _lua_atpanic);

	// Initialise the lua state
	UI_LoadDefaultLibs(L);
	//luaL_openlibs(L);

	// register functions
	luaL_newlib(L, uilib);

	lua_pushvalue(L, -1);
	lua_setglobal(L, "etl");

	UI_LuaSetUpGlobals();

	res = luaL_loadbuffer(L, code, flen, uifile);
	if (res == LUA_ERRSYNTAX)
	{
		Com_Printf("%s API: syntax error during pre-compilation: %s\n", LUA_VERSION, lua_tostring(L, -1));
		lua_pop(L, 1);
		return qfalse;
	}
	else if (res == LUA_ERRMEM)
	{
		Com_Printf("%s API: memory allocation error #1 ( %s )\n", LUA_VERSION, uifile);
		return qfalse;
	}

	// Init the script
	UI_LuaExec("UI_Init", 0, 0);

	//Com_Printf("Derpsin derp: %i\n", luaadd(10, 20));

	return UI_RunLuaFrame();
}

void UI_ShutdownLua(void)
{
	if (L)
	{
		lua_close(L);
		L = NULL;
	}
}

/*
Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11)
{
	switch (command)
	{
	case UI_KEY_EVENT:
		return 0;
	case UI_MOUSE_EVENT:
		return 0;
	case UI_REFRESH:
		UI_RunLuaFrame();
		return 0;
	case UI_IS_FULLSCREEN:
		return qtrue;
	case UI_SET_ACTIVE_MENU:
		return 0;
	case UI_GET_ACTIVE_MENU:
		return 0;
	case UI_CONSOLE_COMMAND:
		return qfalse;
	case UI_DRAW_CONNECT_SCREEN:
		return 0;
	case UI_CHECKEXECKEY:
		return qfalse;
	case UI_WANTSBINDKEYS:
		return qfalse;
	case UI_GETAPIVERSION:
		return UI_API_VERSION;
	case UI_INIT:
		UI_InitLua();
		return 0;
	case UI_SHUTDOWN:
		UI_ShutdownLua();
		return 0;
	case UI_HASUNIQUECDKEY: // obsolete - keep this to avoid 'Bad ui export type' for vanilla clients
		return 0;
	default:
		Com_Printf("Bad ui export type: %ld\n", (long int)command);
		break;
	}

	return -1;
}
*/
