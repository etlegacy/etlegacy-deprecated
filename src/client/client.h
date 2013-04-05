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
 * @file client.h
 * @brief primary header for client
 */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_public.h"
#include "../ui/ui_public.h"
#include "keys.h"
#include "snd_public.h"
#include "../cgame/cg_public.h"
#include "../game/bg_public.h"

#define RETRANSMIT_TIMEOUT  3000    // time between connection packet retransmits

#define LIMBOCHAT_WIDTH     140     // NOTE: buffer size indicator, not related to screen bbox
#define LIMBOCHAT_HEIGHT    7

#define ETKEY_FILE "etkey"
#define ETKEY_SIZE 28

// snapshots are a view of the server at a given time
typedef struct
{
	qboolean valid;                 // cleared if delta parsing was invalid
	int snapFlags;                  // rate delayed and dropped commands

	int serverTime;                 // server time the message is valid for (in msec)

	int messageNum;                 // copied from netchan->incoming_sequence
	int deltaNum;                   // messageNum the delta is from
	int ping;                       // time from when cmdNum-1 was sent to time packet was reeceived
	byte areamask[MAX_MAP_AREA_BYTES]; // portalarea visibility bits

	int cmdNum;                     // the next cmdNum the server is expecting
	playerState_t ps;               // complete information about the current player at this time

	int numEntities;                // all of the entities that need to be presented
	int parseEntitiesNum;           // at the time of this snapshot

	int serverCommandNum;           // execute all commands up to this before
	// making the snapshot current
} clSnapshot_t;

// for double tapping
typedef struct
{
	int pressedTime[DT_NUM];
	int releasedTime[DT_NUM];

	int lastdoubleTap;
} doubleTap_t;

/*
=============================================================================
the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection
=============================================================================
*/

typedef struct
{
	int p_cmdNumber;            // cl.cmdNumber when packet was sent
	int p_serverTime;           // usercmd->serverTime when packet was sent
	int p_realtime;             // cls.realtime when packet was sent
} outPacket_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define MAX_PARSE_ENTITIES  2048

extern int g_console_field_width;

typedef struct
{
	int timeoutcount;               // it requres several frames in a timeout condition
	// to disconnect, preventing debugging breaks from
	// causing immediate disconnects on continue
	clSnapshot_t snap;              // latest received from server

	int serverTime;                 // may be paused during play
	int oldServerTime;              // to prevent time from flowing bakcwards
	int oldFrameServerTime;         // to check tournament restarts
	int serverTimeDelta;            // cl.serverTime = cls.realtime + cl.serverTimeDelta
	// this value changes as net lag varies
	qboolean extrapolatedSnapshot;      // set if any cgame frame has been forced to extrapolate
	// cleared when CL_AdjustTimeDelta looks at it
	qboolean newSnapshots;          // set on parse of any valid packet

	gameState_t gameState;          // configstrings
	char mapname[MAX_QPATH];        // extracted from CS_SERVERINFO

	int parseEntitiesNum;           // index (not anded off) into cl_parse_entities[]

	int mouseDx[2], mouseDy[2];         // added to by mouse events
	int mouseIndex;
	int joystickAxis[MAX_JOYSTICK_AXIS]; // set by joystick events

	// cgame communicates a few values to the client system
	int cgameUserCmdValue;              // current weapon to add to usercmd_t
	int cgameFlags;                     // flags that can be set by the gamecode
	float cgameSensitivity;
	int cgameMpIdentClient;
	vec3_t cgameClientLerpOrigin;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	usercmd_t cmds[CMD_BACKUP];     // each mesage will send several old cmds
	int cmdNumber;                  // incremented each frame, because multiple
	// frames may need to be packed into a single packet

	// double tapping
	doubleTap_t doubleTap;

	outPacket_t outPackets[PACKET_BACKUP];  // information about each packet we have sent out

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t viewangles;

	int serverId;                   // included in each client message so the server
	// can tell if it is for a prior map_restart
	// big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t snapshots[PACKET_BACKUP];

	entityState_t entityBaselines[MAX_GENTITIES];   // for delta compression when not in previous frame

	entityState_t parseEntities[MAX_PARSE_ENTITIES];

	// NOTE - UI uses LIMBOCHAT_WIDTH strings (140),
	// but for the processing in CL_AddToLimboChat we need some safe room
	char limboChatMsgs[LIMBOCHAT_HEIGHT][LIMBOCHAT_WIDTH * 3 + 1];
	int limboChatPos;

	qboolean corruptedTranslationFile;
	char translationVersion[MAX_STRING_TOKENS];

	qboolean cameraMode;
} clientActive_t;

