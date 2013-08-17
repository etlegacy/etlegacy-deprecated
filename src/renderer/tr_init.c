/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 *
 * @file tr_init.c
 * @brief functions that are not called every frame
 */

#include "tr_local.h"

glconfig_t glConfig;
qboolean   textureFilterAnisotropic = qfalse;
int        maxAnisotropy            = 0;
float      displayAspect            = 0.0f;

glstate_t glState;

static void GfxInfo_f(void);

#ifdef USE_RENDERER_DLOPEN
cvar_t *com_altivec;
#endif

#ifdef HAVE_GLES
void ( * glLockArraysEXT )( GLint, GLint );
void ( * glUnlockArraysEXT )( void );
#endif

cvar_t *r_flareSize;
cvar_t *r_flareFade;

cvar_t *r_railWidth;
cvar_t *r_railSegmentLength;

cvar_t *r_ignoreFastPath;

cvar_t *r_ignore;

cvar_t *r_detailTextures;

cvar_t *r_znear;
cvar_t *r_zfar;
// FIXME #176
//cvar_t *r_stereoSeparation;

cvar_t *r_skipBackEnd;

cvar_t *r_stereoEnabled;

cvar_t *r_greyscale;

cvar_t *r_ignorehwgamma;
cvar_t *r_measureOverdraw;

cvar_t *r_fastsky;
cvar_t *r_drawSun;
cvar_t *r_dynamiclight;

cvar_t *r_lodbias;
cvar_t *r_lodscale;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_drawfoliage;
cvar_t *r_speeds;

cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_facePlaneCull;
cvar_t *r_showcluster;
cvar_t *r_nocurves;

cvar_t *r_allowExtensions;

cvar_t *r_ext_compressed_textures;
cvar_t *r_ext_multitexture;
cvar_t *r_ext_compiled_vertex_array;
cvar_t *r_ext_texture_env_add;

cvar_t *r_ext_texture_filter_anisotropic;
cvar_t *r_ext_max_anisotropy;

cvar_t *r_ignoreGLErrors;
cvar_t *r_logFile;

cvar_t *r_stencilbits;
cvar_t *r_depthbits;
cvar_t *r_colorbits;
cvar_t *r_primitives;
cvar_t *r_texturebits;
cvar_t *r_ext_multisample;

cvar_t *r_drawBuffer;
cvar_t *r_lightmap;
cvar_t *r_uiFullScreen;
cvar_t *r_shadows;
cvar_t *r_portalsky;
cvar_t *r_flares;
cvar_t *r_mode;
cvar_t *r_oldMode;
cvar_t *r_nobind;
cvar_t *r_singleShader;
cvar_t *r_roundImagesDown;
cvar_t *r_colorMipLevels;
cvar_t *r_picmip;
cvar_t *r_showtris;
cvar_t *r_trisColor;
cvar_t *r_showsky;
cvar_t *r_shownormals;
cvar_t *r_normallength;
//cvar_t *r_showmodelbounds; // see RB_MDM_SurfaceAnim()
cvar_t *r_finish;
cvar_t *r_clear;
cvar_t *r_swapInterval;
cvar_t *r_textureMode;
cvar_t *r_offsetFactor;
cvar_t *r_offsetUnits;
cvar_t *r_gamma;
cvar_t *r_intensity;
cvar_t *r_lockpvs;
cvar_t *r_noportals;
cvar_t *r_portalOnly;

cvar_t *r_subdivisions;
cvar_t *r_lodCurveError;

cvar_t *r_fullscreen;
cvar_t *r_noborder;

cvar_t *r_customwidth;
cvar_t *r_customheight;
cvar_t *r_customPixelAspect;

cvar_t *r_overBrightBits;
cvar_t *r_mapOverBrightBits;

cvar_t *r_debugSurface;
cvar_t *r_simpleMipMaps;

cvar_t *r_showImages;

cvar_t *r_ambientScale;
cvar_t *r_directedScale;
cvar_t *r_debugLight;
cvar_t *r_debugSort;
cvar_t *r_printShaders;
cvar_t *r_saveFontData;

cvar_t *r_cache;
cvar_t *r_cacheShaders;
cvar_t *r_cacheModels;

cvar_t *r_cacheGathering;

cvar_t *r_bonesDebug;

cvar_t *r_wolffog;

cvar_t *r_screenshotJpegQuality;

cvar_t *r_maxpolys;
cvar_t *r_maxpolyverts;

// TODO: check if this crazy stuff is needed
vec4hack_t     tess_xyz[SHADER_MAX_VERTEXES];
vec4hack_t     tess_normal[SHADER_MAX_VERTEXES];
vec2hack_t     tess_texCoords0[SHADER_MAX_VERTEXES];
vec2hack_t     tess_texCoords1[SHADER_MAX_VERTEXES];
glIndex_t      tess_indexes[SHADER_MAX_INDEXES];
color4ubhack_t tess_vertexColors[SHADER_MAX_VERTEXES];

