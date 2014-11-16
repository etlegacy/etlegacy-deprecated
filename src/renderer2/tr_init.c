/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 */
/**
 * @file renderer2/tr_init.c
 * @brief Functions that are not called every frame
 */

#include "tr_local.h"

glconfig_t  glConfig;
glconfig2_t glConfig2;

glstate_t glState;

float displayAspect = 0.0f;

static void GfxInfo_f(void);

cvar_t *r_glMajorVersion;
cvar_t *r_glMinorVersion;
cvar_t *r_glDebugProfile;

#ifdef USE_GLSL_OPTIMIZER
cvar_t *r_glslOptimizer;
#endif

cvar_t *r_flares;
cvar_t *r_flareSize;
cvar_t *r_flareFade;

cvar_t *r_railWidth;
cvar_t *r_railCoreWidth;
cvar_t *r_railSegmentLength;

cvar_t *r_ignore;

cvar_t *r_znear;
cvar_t *r_zfar;

cvar_t *r_skipBackEnd;
cvar_t *r_skipLightBuffer;

cvar_t *r_ignorehwgamma;
cvar_t *r_measureOverdraw;

cvar_t *r_fastsky;
cvar_t *r_drawSun;

cvar_t *r_lodBias;
cvar_t *r_lodScale;
cvar_t *r_lodTest;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_drawpolies;
cvar_t *r_speeds;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_facePlaneCull;
cvar_t *r_showcluster;
cvar_t *r_nocurves;
cvar_t *r_noLightScissors;
cvar_t *r_noLightVisCull;
cvar_t *r_noInteractionSort;
cvar_t *r_dynamicLight;
cvar_t *r_staticLight;
cvar_t *r_dynamicLightCastShadows;
cvar_t *r_precomputedLighting;
cvar_t *r_vertexLighting;
cvar_t *r_compressDiffuseMaps;
cvar_t *r_compressSpecularMaps;
cvar_t *r_compressNormalMaps;
cvar_t *r_heatHazeFix;
cvar_t *r_noMarksOnTrisurfs;
cvar_t *r_recompileShaders;

cvar_t *r_ext_compressed_textures;
cvar_t *r_ext_occlusion_query;
cvar_t *r_ext_texture_non_power_of_two;
cvar_t *r_ext_draw_buffers;
cvar_t *r_ext_vertex_array_object;
cvar_t *r_ext_half_float_pixel;
cvar_t *r_ext_texture_float;
cvar_t *r_ext_stencil_wrap;
cvar_t *r_ext_texture_filter_anisotropic;
cvar_t *r_ext_stencil_two_side;
cvar_t *r_ext_separate_stencil;
cvar_t *r_ext_depth_bounds_test;
cvar_t *r_ext_framebuffer_object;
cvar_t *r_ext_packed_depth_stencil;
cvar_t *r_ext_framebuffer_blit;
cvar_t *r_extx_framebuffer_mixed_formats;
cvar_t *r_ext_generate_mipmap;

cvar_t *r_ignoreGLErrors;
cvar_t *r_logFile;

cvar_t *r_stencilbits;
cvar_t *r_depthbits;
cvar_t *r_colorbits;
cvar_t *r_stereo;

cvar_t *r_drawBuffer;
cvar_t *r_uiFullScreen;
cvar_t *r_shadows;
cvar_t *r_softShadows;
cvar_t *r_shadowBlur;

cvar_t *r_shadowMapQuality;
cvar_t *r_shadowMapSizeUltra;
cvar_t *r_shadowMapSizeVeryHigh;
cvar_t *r_shadowMapSizeHigh;
cvar_t *r_shadowMapSizeMedium;
cvar_t *r_shadowMapSizeLow;

cvar_t *r_shadowMapSizeSunUltra;
cvar_t *r_shadowMapSizeSunVeryHigh;
cvar_t *r_shadowMapSizeSunHigh;
cvar_t *r_shadowMapSizeSunMedium;
cvar_t *r_shadowMapSizeSunLow;

cvar_t *r_shadowOffsetFactor;
cvar_t *r_shadowOffsetUnits;
cvar_t *r_shadowLodBias;
cvar_t *r_shadowLodScale;
cvar_t *r_noShadowPyramids;
cvar_t *r_cullShadowPyramidFaces;
cvar_t *r_cullShadowPyramidCurves;
cvar_t *r_cullShadowPyramidTriangles;
cvar_t *r_debugShadowMaps;
cvar_t *r_noShadowFrustums;
cvar_t *r_noLightFrustums;
cvar_t *r_shadowMapLuminanceAlpha;
cvar_t *r_shadowMapLinearFilter;
cvar_t *r_lightBleedReduction;
cvar_t *r_overDarkeningFactor;
cvar_t *r_shadowMapDepthScale;
cvar_t *r_parallelShadowSplits;
cvar_t *r_parallelShadowSplitWeight;
cvar_t *r_lightSpacePerspectiveWarping;

cvar_t *r_mode;
cvar_t *r_collapseStages;
cvar_t *r_nobind;
cvar_t *r_singleShader;
cvar_t *r_drawfoliage;
cvar_t *r_roundImagesDown;
cvar_t *r_colorMipLevels;
cvar_t *r_picmip;
cvar_t *r_finish;
cvar_t *r_clear;
cvar_t *r_swapInterval;
cvar_t *r_textureMode;
cvar_t *r_offsetFactor;
cvar_t *r_offsetUnits;
cvar_t *r_forceSpecular;
cvar_t *r_specularExponent;
cvar_t *r_specularExponent2;
cvar_t *r_specularScale;
cvar_t *r_normalScale;
cvar_t *r_normalMapping;
cvar_t *r_wrapAroundLighting;
cvar_t *r_halfLambertLighting;
cvar_t *r_rimLighting;
cvar_t *r_rimExponent;
cvar_t *r_gamma;
cvar_t *r_intensity;
cvar_t *r_lockpvs;
cvar_t *r_noportals;
cvar_t *r_portalOnly;
cvar_t *r_portalSky;

cvar_t *r_subdivisions;
cvar_t *r_stitchCurves;

cvar_t *r_fullscreen;

cvar_t *r_customwidth;
cvar_t *r_customheight;
cvar_t *r_customaspect;

cvar_t *r_overBrightBits;
cvar_t *r_mapOverBrightBits;

cvar_t *r_debugSurface;
cvar_t *r_simpleMipMaps;

cvar_t *r_showImages;

cvar_t *r_wolfFog;
cvar_t *r_noFog;

cvar_t *r_forceAmbient;
cvar_t *r_ambientScale;
cvar_t *r_lightScale;
cvar_t *r_debugLight;
cvar_t *r_debugSort;
cvar_t *r_printShaders;
cvar_t *r_saveFontData;

cvar_t *r_maxpolys;
cvar_t *r_maxpolyverts;

cvar_t *r_showTris;
cvar_t *r_showSky;
cvar_t *r_showShadowVolumes;
cvar_t *r_showShadowLod;
cvar_t *r_showShadowMaps;
cvar_t *r_showSkeleton;
cvar_t *r_showEntityTransforms;
cvar_t *r_showLightTransforms;
cvar_t *r_showLightInteractions;
cvar_t *r_showLightScissors;
cvar_t *r_showLightBatches;
cvar_t *r_showLightGrid;
cvar_t *r_showOcclusionQueries;
cvar_t *r_showBatches;
cvar_t *r_showLightMaps;
cvar_t *r_showDeluxeMaps;
cvar_t *r_showAreaPortals;
cvar_t *r_showCubeProbes;
cvar_t *r_showBspNodes;
cvar_t *r_showParallelShadowSplits;
cvar_t *r_showDecalProjectors;

cvar_t *r_showDeferredDiffuse;
cvar_t *r_showDeferredNormal;
cvar_t *r_showDeferredSpecular;
cvar_t *r_showDeferredPosition;
cvar_t *r_showDeferredRender;
cvar_t *r_showDeferredLight;

cvar_t *r_vboFaces;
cvar_t *r_vboCurves;
cvar_t *r_vboTriangles;
cvar_t *r_vboShadows;
cvar_t *r_vboLighting;
cvar_t *r_vboModels;
cvar_t *r_vboOptimizeVertices;
cvar_t *r_vboVertexSkinning;
cvar_t *r_vboSmoothNormals;

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
cvar_t *r_mergeClusterSurfaces;
cvar_t *r_mergeClusterFaces;
cvar_t *r_mergeClusterCurves;
cvar_t *r_mergeClusterTriangles;
#endif