extern clientActive_t cl;

/*
=============================================================================
the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.
=============================================================================
*/

typedef struct
{
	connstate_t state;                      // connection status

	int clientNum;
	int lastPacketSentTime;                 // for retransmits during connection
	int lastPacketTime;                     // for timeouts

	netadr_t serverAddress;
	int connectTime;                        // for connection retransmits
	int connectPacketCount;                 // for display on connection dialog
	char serverMessage[MAX_STRING_TOKENS];  // for display on connection dialog

	int challenge;                          // from the server to use for connecting
	int checksumFeed;                       // from the server for checksum calculations

	int onlyVisibleClients;

	// these are our reliable messages that go to the server
	int reliableSequence;
	int reliableAcknowledge;                // the last one the server has executed
	// NOTE: incidentally, reliableCommands[0] is never used (always start at reliableAcknowledge+1)
	char reliableCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];

	// unreliable binary data to send to server
	int binaryMessageLength;
	char binaryMessage[MAX_BINARY_MESSAGE];
	qboolean binaryMessageOverflowed;

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int serverMessageSequence;

	// reliable messages received from server
	int serverCommandSequence;
	int lastExecutedServerCommand;              // last server command grabbed or executed with CL_GetServerCommand
	char serverCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];

	// file transfer from server
	fileHandle_t download;
	int downloadNumber;
	int downloadBlock;          // block we are waiting for
	int downloadCount;          // how many bytes we got
	int downloadSize;           // how many bytes we got
	int downloadFlags;         // misc download behaviour flags sent by the server
	char downloadList[MAX_INFO_STRING];        // list of paks we need to download

	// www downloading
	qboolean bWWWDl;    // we have a www download going
	qboolean bWWWDlAborting;    // disable the CL_WWWDownload until server gets us a gamestate (used for aborts)
	char redirectedList[MAX_INFO_STRING];        // list of files that we downloaded through a redirect since last FS_ComparePaks
	char badChecksumList[MAX_INFO_STRING];        // list of files for which wwwdl redirect is broken (wrong checksum)

	// demo information
	char demoName[MAX_QPATH];
	qboolean demorecording;
	qboolean demoplaying;
	qboolean demowaiting;       // don't record until a non-delta message is received
	qboolean firstDemoFrameSkipped;
	fileHandle_t demofile;

	qboolean waverecording;
	fileHandle_t wavefile;
	int wavetime;

	int timeDemoFrames;             // counter of rendered frames
	int timeDemoStart;              // cls.realtime before first frame
	int timeDemoBaseTime;           // each frame will be at this time + frameNum * 50

	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t netchan;
} clientConnection_t;

extern clientConnection_t clc;

/*
==================================================================
the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all
==================================================================
*/

typedef struct
{
	netadr_t adr;
	int start;
	int time;
	char info[MAX_INFO_STRING];
} ping_t;

typedef struct
{
	netadr_t adr;
	char hostName[MAX_NAME_LENGTH];
	int load;
	char mapName[MAX_NAME_LENGTH];
	char game[MAX_NAME_LENGTH];
	int netType;
	int gameType;
	int clients;
	int maxClients;
	int minPing;
	int maxPing;
	int ping;
	qboolean visible;
	int allowAnonymous;
	int friendlyFire;
	int maxlives;
	int needpass;
	int punkbuster;
	int antilag;
	int weaprestrict;
	int balancedteams;
	char gameName[MAX_NAME_LENGTH];
} serverInfo_t;