static void AssertCvarRange(cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral)
{
	if (shouldBeIntegral)
	{
		if (( int ) cv->value != cv->integer)
		{
			ri.Printf(PRINT_WARNING, "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value);
			ri.Cvar_Set(cv->name, va("%d", cv->integer));
		}
	}

	if (cv->value < minVal)
	{
		ri.Printf(PRINT_WARNING, "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal);
		ri.Cvar_Set(cv->name, va("%f", minVal));
	}
	else if (cv->value > maxVal)
	{
		ri.Printf(PRINT_WARNING, "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal);
		ri.Cvar_Set(cv->name, va("%f", maxVal));
	}
}

/*
 * @brief This function is responsible for initializing a valid OpenGL subsystem
 *
 * This is done by calling GLimp_Init (which gives us a working OGL subsystem)
 * then setting variables, checking GL constants, and reporting the gfx system
 * config to the user.
 */
static void InitOpenGL(void)
{
	char renderer_buffer[1024];

	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//          - r_fullscreen
	//          - r_mode
	//          - r_(color|depth|stencil)bits
	//          - r_ignorehwgamma
	//          - r_gamma

	if (glConfig.vidWidth == 0)
	{
		GLint temp;

		GLimp_Init();

		strcpy(renderer_buffer, glConfig.renderer_string);
		Q_strlwr(renderer_buffer);

		// OpenGL driver constants
		qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if (glConfig.maxTextureSize <= 0)
		{
			glConfig.maxTextureSize = 0;
		}
	}

	// print info
	GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors(void)
{
	int  err;
	char s[64];

	err = qglGetError();
	if (err == GL_NO_ERROR)
	{
		return;
	}
	if (r_ignoreGLErrors->integer)
	{
		return;
	}
	switch (err)
	{
	case GL_INVALID_ENUM:
		strcpy(s, "GL_INVALID_ENUM");
		break;
	case GL_INVALID_VALUE:
		strcpy(s, "GL_INVALID_VALUE");
		break;
	case GL_INVALID_OPERATION:
		strcpy(s, "GL_INVALID_OPERATION");
		break;
	case GL_STACK_OVERFLOW:
		strcpy(s, "GL_STACK_OVERFLOW");
		break;
	case GL_STACK_UNDERFLOW:
		strcpy(s, "GL_STACK_UNDERFLOW");
		break;
	case GL_OUT_OF_MEMORY:
		strcpy(s, "GL_OUT_OF_MEMORY");
		break;
	default:
		Com_sprintf(s, sizeof(s), "%i", err);
		break;
	}

	ri.Error(ERR_VID_FATAL, "GL_CheckErrors: %s", s);
}

/*
==================
R_GetModeInfo
==================
*/
typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;              // pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",           320,  240,  1 },
	{ "Mode  1: 400x300",           400,  300,  1 },
	{ "Mode  2: 512x384",           512,  384,  1 },
	{ "Mode  3: 640x480",           640,  480,  1 },
	{ "Mode  4: 800x600",           800,  600,  1 },
	{ "Mode  5: 960x720",           960,  720,  1 },
	{ "Mode  6: 1024x768",          1024, 768,  1 },
	{ "Mode  7: 1152x864",          1152, 864,  1 },
	{ "Mode  8: 1280x1024",         1280, 1024, 1 },
	{ "Mode  9: 1600x1200",         1600, 1200, 1 },
	{ "Mode 10: 2048x1536",         2048, 1536, 1 },
	{ "Mode 11: 800x480 (wide)",    800,  480,  1 },
	{ "Mode 12: 1366x768 (16:9)",   1366, 768,  1 },
	{ "Mode 13: 1440x900 (16:10)",  1440, 900,  1 },
	{ "Mode 14: 1680x1050 (16:10)", 1680, 1050, 1 },
	{ "Mode 15: 1600x1200",         1600, 1200, 1 },
	{ "Mode 16: 1920x1080 (16:9)",  1920, 1080, 1 },
	{ "Mode 17: 1920x1200 (16:10)", 1920, 1200, 1 },
	{ "Mode 18: 2560x1600 (16:10)", 2560, 1600, 1 },
};
static int s_numVidModes = ARRAY_LEN(r_vidModes);

qboolean R_GetModeInfo(int *width, int *height, float *windowAspect, int mode)
{
	vidmode_t *vm;
	float     pixelAspect;

	if (mode < -1)
	{
		return qfalse;
	}
	if (mode >= s_numVidModes)
	{
		return qfalse;
	}

	if (mode == -1)
	{
		*width      = r_customwidth->integer;
		*height     = r_customheight->integer;
		pixelAspect = r_customPixelAspect->value;
	}
	else
	{
		vm = &r_vidModes[mode];

		*width      = vm->width;
		*height     = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	*windowAspect = ( float ) *width / (*height * pixelAspect);

	return qtrue;
}

/**
 * @brief Prints hardcoded screen resolutions
 * @see r_availableModes for supported resolutions
 */
static void R_ModeList_f(void)
{
	int i;

	ri.Printf(PRINT_ALL, "\n");
	ri.Printf(PRINT_ALL, (r_mode->integer == -2) ? "%s ^2(current)\n" : "%s\n",
	          "Mode -2: desktop resolution");
	ri.Printf(PRINT_ALL, (r_mode->integer == -1) ? "%s ^2(current)\n" : "%s\n",
	          "Mode -1: custom resolution");
	for (i = 0; i < s_numVidModes; i++)
	{
		ri.Printf(PRINT_ALL, (i == r_mode->integer) ? "%s ^2(current)\n" : "%s\n",
		          r_vidModes[i].description);
	}
	ri.Printf(PRINT_ALL, "\n");
}

/*
 * ==============================================================================
 *
 *                                                SCREEN SHOTS
 *
 * NOTE: some thoughts about the screenshots system:
 * screenshots get written in fs_homepath + fs_gamedir
 * vanilla W:ET .. etmain/screenshots/<FILE>.tga
 * ET: Legacy   .. legacy/screenshots/<FILE>.tga
 *
 * two commands: "screenshot" and "screenshotJPEG"
 * we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
 * (with FS_FileExists / FS_FOpenFileWrite calls)
 * FIXME: the statics don't get a reinit between fs_game changes
 *
 * ==============================================================================
 */

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri.Hunk_FreeTempMemory()
==================
*/
byte *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte  *buffer, *bufstart;
	int   padwidth, linelen;
	GLint packAlign;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen  = width * 3;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);

	#ifdef HAVE_GLES
	bufstart=buffer;
	padwidth=linelen;
	int p2width=1, p2height=1;
	int xx, yy, aa;
	while (p2width<glConfig.vidWidth) p2width*=2;
	while (p2height<glConfig.vidHeight) p2height*=2;
	byte *source = (byte*) ri.Hunk_AllocateTempMemory( p2width * p2height * 4 );
	qglReadPixels( 0, 0, p2width, p2height, GL_RGBA, GL_UNSIGNED_BYTE, source );
	for (yy=y; yy<height; yy++)
		for (xx=x; xx<width; xx++)
			for (aa=0; aa<3; aa++)
				buffer[yy*width*3+xx*3+aa]=source[(yy+y)*p2width*4+(xx+x)*4+aa];
	ri.Hunk_FreeTempMemory(source);
	#else
	bufstart = PADP(( intptr_t ) buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);
	#endif

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

byte *RB_ReadZBuffer(int x, int y, int width, int height, int *padlen)
{
	byte  *buffer, *bufstart;
	int   padwidth, linelen;
	GLint packAlign;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen  = width;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + packAlign - 1);

	bufstart = PADP(( intptr_t ) buffer, packAlign);
	qglDepthRange(0.0f, 1.0f);
	#ifdef HAVE_GLES
	memset(buffer, 0, padwidth * height + packAlign - 1);	//*TODO* find something to read DepthBuffer ?!
	#else
	qglReadPixels(x, y, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, bufstart);
	#endif

	*padlen = padwidth - linelen;

	return buffer;
}

/**
 * @brief zbuffer writer for the future implementation of the Depth of field effect
 * @note Unused.
 */
void RB_TakeDepthshot(int x, int y, int width, int height, char *fileName)
{
	byte   *allbuf, *buffer;
	byte   *srcptr, *destptr;
	byte   *endline, *endmem;
	int    linelen, padlen;
	size_t offset = 18, memcount;

	allbuf = RB_ReadZBuffer(x, y, width, height, &padlen);
	buffer = ri.Hunk_AllocateTempMemory(width * height * 3 + offset);

	Com_Memset(buffer, 0, 18);
	buffer[2]  = 2;         // uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;        // pixel size

	linelen = width;

	srcptr  = allbuf;
	destptr = buffer + offset;
	endmem  = srcptr + (linelen + padlen) * height;
	while (srcptr < endmem)
	{
		endline = srcptr + linelen;

		while (srcptr < endline)
		{
			*destptr++ = srcptr[0];
			*destptr++ = srcptr[0];
			*destptr++ = srcptr[0];

			srcptr++;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * 3 * height + offset;

	ri.FS_WriteFile(fileName, buffer, memcount);

	ri.Hunk_FreeTempMemory(allbuf);
	ri.Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
{
	byte   *allbuf, *buffer;
	byte   *srcptr, *destptr;
	byte   *endline, *endmem;
	byte   temp;
	int    linelen, padlen;
	size_t offset = 18, memcount;

	allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	buffer = allbuf + offset - 18;

	Com_Memset(buffer, 0, 18);
	buffer[2]  = 2;         // uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;        // pixel size

	// swap rgb to bgr and remove padding from line endings
	linelen = width * 3;

	srcptr = destptr = allbuf + offset;
	endmem = srcptr + (linelen + padlen) * height;

	while (srcptr < endmem)
	{
		endline = srcptr + linelen;

		while (srcptr < endline)
		{
			temp       = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	// gamma correct
	if (glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(allbuf + offset, memcount);
	}

	ri.FS_WriteFile(fileName, buffer, memcount + 18);

	ri.Hunk_FreeTempMemory(allbuf);
}

/*
==================
RB_TakeScreenshotJPEG
==================
 */
void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	byte   *buffer;
	size_t offset = 0, memcount;
	int    padlen;

	buffer   = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if (glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(buffer + offset, memcount);
	}

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri.Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd(const void *data)
{
	const screenshotCommand_t *cmd = ( const screenshotCommand_t * ) data;

	if (cmd->jpeg)
	{
		RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	}
	else
	{
		RB_TakeScreenshot(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	}

	return ( const void * ) (cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
 */
void R_TakeScreenshot(int x, int y, int width, int height, char *name, qboolean jpeg)
{
	static char         fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t *cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x      = x;
	cmd->y      = y;
	cmd->width  = width;
	cmd->height = height;
	Q_strncpyz(fileName, name, sizeof(fileName));
	cmd->fileName = fileName;
	cmd->jpeg     = jpeg;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename(int lastNumber, char *fileName)
{
	int a, b, c, d;

	if (lastNumber < 0 || lastNumber > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.tga");
		return;
	}

	a           = lastNumber / 1000;
	lastNumber -= a * 1000;
	b           = lastNumber / 100;
	lastNumber -= b * 100;
	c           = lastNumber / 10;
	lastNumber -= c * 10;
	d           = lastNumber;

	Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
	            , a, b, c, d);
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilenameJPEG(int lastNumber, char *fileName)
{
	int a, b, c, d;

	if (lastNumber < 0 || lastNumber > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.jpg");
		return;
	}

	a           = lastNumber / 1000;
	lastNumber -= a * 1000;
	b           = lastNumber / 100;
	lastNumber -= b * 100;
	c           = lastNumber / 10;
	lastNumber -= c * 10;
	d           = lastNumber;

	Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
	            , a, b, c, d);
}

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd(const void *data)
{
	const videoFrameCommand_t *cmd = (const videoFrameCommand_t *)data;
	int                       frameSize;
	int                       i;

	// check if the recording is still going on, the buffer might have cmds eventho the recording has stopped
	if (ri.CL_VideoRecording())
	{
		glReadPixels(0, 0, cmd->width, cmd->height, GL_RGBA, GL_UNSIGNED_BYTE, cmd->captureBuffer);
		// gamma correct
		if ((tr.overbrightBits > 0) && glConfig.deviceSupportsGamma)
		{
			R_GammaCorrect(cmd->captureBuffer, cmd->width * cmd->height * 4);
		}
		if (cmd->motionJpeg)
		{
			/* This should be fixed
			frameSize = RE_SaveJPGToBuffer(cmd->encodeBuffer, 90, cmd->width, cmd->height, cmd->captureBuffer);
			ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, frameSize);
			*/
		}
		else
		{
			frameSize = cmd->width * cmd->height;
			for (i = 0; i < frameSize; i++)  // Pack to 24bpp and swap R and B
			{
				cmd->encodeBuffer[i * 3]     = cmd->captureBuffer[i * 4 + 2];
				cmd->encodeBuffer[i * 3 + 1] = cmd->captureBuffer[i * 4 + 1];
				cmd->encodeBuffer[i * 3 + 2] = cmd->captureBuffer[i * 4];
			}
			ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, frameSize * 3);
		}
	}

	return (const void *)(cmd + 1);
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot(void)
{
	char   checkname[MAX_OSPATH];
	byte   *buffer;
	byte   *source, *allsource;
	byte   *src, *dst;
	size_t offset = 0;
	int    padlen;
	int    x, y;
	int    r, g, b;
	float  xScale, yScale;
	int    xx, yy;

	Com_sprintf(checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName);

	allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	source    = allsource + offset;

	buffer = ri.Hunk_AllocateTempMemory(128 * 128 * 3 + 18);
	Com_Memset(buffer, 0, 18);
	buffer[2]  = 2;         // uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;        // pixel size

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for (y = 0 ; y < 128 ; y++)
	{
		for (x = 0 ; x < 128 ; x++)
		{
			r = g = b = 0;
			for (yy = 0 ; yy < 3 ; yy++)
			{
				for (xx = 0 ; xx < 4 ; xx++)
				{
					src = source + (3 * glConfig.vidWidth + padlen) * ( int ) ((y * 3 + yy) * yScale) +
					      3 * ( int ) ((x * 4 + xx) * xScale);
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst    = buffer + 18 + 3 * (y * 128 + x);
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if (glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(buffer + 18, 128 * 128 * 3);
	}

	ri.FS_WriteFile(checkname, buffer, 128 * 128 * 3 + 18);

	ri.Hunk_FreeTempMemory(buffer);
	ri.Hunk_FreeTempMemory(allsource);

	ri.Printf(PRINT_ALL, "Wrote %s\n", checkname);
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f(void)
{
	char       checkname[MAX_OSPATH];
	static int lastNumber = -1;
	qboolean   silent;

	if (!strcmp(ri.Cmd_Argv(1), "levelshot"))
	{
		R_LevelShot();
		return;
	}

	if (!strcmp(ri.Cmd_Argv(1), "silent"))
	{
		silent = qtrue;
	}
	else
	{
		silent = qfalse;
	}

	if (ri.Cmd_Argc() == 2 && !silent)
	{
		// explicit filename
		Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv(1));
	}
	else
	{
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if (lastNumber == -1)
		{
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 99999 ; lastNumber++)
		{
			R_ScreenshotFilename(lastNumber, checkname);

			if (!ri.FS_FileExists(checkname))
			{
				break; // file doesn't exist
			}
		}

		if (lastNumber >= 99999)
		{
			ri.Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qfalse);

	if (!silent)
	{
		ri.Printf(PRINT_ALL, "Wrote %s\n", checkname);
	}
}

void R_ScreenShotJPEG_f(void)
{
	char       checkname[MAX_OSPATH];
	static int lastNumber = -1;
	qboolean   silent;

	if (!strcmp(ri.Cmd_Argv(1), "levelshot"))
	{
		R_LevelShot();
		return;
	}

	if (!strcmp(ri.Cmd_Argv(1), "silent"))
	{
		silent = qtrue;
	}
	else
	{
		silent = qfalse;
	}

	if (ri.Cmd_Argc() == 2 && !silent)
	{
		// explicit filename
		Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv(1));
	}
	else
	{
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if (lastNumber == -1)
		{
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 99999 ; lastNumber++)
		{
			R_ScreenshotFilenameJPEG(lastNumber, checkname);

			if (!ri.FS_FileExists(checkname))
			{
				break; // file doesn't exist
			}
		}

		if (lastNumber == 100000)
		{
			ri.Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qtrue);

	if (!silent)
	{
		ri.Printf(PRINT_ALL, "Wrote %s\n", checkname);
	}
}

//============================================================================

/*
==================
GL_SetDefaultState
==================
*/
void GL_SetDefaultState(void)
{
	qglClearDepth(1.0f);

	qglCullFace(GL_FRONT);

	qglColor4f(1, 1, 1, 1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if (qglActiveTextureARB)
	{
		GL_SelectTexture(1);
		GL_TextureMode(r_textureMode->string);
		GL_TexEnv(GL_MODULATE);
		qglDisable(GL_TEXTURE_2D);
		GL_SelectTexture(0);
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode(r_textureMode->string);
	GL_TexEnv(GL_MODULATE);

	qglShadeModel(GL_SMOOTH);
	qglDepthFunc(GL_LEQUAL);

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState(GL_VERTEX_ARRAY);

	// make sure our GL state vector is set correctly
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

#ifdef HAVE_GLES
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//*SEB* ? not the best place !
	glLockArraysEXT = NULL;
	glUnlockArraysEXT = NULL;
#else
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
	qglDepthMask(GL_TRUE);
	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_SCISSOR_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);
}

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char *string)
{
	char       buffer[1024];
	const char *p   = string;
	int        size = strlen(string);

	while (size > 0)
	{
		Q_strncpyz(buffer, p, sizeof(buffer));
		ri.Printf(PRINT_ALL, "%s", buffer);
		p    += 1023;
		size -= 1023;
	}
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f(void)
{
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	ri.Printf(PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string);
	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string);
	ri.Printf(PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string);

	ri.Printf(PRINT_ALL, "GL_EXTENSIONS: ");
	R_PrintLongString((char *)qglGetString(GL_EXTENSIONS));

	ri.Printf(PRINT_ALL, "\nGL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	ri.Printf(PRINT_ALL, "GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig.maxActiveTextures);
	ri.Printf(PRINT_ALL, "PIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	ri.Printf(PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1]);

	if (glConfig.displayFrequency)
	{
		ri.Printf(PRINT_ALL, "%d\n", glConfig.displayFrequency);
	}
	else
	{
		ri.Printf(PRINT_ALL, "N/A\n");
	}

	if (glConfig.deviceSupportsGamma)
	{
		ri.Printf(PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits);
	}
	else
	{
		ri.Printf(PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits);
	}

	// rendering primitives
	{
		int primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri.Printf(PRINT_ALL, "rendering primitives: ");
		primitives = r_primitives->integer;
#ifdef HAVE_GLES
		primitives = 2;
#else
		if (primitives == 0)
		{
			if (qglLockArraysEXT)
			{
				primitives = 2;
			}
			else
			{
				primitives = 1;
			}
		}
#endif
		if (primitives == -1)
		{
			ri.Printf(PRINT_ALL, "none\n");
		}
		else if (primitives == 2)
		{
			ri.Printf(PRINT_ALL, "single glDrawElements\n");
		}
		else if (primitives == 1)
		{
			ri.Printf(PRINT_ALL, "multiple glArrayElement\n");
		}
		else if (primitives == 3)
		{
			ri.Printf(PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n");
		}
	}

	ri.Printf(PRINT_ALL, "texturemode: %s\n", r_textureMode->string);
	ri.Printf(PRINT_ALL, "picmip: %d\n", r_picmip->integer);
	ri.Printf(PRINT_ALL, "texture bits: %d\n", r_texturebits->integer);
	ri.Printf(PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0]);
#ifdef HAVE_GLES
	ri.Printf(PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[0]);
#else
	ri.Printf(PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0]);
#endif
	ri.Printf(PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0]);
	ri.Printf(PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE]);

	if (r_finish->integer)
	{
		ri.Printf(PRINT_ALL, "Forcing glFinish\n");
	}
	ri.Printf(PRINT_ALL, "Renderer: vanilla\n");
}

/*
===============
R_Register
===============
*/
void R_Register(void)
{
#ifdef USE_RENDERER_DLOPEN
	com_altivec = ri.Cvar_Get("com_altivec", "1", CVAR_ARCHIVE);
#endif

	// latched and archived variables
	r_allowExtensions           = ri.Cvar_Get("r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ext_compressed_textures   = ri.Cvar_Get("r_ext_compressed_textures", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ext_multitexture          = ri.Cvar_Get("r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ext_compiled_vertex_array = ri.Cvar_Get("r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ext_texture_env_add       = ri.Cvar_Get("r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_ext_texture_filter_anisotropic = ri.Cvar_Get("r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ext_max_anisotropy             = ri.Cvar_Get("r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH);

	r_picmip          = ri.Cvar_Get("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH); //----(SA)   mod for DM and DK for id build.  was "1" // JPW NERVE pushed back to 1
	r_roundImagesDown = ri.Cvar_Get("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_colorMipLevels = ri.Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH);
	AssertCvarRange(r_picmip, 0, 3, qtrue);
	r_detailTextures = ri.Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_texturebits    = ri.Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_colorbits      = ri.Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_stencilbits    = ri.Cvar_Get("r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);

	r_depthbits = ri.Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);

	r_ext_multisample = ri.Cvar_Get("r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_ext_multisample, 0, 4, qtrue);
	r_overBrightBits = ri.Cvar_Get("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH);    // disable overbrightbits by default
	AssertCvarRange(r_overBrightBits, 0, 1, qtrue);                                     // limit to overbrightbits 1 (sorry 1337 players)
	r_ignorehwgamma     = ri.Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);      // use hw gamma by default
	r_mode              = ri.Cvar_Get("r_mode", "4", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_oldMode           = ri.Cvar_Get("r_oldMode", "", CVAR_ARCHIVE);                     // previous "good" video mode
	r_fullscreen        = ri.Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_noborder          = ri.Cvar_Get("r_noborder", "0", CVAR_ARCHIVE);
	r_customwidth       = ri.Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH);
	r_customheight      = ri.Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	r_customPixelAspect = ri.Cvar_Get("r_customPixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_simpleMipMaps     = ri.Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_uiFullScreen      = ri.Cvar_Get("r_uifullscreen", "0", 0);

	r_subdivisions = ri.Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);

	r_stereoEnabled  = ri.Cvar_Get("r_stereoEnabled", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignoreFastPath = ri.Cvar_Get("r_ignoreFastPath", "0", CVAR_ARCHIVE | CVAR_LATCH);    // use fast path by default
	r_greyscale      = ri.Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH);

	// temporary latched variables that can only change over a restart
	r_mapOverBrightBits = ri.Cvar_Get("r_mapOverBrightBits", "2", CVAR_LATCH);
	AssertCvarRange(r_mapOverBrightBits, 0, 3, qtrue);
	r_intensity = ri.Cvar_Get("r_intensity", "1", CVAR_LATCH);
	AssertCvarRange(r_intensity, 0, 1.5, qfalse);
	r_singleShader = ri.Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH);

	// archived variables that can change at any time
	r_lodCurveError = ri.Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE);
	r_lodbias       = ri.Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE);
	r_flares        = ri.Cvar_Get("r_flares", "1", CVAR_ARCHIVE);
	r_znear         = ri.Cvar_Get("r_znear", "3", CVAR_CHEAT); // changed it to 3 (from 4) because of lean/fov cheats
	AssertCvarRange(r_znear, 0.001f, 200, qtrue);
	r_zfar = ri.Cvar_Get("r_zfar", "0", CVAR_CHEAT);

	r_ignoreGLErrors = ri.Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_fastsky        = ri.Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE);

	r_drawSun      = ri.Cvar_Get("r_drawSun", "1", CVAR_ARCHIVE);
	r_dynamiclight = ri.Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE);
	r_finish       = ri.Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode  = ri.Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_swapInterval = ri.Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
	r_gamma        = ri.Cvar_Get("r_gamma", "1.3", CVAR_ARCHIVE);

	r_facePlaneCull = ri.Cvar_Get("r_facePlaneCull", "1", CVAR_ARCHIVE);

	r_railWidth         = ri.Cvar_Get("r_railWidth", "16", CVAR_ARCHIVE);
	r_railSegmentLength = ri.Cvar_Get("r_railSegmentLength", "32", CVAR_ARCHIVE);

	r_primitives = ri.Cvar_Get("r_primitives", "0", CVAR_ARCHIVE);

	r_ambientScale  = ri.Cvar_Get("r_ambientScale", "0.5", CVAR_CHEAT);
	r_directedScale = ri.Cvar_Get("r_directedScale", "1", CVAR_CHEAT);

	// temporary variables that can change at any time
	r_showImages = ri.Cvar_Get("r_showImages", "0", CVAR_TEMP);

	r_debugLight   = ri.Cvar_Get("r_debuglight", "0", CVAR_TEMP);
	r_debugSort    = ri.Cvar_Get("r_debugSort", "0", CVAR_CHEAT);
	r_printShaders = ri.Cvar_Get("r_printShaders", "0", 0);
	r_saveFontData = ri.Cvar_Get("r_saveFontData", "0", 0);

	// with r_cache enabled, non-win32 OSes were leaking 24Mb per R_Init..
	r_cache        = ri.Cvar_Get("r_cache", "1", CVAR_LATCH); // leaving it as this for backwards compability. but it caches models and shaders also
	r_cacheShaders = ri.Cvar_Get("r_cacheShaders", "1", CVAR_LATCH);

	r_cacheModels    = ri.Cvar_Get("r_cacheModels", "1", CVAR_LATCH);
	r_cacheGathering = ri.Cvar_Get("cl_cacheGathering", "0", 0);
	r_bonesDebug     = ri.Cvar_Get("r_bonesDebug", "0", CVAR_CHEAT);

	r_wolffog = ri.Cvar_Get("r_wolffog", "1", CVAR_ARCHIVE);

	r_nocurves    = ri.Cvar_Get("r_nocurves", "0", CVAR_CHEAT);
	r_drawworld   = ri.Cvar_Get("r_drawworld", "1", CVAR_CHEAT);
	r_drawfoliage = ri.Cvar_Get("r_drawfoliage", "1", CVAR_CHEAT);
	r_lightmap    = ri.Cvar_Get("r_lightmap", "0", CVAR_CHEAT);
	r_portalOnly  = ri.Cvar_Get("r_portalOnly", "0", CVAR_CHEAT);

	r_flareSize = ri.Cvar_Get("r_flareSize", "40", CVAR_CHEAT);
	ri.Cvar_Set("r_flareFade", "5");    // to force this when people already have "7" in their config
	r_flareFade = ri.Cvar_Get("r_flareFade", "5", CVAR_CHEAT);

	r_skipBackEnd = ri.Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = ri.Cvar_Get("r_measureOverdraw", "0", CVAR_CHEAT);
	r_lodscale        = ri.Cvar_Get("r_lodscale", "5", CVAR_CHEAT);
	r_norefresh       = ri.Cvar_Get("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities    = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_ignore          = ri.Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_nocull          = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_novis           = ri.Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_showcluster     = ri.Cvar_Get("r_showcluster", "0", CVAR_CHEAT);
	r_speeds          = ri.Cvar_Get("r_speeds", "0", CVAR_CHEAT);

	r_logFile      = ri.Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_debugSurface = ri.Cvar_Get("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind       = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_showtris     = ri.Cvar_Get("r_showtris", "0", CVAR_CHEAT);
	r_trisColor    = ri.Cvar_Get("r_trisColor", "1.0 1.0 1.0 1.0", CVAR_ARCHIVE);
	r_showsky      = ri.Cvar_Get("r_showsky", "0", CVAR_CHEAT);
	r_shownormals  = ri.Cvar_Get("r_shownormals", "0", CVAR_CHEAT);
	r_normallength = ri.Cvar_Get("r_normallength", "0.5", CVAR_ARCHIVE);
	//r_showmodelbounds = ri.Cvar_Get("r_showmodelbounds", "0", CVAR_CHEAT); // see RB_MDM_SurfaceAnim()
	r_clear        = ri.Cvar_Get("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = ri.Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT);
	r_offsetUnits  = ri.Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT);
	r_drawBuffer   = ri.Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
	r_lockpvs      = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals    = ri.Cvar_Get("r_noportals", "0", CVAR_CHEAT);
	r_shadows      = ri.Cvar_Get("cg_shadows", "1", 0);

	r_screenshotJpegQuality = ri.Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

	r_portalsky = ri.Cvar_Get("cg_skybox", "1", 0);

	// note: MAX_POLYS and MAX_POLYVERTS are heavily increased in ET compared to q3
	//       - but run 20 bots on oasis and you'll see limits reached (developer 1)
	//       - modern computers can deal with more than our old default values -> users can increase this now to MAX_POLYS/MAX_POLYVERTS
	r_maxpolys = ri.Cvar_Get("r_maxpolys", va("%d", MIN_POLYS), CVAR_LATCH);     // now latched to check against used r_maxpolys and not MAX_POLYS
	AssertCvarRange(r_maxpolys, MIN_POLYS, MAX_POLYS, qtrue); // MIN_POLYS was old static value
	r_maxpolyverts = ri.Cvar_Get("r_maxpolyverts", va("%d", MIN_POLYVERTS), CVAR_LATCH); // now latched to check against used r_maxpolyverts and not MAX_POLYVERTS
	AssertCvarRange(r_maxpolyverts, MIN_POLYVERTS, MAX_POLYVERTS, qtrue); // MIN_POLYVERTS was old static value

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand("imagelist", R_ImageList_f);
	ri.Cmd_AddCommand("shaderlist", R_ShaderList_f);
	ri.Cmd_AddCommand("skinlist", R_SkinList_f);
	ri.Cmd_AddCommand("modellist", R_Modellist_f);
	ri.Cmd_AddCommand("modelist", R_ModeList_f);
	ri.Cmd_AddCommand("screenshot", R_ScreenShot_f);
	ri.Cmd_AddCommand("screenshotJPEG", R_ScreenShotJPEG_f);
	ri.Cmd_AddCommand("gfxinfo", GfxInfo_f);
	ri.Cmd_AddCommand("taginfo", R_TagInfo_f);
	ri.Cmd_AddCommand("minimize", GLimp_Minimize);
}

/*
===============
R_Init
===============
*/
void R_Init(void)
{
	int  err;
	int  i;
	byte *ptr;

	ri.Printf(PRINT_ALL, "----- R_Init -----\n");

	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));

	tess.xyz          = tess_xyz;
	tess.texCoords0   = tess_texCoords0;
	tess.texCoords1   = tess_texCoords1;
	tess.indexes      = tess_indexes;
	tess.normal       = tess_normal;
	tess.vertexColors = tess_vertexColors;

	tess.maxShaderVerts    = SHADER_MAX_VERTEXES;
	tess.maxShaderIndicies = SHADER_MAX_INDEXES;

	if ((intptr_t) tess.xyz & 15)
	{
		ri.Printf(PRINT_WARNING, "tess.xyz not 16 byte aligned\n");
	}
	Com_Memset(tess.constantColor255, 255, sizeof(tess.constantColor255));

	// init function tables
	for (i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i]             = sin(DEG2RAD(i * 360.0f / (( float ) (FUNCTABLE_SIZE - 1))));
		tr.squareTable[i]          = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i]        = ( float ) i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if (i < FUNCTABLE_SIZE / 2)
		{
			if (i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = ( float ) i / (FUNCTABLE_SIZE / 4);
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}

	// init the virtual memory
	R_Hunk_Begin();

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	ptr = ri.Hunk_Alloc(sizeof(*backEndData) + sizeof(srfPoly_t) * r_maxpolys->integer + sizeof(polyVert_t) * r_maxpolyverts->integer, h_low);

	backEndData            = (backEndData_t *) ptr;
	backEndData->polys     = (srfPoly_t *) ((char *) ptr + sizeof(*backEndData));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof(*backEndData) + sizeof(srfPoly_t) * r_maxpolys->integer);

	R_InitNextFrame();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	err = qglGetError();
	if (err != GL_NO_ERROR)
	{
		ri.Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);
	}

	ri.Printf(PRINT_ALL, "----- finished R_Init -----\n");
}

void R_PurgeCache(void)
{
	R_PurgeShaders(9999999);
	R_PurgeBackupImages(9999999);
	R_PurgeModels(9999999);
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown(qboolean destroyWindow)
{
	ri.Printf(PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow);

	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshotJPEG");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("shaderlist");
	ri.Cmd_RemoveCommand("skinlist");
	ri.Cmd_RemoveCommand("gfxinfo");
	ri.Cmd_RemoveCommand("minimize");
	ri.Cmd_RemoveCommand("modelist");
	ri.Cmd_RemoveCommand("shaderstate");
	ri.Cmd_RemoveCommand("taginfo");

	// keep a backup of the current images if possible
	// clean out any remaining unused media from the last backup
	R_PurgeCache();

	if (r_cache->integer)
	{
		if (tr.registered)
		{
			if (destroyWindow)
			{
				R_IssuePendingRenderCommands();
				R_DeleteTextures();
			}
			else
			{
				// backup the current media
				R_BackupModels();
				R_BackupShaders();
				R_BackupImages();
			}
		}
	}
	else if (tr.registered)
	{
		R_IssuePendingRenderCommands();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if (destroyWindow)
	{
		GLimp_Shutdown();

		// release the virtual memory
		R_Hunk_End();
		R_FreeImageBuffer();
		ri.Tag_Free();  // wipe all render alloc'd zone memory
	}

	tr.registered = qfalse;
}

/**
 * @brief Touch all images to make sure they are resident
 */
void RE_EndRegistration(void)
{
	R_IssuePendingRenderCommands();
	/*
	RB: disabled unneeded reference to Sys_LowPhysicalMemory
	if (!Sys_LowPhysicalMemory())
	{
	//              RB_ShowImages();
	}
	*/
}

void R_DebugPolygon(int color, int numPoints, float *points);

/*
==================
GetRefAPI
==================
*/
#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t * QDECL GetRefAPI(int apiVersion, refimport_t *rimp)
#else
refexport_t * GetRefAPI(int apiVersion, refimport_t * rimp)
#endif
{
	static refexport_t re;

	ri = *rimp;

	Com_Memset(&re, 0, sizeof(re));

	if (apiVersion != REF_API_VERSION)
	{
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
		          REF_API_VERSION, apiVersion);
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel     = RE_RegisterModel;
	re.RegisterSkin      = RE_RegisterSkin;

	re.GetSkinModel       = RE_GetSkinModel;
	re.GetShaderFromModel = RE_GetShaderFromModel;

	re.RegisterShader      = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld           = RE_LoadWorldMap;
	re.SetWorldVisData     = RE_SetWorldVisData;
	re.EndRegistration     = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame   = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.ProjectDecal  = RE_ProjectDecal;
	re.ClearDecals   = RE_ClearDecals;

	re.LerpTag     = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene          = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;

	re.AddPolyToScene  = RE_AddPolyToScene;
	re.AddPolysToScene = RE_AddPolysToScene;
	re.AddLightToScene = RE_AddLightToScene;

	re.AddCoronaToScene = RE_AddCoronaToScene;
	re.SetFog           = R_SetFog;

	re.RenderScene      = RE_RenderScene;
	re.SaveViewParms    = RE_SaveViewParms;
	re.RestoreViewParms = RE_RestoreViewParms;

	re.SetColor               = RE_SetColor;
	re.DrawStretchPic         = RE_StretchPic;
	re.DrawRotatedPic         = RE_RotatedPic;
	re.Add2dPolys             = RE_2DPolyies;
	re.DrawStretchPicGradient = RE_StretchPicGradient;
	re.DrawStretchRaw         = RE_StretchRaw;
	re.UploadCinematic        = RE_UploadCinematic;
	re.RegisterFont           = RE_RegisterFont;
	re.RemapShader            = R_RemapShader;
	re.GetEntityToken         = R_GetEntityToken;

	re.DrawDebugPolygon = R_DebugPolygon;
	re.DrawDebugText    = R_DebugText;

	re.AddPolyBufferToScene = RE_AddPolyBufferToScene;

	re.SetGlobalFog = RE_SetGlobalFog;

	re.inPVS = R_inPVS;

	re.purgeCache = R_PurgeCache;

	re.LoadDynamicShader = RE_LoadDynamicShader;
	re.GetTextureId      = R_GetTextureId;

	re.RenderToTexture = RE_RenderToTexture;

	re.Finish         = RE_Finish;
	re.TakeVideoFrame = RE_TakeVideoFrame;

	return &re;
}

#ifdef USE_RENDERER_DLOPEN
void QDECL Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri.Printf(PRINT_ALL, "%s", text);
}

/*
void QDECL Com_DPrintf(const char *msg, ...)
{
    va_list argptr;
    char    text[1024];

    va_start(argptr, msg);
    Q_vsnprintf(text, sizeof(text), msg, argptr);
    va_end(argptr);

    ri.Printf(PRINT_DEVELOPER, "%s", text);
}
*/

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}
#endif