cvar_t *r_deferredShading;
cvar_t *r_parallaxMapping;
cvar_t *r_parallaxDepthScale;

cvar_t *r_dynamicBspOcclusionCulling;
cvar_t *r_dynamicEntityOcclusionCulling;
cvar_t *r_dynamicLightOcclusionCulling;
cvar_t *r_chcMaxPrevInvisNodesBatchSize;
cvar_t *r_chcMaxVisibleFrames;
cvar_t *r_chcVisibilityThreshold;
cvar_t *r_chcIgnoreLeaves;

cvar_t *r_hdrRendering;
cvar_t *r_hdrMinLuminance;
cvar_t *r_hdrMaxLuminance;
cvar_t *r_hdrKey;
cvar_t *r_hdrContrastThreshold;
cvar_t *r_hdrContrastOffset;
cvar_t *r_hdrLightmap;
cvar_t *r_hdrLightmapExposure;
cvar_t *r_hdrLightmapGamma;
cvar_t *r_hdrLightmapCompensate;
cvar_t *r_hdrToneMappingOperator;
cvar_t *r_hdrGamma;
cvar_t *r_hdrDebug;

cvar_t *r_screenSpaceAmbientOcclusion;
cvar_t *r_depthOfField;
cvar_t *r_reflectionMapping;
cvar_t *r_highQualityNormalMapping;
cvar_t *r_bloom;
cvar_t *r_bloomBlur;
cvar_t *r_bloomPasses;
cvar_t *r_rotoscope;
cvar_t *r_rotoscopeBlur;
cvar_t *r_cameraPostFX;
cvar_t *r_cameraVignette;
cvar_t *r_cameraFilmGrainScale;

cvar_t *r_evsmPostProcess;
cvar_t *r_detailTextures;

cvar_t *r_noborder;
cvar_t *r_stereoEnabled;
cvar_t *r_ext_multisample;
cvar_t *r_ext_multitexture;
cvar_t *r_ext_texture_env_add;
cvar_t *r_allowExtensions;

cvar_t *r_screenshotJpegQuality;

/**
* @brief This function is responsible for initializing a valid OpenGL subsystem.  This
* is done by calling GLimp_Init (which gives us a working OGL subsystem) then
* setting variables, checking GL constants, and reporting the gfx system config
* to the user.
*/
static qboolean InitOpenGL(void)
{
	char renderer_buffer[1024];

	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//      - r_fullscreen
	//      - r_glDriver
	//      - r_mode
	//      - r_(color|depth|stencil)bits
	//      - r_ignorehwgamma
	//      - r_gamma
	//

	if (glConfig.vidWidth == 0)
	{
		GLint temp;

		GLimp_Init();

		GL_CheckErrors();

		strcpy(renderer_buffer, glConfig.renderer_string);
		Q_strlwr(renderer_buffer);

		// OpenGL driver constants
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if (glConfig.maxTextureSize <= 0)
		{
			glConfig.maxTextureSize = 0;
		}

		GLSL_InitGPUShaders();
	}

	GL_CheckErrors();

	// print info
	GfxInfo_f();
	GL_CheckErrors();

	// set default state
	GL_SetDefaultState();
	GL_CheckErrors();

	return qtrue;
}

void GL_CheckErrors_(const char *fileName, int line)
{
	int  err;
	char s[128];

	if (r_ignoreGLErrors->integer)
	{
		return;
	}

	err = glGetError();
	if (err == GL_NO_ERROR)
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
	case GL_TABLE_TOO_LARGE:
		strcpy(s, "GL_TABLE_TOO_LARGE");
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
		strcpy(s, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT");
		break;
	default:
		Com_sprintf(s, sizeof(s), "0x%X", err);
		break;
	}

	Ren_Fatal("caught OpenGL error: %s in file %s line %i", s, fileName, line);
}