typedef struct
{
	connstate_t state;              // connection status
	int keyCatchers;                // bit flags

	qboolean doCachePurge;          // empty the renderer cache as soon as possible

	char servername[MAX_OSPATH];    // name of server from original connect (used by reconnect)

	// when the server clears the hunk, all of these must be restarted
	qboolean rendererStarted;
	qboolean soundStarted;
	qboolean soundRegistered;
	qboolean uiStarted;
	qboolean cgameStarted;

	int framecount;
	int frametime;                  // msec since last frame

	int realtime;                   // ignores pause
	int realFrametime;              // ignoring pause, so console always works

	int numlocalservers;
	serverInfo_t localServers[MAX_OTHER_SERVERS];

	int numglobalservers;
	serverInfo_t globalServers[MAX_GLOBAL_SERVERS];
	// additional global servers
	int numGlobalServerAddresses;
	netadr_t globalServerAddresses[MAX_GLOBAL_SERVERS];

	int numfavoriteservers;
	serverInfo_t favoriteServers[MAX_OTHER_SERVERS];

	int pingUpdateSource;       // source currently pinging or updating

	int masterNum;

	char updateInfoString[MAX_INFO_STRING];

	// rendering info
	glconfig_t glconfig;
	qhandle_t charSetShader;
	qhandle_t whiteShader;
	qhandle_t consoleShader;
	qhandle_t consoleShader2;

	// www downloading
	// in the static stuff since this may have to survive server disconnects
	// if new stuff gets added, CL_ClearStaticDownload code needs to be updated for clear up
	qboolean bWWWDlDisconnected; // keep going with the download after server disconnect
	char downloadName[MAX_OSPATH];
	char downloadTempName[MAX_OSPATH];    // in wwwdl mode, this is OS path (it's a qpath otherwise)
	char originalDownloadName[MAX_QPATH];    // if we get a redirect, keep a copy of the original file path
	qboolean downloadRestart; // if true, we need to do another FS_Restart because we downloaded a pak
} clientStatic_t;

extern clientStatic_t cls;

//=============================================================================

extern vm_t        *cgvm;       // interface to cgame dll or vm
extern vm_t        *uivm;       // interface to ui dll or vm
extern refexport_t re;          // interface to refresh .dll

// cvars

extern cvar_t *cl_nodelta;
extern cvar_t *cl_debugMove;
extern cvar_t *cl_noprint;
extern cvar_t *cl_timegraph;
extern cvar_t *cl_maxpackets;
extern cvar_t *cl_packetdup;
extern cvar_t *cl_shownet;
extern cvar_t *cl_shownuments;
extern cvar_t *cl_showSend;
extern cvar_t *cl_showServerCommands;
extern cvar_t *cl_timeNudge;
extern cvar_t *cl_showTimeDelta;
extern cvar_t *cl_freezeDemo;

extern cvar_t *cl_yawspeed;
extern cvar_t *cl_pitchspeed;
extern cvar_t *cl_run;
extern cvar_t *cl_anglespeedkey;

extern cvar_t *cl_recoilPitch;

extern cvar_t *cl_bypassMouseInput;

extern cvar_t *cl_doubletapdelay;

extern cvar_t *cl_sensitivity;
extern cvar_t *cl_freelook;

extern cvar_t *cl_mouseAccel;
extern cvar_t *cl_showMouseRate;

extern cvar_t *cl_avidemo;
extern cvar_t *cl_aviMotionJpeg;

extern cvar_t *m_pitch;
extern cvar_t *m_yaw;
extern cvar_t *m_forward;
extern cvar_t *m_side;
extern cvar_t *m_filter;

extern cvar_t *cl_timedemo;

extern cvar_t *cl_activeAction;
extern cvar_t *cl_autorecord;

extern cvar_t *cl_allowDownload;
extern cvar_t *cl_conXOffset;
extern cvar_t *cl_inGameVideo;

extern cvar_t *cl_missionStats;

// localization
extern cvar_t *cl_language;

extern cvar_t *cl_profile;
extern cvar_t *cl_defaultProfile;

extern cvar_t *cl_consoleKeys;

//=================================================

// cl_main

void CL_Init(void);
void CL_FlushMemory(void);
void CL_ShutdownAll(void);
void CL_AddReliableCommand(const char *cmd);

void CL_StartHunkUsers(void);

void CL_RequestMotd(void);
void CL_CheckAutoUpdate(void);
void CL_GetAutoUpdate(void);

void CL_Disconnect_f(void);
void CL_GetChallengePacket(void);
void CL_Vid_Restart_f(void);
void CL_Snd_Restart_f(void);
void CL_NextDemo(void);
void CL_ReadDemoMessage(void);

void CL_InitDownloads(void);
void CL_NextDownload(void);
void CL_WWWDownload(void);

void CL_GetPing(int n, char *buf, int buflen, int *pingtime);
void CL_GetPingInfo(int n, char *buf, int buflen);
void CL_ClearPing(int n);
int CL_GetPingQueueCount(void);

void CL_ShutdownRef(void);
void CL_InitRef(void);
int CL_ServerStatus(char *serverAddress, char *serverStatusString, int maxLen);

void CL_AddToLimboChat(const char *str);
qboolean CL_GetLimboString(int index, char *buf);

// cl_avi

qboolean CL_OpenAVIForWriting(const char *filename);
void CL_TakeVideoFrame(void);
void CL_WriteAVIVideoFrame(const byte *imageBuffer, int size);
void CL_WriteAVIAudioFrame(const byte *pcmBuffer, int size);
qboolean CL_CloseAVI(void);
qboolean CL_VideoRecording(void);

// localization
void CL_InitTranslation(void);
void CL_SaveTransTable(const char *fileName, qboolean newOnly);
void CL_ReloadTranslation(void);
void CL_TranslateString(const char *string, char *dest_buffer);
const char *CL_TranslateStringBuf(const char *string);

void CL_OpenURL(const char *url);

void CL_Record(const char *name);

// cl_input

typedef struct
{
	int down[2];                // key nums holding it down
	unsigned downtime;          // msec timestamp
	unsigned msec;              // msec down this frame if both a down and up happened
	qboolean active;            // current state
	qboolean wasPressed;        // set when down, not cleared when up
} kbutton_t;

typedef enum
{
	KB_LEFT,
	KB_RIGHT,
	KB_FORWARD,
	KB_BACK,
	KB_LOOKUP,
	KB_LOOKDOWN,
	KB_MOVELEFT,
	KB_MOVERIGHT,
	KB_STRAFE,
	KB_SPEED,
	KB_UP,
	KB_DOWN,
	KB_BUTTONS0,
	KB_BUTTONS1,
	KB_BUTTONS2,
	KB_BUTTONS3,
	KB_BUTTONS4,
	KB_BUTTONS5,
	KB_BUTTONS6,
	KB_BUTTONS7, // unused
	KB_WBUTTONS0,
	KB_WBUTTONS1,
	KB_WBUTTONS2,
	KB_WBUTTONS3,
	KB_WBUTTONS4,
	KB_WBUTTONS5,
	KB_WBUTTONS6,
	KB_WBUTTONS7,
	KB_MLOOK,
	NUM_BUTTONS
} kbuttons_t;

void CL_ClearKeys(void);

void CL_InitInput(void);
void CL_SendCmd(void);
void CL_ClearState(void);
void CL_ReadPackets(void);

void CL_WritePacket(void);
void IN_Help(void);

float CL_KeyState(kbutton_t *key);
int Key_StringToKeynum(char *str);
char *Key_KeynumToString(int keynum);

// cl_parse.c

extern int cl_connectedToPureServer;
extern int cl_connectedToCheatServer;

void CL_SystemInfoChanged(void);
void CL_ParseServerMessage(msg_t *msg);

//====================================================================

void CL_UpdateInfoPacket(netadr_t from);