/*
==============================================================================
                        SCREEN SHOTS

screenshots get written in fs_homepath + fs_gamedir
.. base/screenshots\*.*

three commands: "screenshot", "screenshotJPEG" and "screenshotPNG"

the format is etxreal-YYYY_MM_DD-HH_MM_SS-MS.tga/jpeg/png
==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.
Prepends the specified number of (uninitialized) bytes to the buffer.

The returned buffer must be freed with ri.Hunk_FreeTempMemory().
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
	buffer = (byte *)ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);

	bufstart = (byte *)PADP(( intptr_t ) buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/*
==================
R_TakeScreenshot
==================
*/
static void RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
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
static void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
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
	const screenshotCommand_t *cmd = (const screenshotCommand_t *)data;

	switch (cmd->format)
	{
	case SSF_TGA:
		RB_TakeScreenshot(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
		break;
	case SSF_JPEG:
		RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
		break;
	case SSF_PNG:
		Ren_Print("PNG output is not implemented");
		break;
	}

	return (const void *)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot(const char *name, ssFormat_t format)
{
	static char         fileName[MAX_OSPATH]; // bad things may happen if two screenshots per frame are taken.
	screenshotCommand_t *cmd;
	int                 lastNumber;

	cmd = (screenshotCommand_t *) R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	if (ri.Cmd_Argc() == 2)
	{
		Com_sprintf(fileName, sizeof(fileName), "screenshots/" PRODUCT_NAME "-%s.%s", ri.Cmd_Argv(1), name);
	}
	else
	{
		qtime_t t;

		ri.RealTime(&t);

		// scan for a free filename
		for (lastNumber = 0; lastNumber <= 999; lastNumber++)
		{
			Com_sprintf(fileName, sizeof(fileName), "screenshots/" PRODUCT_NAME "-%04d%02d%02d-%02d%02d%02d-%03d.%s",
			            1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, lastNumber, name);

			if (!ri.FS_FileExists(fileName))
			{
				break;          // file doesn't exist
			}
		}

		if (lastNumber == 1000)
		{
			Ren_Print("ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}
	Ren_Print("Wrote %s\n", fileName);

	cmd->commandId = RC_SCREENSHOT;
	cmd->x         = 0;
	cmd->y         = 0;
	cmd->width     = glConfig.vidWidth;
	cmd->height    = glConfig.vidHeight;
	cmd->fileName  = fileName;
	cmd->format    = format;
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [filename]
==================
*/
static void R_ScreenShot_f(void)
{
	R_TakeScreenshot("tga", SSF_TGA);
}

static void R_ScreenShotJPEG_f(void)
{
	R_TakeScreenshot("jpg", SSF_JPEG);
}

static void R_ScreenShotPNG_f(void)
{
	R_TakeScreenshot("png", SSF_PNG);
}

/*
static int QDECL MaterialNameCompare(const void *a, const void *b)
{
    char           *s1, *s2;
    int             c1, c2;

    s1 = *(char **)a;
    s2 = *(char **)b;

    do
    {
        c1 = *s1++;
        c2 = *s2++;

        if(c1 >= 'a' && c1 <= 'z')
        {
            c1 -= ('a' - 'A');
        }
        if(c2 >= 'a' && c2 <= 'z')
        {
            c2 -= ('a' - 'A');
        }

        if(c1 == '\\' || c1 == ':')
        {
            c1 = '/';
        }
        if(c2 == '\\' || c2 == ':')
        {
            c2 = '/';
        }

        if(c1 < c2)
        {
            // strings not equal
            return -1;
        }
        if(c1 > c2)
        {
            return 1;
        }
    } while(c1);

    // strings are equal
    return 0;
}
*/

/*
static void R_GenerateMaterialFile_f(void)
{
    char          **dirnames;
    int             ndirs;
    int             i;
    fileHandle_t    f;
    char            fileName[MAX_QPATH];
    char            fileName2[MAX_QPATH];
    char            cleanName[MAX_QPATH];
    char            cleanName2[MAX_QPATH];
    char            baseName[MAX_QPATH];
    char            baseName2[MAX_QPATH];
    char            path[MAX_QPATH];
    char            extension[MAX_QPATH];
    int				len;

    if(Cmd_Argc() < 3)
    {
        Ren_Print("usage: generatemtr <directory> [image extension]\n");
        return;
    }

    Q_strncpyz(path, ri.Cmd_Argv(1), sizeof(path));

    Q_strncpyz(extension, ri.Cmd_Argv(2), sizeof(extension));
    Q_strreplace(extension, sizeof(extension), ".", "");

    Q_strncpyz(fileName, ri.Cmd_Argv(1), sizeof(fileName));
    Com_DefaultExtension(fileName, sizeof(fileName), ".mtr");
    Ren_Print("Writing %s.\n", fileName);
    f = FS_FOpenFileWrite(fileName);
    if(!f)
    {
        Ren_Print("Couldn't write %s.\n", fileName);
        return;
    }

    FS_Printf(f, "// generated by XreaL\n\n");

    dirnames = FS_ListFiles(path, extension, &ndirs);

    qsort(dirnames, ndirs, sizeof(char *), MaterialNameCompare);

    for(i = 0; i < ndirs; i++)
    {
        // clean name
        Q_strncpyz(fileName, dirnames[i], sizeof(fileName));
        Q_strncpyz(cleanName, dirnames[i], sizeof(cleanName));

        Q_strreplace(cleanName, sizeof(cleanName), "MaPZone[", "");
        Q_strreplace(cleanName, sizeof(cleanName), "]", "");
        Q_strreplace(cleanName, sizeof(cleanName), "&", "_");

        if(strcmp(fileName, cleanName))
        {
            Com_sprintf(fileName2, sizeof(fileName2), "%s/%s", path, fileName);
            Com_sprintf(cleanName2, sizeof(cleanName2), "%s/%s", path, cleanName);

            Ren_Print("renaming '%s' into '%s'\n", fileName2, cleanName2);
            FS_Rename(fileName2, cleanName2);
        }

        COM_StripExtension(cleanName, cleanName, sizeof(cleanName));
        if(!Q_stristr(cleanName, "_nm") && !Q_stristr(cleanName, "blend"))
        {
            Q_strncpyz(baseName, cleanName, sizeof(baseName));
            Q_strreplace(baseName, sizeof(baseName), "_diffuse", "");

            FS_Printf(f, "%s/%s\n", path, baseName);
            FS_Printf(f, "{\n");
            FS_Printf(f, "\t qer_editorImage\t %s/%s.%s\n", path, cleanName, extension);
            FS_Printf(f, "\n");
            FS_Printf(f, "\t parallax\n");
            FS_Printf(f, "\n");
            FS_Printf(f, "\t diffuseMap\t\t %s/%s.%s\n", path, baseName, extension);

            Com_sprintf(fileName, sizeof(fileName), "%s/%s_nm.%s", path, baseName, extension);
            if(ri.FS_ReadFile(fileName, NULL) > 0)
            {
                FS_Printf(f, "\t normalMap\t\t %s/%s_nm.%s\n", path, baseName, extension);
            }

            Q_strncpyz(baseName2, baseName, sizeof(baseName2));
            len = strlen(baseName);
            baseName2[len -1] = '\0';

            Com_sprintf(fileName, sizeof(fileName), "%s/speculars/%s_s.%s", path, baseName2, extension);
            if(ri.FS_ReadFile(fileName, NULL) > 0)
            {
                FS_Printf(f, "\t specularMap\t %s/speculars/%s_s.%s\n", path, baseName2, extension);
            }

            Com_sprintf(fileName, sizeof(fileName), "%s/%s.blend.%s", path, baseName, extension);
            if(ri.FS_ReadFile(fileName, NULL) > 0)
            {
                FS_Printf(f, "\t glowMap\t\t %s/%s.blend.%s\n", path, baseName, extension);
            }

            FS_Printf(f, "}\n\n");
        }
    }
    ri.FS_FreeFileList(dirnames);

    ri.FS_FCloseFile(f);
}
*/

//============================================================================

const void *RB_TakeVideoFrameCmd(const void *data)
{
	const videoFrameCommand_t *cmd = (const videoFrameCommand_t *)data;
	GLint                     packAlign;
	int                       lineLen, captureLineLen;
	byte                      *pixels;
	int                       i;
	int                       outputSize;
	int                       j;
	int                       aviLineLen;

	// RB: it is possible to we still have a videoFrameCommand_t but we already stopped
	// video recording
	if (ri.CL_VideoRecording())
	{
		// take care of alignment issues for reading RGB images..
		glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

		lineLen        = cmd->width * 3;
		captureLineLen = PAD(lineLen, packAlign);

		pixels = (byte *)PADP(cmd->captureBuffer, packAlign);
		glReadPixels(0, 0, cmd->width, cmd->height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

		if (tr.overbrightBits > 0 && glConfig.deviceSupportsGamma)
		{
			// this also runs over the padding...
			R_GammaCorrect(pixels, captureLineLen * cmd->height);
		}

		if (cmd->motionJpeg)
		{
			// Drop alignment and line padding bytes
			for (i = 0; i < cmd->height; ++i)
			{
				memmove(cmd->captureBuffer + i * lineLen, pixels + i * captureLineLen, lineLen);
			}

			outputSize = RE_SaveJPGToBuffer(cmd->encodeBuffer, 3 * cmd->width * cmd->height, r_screenshotJpegQuality->integer, cmd->width, cmd->height, cmd->captureBuffer, 0);
			ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, outputSize);
		}
		else
		{
			aviLineLen = PAD(lineLen, AVI_LINE_PADDING);

			for (i = 0; i < cmd->height; ++i)
			{
				for (j = 0; j < lineLen; j += 3)
				{
					cmd->encodeBuffer[i * aviLineLen + j + 0] = pixels[i * captureLineLen + j + 2];
					cmd->encodeBuffer[i * aviLineLen + j + 1] = pixels[i * captureLineLen + j + 1];
					cmd->encodeBuffer[i * aviLineLen + j + 2] = pixels[i * captureLineLen + j + 0];
				}
				while (j < aviLineLen)
				{
					cmd->encodeBuffer[i * aviLineLen + j++] = 0;
				}
			}

			ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, aviLineLen * cmd->height);
		}
	}

	return (const void *)(cmd + 1);
}

//============================================================================

void GL_SetDefaultState(void)
{
	int i;

	Ren_LogComment("--- GL_SetDefaultState ---\n");

	GL_ClearDepth(1.0f);

	if (glConfig.stencilBits >= 4)
	{
		GL_ClearStencil(128);
	}

	GL_FrontFace(GL_CCW);
	GL_CullFace(GL_FRONT);

	glState.faceCulling = CT_TWO_SIDED;
	glDisable(GL_CULL_FACE);

	GL_CheckErrors();

	glVertexAttrib4f(ATTR_INDEX_COLOR, 1, 1, 1, 1);

	GL_CheckErrors();

	// initialize downstream texture units if we're running
	// in a multitexture environment
	if (glConfig.driverType == GLDRV_OPENGL3)
	{
		i = 31;
	}
	else if (GLEW_ARB_multitexture)
	{
		i = glConfig.maxActiveTextures - 1;
	}
	else
	{
		i = 0;
	}

	for (; i >= 0; i--)
	{
		GL_SelectTexture(i);
		GL_TextureMode(r_textureMode->string);
	}

	GL_CheckErrors();

	GL_DepthFunc(GL_LEQUAL);

	// make sure our GL state vector is set correctly
	glState.glStateBits             = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
	glState.vertexAttribsState      = 0;
	glState.vertexAttribPointersSet = 0;

	glState.currentProgram = 0;
	glUseProgram(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glState.currentVBO = NULL;
	glState.currentIBO = NULL;

	GL_CheckErrors();

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	glEnableVertexAttribArray(ATTR_INDEX_POSITION);

	R_SetDefaultFBO();

	GL_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GL_DepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_ClearColor(GLCOLOR_BLACK);
	GL_ClearDepth(1.0);

	glDrawBuffer(GL_BACK);
	GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	GL_CheckErrors();

	glState.stackIndex = 0;
	for (i = 0; i < MAX_GLSTACK; i++)
	{
		MatrixIdentity(glState.modelViewMatrix[i]);
		MatrixIdentity(glState.projectionMatrix[i]);
		MatrixIdentity(glState.modelViewProjectionMatrix[i]);
	}
}

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.

FIXME: move to renderercommon
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
		Ren_Print("%s", buffer);
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
	/*const char     *enablestrings[] = {
	    "disabled",
	    "enabled"
	};*/
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	Ren_Print("\nGL_VENDOR: %s\n", glConfig.vendor_string);
	Ren_Print("GL_RENDERER: %s\n", glConfig.renderer_string);
	Ren_Print("GL_VERSION: %s\n", glConfig.version_string);

	//Lets not do this on gl3.2 context as the functionality is not supported.
	/*
	Ren_Print("GL_EXTENSIONS: ");
	R_PrintLongString((char *)qglGetString(GL_EXTENSIONS));
	*/

	Ren_Print("GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);

	if (glConfig.driverType != GLDRV_OPENGL3)
	{
		Ren_Print("GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.maxActiveTextures);
	}

	/*
	   if(glConfig.fragmentProgramAvailable)
	   {
	   Ren_Print("GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %d\n", glConfig.maxTextureImageUnits);
	   }
	 */

	Ren_Print("GL_SHADING_LANGUAGE_VERSION_ARB: %s\n", glConfig2.shadingLanguageVersion);

	Ren_Print("GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB %d\n", glConfig2.maxVertexUniforms);
	//Ren_Print("GL_MAX_VARYING_FLOATS_ARB %d\n", glConfig2.maxVaryingFloats);
	Ren_Print("GL_MAX_VERTEX_ATTRIBS_ARB %d\n", glConfig2.maxVertexAttribs);

	if (glConfig2.occlusionQueryAvailable)
	{
		Ren_Print("%d occlusion query bits\n", glConfig2.occlusionQueryBits);
	}

	if (glConfig2.drawBuffersAvailable)
	{
		Ren_Print("GL_MAX_DRAW_BUFFERS_ARB: %d\n", glConfig2.maxDrawBuffers);
	}

	if (glConfig2.textureAnisotropyAvailable)
	{
		Ren_Print("GL_TEXTURE_MAX_ANISOTROPY_EXT: %f\n", glConfig2.maxTextureAnisotropy);
	}

	if (glConfig2.framebufferObjectAvailable)
	{
		Ren_Print("GL_MAX_RENDERBUFFER_SIZE_EXT: %d\n", glConfig2.maxRenderbufferSize);
		Ren_Print("GL_MAX_COLOR_ATTACHMENTS_EXT: %d\n", glConfig2.maxColorAttachments);
	}

	Ren_Print("\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits,
	          glConfig.depthBits, glConfig.stencilBits);
	Ren_Print("MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight,
	          fsstrings[r_fullscreen->integer == 1]);

	if (glConfig.displayFrequency)
	{
		Ren_Print("%d\n", glConfig.displayFrequency);
	}
	else
	{
		Ren_Print("N/A\n");
	}

	Ren_Print("ASPECT RATIO: %.4f\n", glConfig.windowAspect);

	if (glConfig.deviceSupportsGamma)
	{
		Ren_Print("GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits);
	}
	else
	{
		Ren_Print("GAMMA: software w/ %d overbright bits\n", tr.overbrightBits);
	}

	Ren_Print("texturemode: %s\n", r_textureMode->string);
	Ren_Print("picmip: %d\n", r_picmip->integer);

	if (glConfig.driverType == GLDRV_OPENGL3)
	{
		int contextFlags, profile;

		Ren_Print(S_COLOR_GREEN "Using OpenGL 3.x context\n");

		// check if we have a core-profile
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
		if (profile == GL_CONTEXT_CORE_PROFILE_BIT)
		{
			Ren_Print(S_COLOR_GREEN "Having a core profile\n");
		}
		else
		{
			Ren_Print(S_COLOR_RED "Having a compatibility profile\n");
		}

		// check if context is forward compatible
		glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
		if (contextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
		{
			Ren_Print(S_COLOR_GREEN "Context is forward compatible\n");
		}
		else
		{
			Ren_Print(S_COLOR_RED "Context is NOT forward compatible\n");
		}
	}

	if (glConfig.hardwareType == GLHW_ATI)
	{
		Ren_Print("HACK: ATI approximations\n");
	}

	if (glConfig.textureCompression != TC_NONE)
	{
		Ren_Print("Using S3TC (DXTC) texture compression\n");
	}

	if (glConfig.hardwareType == GLHW_ATI_DX10)
	{
		Ren_Print("Using ATI DirectX 10 hardware features\n");
	}

	if (glConfig.hardwareType == GLHW_NV_DX10)
	{
		Ren_Print("Using NVIDIA DirectX 10 hardware features\n");
	}

	if (glConfig2.vboVertexSkinningAvailable)
	{
		Ren_Print("Using GPU vertex skinning with max %i bones in a single pass\n", glConfig2.maxVertexSkinningBones);
	}

	if (r_finish->integer)
	{
		Ren_Print("Forcing glFinish\n");
	}
	Ren_Print("Renderer: legacy\n");
}

static void GLSL_restart_f(void)
{
	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	GLSL_ShutdownGPUShaders();
	GLSL_InitGPUShaders();
	GLSL_CompileGPUShaders();
}

void R_Register(void)
{
	// OpenGL context selection
	r_glMajorVersion = ri.Cvar_Get("r_glMajorVersion", "", CVAR_LATCH);
	r_glMinorVersion = ri.Cvar_Get("r_glMinorVersion", "", CVAR_LATCH);
	r_glDebugProfile = ri.Cvar_Get("r_glDebugProfile", "", CVAR_LATCH);

#ifdef USE_GLSL_OPTIMIZER
	r_glslOptimizer = ri.Cvar_Get("r_glslOptimizer", "1", CVAR_ARCHIVE | CVAR_LATCH);
#endif

	// latched and archived variables
	r_ext_compressed_textures        = ri.Cvar_Get("r_ext_compressed_textures", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_occlusion_query            = ri.Cvar_Get("r_ext_occlusion_query", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_texture_non_power_of_two   = ri.Cvar_Get("r_ext_texture_non_power_of_two", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_draw_buffers               = ri.Cvar_Get("r_ext_draw_buffers", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_vertex_array_object        = ri.Cvar_Get("r_ext_vertex_array_object", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_half_float_pixel           = ri.Cvar_Get("r_ext_half_float_pixel", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_texture_float              = ri.Cvar_Get("r_ext_texture_float", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_stencil_wrap               = ri.Cvar_Get("r_ext_stencil_wrap", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_texture_filter_anisotropic = ri.Cvar_Get("r_ext_texture_filter_anisotropic", "4", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_stencil_two_side           = ri.Cvar_Get("r_ext_stencil_two_side", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_depth_bounds_test          = ri.Cvar_Get("r_ext_depth_bounds_test", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_framebuffer_object         = ri.Cvar_Get("r_ext_framebuffer_object", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_packed_depth_stencil       = ri.Cvar_Get("r_ext_packed_depth_stencil", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_framebuffer_blit           = ri.Cvar_Get("r_ext_framebuffer_blit", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_generate_mipmap            = ri.Cvar_Get("r_ext_generate_mipmap", "1", CVAR_CHEAT | CVAR_LATCH);

	r_collapseStages = ri.Cvar_Get("r_collapseStages", "1", CVAR_LATCH | CVAR_CHEAT);
	r_picmip         = ri.Cvar_Get("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_picmip, 0, 3, qtrue);
	r_roundImagesDown         = ri.Cvar_Get("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_colorMipLevels          = ri.Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH);
	r_colorbits               = ri.Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_stereo                  = ri.Cvar_Get("r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_CHEAT);
	r_stencilbits             = ri.Cvar_Get("r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH);
	r_depthbits               = ri.Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignorehwgamma           = ri.Cvar_Get("r_ignorehwgamma", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode                    = ri.Cvar_Get("r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH);
	r_fullscreen              = ri.Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth             = ri.Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH);
	r_customheight            = ri.Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	r_customaspect            = ri.Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_simpleMipMaps           = ri.Cvar_Get("r_simpleMipMaps", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_uiFullScreen            = ri.Cvar_Get("r_uifullscreen", "0", 0);
	r_subdivisions            = ri.Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	r_deferredShading         = ri.Cvar_Get("r_deferredShading", "0", CVAR_CHEAT | CVAR_LATCH);
	r_parallaxMapping         = ri.Cvar_Get("r_parallaxMapping", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_dynamicLightCastShadows = ri.Cvar_Get("r_dynamicLightCastShadows", "1", CVAR_ARCHIVE);
	r_precomputedLighting     = ri.Cvar_Get("r_precomputedLighting", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_vertexLighting          = ri.Cvar_Get("r_vertexLighting", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_compressDiffuseMaps     = ri.Cvar_Get("r_compressDiffuseMaps", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_compressSpecularMaps    = ri.Cvar_Get("r_compressSpecularMaps", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_compressNormalMaps      = ri.Cvar_Get("r_compressNormalMaps", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_heatHazeFix             = ri.Cvar_Get("r_heatHazeFix", "0", CVAR_CHEAT);
	r_noMarksOnTrisurfs       = ri.Cvar_Get("r_noMarksOnTrisurfs", "1", CVAR_CHEAT);
	r_recompileShaders        = ri.Cvar_Get("r_recompileShaders", "0", CVAR_ARCHIVE);

	ri.Cvar_AssertCvarRange(ri.Cvar_Get("r_displayRefresh", "0", CVAR_LATCH), 0, 200, qtrue);

	r_wolfFog = ri.Cvar_Get("r_wolfFog", "1", CVAR_ARCHIVE);
	r_noFog   = ri.Cvar_Get("r_noFog", "0", CVAR_CHEAT);

	r_screenSpaceAmbientOcclusion = ri.Cvar_Get("r_screenSpaceAmbientOcclusion", "0", CVAR_ARCHIVE);
	ri.Cvar_AssertCvarRange(r_screenSpaceAmbientOcclusion, 0, 2, qtrue);
	r_depthOfField = ri.Cvar_Get("r_depthOfField", "0", CVAR_ARCHIVE);

	r_reflectionMapping        = ri.Cvar_Get("r_reflectionMapping", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_highQualityNormalMapping = ri.Cvar_Get("r_highQualityNormalMapping", "0", CVAR_ARCHIVE | CVAR_LATCH);

	r_forceAmbient = ri.Cvar_Get("r_forceAmbient", "0.125", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_forceAmbient, 0.0f, 0.3f, qfalse);

	// temporary latched variables that can only change over a restart
	r_overBrightBits    = ri.Cvar_Get("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mapOverBrightBits = ri.Cvar_Get("r_mapOverBrightBits", "2", CVAR_LATCH);

	ri.Cvar_AssertCvarRange(r_overBrightBits, 0, 1, qtrue); // limit to overbrightbits 1 (sorry 1337 players)
	ri.Cvar_AssertCvarRange(r_mapOverBrightBits, 0, 3, qtrue);

	r_intensity = ri.Cvar_Get("r_intensity", "1", CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_intensity, 0, 1.5, qfalse);

	r_singleShader            = ri.Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH);
	r_drawfoliage             = ri.Cvar_Get("r_drawfoliage", "1", CVAR_CHEAT | CVAR_LATCH);
	r_stitchCurves            = ri.Cvar_Get("r_stitchCurves", "1", CVAR_CHEAT | CVAR_LATCH);
	r_debugShadowMaps         = ri.Cvar_Get("r_debugShadowMaps", "0", CVAR_CHEAT | CVAR_LATCH);
	r_shadowMapLuminanceAlpha = ri.Cvar_Get("r_shadowMapLuminanceAlpha", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_shadowMapLinearFilter   = ri.Cvar_Get("r_shadowMapLinearFilter", "1", CVAR_CHEAT | CVAR_LATCH);
	r_lightBleedReduction     = ri.Cvar_Get("r_lightBleedReduction", "0", CVAR_CHEAT | CVAR_LATCH);
	r_overDarkeningFactor     = ri.Cvar_Get("r_overDarkeningFactor", "30.0", CVAR_CHEAT | CVAR_LATCH);
	r_shadowMapDepthScale     = ri.Cvar_Get("r_shadowMapDepthScale", "1.41", CVAR_CHEAT | CVAR_LATCH);

	r_parallelShadowSplitWeight = ri.Cvar_Get("r_parallelShadowSplitWeight", "0.9", CVAR_CHEAT);
	r_parallelShadowSplits      = ri.Cvar_Get("r_parallelShadowSplits", "2", CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_parallelShadowSplits, 0, MAX_SHADOWMAPS - 1, qtrue);

	r_lightSpacePerspectiveWarping = ri.Cvar_Get("r_lightSpacePerspectiveWarping", "1", CVAR_CHEAT);

	r_screenshotJpegQuality = ri.Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

	// archived variables that can change at any time
	r_lodBias        = ri.Cvar_Get("r_lodBias", "0", CVAR_ARCHIVE);
	r_flares         = ri.Cvar_Get("r_flares", "0", CVAR_ARCHIVE);
	r_znear          = ri.Cvar_Get("r_znear", "3", CVAR_CHEAT); // changed it to 3 (from 4) because of lean/fov cheats
	r_zfar           = ri.Cvar_Get("r_zfar", "0", CVAR_CHEAT);
	r_ignoreGLErrors = ri.Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_fastsky        = ri.Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE);

	r_drawSun       = ri.Cvar_Get("r_drawSun", "1", CVAR_ARCHIVE);
	r_finish        = ri.Cvar_Get("r_finish", "0", CVAR_CHEAT);
	r_textureMode   = ri.Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_swapInterval  = ri.Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
	r_gamma         = ri.Cvar_Get("r_gamma", "1.3", CVAR_ARCHIVE);
	r_facePlaneCull = ri.Cvar_Get("r_facePlaneCull", "1", CVAR_ARCHIVE);

	r_railWidth         = ri.Cvar_Get("r_railWidth", "96", CVAR_ARCHIVE);
	r_railCoreWidth     = ri.Cvar_Get("r_railCoreWidth", "16", CVAR_ARCHIVE);
	r_railSegmentLength = ri.Cvar_Get("r_railSegmentLength", "32", CVAR_ARCHIVE);

	r_ambientScale = ri.Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT);
	r_lightScale   = ri.Cvar_Get("r_lightScale", "2", CVAR_CHEAT);

	r_vboFaces            = ri.Cvar_Get("r_vboFaces", "1", CVAR_CHEAT);
	r_vboCurves           = ri.Cvar_Get("r_vboCurves", "1", CVAR_CHEAT);
	r_vboTriangles        = ri.Cvar_Get("r_vboTriangles", "1", CVAR_CHEAT);
	r_vboShadows          = ri.Cvar_Get("r_vboShadows", "1", CVAR_CHEAT);
	r_vboLighting         = ri.Cvar_Get("r_vboLighting", "1", CVAR_CHEAT);
	r_vboModels           = ri.Cvar_Get("r_vboModels", "1", CVAR_CHEAT);
	r_vboOptimizeVertices = ri.Cvar_Get("r_vboOptimizeVertices", "1", CVAR_CHEAT | CVAR_LATCH);
	r_vboVertexSkinning   = ri.Cvar_Get("r_vboVertexSkinning", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_vboSmoothNormals    = ri.Cvar_Get("r_vboSmoothNormals", "1", CVAR_ARCHIVE | CVAR_LATCH);

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	r_mergeClusterSurfaces  = ri.Cvar_Get("r_mergeClusterSurfaces", "0", CVAR_CHEAT);
	r_mergeClusterFaces     = ri.Cvar_Get("r_mergeClusterFaces", "1", CVAR_CHEAT);
	r_mergeClusterCurves    = ri.Cvar_Get("r_mergeClusterCurves", "1", CVAR_CHEAT);
	r_mergeClusterTriangles = ri.Cvar_Get("r_mergeClusterTriangles", "1", CVAR_CHEAT);
#endif

	r_dynamicBspOcclusionCulling    = ri.Cvar_Get("r_dynamicBspOcclusionCulling", "0", CVAR_ARCHIVE);
	r_dynamicEntityOcclusionCulling = ri.Cvar_Get("r_dynamicEntityOcclusionCulling", "0", CVAR_ARCHIVE);
	r_dynamicLightOcclusionCulling  = ri.Cvar_Get("r_dynamicLightOcclusionCulling", "0", CVAR_CHEAT);
	r_chcMaxPrevInvisNodesBatchSize = ri.Cvar_Get("r_chcMaxPrevInvisNodesBatchSize", "50", CVAR_CHEAT);
	r_chcMaxVisibleFrames           = ri.Cvar_Get("r_chcMaxVisibleFrames", "10", CVAR_CHEAT);
	r_chcVisibilityThreshold        = ri.Cvar_Get("r_chcVisibilityThreshold", "20", CVAR_CHEAT);
	r_chcIgnoreLeaves               = ri.Cvar_Get("r_chcIgnoreLeaves", "0", CVAR_CHEAT);

	r_hdrRendering = ri.Cvar_Get("r_hdrRendering", "0", CVAR_ARCHIVE | CVAR_LATCH);

	// HACK turn off HDR for development
	if (r_deferredShading->integer)
	{
		ri.Cvar_AssertCvarRange(r_hdrRendering, 0, 0, qtrue);
	}

	r_hdrMinLuminance        = ri.Cvar_Get("r_hdrMinLuminance", "0.18", CVAR_CHEAT);
	r_hdrMaxLuminance        = ri.Cvar_Get("r_hdrMaxLuminance", "3000", CVAR_CHEAT);
	r_hdrKey                 = ri.Cvar_Get("r_hdrKey", "0.28", CVAR_CHEAT);
	r_hdrContrastThreshold   = ri.Cvar_Get("r_hdrContrastThreshold", "1.3", CVAR_CHEAT);
	r_hdrContrastOffset      = ri.Cvar_Get("r_hdrContrastOffset", "3.0", CVAR_CHEAT);
	r_hdrLightmap            = ri.Cvar_Get("r_hdrLightmap", "1", CVAR_CHEAT | CVAR_LATCH);
	r_hdrLightmapExposure    = ri.Cvar_Get("r_hdrLightmapExposure", "1.0", CVAR_CHEAT | CVAR_LATCH);
	r_hdrLightmapGamma       = ri.Cvar_Get("r_hdrLightmapGamma", "1.7", CVAR_CHEAT | CVAR_LATCH);
	r_hdrLightmapCompensate  = ri.Cvar_Get("r_hdrLightmapCompensate", "1.0", CVAR_CHEAT | CVAR_LATCH);
	r_hdrToneMappingOperator = ri.Cvar_Get("r_hdrToneMappingOperator", "1", CVAR_CHEAT);
	r_hdrGamma               = ri.Cvar_Get("r_hdrGamma", "1.1", CVAR_CHEAT);
	r_hdrDebug               = ri.Cvar_Get("r_hdrDebug", "0", CVAR_CHEAT);

	r_evsmPostProcess = ri.Cvar_Get("r_evsmPostProcess", "0", CVAR_ARCHIVE | CVAR_LATCH);

	r_printShaders = ri.Cvar_Get("r_printShaders", "0", 0);
	r_saveFontData = ri.Cvar_Get("r_saveFontData", "0", 0);

	r_bloom       = ri.Cvar_Get("r_bloom", "0", CVAR_ARCHIVE);
	r_bloomBlur   = ri.Cvar_Get("r_bloomBlur", "5.0", CVAR_ARCHIVE);
	r_bloomPasses = ri.Cvar_Get("r_bloomPasses", "2", CVAR_CHEAT);

	r_rotoscope     = ri.Cvar_Get("r_rotoscope", "0", CVAR_ARCHIVE);
	r_rotoscopeBlur = ri.Cvar_Get("r_rotoscopeBlur", "5.0", CVAR_ARCHIVE);

	r_cameraPostFX         = ri.Cvar_Get("r_cameraPostFX", "0", CVAR_ARCHIVE);
	r_cameraVignette       = ri.Cvar_Get("r_cameraVignette", "1", CVAR_ARCHIVE);
	r_cameraFilmGrainScale = ri.Cvar_Get("r_cameraFilmGrainScale", "3", CVAR_ARCHIVE);

	// temporary variables that can change at any time
	r_showImages = ri.Cvar_Get("r_showImages", "0", CVAR_TEMP);

	r_debugLight = ri.Cvar_Get("r_debuglight", "0", CVAR_TEMP);
	r_debugSort  = ri.Cvar_Get("r_debugSort", "0", CVAR_CHEAT);

	r_nocurves          = ri.Cvar_Get("r_nocurves", "0", CVAR_CHEAT);
	r_noLightScissors   = ri.Cvar_Get("r_noLightScissors", "0", CVAR_CHEAT);
	r_noLightVisCull    = ri.Cvar_Get("r_noLightVisCull", "0", CVAR_CHEAT);
	r_noInteractionSort = ri.Cvar_Get("r_noInteractionSort", "0", CVAR_CHEAT);
	r_dynamicLight      = ri.Cvar_Get("r_dynamicLight", "1", CVAR_ARCHIVE);
	r_staticLight       = ri.Cvar_Get("r_staticLight", "1", CVAR_CHEAT);
	r_drawworld         = ri.Cvar_Get("r_drawworld", "1", CVAR_CHEAT);
	r_portalOnly        = ri.Cvar_Get("r_portalOnly", "0", CVAR_CHEAT);
	r_portalSky         = ri.Cvar_Get("cg_skybox", "1", 0);

	r_flareSize = ri.Cvar_Get("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = ri.Cvar_Get("r_flareFade", "7", CVAR_CHEAT);

	r_skipBackEnd     = ri.Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);
	r_skipLightBuffer = ri.Cvar_Get("r_skipLightBuffer", "0", CVAR_CHEAT);

	r_measureOverdraw    = ri.Cvar_Get("r_measureOverdraw", "0", CVAR_CHEAT);
	r_lodScale           = ri.Cvar_Get("r_lodScale", "5", CVAR_CHEAT);
	r_lodTest            = ri.Cvar_Get("r_lodTest", "0.5", CVAR_CHEAT);
	r_norefresh          = ri.Cvar_Get("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities       = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_drawpolies         = ri.Cvar_Get("r_drawpolies", "1", CVAR_CHEAT);
	r_ignore             = ri.Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_nocull             = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_novis              = ri.Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_showcluster        = ri.Cvar_Get("r_showcluster", "0", CVAR_CHEAT);
	r_speeds             = ri.Cvar_Get("r_speeds", "0", 0);
	r_logFile            = ri.Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_debugSurface       = ri.Cvar_Get("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind             = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_clear              = ri.Cvar_Get("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor       = ri.Cvar_Get("r_offsetFactor", "-1", CVAR_CHEAT);
	r_offsetUnits        = ri.Cvar_Get("r_offsetUnits", "-2", CVAR_CHEAT);
	r_forceSpecular      = ri.Cvar_Get("r_forceSpecular", "0", CVAR_CHEAT);
	r_specularExponent   = ri.Cvar_Get("r_specularExponent", "16", CVAR_CHEAT | CVAR_LATCH);
	r_specularExponent2  = ri.Cvar_Get("r_specularExponent2", "3", CVAR_CHEAT | CVAR_LATCH);
	r_specularScale      = ri.Cvar_Get("r_specularScale", "1.4", CVAR_CHEAT);
	r_normalScale        = ri.Cvar_Get("r_normalScale", "1.1", CVAR_CHEAT);
	r_normalMapping      = ri.Cvar_Get("r_normalMapping", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_parallaxDepthScale = ri.Cvar_Get("r_parallaxDepthScale", "0.03", CVAR_CHEAT);

	r_wrapAroundLighting  = ri.Cvar_Get("r_wrapAroundLighting", "0.7", CVAR_CHEAT | CVAR_LATCH);
	r_halfLambertLighting = ri.Cvar_Get("r_halfLambertLighting", "1", CVAR_CHEAT | CVAR_LATCH);
	r_rimLighting         = ri.Cvar_Get("r_rimLighting", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_rimExponent         = ri.Cvar_Get("r_rimExponent", "3", CVAR_CHEAT);
	ri.Cvar_AssertCvarRange(r_rimExponent, 0.5, 8.0, qfalse);

	r_drawBuffer = ri.Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
	r_lockpvs    = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals  = ri.Cvar_Get("r_noportals", "0", CVAR_CHEAT);

	r_shadows = ri.Cvar_Get("cg_shadows", "1", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadows, 0, SHADOWING_STENCIL, qtrue);

	r_softShadows = ri.Cvar_Get("r_softShadows", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_softShadows, 0, 6, qtrue);

	r_shadowBlur = ri.Cvar_Get("r_shadowBlur", "2", CVAR_ARCHIVE | CVAR_LATCH);

	r_shadowMapQuality = ri.Cvar_Get("r_shadowMapQuality", "3", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapQuality, 0, 4, qtrue);

	r_shadowMapSizeUltra = ri.Cvar_Get("r_shadowMapSizeUltra", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeUltra, 32, 2048, qtrue);

	r_shadowMapSizeVeryHigh = ri.Cvar_Get("r_shadowMapSizeVeryHigh", "512", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeVeryHigh, 32, 2048, qtrue);

	r_shadowMapSizeHigh = ri.Cvar_Get("r_shadowMapSizeHigh", "256", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeHigh, 32, 2048, qtrue);

	r_shadowMapSizeMedium = ri.Cvar_Get("r_shadowMapSizeMedium", "128", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeMedium, 32, 2048, qtrue);

	r_shadowMapSizeLow = ri.Cvar_Get("r_shadowMapSizeLow", "64", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeLow, 32, 2048, qtrue);

	shadowMapResolutions[0] = r_shadowMapSizeUltra->integer;
	shadowMapResolutions[1] = r_shadowMapSizeVeryHigh->integer;
	shadowMapResolutions[2] = r_shadowMapSizeHigh->integer;
	shadowMapResolutions[3] = r_shadowMapSizeMedium->integer;
	shadowMapResolutions[4] = r_shadowMapSizeLow->integer;

	r_shadowMapSizeSunUltra = ri.Cvar_Get("r_shadowMapSizeSunUltra", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeSunUltra, 32, 2048, qtrue);

	r_shadowMapSizeSunVeryHigh = ri.Cvar_Get("r_shadowMapSizeSunVeryHigh", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeSunVeryHigh, 512, 2048, qtrue);

	r_shadowMapSizeSunHigh = ri.Cvar_Get("r_shadowMapSizeSunHigh", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeSunHigh, 512, 2048, qtrue);

	r_shadowMapSizeSunMedium = ri.Cvar_Get("r_shadowMapSizeSunMedium", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeSunMedium, 512, 2048, qtrue);

	r_shadowMapSizeSunLow = ri.Cvar_Get("r_shadowMapSizeSunLow", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_shadowMapSizeSunLow, 512, 2048, qtrue);

	sunShadowMapResolutions[0] = r_shadowMapSizeSunUltra->integer;
	sunShadowMapResolutions[1] = r_shadowMapSizeSunVeryHigh->integer;
	sunShadowMapResolutions[2] = r_shadowMapSizeSunHigh->integer;
	sunShadowMapResolutions[3] = r_shadowMapSizeSunMedium->integer;
	sunShadowMapResolutions[4] = r_shadowMapSizeSunLow->integer;

	r_shadowOffsetFactor         = ri.Cvar_Get("r_shadowOffsetFactor", "0", CVAR_CHEAT);
	r_shadowOffsetUnits          = ri.Cvar_Get("r_shadowOffsetUnits", "0", CVAR_CHEAT);
	r_shadowLodBias              = ri.Cvar_Get("r_shadowLodBias", "0", CVAR_CHEAT);
	r_shadowLodScale             = ri.Cvar_Get("r_shadowLodScale", "0.8", CVAR_CHEAT);
	r_noShadowPyramids           = ri.Cvar_Get("r_noShadowPyramids", "0", CVAR_CHEAT);
	r_cullShadowPyramidFaces     = ri.Cvar_Get("r_cullShadowPyramidFaces", "0", CVAR_CHEAT);
	r_cullShadowPyramidCurves    = ri.Cvar_Get("r_cullShadowPyramidCurves", "1", CVAR_CHEAT);
	r_cullShadowPyramidTriangles = ri.Cvar_Get("r_cullShadowPyramidTriangles", "1", CVAR_CHEAT);
	r_noShadowFrustums           = ri.Cvar_Get("r_noShadowFrustums", "0", CVAR_CHEAT);
	r_noLightFrustums            = ri.Cvar_Get("r_noLightFrustums", "0", CVAR_CHEAT);

	// note: MAX_POLYS and MAX_POLYVERTS are heavily increased in ET compared to q3
	//       - but run 20 bots on oasis and you'll see limits reached (developer 1)
	//       - modern computers can deal with more than our old default values -> users can increase this now to MAX_POLYS/MAX_POLYVERTS
	r_maxpolys = ri.Cvar_Get("r_maxpolys", va("%d", MIN_POLYS), CVAR_LATCH);     // now latched to check against used r_maxpolys and not MAX_POLYS
	ri.Cvar_AssertCvarRange(r_maxpolys, MIN_POLYS, MAX_POLYS, qtrue); // MIN_POLYS was old static value
	r_maxpolyverts = ri.Cvar_Get("r_maxpolyverts", va("%d", MIN_POLYVERTS), CVAR_LATCH); // now latched to check against used r_maxpolyverts and not MAX_POLYVERTS
	ri.Cvar_AssertCvarRange(r_maxpolyverts, MIN_POLYVERTS, MAX_POLYVERTS, qtrue); // MIN_POLYVERTS was old static value

	r_showTris                 = ri.Cvar_Get("r_showTris", "0", CVAR_CHEAT);
	r_showSky                  = ri.Cvar_Get("r_showSky", "0", CVAR_CHEAT);
	r_showShadowVolumes        = ri.Cvar_Get("r_showShadowVolumes", "0", CVAR_CHEAT);
	r_showShadowLod            = ri.Cvar_Get("r_showShadowLod", "0", CVAR_CHEAT);
	r_showShadowMaps           = ri.Cvar_Get("r_showShadowMaps", "0", CVAR_CHEAT);
	r_showSkeleton             = ri.Cvar_Get("r_showSkeleton", "0", CVAR_CHEAT);
	r_showEntityTransforms     = ri.Cvar_Get("r_showEntityTransforms", "0", CVAR_CHEAT);
	r_showLightTransforms      = ri.Cvar_Get("r_showLightTransforms", "0", CVAR_CHEAT);
	r_showLightInteractions    = ri.Cvar_Get("r_showLightInteractions", "0", CVAR_CHEAT);
	r_showLightScissors        = ri.Cvar_Get("r_showLightScissors", "0", CVAR_CHEAT);
	r_showLightBatches         = ri.Cvar_Get("r_showLightBatches", "0", CVAR_CHEAT);
	r_showLightGrid            = ri.Cvar_Get("r_showLightGrid", "0", CVAR_CHEAT);
	r_showOcclusionQueries     = ri.Cvar_Get("r_showOcclusionQueries", "0", CVAR_CHEAT);
	r_showBatches              = ri.Cvar_Get("r_showBatches", "0", CVAR_CHEAT);
	r_showLightMaps            = ri.Cvar_Get("r_showLightMaps", "0", CVAR_CHEAT);
	r_showDeluxeMaps           = ri.Cvar_Get("r_showDeluxeMaps", "0", CVAR_CHEAT);
	r_showAreaPortals          = ri.Cvar_Get("r_showAreaPortals", "0", CVAR_CHEAT);
	r_showCubeProbes           = ri.Cvar_Get("r_showCubeProbes", "0", CVAR_CHEAT);
	r_showBspNodes             = ri.Cvar_Get("r_showBspNodes", "0", CVAR_CHEAT);
	r_showParallelShadowSplits = ri.Cvar_Get("r_showParallelShadowSplits", "0", CVAR_CHEAT | CVAR_LATCH);
	r_showDecalProjectors      = ri.Cvar_Get("r_showDecalProjectors", "0", CVAR_CHEAT);

	r_showDeferredDiffuse  = ri.Cvar_Get("r_showDeferredDiffuse", "0", CVAR_CHEAT);
	r_showDeferredNormal   = ri.Cvar_Get("r_showDeferredNormal", "0", CVAR_CHEAT);
	r_showDeferredSpecular = ri.Cvar_Get("r_showDeferredSpecular", "0", CVAR_CHEAT);
	r_showDeferredPosition = ri.Cvar_Get("r_showDeferredPosition", "0", CVAR_CHEAT);
	r_showDeferredRender   = ri.Cvar_Get("r_showDeferredRender", "0", CVAR_CHEAT);
	r_showDeferredLight    = ri.Cvar_Get("r_showDeferredLight", "0", CVAR_CHEAT);

	r_detailTextures = ri.Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH);

	//FOR sdl_glimp.c
	r_noborder        = ri.Cvar_Get("r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_stereoEnabled   = ri.Cvar_Get("r_stereoEnabled", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_multisample = ri.Cvar_Get("r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_AssertCvarRange(r_ext_multisample, 0, 4, qtrue);
	r_ext_multitexture    = ri.Cvar_Get("r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ext_texture_env_add = ri.Cvar_Get("r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_allowExtensions     = ri.Cvar_Get("r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);

	// make sure all the commands added here are also removed in R_Shutdown
	ri.Cmd_AddCommand("imagelist", R_ImageList_f);
	ri.Cmd_AddCommand("shaderlist", R_ShaderList_f);
	ri.Cmd_AddCommand("shaderexp", R_ShaderExp_f);
	ri.Cmd_AddCommand("skinlist", R_SkinList_f);
	ri.Cmd_AddCommand("modellist", R_Modellist_f);
	ri.Cmd_AddCommand("modelist", R_ModeList_f);

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	ri.Cmd_AddCommand("animationlist", R_AnimationList_f);
#endif

	ri.Cmd_AddCommand("fbolist", R_FBOList_f);
	ri.Cmd_AddCommand("vbolist", R_VBOList_f);
	ri.Cmd_AddCommand("screenshot", R_ScreenShot_f);
	ri.Cmd_AddCommand("screenshotJPEG", R_ScreenShotJPEG_f);
	ri.Cmd_AddCommand("screenshotPNG", R_ScreenShotPNG_f);
	ri.Cmd_AddCommand("gfxinfo", GfxInfo_f);
	//	ri.Cmd_AddCommand("generatemtr", R_GenerateMaterialFile_f);
	ri.Cmd_AddCommand("buildcubemaps", R_BuildCubeMaps);
	ri.Cmd_AddCommand("minimize", GLimp_Minimize);

	ri.Cmd_AddCommand("glsl_restart", GLSL_restart_f);
}

void R_Init(void)
{
	int i;

	Ren_Print("----- R_Init -----\n");

	//Swap_Init();

	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));

	if ((intptr_t) tess.xyz & 15)
	{
		Ren_Print("WARNING: tess.xyz not 16 byte aligned\n");
	}

	// init function tables
	for (i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i]             = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));
		tr.squareTable[i]          = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i]        = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if (i < FUNCTABLE_SIZE / 2)
		{
			if (i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = (float)i / (FUNCTABLE_SIZE / 4);
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

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	if (!InitOpenGL())
	{
		ri.Error(ERR_VID_FATAL, "OpenGL initialization failed!");
	}

	backEndData              = (backEndData_t *) ri.Hunk_Alloc(sizeof(*backEndData), h_low);
	backEndData->polys       = (srfPoly_t *) ri.Hunk_Alloc(r_maxpolys->integer * sizeof(srfPoly_t), h_low);
	backEndData->polyVerts   = (polyVert_t *) ri.Hunk_Alloc(r_maxpolyverts->integer * sizeof(polyVert_t), h_low);
	backEndData->polybuffers = (srfPolyBuffer_t *) ri.Hunk_Alloc(MAX_POLYBUFFERS * sizeof(srfPolyBuffer_t), h_low);

	R_InitNextFrame();

	R_InitImages();

	R_InitFBOs();

	if (glConfig.driverType == GLDRV_OPENGL3)
	{
		tr.vao = 0;
		glGenVertexArrays(1, &tr.vao);
		glBindVertexArray(tr.vao);
	}

	R_InitVBOs();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	R_InitAnimations();
#endif

	R_InitFreeType();

	if (glConfig2.textureAnisotropyAvailable)
	{
		ri.Cvar_AssertCvarRange(r_ext_texture_filter_anisotropic, 0, glConfig2.maxTextureAnisotropy, qfalse);
	}

	if (glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA)
	{
		glGenQueries(MAX_OCCLUSION_QUERIES, tr.occlusionQueryObjects);
	}

	GLSL_CompileGPUShaders();

	GL_CheckErrors();

	Ren_Print("----- finished R_Init -----\n");
}

void RE_Shutdown(qboolean destroyWindow)
{
	Ren_Print("RE_Shutdown( destroyWindow = %i )\n", destroyWindow);

	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshotPNG");
	ri.Cmd_RemoveCommand("screenshotJPEG");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("shaderlist");
	ri.Cmd_RemoveCommand("shaderexp");
	ri.Cmd_RemoveCommand("skinlist");
	ri.Cmd_RemoveCommand("gfxinfo");
	ri.Cmd_RemoveCommand("modelist");
	ri.Cmd_RemoveCommand("animationlist");
	ri.Cmd_RemoveCommand("fbolist");
	ri.Cmd_RemoveCommand("vbolist");
	ri.Cmd_RemoveCommand("generatemtr");
	ri.Cmd_RemoveCommand("buildcubemaps");

	ri.Cmd_RemoveCommand("glsl_restart");

	if (tr.registered)
	{
		R_IssuePendingRenderCommands();

		R_ShutdownImages();
		R_ShutdownVBOs();
		R_ShutdownFBOs();

		if (glConfig.driverType == GLDRV_OPENGL3)
		{
			glDeleteVertexArrays(1, &tr.vao);
			tr.vao = 0;
		}

		if (glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA)
		{
			glDeleteQueries(MAX_OCCLUSION_QUERIES, tr.occlusionQueryObjects);

			if (tr.world)
			{
				int       j;
				bspNode_t *node;
				//trRefLight_t   *light;

				for (j = 0; j < tr.world->numnodes; j++)
				{
					node = &tr.world->nodes[j];

					glDeleteQueries(MAX_VIEWS, node->occlusionQueryObjects);
				}

				/*
				for(j = 0; j < tr.world->numLights; j++)
				{
				    light = &tr.world->lights[j];

				    glDeleteQueries(MAX_VIEWS, light->occlusionQueryObjects);
				}
				*/
			}
		}
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff

	if (destroyWindow)
	{
		GLSL_ShutdownGPUShaders();
		GLimp_Shutdown();

		ri.Tag_Free();
	}

	tr.registered = qfalse;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration(void)
{
	R_IssuePendingRenderCommands();

	/*
	   if(!Sys_LowPhysicalMemory())
	   {
	   RB_ShowImages();
	   }
	 */
}

static void RE_PurgeCache(void)
{
	Ren_Print(S_COLOR_RED "TODO RE_PurgeCache\n");

	/*
	R_PurgeShaders(9999999);
	R_PurgeBackupImages(9999999);
	R_PurgeModels(9999999);
	*/
}

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
		Ren_Print("Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion);
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown               = RE_Shutdown;
	re.MainWindow             = GLimp_MainWindow;
	re.BeginRegistration      = RE_BeginRegistration;
	re.RegisterModel          = RE_RegisterModel;
	re.RegisterSkin           = RE_RegisterSkin;
	re.RegisterShader         = RE_RegisterShader;
	re.RegisterShaderNoMip    = RE_RegisterShaderNoMip;
	re.LoadWorld              = RE_LoadWorldMap;
	re.SetWorldVisData        = RE_SetWorldVisData;
	re.EndRegistration        = RE_EndRegistration;
	re.BeginFrame             = RE_BeginFrame;
	re.EndFrame               = RE_EndFrame;
	re.MarkFragments          = R_MarkFragments;
	re.LerpTag                = RE_LerpTagET;
	re.ModelBounds            = R_ModelBounds;
	re.ClearScene             = RE_ClearScene;
	re.AddRefEntityToScene    = RE_AddRefEntityToScene;
	re.AddPolyToScene         = RE_AddPolyToScene;
	re.AddPolysToScene        = RE_AddPolysToScene;
	re.AddLightToScene        = RE_AddDynamicLightToScene;
	re.RenderScene            = RE_RenderScene;
	re.SetColor               = RE_SetColor;
	re.DrawStretchPic         = RE_StretchPic;
	re.DrawStretchRaw         = RE_StretchRaw;
	re.UploadCinematic        = RE_UploadCinematic;
	re.DrawRotatedPic         = RE_RotatedPic;
	re.Add2dPolys             = RE_2DPolyies;
	re.DrawStretchPicGradient = RE_StretchPicGradient;
	re.RegisterFont           = RE_RegisterFont;
	re.RemapShader            = R_RemapShader;
	re.GetEntityToken         = R_GetEntityToken;
	re.inPVS                  = R_inPVS;
	re.GetSkinModel           = RE_GetSkinModel;
	re.GetShaderFromModel     = RE_GetShaderFromModel;
	re.ProjectDecal           = RE_ProjectDecal;
	re.ClearDecals            = RE_ClearDecals;
	re.DrawDebugPolygon       = R_DebugPolygon;
	re.DrawDebugText          = R_DebugText;
	re.AddCoronaToScene       = RE_AddCoronaToScene;
	re.AddPolyBufferToScene   = RE_AddPolyBufferToScene;
	re.SetFog                 = RE_SetFog;
	re.SetGlobalFog           = RE_SetGlobalFog;
	re.purgeCache             = RE_PurgeCache;
	re.LoadDynamicShader      = RE_LoadDynamicShader;
	re.GetTextureId           = RE_GetTextureId;
	re.RenderToTexture        = RE_RenderToTexture;
	re.Finish                 = RE_Finish;
	re.TakeVideoFrame         = RE_TakeVideoFrame;
	//re.SetClipRegion = RE_SetClipRegion;

#if defined(USE_REFLIGHT)
	re.RegisterShaderLightAttenuation = RE_RegisterShaderLightAttenuation;
	re.AddRefLightToScene             = RE_AddRefLightToScene;
#endif

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	re.RegisterAnimation = RE_RegisterAnimation;
	re.CheckSkeleton     = RE_CheckSkeleton;
	re.BuildSkeleton     = RE_BuildSkeleton;
	re.BlendSkeleton     = RE_BlendSkeleton;
	re.BoneIndex         = RE_BoneIndex;
	re.AnimNumFrames     = RE_AnimNumFrames;
	re.AnimFrameRate     = RE_AnimFrameRate;
#endif

	return &re;
}