void CL_ServerInfoPacket(netadr_t from, msg_t *msg);
void CL_LocalServers_f(void);
void CL_GlobalServers_f(void);
//FIXME: void    CL_FavoriteServers_f( void );
void CL_Ping_f(void);
qboolean CL_UpdateVisiblePings_f(int source);

// console

#define NUM_CON_TIMES 4

#define CON_TEXTSIZE    65536

typedef struct
{
	qboolean initialized;

	short text[CON_TEXTSIZE];
	int current;            // line where next message will be printed
	int x;                  // offset in current line for next print
	int display;            // bottom of console displays this line

	int linewidth;          // characters across screen
	int totallines;         // total lines in console scrollback

	float xadjust;          // for wide aspect screens

	float displayFrac;      // aproaches finalFrac at scr_conspeed
	float finalFrac;        // 0.0 to 1.0 lines of console to display
	float desiredFrac;      // ydnar: for variable console heights

	int vislines;           // in scanlines

	int times[NUM_CON_TIMES];       // cls.realtime time the line was generated
	// for transparent notify lines
	vec4_t color;

	int acLength;           // utocomplete buffer length
} console_t;

extern console_t con;

void Con_DrawCharacter(int cx, int line, int num);

void Con_CheckResize(void);
void Con_Init(void);
void Con_Clear_f(void);
void Con_ToggleConsole_f(void);
void Con_DrawNotify(void);
void Con_ClearNotify(void);
void Con_RunConsole(void);
void Con_DrawConsole(void);
void Con_PageUp(void);
void Con_PageDown(void);
void Con_Top(void);
void Con_Bottom(void);
void Con_Close(void);

// cl_scrn.c

void SCR_Init(void);
void SCR_UpdateScreen(void);

void SCR_DebugGraph(float value);

int SCR_GetBigStringWidth(const char *str);         // returns in virtual 640x480 coordinates

void SCR_AdjustFrom640(float *x, float *y, float *w, float *h);
void SCR_FillRect(float x, float y, float width, float height,
                  const float *color);
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader);

void SCR_DrawBigString(int x, int y, const char *s, float alpha, qboolean noColorEscape);               // draws a string with embedded color control characters with fade
void SCR_DrawBigStringColor(int x, int y, const char *s, vec4_t color, qboolean noColorEscape);         // ignores embedded color control characters
void SCR_DrawSmallStringExt(int x, int y, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape);
void SCR_DrawSmallChar(int x, int y, int ch);

// cl_cin.c

void CL_PlayCinematic_f(void);
void SCR_DrawCinematic(void);
void SCR_RunCinematic(void);
void SCR_StopCinematic(void);
int CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status CIN_StopCinematic(int handle);
e_status CIN_RunCinematic(int handle);
void CIN_DrawCinematic(int handle);
void CIN_SetExtents(int handle, int x, int y, int w, int h);
void CIN_SetLooping(int handle, qboolean loop);
void CIN_UploadCinematic(int handle);
void CIN_CloseAllVideos(void);

// cl_cgame.c

void CL_InitCGame(void);
void CL_ShutdownCGame(void);
qboolean CL_GameCommand(void);
void CL_CGameRendering(stereoFrame_t stereo);
void CL_SetCGameTime(void);
void CL_FirstSnapshot(void);
void CL_ShaderStateChanged(void);
void CL_UpdateLevelHunkUsage(void);
void CL_CGameBinaryMessageReceived(const byte *buf, int buflen, int serverTime);

// cl_ui.c

void CL_InitUI(void);
void CL_ShutdownUI(void);
int Key_GetCatcher(void);
void Key_SetCatcher(int catcher);
void LAN_LoadCachedServers(void);
void LAN_SaveServersToCache(void);

// cl_net_chan.c

void CL_Netchan_Transmit(netchan_t *chan, msg_t *msg);   //int length, const byte *data );
void CL_Netchan_TransmitNextFragment(netchan_t *chan);
qboolean CL_Netchan_Process(netchan_t *chan, msg_t *msg);
