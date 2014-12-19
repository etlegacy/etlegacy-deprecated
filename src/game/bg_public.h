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
 */
/**
 * @file bg_public.h
 * @brief Definitions shared by both the server game and client game modules. (server.h includes this)
 */

/*
 * because games can change separately from the main system version, we need a
 * second version that must match between game and cgame
 */

#ifndef INCLUDE_BG_PUBLIC_H
#define INCLUDE_BG_PUBLIC_H

#define GAME_VERSION        "Enemy Territory"
#define GAME_VERSION_DATED  (GAME_VERSION ", ET 2.60b")

#ifdef __GNUC__
#define _attribute(x) __attribute__(x)
#else
#define _attribute(x)
#endif

#define LEGACY_MOD         "Legacy"
#define LEGACY_MOD_VERSION ETLEGACY_VERSION_SHORT
#define LEGACY // for omnibot

#if defined(CGAMEDLL) || defined(FEATURE_SERVERMDX)
#define USE_MDXFILE
#endif

#define SPRINTTIME 20000.0f

#define DEFAULT_GRAVITY     800
#define FORCE_LIMBO_HEALTH  -75
#define GIB_HEALTH          -175

#define HOLDBREATHTIME      12000

#define RANK_TIED_FLAG      0x4000

#define ITEM_RADIUS         10      // item sizes are needed for client side pickup detection
                                    // - changed the radius so that the items would fit in the 3 new containers

#define MAX_TRACE           8192.0f // whenever you change this make sure bullet_Endpos for scope weapons is in sync!

#define VOTE_TIME           30000   // 30 seconds before vote times out

#define DEFAULT_VIEWHEIGHT  40
#define CROUCH_VIEWHEIGHT   16
#define DEAD_VIEWHEIGHT     -16

#define PRONE_VIEWHEIGHT    -8

extern vec3_t playerlegsProneMins;
extern vec3_t playerlegsProneMaxs;

#define MAX_COMMANDMAP_LAYERS   16

// on fire effects
#define FIRE_FLASH_TIME         2000
#define FIRE_FLASH_FADEIN_TIME  1000

#define LIGHTNING_FLASH_TIME    150

#define AAGUN_DAMAGE        25
#define AAGUN_SPREAD        10

#define MG42_SPREAD_MP      100

#define MG42_DAMAGE_MP          20
#define MG42_RATE_OF_FIRE_MP    66

#define AAGUN_RATE_OF_FIRE  100
#define MG42_YAWSPEED       300.f       // degrees per second

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_BUDDY   2
#define SAY_TEAMNL  3

#define MAX_FIRETEAMS           12
#define MAX_FIRETEAM_MEMBERS    8

#define MAX_SVCVARS 128

#define SVC_EQUAL           0
#define SVC_GREATER         1
#define SVC_GREATEREQUAL    2
#define SVC_LOWER           3
#define SVC_LOWEREQUAL      4
#define SVC_INSIDE          5
#define SVC_OUTSIDE         6
#define SVC_INCLUDE         7
#define SVC_EXCLUDE         8

typedef struct svCvar_s
{
	char cvarName[MAX_CVAR_VALUE_STRING];
	int mode;
	char Val1[MAX_CVAR_VALUE_STRING];
	char Val2[MAX_CVAR_VALUE_STRING];
} svCvar_t;

typedef struct forceCvar_s
{
	char cvarName[MAX_CVAR_VALUE_STRING];
	char cvarValue[MAX_CVAR_VALUE_STRING];
} forceCvar_t;

// client damage identifiers

// different entity states
typedef enum
{
	STATE_DEFAULT = 0,      // ent is linked, can be used and is solid
	STATE_INVISIBLE,        // ent is unlinked, can't be used, doesn't think and is not solid
	STATE_UNDERCONSTRUCTION // ent is being constructed
} entState_t;

typedef enum
{
	SELECT_BUDDY_ALL = 0,
	SELECT_BUDDY_1,
	SELECT_BUDDY_2,
	SELECT_BUDDY_3,
	SELECT_BUDDY_4,
	SELECT_BUDDY_5,
	SELECT_BUDDY_6,

	SELECT_BUDDY_LAST // must be the last one in the enum

} SelectBuddyFlag;

#define MAX_TAGCONNECTS     64

// zoom sway values
#define ZOOM_PITCH_AMPLITUDE        0.13f
#define ZOOM_PITCH_FREQUENCY        0.24f
#define ZOOM_PITCH_MIN_AMPLITUDE    0.1f        // minimum amount of sway even if completely settled on target

#define ZOOM_YAW_AMPLITUDE          0.7f
#define ZOOM_YAW_FREQUENCY          0.12f
#define ZOOM_YAW_MIN_AMPLITUDE      0.2f

#define MAX_OBJECTIVES      8
#define MAX_OID_TRIGGERS    18

#define MAX_GAMETYPES 16

typedef struct
{
	const char *mapName;
	const char *mapLoadName;
	const char *imageName;

	int typeBits;
	int cinematic;

	qhandle_t levelShot;
	qboolean active;

	int Timelimit;
	int AxisRespawnTime;
	int AlliedRespawnTime;

	vec2_t mappos;

	const char *briefing;
	const char *lmsbriefing;
	const char *objectives;
} mapInfo;

// Campaign saves
#define MAX_CAMPAIGNS           512

// changed this from 6 to 10
#define MAX_MAPS_PER_CAMPAIGN   10

typedef struct
{
	int mapnameHash;
} cpsMap_t;

typedef struct
{
	int shortnameHash;
	int progress;

	cpsMap_t maps[MAX_MAPS_PER_CAMPAIGN];
} cpsCampaign_t;

typedef struct
{
	int ident;
	int version;

	int numCampaigns;
	int profileHash;
} cpsHeader_t;

typedef struct
{
	cpsHeader_t header;
	cpsCampaign_t campaigns[MAX_CAMPAIGNS];
} cpsFile_t;

typedef struct
{
	const char *campaignShortName;
	const char *campaignName;
	const char *campaignDescription;
	const char *nextCampaignShortName;
	const char *maps;
	int mapCount;
	mapInfo *mapInfos[MAX_MAPS_PER_CAMPAIGN];
	vec2_t mapTC[2];
	cpsCampaign_t *cpsCampaign;   // if this campaign was found in the campaignsave, more detailed info can be found here

	const char *campaignShotName;
	int campaignCinematic;
	qhandle_t campaignShot;

	qboolean unlocked;
	int progress;

	qboolean initial;
	int order;

	int typeBits;
} campaignInfo_t;

// Random reinforcement seed settings
#define MAX_REINFSEEDS  8
#define REINF_RANGE     16      // (0 to n-1 second offset)
#define REINF_BLUEDELT  3       // Allies shift offset
#define REINF_REDDELT   2       // Axis shift offset
extern const unsigned int aReinfSeeds[MAX_REINFSEEDS];

// Client flags for server processing
#define CGF_AUTORELOAD      0x01
#define CGF_STATSDUMP       0x02
#define CGF_AUTOACTIVATE    0x04
#define CGF_PREDICTITEMS    0x08

#define MAX_MOTDLINES   6

#ifdef FEATURE_MULTIVIEW
// Multiview settings
#define MAX_MVCLIENTS               32
#define MV_SCOREUPDATE_INTERVAL     5000    // in msec
#endif

#define MAX_CHARACTERS  16

// config strings are a general means of communicating variable length strings
// from the server to all connected clients.

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
/**
 * @addtogroup lua_etvars
 * @{
 */
#define CS_MUSIC                        2
#define CS_MESSAGE                      3       //!< from the map worldspawn's message field
#define CS_MOTD                         4       //!< g_motd string for server message of the day
#define CS_WARMUP                       5       //!< server time when the match will be restarted
#define CS_VOTE_TIME                    6
#define CS_VOTE_STRING                  7
#define CS_VOTE_YES                     8
#define CS_VOTE_NO                      9
#define CS_GAME_VERSION                 10

#define CS_LEVEL_START_TIME             11      //!< so the timer only shows the current level
#define CS_INTERMISSION                 12      //!< when 1, intermission will start in a second or two
#define CS_MULTI_INFO                   13
#define CS_MULTI_MAPWINNER              14
#define CS_MULTI_OBJECTIVE              15

#define CS_SCREENFADE                   17      //!< used to tell clients to fade their screen to black/normal
#define CS_FOGVARS                      18      //!< used for saving the current state/settings of the fog
#define CS_SKYBOXORG                    19      //!< this is where we should view the skybox from

#define CS_TARGETEFFECT                 20
#define CS_WOLFINFO                     21
#define CS_FIRSTBLOOD                   22      //!< Team that has first blood
#define CS_ROUNDSCORES1                 23      //!< Axis round wins
#define CS_ROUNDSCORES2                 24      //!< Allied round wins
#define CS_MAIN_AXIS_OBJECTIVE          25      //!< unused - Most important current objective
#define CS_MAIN_ALLIES_OBJECTIVE        26      //!< unused - Most important current objective
#define CS_MUSIC_QUEUE                  27
#define CS_SCRIPT_MOVER_NAMES           28
#define CS_CONSTRUCTION_NAMES           29

#define CS_VERSIONINFO                  30      //!< Versioning info for demo playback compatibility
#define CS_REINFSEEDS                   31      //!< Reinforcement seeds
#define CS_SERVERTOGGLES                32      //!< Shows current enable/disabled settings (for voting UI)
#define CS_GLOBALFOGVARS                33
#define CS_AXIS_MAPS_XP                 34
#define CS_ALLIED_MAPS_XP               35
#define CS_INTERMISSION_START_TIME      36
#define CS_ENDGAME_STATS                37
#define CS_CHARGETIMES                  38
#define CS_FILTERCAMS                   39

#define CS_LEGACYINFO                   40
#define CS_SVCVAR                       41
#define CS_CONFIGNAME                   42

#define CS_TEAMRESTRICTIONS             43      //!< Class restrictions have been changed
#define CS_UPGRADERANGE                 44      //!< Upgrade range levels have been changed

#define CS_MODELS                       64
#define CS_SOUNDS                       (CS_MODELS +               MAX_MODELS)               // 320 (256)
#define CS_SHADERS                      (CS_SOUNDS +               MAX_SOUNDS)               // 576 (256)
#define CS_SHADERSTATE                  (CS_SHADERS +              MAX_CS_SHADERS)           // 608 (32) this MUST be after CS_SHADERS
#define CS_SKINS                        (CS_SHADERSTATE +          1)                        // 609 (1)
#define CS_CHARACTERS                   (CS_SKINS +                MAX_CS_SKINS)             // 673 (64)
#define CS_PLAYERS                      (CS_CHARACTERS +           MAX_CHARACTERS)           // 689 (16)
#define CS_MULTI_SPAWNTARGETS           (CS_PLAYERS +              MAX_CLIENTS)              // 753 (64)
#define CS_OID_TRIGGERS                 (CS_MULTI_SPAWNTARGETS +   MAX_MULTI_SPAWNTARGETS)   // 769 (16)
#define CS_OID_DATA                     (CS_OID_TRIGGERS +         MAX_OID_TRIGGERS)         // 787 (18)
#define CS_DLIGHTS                      (CS_OID_DATA +             MAX_OID_TRIGGERS)         // 805 (18)
#define CS_SPLINES                      (CS_DLIGHTS +              MAX_DLIGHT_CONFIGSTRINGS) // 821 (16)
#define CS_TAGCONNECTS                  (CS_SPLINES +              MAX_SPLINE_CONFIGSTRINGS) // 829 (8)
#define CS_FIRETEAMS                    (CS_TAGCONNECTS +          MAX_TAGCONNECTS)          // 893 (64)
#define CS_CUSTMOTD                     (CS_FIRETEAMS +            MAX_FIRETEAMS)            // 905 (12)
#define CS_STRINGS                      (CS_CUSTMOTD +             MAX_MOTDLINES)            // 911 (6)
#define CS_MAX                          (CS_STRINGS +              MAX_CSSTRINGS)            // 943 (32)
/** @}*/ // doxygen addtogroup lua_etvars

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum
{
	GT_SINGLE_PLAYER = 0, // obsolete
	GT_COOP,              // obsolete
	GT_WOLF,
	GT_WOLF_STOPWATCH,
	GT_WOLF_CAMPAIGN,     // Exactly the same as GT_WOLF, but uses campaign roulation (multiple maps form one virtual map)
	GT_WOLF_LMS,
	GT_WOLF_MAPVOTE,      // ETPub gametype map voting - Credits go to their team. TU!
	GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum
{
	PM_NORMAL = 0,  // can accelerate and turn
	PM_NOCLIP,      // noclip movement
	PM_SPECTATOR,   // still run into walls
	PM_DEAD,        // no acceleration or turning, but free falling
	PM_FREEZE,      // stuck in place with no control
	PM_INTERMISSION // no movement or status bar
} pmtype_t;

typedef enum
{
	WEAPON_READY = 0,
	WEAPON_RAISING,
	WEAPON_RAISING_TORELOAD,
	WEAPON_DROPPING,
	WEAPON_DROPPING_TORELOAD,
	WEAPON_READYING,    // getting from 'ready' to 'firing'
	WEAPON_RELAXING,    // weapon is ready, but since not firing, it's on it's way to a "relaxed" stance
	WEAPON_FIRING,
	WEAPON_FIRINGALT,
	WEAPON_RELOADING,
} weaponstate_t;

typedef enum
{
	WSTATE_IDLE = 0,
	WSTATE_SWITCH,
	WSTATE_FIRE,
	WSTATE_RELOAD
} weaponstateCompact_t;

// pmove->pm_flags	(sent as max 16 bits in msg.c)
#define PMF_DUCKED          1
#define PMF_JUMP_HELD       2
#define PMF_LADDER          4       // player is on a ladder
#define PMF_BACKWARDS_JUMP  8       // go into backwards land
#define PMF_BACKWARDS_RUN   16      // coast down to backwards run
#define PMF_TIME_LAND       32      // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK  64      // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP  256     // pm_time is waterjump
#define PMF_RESPAWNED       512     // clear after attack and jump buttons come up
//#define PMF_PRONE_BIPOD		1024	// prone with a bipod set
#define PMF_FLAILING        2048
#define PMF_FOLLOW          4096    // spectate following another player
#define PMF_TIME_LOAD       8192    // hold for this time after a load game, and prevent large thinks
#define PMF_LIMBO           16384   // limbo state, pm_time is time until reinforce
#define PMF_TIME_LOCKPLAYER 32768   // Lock all movement and view changes

#define PMF_ALL_TIMES   (PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK | PMF_TIME_LOCKPLAYER /*|PMF_TIME_LOAD*/)

typedef struct pmoveExt_s
{
	qboolean bAutoReload;           // do we predict autoreload of weapons

	int jumpTime;                   // used in MP to prevent jump accel

	int weapAnimTimer;              // don't change low priority animations until this runs out
	int silencedSideArm;            // Keep track of whether the luger/colt is silenced "in holster", prolly want to do this for the kar98 etc too
	int sprintTime;

	int airleft;

	// MG42 aiming
	float varc, harc;
	vec3_t centerangles;

	int proneTime;                  // time a go-prone or stop-prone move starts, to sync the animation to

	float proneLegsOffset;          // offset legs bounding box

	vec3_t mountedWeaponAngles;     // mortar, mg42 (prone), etc

	int weapRecoilTime;             // time at which a weapon that has a recoil kickback has been fired last
	int weapRecoilDuration;
	float weapRecoilYaw;
	float weapRecoilPitch;
	int lastRecoilDeltaTime;

	int shoveTime;

#ifdef GAMEDLL
	qboolean shoved;
	int pusher;
#endif

	qboolean releasedFire;

} pmoveExt_t;   // data used both in client and server - store it here
                // instead of playerstate to prevent different engine versions of playerstate between XP and MP

#define MAXTOUCH    32
typedef struct
{
	// state (in / out)
	playerState_t *ps;
	pmoveExt_t *pmext;
	struct bg_character_s *character;

	// command (in)
	usercmd_t cmd, oldcmd;
	int tracemask;                  // collide against these types of surfaces
	int debugLevel;                 // if set, diagnostic output will be printed
	qboolean noFootsteps;           // if the game is setup for no footsteps by the server
	qboolean noWeapClips;           // if the game is setup for no weapon clips by the server

	int gametype;
	int ltChargeTime;               // fieldopsChargeTime in cgame and ui. Cannot change here because of compatibility
	int soldierChargeTime;
	int engineerChargeTime;
	int medicChargeTime;

	int covertopsChargeTime;

	// results (out)
	int numtouch;
	int touchents[MAXTOUCH];

	vec3_t mins, maxs;              // bounding box size

	int watertype;
	int waterlevel;

	float xyspeed;

	int *skill;                     // player skills

	// for fixed msec Pmove
	int pmove_fixed;
	int pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void (*trace)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
	int (*pointcontents)(const vec3_t point, int passEntityNum);

	// used to determine if the player move is for prediction if it is, the movement should trigger no events
	qboolean predict;

} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles(playerState_t * ps, pmoveExt_t * pmext, usercmd_t * cmd, void (trace) (trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int tracemask);
int Pmove(pmove_t *pmove);
void PmovePredict(pmove_t *pmove, float frametime);

//===================================================================================

#define PC_SOLDIER              0   //	shoot stuff
#define PC_MEDIC                1   //	heal stuff
#define PC_ENGINEER             2   //	build stuff
#define PC_FIELDOPS             3   //	bomb stuff
#define PC_COVERTOPS            4   //	sneak about ;o

#define NUM_PLAYER_CLASSES      5

#define MAX_WEAPS_IN_BANK_MP    17
#define MAX_WEAP_BANKS_MP       10

// leaning flags..
#define STAT_LEAN_LEFT          0x00000001
#define STAT_LEAN_RIGHT         0x00000002

// player_state->stats[] indexes
typedef enum
{
	STAT_HEALTH = 0,
	STAT_KEYS,                      // 16 bit fields
	STAT_DEAD_YAW,                  // look this direction when dead (FIXME: get rid of?)
	STAT_MAX_HEALTH,                // health / armor limit, changable by handicap
	STAT_PLAYER_CLASS,              // player class in multiplayer
	STAT_XP,                        // "realtime" version of xp that doesnt need to go thru the scoreboard
	STAT_XP_OVERFLOW,               // count XP overflow(every 2^15)
	STAT_PS_FLAGS,
	STAT_AIRLEFT,                   // airtime for CG_DrawBreathBar()
} statIndex_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum
{
	PERS_SCORE = 0,                 // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,                      // total points damage inflicted so damage beeps can sound on change
	PERS_RANK,
	PERS_TEAM,
	PERS_SPAWN_COUNT,               // incremented every respawn
	PERS_ATTACKER,                  // clientnum of last damage inflicter
	PERS_KILLED,                    // count of the number of times you died
	// these were added for single player awards tracking
	PERS_RESPAWNS_LEFT,             // number of remaining respawns
	PERS_RESPAWNS_PENALTY,          // how many respawns you have to sit through before respawning again

	PERS_REVIVE_COUNT,
	PERS_HEADSHOTS,
	PERS_BLEH_3,

	// mg42                // I don't understand these here.  can someone explain?
	PERS_HWEAPON_USE,      // enum 12 - don't change
	// wolfkick
	PERS_WOLFKICK
} persEnum_t;

// entityState_t->eFlags
#define EF_DEAD             0x00000001      // don't draw a foe marker over players with EF_DEAD
#define EF_NONSOLID_BMODEL  0x00000002      // bmodel is visible, but not solid
#define EF_FORCE_END_FRAME  EF_NONSOLID_BMODEL  // force client to end of current animation (after loading a savegame)
#define EF_TELEPORT_BIT     0x00000004      // toggled every time the origin abruptly changes
#define EF_READY            0x00000008      // player is ready

#define EF_CROUCHING        0x00000010      // player is crouching
#define EF_MG42_ACTIVE      0x00000020      // currently using an MG42
#define EF_NODRAW           0x00000040      // may have an event, but no model (unspawned items)
#define EF_FIRING           0x00000080      // for lightning gun
#define EF_INHERITSHADER    EF_FIRING       // some ents will never use EF_FIRING, hijack it for "USESHADER"

#define EF_SPINNING         0x00000100      // added for level editor control of spinning pickup items
#define EF_BREATH           EF_SPINNING     // Characters will not have EF_SPINNING set, hijack for drawing character breath
#define EF_TALK             0x00000200      // draw a talk balloon
#define EF_CONNECTION       0x00000400      // draw a connection trouble sprite
#define EF_SMOKINGBLACK     0x00000800      // like EF_SMOKING only darker & bigger

#define EF_HEADSHOT         0x00001000      // last hit to player was head shot (NOTE: not last hit, but has BEEN shot in the head since respawn)
#define EF_SMOKING          0x00002000      // ET_GENERAL ents will emit smoke if set // JPW switched to this after my code change
#define EF_OVERHEATING      (EF_SMOKING | EF_SMOKINGBLACK)    // ydnar: light smoke/steam effect
#define EF_VOTED            0x00004000      // already cast a vote
#define EF_TAGCONNECT       0x00008000      // connected to another entity via tag
#define EF_MOUNTEDTANK      EF_TAGCONNECT   // duplicated for clarity

#define EF_FAKEBMODEL       0x00010000      // from etpro
#define EF_PATH_LINK        0x00020000      // linking trains together
#define EF_ZOOMING          0x00040000      // client is zooming
#define EF_PRONE            0x00080000      // player is prone

#define EF_PRONE_MOVING     0x00100000      // player is prone and moving
//#ifdef FEATURE_MULTIVIEW
#define EF_VIEWING_CAMERA   0x00200000      // player is viewing a camera
//#endif
#define EF_AAGUN_ACTIVE     0x00400000      // player is manning an AA gun
#define EF_SPARE0           0x00800000      // freed

// !! NOTE: only place flags that don't need to go to the client beyond 0x00800000
#define EF_SPARE1           0x01000000      // freed
#define EF_SPARE2           0x02000000      // freed
#define EF_BOUNCE           0x04000000      // for missiles
#define EF_BOUNCE_HALF      0x08000000      // for missiles
#define EF_MOVER_STOP       0x10000000      // will push otherwise	// moved down to make space for one more client flag
#define EF_MOVER_BLOCKED    0x20000000      // mover was blocked dont lerp on the client // xkan, moved down to make space for client flag

#define BG_PlayerMounted(eFlags) ((eFlags & EF_MG42_ACTIVE) || (eFlags & EF_MOUNTEDTANK) || (eFlags & EF_AAGUN_ACTIVE))

// !! NOTE: only place flags that don't need to go to the client beyond 0x00800000
typedef enum
{
	PW_NONE = 0,

	PW_INVULNERABLE,

	PW_NOFATIGUE = 4,

	PW_REDFLAG,
	PW_BLUEFLAG,

	PW_OPS_DISGUISED,
	PW_OPS_CLASS_1,
	PW_OPS_CLASS_2,
	PW_OPS_CLASS_3,

	PW_ADRENALINE,

	PW_BLACKOUT = 14,       // spec blackouts. FIXME: we don't need 32bits here...relocate

#ifdef FEATURE_MULTIVIEW
	PW_MVCLIENTLIST = 15,   // MV client info.. need a full 32 bits
#endif

	PW_NUM_POWERUPS
} powerup_t;

typedef enum
{
	//			These will probably all change to INV_n to get the word 'key' out of the game.
	//			id and DM don't want references to 'keys' in the game.
	//			I'll change to 'INV' as the item becomes 'permanent' and not a test item.

	KEY_NONE = 0,
	KEY_1,      // skull
	KEY_2,      // chalice
	KEY_3,      // eye
	KEY_4,      // field radio          unused
	KEY_5,      // satchel charge
	INV_BINOCS, // binoculars
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_10,
	KEY_11,
	KEY_12,
	KEY_13,
	KEY_14,
	KEY_15,
	KEY_16,
	KEY_LOCKED_PICKABLE, // ent can be unlocked with the WP_LOCKPICK. FIXME: remove
	KEY_NUM_KEYS
} wkey_t;           // conflicts with types.h

#define NO_AIRSTRIKE    1
#define NO_ARTILLERY    2

// NOTE: we can only use up to 15 in the client-server stream
// NOTE: should be 31 now (I added 1 bit in msg.c)
typedef enum
{
	WP_NONE = 0,            // 0
	WP_KNIFE,               // 1
	WP_LUGER,               // 2
	WP_MP40,                // 3
	WP_GRENADE_LAUNCHER,    // 4
	WP_PANZERFAUST,         // 5
	WP_FLAMETHROWER,        // 6
	WP_COLT,                // 7	// equivalent american weapon to german luger
	WP_THOMPSON,            // 8	// equivalent american weapon to german mp40
	WP_GRENADE_PINEAPPLE,   // 9

	WP_STEN,                // 10	// silenced sten sub-machinegun
	WP_MEDIC_SYRINGE,       // 11	// broken out from CLASS_SPECIAL per Id request
	WP_AMMO,                // 12	// likewise
	WP_ARTY,                // 13
	WP_SILENCER,            // 14	// used to be sp5
	WP_DYNAMITE,            // 15
	WP_SMOKETRAIL,          // 16
	WP_MAPMORTAR,           // 17
	VERYBIGEXPLOSION,       // 18	// explosion effect for airplanes
	WP_MEDKIT,              // 19

	WP_BINOCULARS,          // 20
	WP_PLIERS,              // 21
	WP_SMOKE_MARKER,        // 22	// changed name to cause less confusion
	WP_KAR98,               // 23	// WolfXP weapons
	WP_CARBINE,             // 24
	WP_GARAND,              // 25
	WP_LANDMINE,            // 26
	WP_SATCHEL,             // 27
	WP_SATCHEL_DET,         // 28
	WP_SMOKE_BOMB,          // 29

	WP_MOBILE_MG42,         // 30
	WP_K43,                 // 31
	WP_FG42,                // 32
	WP_DUMMY_MG42,          // 33   // for storing heat on mounted mg42s...
	WP_MORTAR,              // 34
	WP_AKIMBO_COLT,         // 35
	WP_AKIMBO_LUGER,        // 36

	WP_GPG40,               // 37
	WP_M7,                  // 38
	WP_SILENCED_COLT,       // 39

	WP_GARAND_SCOPE,        // 40
	WP_K43_SCOPE,           // 41
	WP_FG42SCOPE,           // 42
	WP_MORTAR_SET,          // 43
	WP_MEDIC_ADRENALINE,    // 44
	WP_AKIMBO_SILENCEDCOLT, // 45
	WP_AKIMBO_SILENCEDLUGER, // 46
	WP_MOBILE_MG42_SET,     // 47

	// legacy weapons
	WP_KNIFE_KABAR,         // 48
	WP_MOBILE_BROWNING,     // 49
	WP_MOBILE_BROWNING_SET, // 50
	WP_MORTAR2,             // 51
	WP_MORTAR2_SET,         // 52
	WP_BAZOOKA,             // 53

	WP_NUM_WEAPONS          // 54
	                        // NOTE: this cannot be larger than 64 for AI/player weapons!
} weapon_t;

// moved from cg_weapons (now used in g_active) for drop command, actual array in bg_misc.c
extern int weapBanksMultiPlayer[MAX_WEAP_BANKS_MP][MAX_WEAPS_IN_BANK_MP];

typedef struct
{
	int kills, teamkills, killedby;
} weaponStats_t;

typedef enum
{
	HR_HEAD = 0,
	HR_ARMS,
	HR_BODY,
	HR_LEGS,
	HR_NUM_HITREGIONS,
} hitRegion_t;

// MAPVOTE
#define MAX_VOTE_MAPS 32

typedef enum
{
	SK_BATTLE_SENSE = 0,
	SK_EXPLOSIVES_AND_CONSTRUCTION,
	SK_FIRST_AID,
	SK_SIGNALS,
	SK_LIGHT_WEAPONS,
	SK_HEAVY_WEAPONS,
	SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS,
	SK_NUM_SKILLS
} skillType_t;

// skill name 'shortcuts'
#define SK_SOLDIER      SK_HEAVY_WEAPONS
#define SK_MEDIC        SK_FIRST_AID
#define SK_ENGINEER     SK_EXPLOSIVES_AND_CONSTRUCTION
#define SK_FIELDOPS     SK_SIGNALS
#define SK_COVERTOPS    SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS

extern const char *skillNames[SK_NUM_SKILLS];
extern const char *skillNamesLine1[SK_NUM_SKILLS];
extern const char *skillNamesLine2[SK_NUM_SKILLS];
extern const char *medalNames[SK_NUM_SKILLS];

#define NUM_SKILL_LEVELS 5
extern int skillLevels[SK_NUM_SKILLS][NUM_SKILL_LEVELS];

typedef struct
{
	weaponStats_t weaponStats[WP_NUM_WEAPONS];
	int suicides;
	int hitRegions[HR_NUM_HITREGIONS];
	int objectiveStats[MAX_OBJECTIVES];
} playerStats_t;

typedef struct ammotable_s
{
	int maxammo;            //
	int uses;               //
	int maxclip;            //
	int defaultStartingAmmo;
	int defaultStartingClip;
	int reloadTime;         //
	int fireDelayTime;      //
	int nextShotTime;       //

	int maxHeat;            // max active firing time before weapon 'overheats' (at which point the weapon will fail)
	int coolRate;           // how fast the weapon cools down. (per second)

	int mod;                // means of death
} ammotable_t;

// Lookup table to find ammo table entry
extern ammotable_t *GetAmmoTableData(int ammoIndex);

typedef struct weapontable_s
{
	int weapon;               // reference
	int weapAlts;             // bg
	int akimboSideram;        // bg

	int ammoIndex;            // bg type of weapon ammo this uses.  (ex. WP_MP40 and WP_LUGER share 9mm ammo, so they both have WP_LUGER for giAmmoIndex)
	int clipIndex;            // bg which clip this weapon uses.  this allows the sniper rifle to use the same clip as the garand, etc.

	qboolean isScoped;        // bg

	int damage;               // g
	qboolean canGib;          // g
	qboolean isReload;        // g

	float spread;             // g
	//int splashDamage;         // g
	//int splashRadius;         // g

	//qboolean keepDisguise;    // g

	//qboolean isAutoReload;    // bg

	//qboolean isAkimbo;        // bg
	//qboolean isPanzer;        // bg
	//qboolean isRiflenade;     // bg
	//qboolean isMortar;        // bg
	//qboolean isMortarSet;     // bg

	//qboolean isHeavyWeapon;   // bg
	//qboolean isSetWeapon;     // bg

	//qboolean isUnderWaterFire; // bg

	//qboolean isValidStatWeapon; // bg (just check)

	// client
	// icons
	char *desc; // c - description for spawn weapons

} weaponTable_t;

#define WEAPON_CLASS_FOR_MOD_NO       -1
#define WEAPON_CLASS_FOR_MOD_EXPLOSIVE 0
#define WEAPON_CLASS_FOR_MOD_SATCHEL   1
#define WEAPON_CLASS_FOR_MOD_DYNAMITE  2

typedef struct modtable_s
{
	int mod;    // reference

	qboolean isHeadshot;   // g
	qboolean isExplosive;  // g

	int weaponClassForMOD; // g
} modTable_t;

extern weaponTable_t *GetWeaponTableData(int weaponIndex);

extern int weapAlts[];  // defined in bg_misc.c

// FIXME: weapon table - put following macros in
#define IS_RIFLENADE_WEAPON(w) \
	(w == WP_GPG40               || w == WP_M7)

#define IS_PANZER_WEAPON(w) \
	(w == WP_PANZERFAUST         || w == WP_BAZOOKA)

#define IS_AKIMBO_WEAPON(w) \
	(w == WP_AKIMBO_COLT         || w == WP_AKIMBO_LUGER         || \
	 w == WP_AKIMBO_SILENCEDCOLT || w == WP_AKIMBO_SILENCEDLUGER)

#define IS_MORTAR_WEAPON(w) \
	(w == WP_MORTAR              || w == WP_MORTAR2              || \
	 w == WP_MORTAR_SET          || w == WP_MORTAR2_SET)

#define IS_MORTAR_WEAPON_SET(w) \
	(w == WP_MORTAR_SET          || w == WP_MORTAR2_SET)

#define IS_SET_WEAPON(w)    \
	(w == WP_MORTAR_SET      || w == WP_MORTAR2_SET             || \
	 w == WP_MOBILE_MG42_SET  || w == WP_MOBILE_BROWNING_SET)

#define IS_HEAVY_WEAPON(w) \
	(w == WP_FLAMETHROWER  || \
	 w == WP_MOBILE_MG42 || w == WP_MOBILE_MG42_SET || \
	 w == WP_PANZERFAUST || w == WP_BAZOOKA || \
	 w == WP_MORTAR          || w == WP_MORTAR_SET  || \
	 w == WP_MOBILE_BROWNING || w == WP_MOBILE_BROWNING_SET || \
	 w == WP_MORTAR2         || w == WP_MORTAR2_SET)

#define IS_MG_WEAPON(w) \
	(w == WP_MOBILE_MG42          || w == WP_MOBILE_BROWNING)

#define IS_MG_WEAPON_SET(w) \
	(w == WP_MOBILE_MG42_SET          || w == WP_MOBILE_BROWNING_SET)

#define IS_AUTORELOAD_WEAPON(w) \
	(w == WP_LUGER    || w == WP_COLT          || w == WP_MP40          || \
	 w == WP_THOMPSON || w == WP_STEN          || \
	 w == WP_KAR98    || w == WP_CARBINE       || w == WP_GARAND_SCOPE  || \
	 w == WP_FG42     || w == WP_K43           || w == WP_MOBILE_MG42   || \
	 w == WP_MOBILE_BROWNING || w == WP_SILENCED_COLT    || w == WP_SILENCER      || \
	 w == WP_GARAND   || w == WP_K43_SCOPE     || w == WP_FG42SCOPE     || \
	 IS_AKIMBO_WEAPON(w) || w == WP_MOBILE_MG42_SET || w == WP_MOBILE_BROWNING_SET \
	)

#define IS_VALID_WEAPON(w) (w > WP_NONE && w < WP_NUM_WEAPONS)

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1       0x00000100
#define EV_EVENT_BIT2       0x00000200
#define EV_EVENT_BITS       (EV_EVENT_BIT1 | EV_EVENT_BIT2)

// note: if you add events also add eventnames - see bg_misc.c
typedef enum
{
	EV_NONE = 0,
	EV_FOOTSTEP,
	//EV_FOOTSTEP_METAL,
	//EV_FOOTSTEP_WOOD,
	//EV_FOOTSTEP_GRASS,
	//EV_FOOTSTEP_GRAVEL,
	//EV_FOOTSTEP_ROOF,
	//EV_FOOTSTEP_SNOW,
	//EV_FOOTSTEP_CARPET,
	EV_FOOTSPLASH = 9,
	//EV_FOOTWADE,
	EV_SWIM = 11,
	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,
	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,
	EV_FALL_NDIE,
	EV_FALL_DMG_10,
	EV_FALL_DMG_15,
	EV_FALL_DMG_25,
	EV_FALL_DMG_50,
	EV_WATER_TOUCH,         // foot touches
	EV_WATER_LEAVE,         // foot leaves
	EV_WATER_UNDER,         // head touches
	EV_WATER_CLEAR,         // head leaves
	EV_ITEM_PICKUP,         // normal item pickups are predictable
	EV_ITEM_PICKUP_QUIET,   // same, but don't play the default pickup sound as it was specified in the ent
	EV_GLOBAL_ITEM_PICKUP,  // powerup / team sounds are broadcast to everyone
	EV_NOAMMO,
	EV_WEAPONSWITCHED,
	//EV_EMPTYCLIP,
	EV_FILL_CLIP = 34,
	EV_MG42_FIXED,
	EV_WEAP_OVERHEAT,
	EV_CHANGE_WEAPON,
	EV_CHANGE_WEAPON_2,
	EV_FIRE_WEAPON,
	EV_FIRE_WEAPONB,
	EV_FIRE_WEAPON_LASTSHOT,
	EV_NOFIRE_UNDERWATER,
	EV_FIRE_WEAPON_MG42,        // mounted MG
	EV_FIRE_WEAPON_MOUNTEDMG42, // tank MG
	//EV_ITEM_RESPAWN,
	//EV_ITEM_POP,
	//EV_PLAYER_TELEPORT_IN,
	//EV_PLAYER_TELEPORT_OUT,
	EV_GRENADE_BOUNCE = 49,          // eventParm will be the soundindex
	EV_GENERAL_SOUND,
	EV_GENERAL_SOUND_VOLUME,
	EV_GLOBAL_SOUND,        // no attenuation
	EV_GLOBAL_CLIENT_SOUND, // no attenuation, only plays for specified client
	EV_GLOBAL_TEAM_SOUND,   // no attenuation, team only
	EV_FX_SOUND,
	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,
	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_RAILTRAIL,
	EV_BULLET,              // otherEntity is the shooter
	EV_LOSE_HAT,
	EV_PAIN,
	//EV_CROUCH_PAIN,
	//EV_DEATH1,
	//EV_DEATH2,
	//EV_DEATH3,
	EV_OBITUARY = 68,
	EV_STOPSTREAMINGSOUND, // swiped from sherman
	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,
	EV_GIB_PLAYER,          // gib a previously living player
	//EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND = 75,  // unused
	//EV_TAUNT,
	EV_SMOKE = 77,
	EV_SPARKS,
	EV_SPARKS_ELECTRIC,
	EV_EXPLODE,     // func_explosive
	EV_RUBBLE,
	EV_EFFECT,      // target_effect
	EV_MORTAREFX,   // mortar firing
	EV_SPINUP,      // panzerfaust preamble
	//EV_SNOW_ON,
	//EV_SNOW_OFF,
	EV_MISSILE_MISS_SMALL = 87,
	EV_MISSILE_MISS_LARGE,
	EV_MORTAR_IMPACT,
	EV_MORTAR_MISS,
	//EV_SPIT_HIT,
	//EV_SPIT_MISS,
	EV_SHARD = 93,
	EV_JUNK,
	EV_EMITTER, // generic particle emitter that uses client-side particle scripts
	EV_OILPARTICLES,
	EV_OILSLICK,
	EV_OILSLICKREMOVE,
	//EV_MG42EFX,
	//EV_FLAKGUN1,
	//EV_FLAKGUN2,
	//EV_FLAKGUN3,
	//EV_FLAKGUN4,
	//EV_EXERT1,
	//EV_EXERT2,
	//EV_EXERT3,
	EV_SNOWFLURRY = 107,
	//EV_CONCUSSIVE,
	EV_DUST = 109,
	EV_RUMBLE_EFX,
	EV_GUNSPARKS,
	EV_FLAMETHROWER_EFFECT,
	//EV_POPUP,
	//EV_POPUPBOOK,
	//EV_GIVEPAGE,
	EV_MG42BULLET_HIT_FLESH = 116,  // these two send the seed as well
	EV_MG42BULLET_HIT_WALL,
	EV_SHAKE,
	EV_DISGUISE_SOUND,
	EV_BUILDDECAYED_SOUND,
	EV_FIRE_WEAPON_AAGUN,
	EV_DEBRIS,
	EV_ALERT_SPEAKER,
	EV_POPUPMESSAGE,
	EV_ARTYMESSAGE,
	EV_AIRSTRIKEMESSAGE,
	EV_MEDIC_CALL,  // end of vanilla events
	EV_SHOVE_SOUND, // 127 - ETL shove
	EV_BODY_DP,     // 128
	EV_MAX_EVENTS   // 129 - just added as an 'endcap'
} entity_event_t;

extern const char *eventnames[EV_MAX_EVENTS];

typedef enum
{
	BOTH_DEATH1 = 0,
	BOTH_DEAD1,
	BOTH_DEAD1_WATER,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEAD2_WATER,
	BOTH_DEATH3,
	BOTH_DEAD3,
	BOTH_DEAD3_WATER,

	BOTH_CLIMB,
/*10*/ BOTH_CLIMB_DOWN,
	BOTH_CLIMB_DISMOUNT,

	BOTH_SALUTE,

	BOTH_PAIN1,     // head
	BOTH_PAIN2,     // chest
	BOTH_PAIN3,     // groin
	BOTH_PAIN4,     // right shoulder
	BOTH_PAIN5,     // left shoulder
	BOTH_PAIN6,     // right knee
	BOTH_PAIN7,     // left knee
/*20*/ BOTH_PAIN8,      // dazed

	BOTH_GRAB_GRENADE,

	BOTH_ATTACK1,
	BOTH_ATTACK2,
	BOTH_ATTACK3,
	BOTH_ATTACK4,
	BOTH_ATTACK5,

	BOTH_EXTRA1,
	BOTH_EXTRA2,
	BOTH_EXTRA3,
/*30*/ BOTH_EXTRA4,
	BOTH_EXTRA5,
	BOTH_EXTRA6,
	BOTH_EXTRA7,
	BOTH_EXTRA8,
	BOTH_EXTRA9,
	BOTH_EXTRA10,
	BOTH_EXTRA11,
	BOTH_EXTRA12,
	BOTH_EXTRA13,
/*40*/ BOTH_EXTRA14,
	BOTH_EXTRA15,
	BOTH_EXTRA16,
	BOTH_EXTRA17,
	BOTH_EXTRA18,
	BOTH_EXTRA19,
	BOTH_EXTRA20,

	TORSO_GESTURE,
	TORSO_GESTURE2,
	TORSO_GESTURE3,
/*50*/ TORSO_GESTURE4,

	TORSO_DROP,

	TORSO_RAISE,    // (low)
	TORSO_ATTACK,
	TORSO_STAND,
	TORSO_STAND_ALT1,
	TORSO_STAND_ALT2,
	TORSO_READY,
	TORSO_RELAX,

	TORSO_RAISE2,   // (high)
/*60*/ TORSO_ATTACK2,
	TORSO_STAND2,
	TORSO_STAND2_ALT1,
	TORSO_STAND2_ALT2,
	TORSO_READY2,
	TORSO_RELAX2,

	TORSO_RAISE3,   // (pistol)
	TORSO_ATTACK3,
	TORSO_STAND3,
	TORSO_STAND3_ALT1,
/*70*/ TORSO_STAND3_ALT2,
	TORSO_READY3,
	TORSO_RELAX3,

	TORSO_RAISE4,   // (shoulder)
	TORSO_ATTACK4,
	TORSO_STAND4,
	TORSO_STAND4_ALT1,
	TORSO_STAND4_ALT2,
	TORSO_READY4,
	TORSO_RELAX4,

/*80*/ TORSO_RAISE5,    // (throw)
	TORSO_ATTACK5,
	TORSO_ATTACK5B,
	TORSO_STAND5,
	TORSO_STAND5_ALT1,
	TORSO_STAND5_ALT2,
	TORSO_READY5,
	TORSO_RELAX5,

	TORSO_RELOAD1,  // (low)
	TORSO_RELOAD2,  // (high)
/*90*/ TORSO_RELOAD3,   // (pistol)
	TORSO_RELOAD4,  // (shoulder)

	TORSO_MG42,     // firing tripod mounted weapon animation

	TORSO_MOVE,     // torso anim to play while moving and not firing (swinging arms type thing)
	TORSO_MOVE_ALT,

	TORSO_EXTRA,
	TORSO_EXTRA2,
	TORSO_EXTRA3,
	TORSO_EXTRA4,
	TORSO_EXTRA5,
/*100*/ TORSO_EXTRA6,
	TORSO_EXTRA7,
	TORSO_EXTRA8,
	TORSO_EXTRA9,
	TORSO_EXTRA10,

	LEGS_WALKCR,
	LEGS_WALKCR_BACK,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
/*110*/ LEGS_SWIM,
	LEGS_SWIM_IDLE,

	LEGS_JUMP,
	LEGS_JUMPB,
	LEGS_LAND,

	LEGS_IDLE,
	LEGS_IDLE_ALT, // LEGS_IDLE2
	LEGS_IDLECR,

	LEGS_TURN,

	LEGS_BOOT,      // kicking animation

/*120*/ LEGS_EXTRA1,
	LEGS_EXTRA2,
	LEGS_EXTRA3,
	LEGS_EXTRA4,
	LEGS_EXTRA5,
	LEGS_EXTRA6,
	LEGS_EXTRA7,
	LEGS_EXTRA8,
	LEGS_EXTRA9,
	LEGS_EXTRA10,

/*130*/ MAX_ANIMATIONS
} animNumber_t;

// text represenation for scripting
extern const char *animStrings[];     // defined in bg_misc.c

typedef enum
{
	WEAP_IDLE1 = 0,
	WEAP_IDLE2,
	WEAP_ATTACK1,
	WEAP_ATTACK2,
	WEAP_ATTACK_LASTSHOT,   // used when firing the last round before having an empty clip.
	WEAP_DROP,
	WEAP_RAISE,
	WEAP_RELOAD1,
	WEAP_RELOAD2,
	WEAP_RELOAD3,
	WEAP_ALTSWITCHFROM, // switch from alt fire mode weap (scoped/silencer/etc)
	WEAP_ALTSWITCHTO,   // switch to alt fire mode weap
	WEAP_DROP2,
	MAX_WP_ANIMATIONS
} weapAnimNumber_t;

typedef enum hudHeadAnimNumber_s
{
	HD_IDLE1 = 0,
	HD_IDLE2,
	HD_IDLE3,
	HD_IDLE4,
	HD_IDLE5,
	HD_IDLE6,
	HD_IDLE7,
	HD_IDLE8,
	HD_DAMAGED_IDLE1,
	HD_DAMAGED_IDLE2,
	HD_DAMAGED_IDLE3,
	HD_LEFT,
	HD_RIGHT,
	HD_ATTACK,
	HD_ATTACK_END,
	HD_PAIN,
	MAX_HD_ANIMATIONS
} hudHeadAnimNumber_t;

#define ANIMFL_LADDERANIM   0x1
#define ANIMFL_FIRINGANIM   0x2
#define ANIMFL_REVERSED     0x4

typedef struct animation_s
{
#ifdef USE_MDXFILE
	qhandle_t mdxFile;
#else
	char mdxFileName[MAX_QPATH];
#endif // CGAMEDLL
	char name[MAX_QPATH];
	int firstFrame;
	int numFrames;
	int loopFrames;                 // 0 to numFrames
	int frameLerp;                  // msec between frames
	int initialLerp;                // msec to get to first frame
	int moveSpeed;
	int animBlend;                  // take this long to blend to next anim

	// derived
	int duration;
	int nameHash;
	int flags;
	int movetype;
} animation_t;

// head animations
typedef enum
{
	HEAD_NEUTRAL_CLOSED = 0,
	HEAD_NEUTRAL_A,
	HEAD_NEUTRAL_O,
	HEAD_NEUTRAL_I,
	HEAD_NEUTRAL_E,
	HEAD_HAPPY_CLOSED,
	HEAD_HAPPY_O,
	HEAD_HAPPY_I,
	HEAD_HAPPY_E,
	HEAD_HAPPY_A,
	HEAD_ANGRY_CLOSED,
	HEAD_ANGRY_O,
	HEAD_ANGRY_I,
	HEAD_ANGRY_E,
	HEAD_ANGRY_A,

	MAX_HEAD_ANIMS
} animHeadNumber_t;

typedef struct headAnimation_s
{
	int firstFrame;
	int numFrames;
} headAnimation_t;

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT      (1 << (ANIM_BITS - 1))

// renamed these to team_axis/allies, it really was awful....
typedef enum
{
	TEAM_FREE = 0,
	TEAM_AXIS,
	TEAM_ALLIES,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME       1000

// weapon stat info: mapping between MOD_ and WP_ types (FIXME for new ET weapons)
typedef enum extWeaponStats_s
{
	WS_KNIFE = 0,       // 0
	WS_KNIFE_KBAR,      // 1
	WS_LUGER,           // 2
	WS_COLT,            // 3
	WS_MP40,            // 4
	WS_THOMPSON,        // 5
	WS_STEN,            // 6
	WS_FG42,            // 7
	WS_PANZERFAUST,     // 8
	WS_BAZOOKA,         // 9
	WS_FLAMETHROWER,    // 10
	WS_GRENADE,         // 11   -- includes axis and allies grenade types
	WS_MORTAR,          // 12
	WS_MORTAR2,         // 13
	WS_DYNAMITE,        // 14
	WS_AIRSTRIKE,       // 15   -- Fieldops smoke grenade attack
	WS_ARTILLERY,       // 16   -- Fieldops binocular attack
	WS_SATCHEL,         // 17
	WS_GRENADELAUNCHER, // 18
	WS_LANDMINE,        // 19
	WS_MG42,            // 20
	WS_BROWNING,        // 21
	WS_GARAND,          // 22   -- carbine and garand
	WS_K43,             // 23   -- kar98 and k43

	WS_MAX
} extWeaponStats_t;

typedef struct
{
	qboolean fHasHeadShots;
	const char *pszCode;
	const char *pszName;
} weap_ws_t;

extern const weap_ws_t aWeaponInfo[WS_MAX];

// means of death
typedef enum
{
	MOD_UNKNOWN = 0,
	MOD_MACHINEGUN,
	MOD_BROWNING,
	MOD_MG42,
	MOD_GRENADE,

	// modified wolf weap mods
	MOD_KNIFE,
	MOD_LUGER,
	MOD_COLT,
	MOD_MP40,
	MOD_THOMPSON,
	MOD_STEN,
	MOD_GARAND,

	MOD_SILENCER,
	MOD_FG42,
	MOD_FG42SCOPE,
	MOD_PANZERFAUST,
	MOD_GRENADE_LAUNCHER,
	MOD_FLAMETHROWER,
	MOD_GRENADE_PINEAPPLE,

	MOD_MAPMORTAR,
	MOD_MAPMORTAR_SPLASH,

	MOD_KICKED,

	MOD_DYNAMITE,
	MOD_AIRSTRIKE,
	MOD_SYRINGE,
	MOD_AMMO,
	MOD_ARTY,

	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_EXPLOSIVE,

	MOD_CARBINE,
	MOD_KAR98,
	MOD_GPG40,
	MOD_M7,
	MOD_LANDMINE,
	MOD_SATCHEL,

	MOD_SMOKEBOMB,
	MOD_MOBILE_MG42,
	MOD_SILENCED_COLT,
	MOD_GARAND_SCOPE,

	MOD_CRUSH_CONSTRUCTION,
	MOD_CRUSH_CONSTRUCTIONDEATH,
	MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER,

	MOD_K43,
	MOD_K43_SCOPE,

	MOD_MORTAR,

	MOD_AKIMBO_COLT,
	MOD_AKIMBO_LUGER,
	MOD_AKIMBO_SILENCEDCOLT,
	MOD_AKIMBO_SILENCEDLUGER,

	MOD_SMOKEGRENADE,

	MOD_SWAP_PLACES,

	// keep these 2 entries last
	MOD_SWITCHTEAM,

	MOD_SHOVE,

	MOD_KNIFE_KABAR,
	MOD_MOBILE_BROWNING,
	MOD_MORTAR2,
	MOD_BAZOOKA,

	MOD_NUM_MODS

} meansOfDeath_t;

//---------------------------------------------------------

// gitem_t->type
typedef enum
{
	IT_BAD = 0,
	IT_WEAPON,              // EFX: rotate + upscale + minlight

	IT_AMMO,                // EFX: rotate
	IT_ARMOR,               // EFX: rotate + minlight
	IT_HEALTH,              // EFX: static external sphere + rotating internal
	IT_HOLDABLE,            // #100 obsolete - remove! (also HINT_HOLDABLE)
	                        // EFX: rotate + bob
	IT_KEY,
	IT_TREASURE,            // #100 obsolete - remove! gold bars, etc.  things that can be picked up and counted for a tally at end-level
	IT_TEAM,
} itemType_t;

#define MAX_ITEM_MODELS 3

#ifdef CGAMEDLL

#define MAX_ITEM_ICONS 4

// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct
{
	qboolean registered;
	qhandle_t models[MAX_ITEM_MODELS];
	qhandle_t icons[MAX_ITEM_ICONS];
} itemInfo_t;
#endif

typedef struct gitem_s
{
	char *classname;        // spawning name
	char *pickup_sound;
	char *world_model[MAX_ITEM_MODELS];

	char *icon;
	char *ammoicon;
	char *pickup_name;          // for printing on pickup

	int quantity;               // for ammo how much, or duration of powerup (value not necessary for ammo/health.  that value set in gameskillnumber[] below)
	itemType_t giType;          // IT_* flags

	int giTag;

#ifdef CGAMEDLL
	itemInfo_t itemInfo; // FIXME: fix default value in bg_itemlist
#endif

} gitem_t;

// included in both the game dll and the client
extern gitem_t bg_itemlist[];
extern int     bg_numItems;

#define FIRST_WEAPON_ITEM 9 // bg_itemlist is sorted and weapons start at 9

// FIXME: create enum for this with all items so we don't have to adjust this for item changes ... see bg_itemlist
#define ITEM_HEALTH 3
#define ITEM_HEALTH_CABINET 5
#define ITEM_AMMO_PACK 35
#define ITEM_MEGA_AMMO_PACK 36
#define ITEM_RED_FLAG 79
#define ITEM_BLUE_FLAG 80
#define ITEM_MAX_ITEMS 81 // keep in sync with bg_numItems!

gitem_t *BG_FindItem(const char *pickupName);
gitem_t *BG_FindItemForClassName(const char *className);
gitem_t *BG_FindItemForWeapon(weapon_t weapon);
gitem_t *BG_GetItem(int index);

weapon_t BG_FindAmmoForWeapon(weapon_t weapon);
weapon_t BG_FindClipForWeapon(weapon_t weapon);

qboolean BG_AkimboFireSequence(int weapon, int akimboClip, int mainClip);

qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, int *skill, int teamNum);

// content masks
#define MASK_ALL                (-1)
#define MASK_SOLID              (CONTENTS_SOLID)
#define MASK_PLAYERSOLID        (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY)
#define MASK_DEADSOLID          (CONTENTS_SOLID | CONTENTS_PLAYERCLIP)
#define MASK_WATER              (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME)
//#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_OPAQUE             (CONTENTS_SOLID | CONTENTS_LAVA)        // modified since slime is no longer deadly
#define MASK_SHOT               (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE)
#define MASK_MISSILESHOT        (MASK_SHOT | CONTENTS_MISSILECLIP)

// entityState_t->eType

// cursorhints (stored in ent->s.dmgFlags since that's only used for players at the moment)
// FIXME: clean this - many hint types are obsolete but keep enum numbers!
// note: adjust omnibot while cleaning this see OB ET_Config.h
typedef enum
{
	HINT_NONE = 0,      // reserved
	HINT_FORCENONE,     // reserved
	HINT_PLAYER,
	HINT_ACTIVATE,
	HINT_DOOR,
	HINT_DOOR_ROTATING,
	HINT_DOOR_LOCKED,
	HINT_DOOR_ROTATING_LOCKED,
	HINT_MG42,
	HINT_BREAKABLE,             // FIXME: remove - never set!
	HINT_BREAKABLE_DYNAMITE,    // FIXME: remove - never set!
	HINT_CHAIR,
	HINT_ALARM,
	HINT_HEALTH,
	HINT_TREASURE,
	HINT_KNIFE,
	HINT_LADDER,
	HINT_BUTTON,
	HINT_WATER,
	HINT_CAUTION,
	HINT_DANGER,
	HINT_SECRET,
	HINT_QUESTION,
	HINT_EXCLAMATION,
	HINT_CLIPBOARD,
	HINT_WEAPON,
	HINT_AMMO,
	HINT_ARMOR,
	HINT_POWERUP,
	HINT_HOLDABLE,
	HINT_INVENTORY,
	HINT_SCENARIC,
	HINT_EXIT,         // FIXME: remove me - never set!
	HINT_NOEXIT,       // FIXME: remove me - never set!
	HINT_PLYR_FRIEND,  // FIXME: remove this!
	HINT_PLYR_NEUTRAL, // FIXME: remove this!
	HINT_PLYR_ENEMY,
	HINT_PLYR_UNKNOWN,
	HINT_BUILD,
	HINT_DISARM,
	HINT_REVIVE,
	HINT_DYNAMITE,
	HINT_CONSTRUCTIBLE,
	HINT_UNIFORM,
	HINT_LANDMINE,
	HINT_TANK,
	HINT_SATCHELCHARGE,
	HINT_LOCKPICK, // @brief unused

	HINT_BAD_USER,  // invisible user with no target

	HINT_NUM_HINTS
} hintType_t;

void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splinePath);
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splineData);
void BG_GetMarkDir(const vec3_t dir, const vec3_t normal, vec3_t out);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);

void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, int time, qboolean snap);
weapon_t BG_WeaponForMOD(int MOD);

qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime);
qboolean BG_PlayerSeesItem(playerState_t *ps, entityState_t *item, int atTime);
qboolean BG_AddMagicAmmo(playerState_t *ps, int *skill, int teamNum, int numOfClips);

#define OVERCLIP        1.001

void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);

#define MAX_ARENAS          64
#define MAX_ARENAS_TEXT     8192

#define MAX_CAMPAIGNS_TEXT  8192

typedef enum
{
	FOOTSTEP_NORMAL = 0,
	FOOTSTEP_METAL,
	FOOTSTEP_WOOD,
	FOOTSTEP_GRASS,
	FOOTSTEP_GRAVEL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_ROOF,
	FOOTSTEP_SNOW,
	FOOTSTEP_CARPET,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum
{
	BRASSSOUND_METAL = 0,
	BRASSSOUND_SOFT,
	BRASSSOUND_STONE,
	BRASSSOUND_WOOD,
	BRASSSOUND_MAX,
} brassSound_t;

typedef enum
{
	FXTYPE_WOOD = 0,
	FXTYPE_GLASS,
	FXTYPE_METAL,
	FXTYPE_GIBS,
	FXTYPE_BRICK,
	FXTYPE_STONE,
	FXTYPE_FABRIC,
	FXTYPE_MAX
} fxType_t;

//==================================================================
// New Animation Scripting Defines

#define MAX_ANIMSCRIPT_MODELS               32
#define MAX_ANIMSCRIPT_ITEMS_PER_MODEL      2048
#define MAX_MODEL_ANIMATIONS                512     // animations per model
#define MAX_ANIMSCRIPT_ANIMCOMMANDS         8
#define MAX_ANIMSCRIPT_ITEMS                128
// NOTE: these must all be in sync with string tables in bg_animation.c

typedef enum
{
	ANIM_MT_UNUSED = 0,
	ANIM_MT_IDLE,
	ANIM_MT_IDLECR,
	ANIM_MT_WALK,
	ANIM_MT_WALKBK,
	ANIM_MT_WALKCR,
	ANIM_MT_WALKCRBK,
	ANIM_MT_RUN,
	ANIM_MT_RUNBK,
	ANIM_MT_SWIM,
	ANIM_MT_SWIMBK,
	ANIM_MT_STRAFERIGHT,
	ANIM_MT_STRAFELEFT,
	ANIM_MT_TURNRIGHT,
	ANIM_MT_TURNLEFT,
	ANIM_MT_CLIMBUP,
	ANIM_MT_CLIMBDOWN,
	ANIM_MT_FALLEN,                 // dead, before limbo
	ANIM_MT_PRONE,
	ANIM_MT_PRONEBK,
	ANIM_MT_IDLEPRONE,
	ANIM_MT_FLAILING,
	//ANIM_MT_TALK,
	ANIM_MT_SNEAK,
	ANIM_MT_AFTERBATTLE,            // just finished battle

	NUM_ANIM_MOVETYPES
} scriptAnimMoveTypes_t;

typedef enum
{
	ANIM_ET_PAIN = 0,
	ANIM_ET_DEATH,
	ANIM_ET_FIREWEAPON,
	ANIM_ET_FIREWEAPON2,
	ANIM_ET_JUMP,
	ANIM_ET_JUMPBK,
	ANIM_ET_LAND,
	ANIM_ET_DROPWEAPON,
	ANIM_ET_RAISEWEAPON,
	ANIM_ET_CLIMB_MOUNT,
	ANIM_ET_CLIMB_DISMOUNT,
	ANIM_ET_RELOAD,
	ANIM_ET_PICKUPGRENADE,
	ANIM_ET_KICKGRENADE,
	ANIM_ET_QUERY,
	ANIM_ET_INFORM_FRIENDLY_OF_ENEMY,
	ANIM_ET_KICK,
	ANIM_ET_REVIVE,
	ANIM_ET_FIRSTSIGHT,
	ANIM_ET_ROLL,
	ANIM_ET_FLIP,
	ANIM_ET_DIVE,
	ANIM_ET_PRONE_TO_CROUCH,
	ANIM_ET_BULLETIMPACT,
	ANIM_ET_INSPECTSOUND,
	ANIM_ET_SECONDLIFE,
	ANIM_ET_DO_ALT_WEAPON_MODE,
	ANIM_ET_UNDO_ALT_WEAPON_MODE,
	ANIM_ET_DO_ALT_WEAPON_MODE_PRONE,
	ANIM_ET_UNDO_ALT_WEAPON_MODE_PRONE,
	ANIM_ET_FIREWEAPONPRONE,
	ANIM_ET_FIREWEAPON2PRONE,
	ANIM_ET_RAISEWEAPONPRONE,
	ANIM_ET_RELOADPRONE,
	ANIM_ET_TALK,
	ANIM_ET_NOPOWER,

	NUM_ANIM_EVENTTYPES
} scriptAnimEventTypes_t;

typedef enum
{
	ANIM_BP_UNUSED = 0,
	ANIM_BP_LEGS,
	ANIM_BP_TORSO,
	ANIM_BP_BOTH,

	NUM_ANIM_BODYPARTS
} animBodyPart_t;

typedef enum
{
	ANIM_COND_WEAPON = 0,
	ANIM_COND_ENEMY_POSITION,
	ANIM_COND_ENEMY_WEAPON,
	ANIM_COND_UNDERWATER,
	ANIM_COND_MOUNTED,
	ANIM_COND_MOVETYPE,
	ANIM_COND_UNDERHAND,
	ANIM_COND_LEANING,
	ANIM_COND_IMPACT_POINT,
	ANIM_COND_CROUCHING,
	ANIM_COND_STUNNED,
	ANIM_COND_FIRING,
	ANIM_COND_SHORT_REACTION,
	ANIM_COND_ENEMY_TEAM,
	ANIM_COND_PARACHUTE,
	ANIM_COND_CHARGING,
	ANIM_COND_SECONDLIFE,
	ANIM_COND_HEALTH_LEVEL,
	ANIM_COND_FLAILING_TYPE,
	ANIM_COND_GEN_BITFLAG,      // general bit flags (to save some space)
	ANIM_COND_AISTATE,          // our current ai state (sometimes more convenient than creating a separate section)

	NUM_ANIM_CONDITIONS
} scriptAnimConditions_t;

//-------------------------------------------------------------------

typedef struct
{
	char *string;
	int hash;
} animStringItem_t;

typedef struct
{
	int index;           // reference into the table of possible conditionals
	int value[2];        // can store anything from weapon bits, to position enums, etc
	qboolean negative;   // (,)NOT <condition>
} animScriptCondition_t;

typedef struct
{
	short int bodyPart[2];      // play this animation on legs/torso/both
	short int animIndex[2];     // animation index in our list of animations
	short int animDuration[2];
	short int soundIndex;
} animScriptCommand_t;

typedef struct
{
	int numConditions;
	animScriptCondition_t conditions[NUM_ANIM_CONDITIONS];
	int numCommands;
	animScriptCommand_t commands[MAX_ANIMSCRIPT_ANIMCOMMANDS];
} animScriptItem_t;

typedef struct
{
	int numItems;
	animScriptItem_t *items[MAX_ANIMSCRIPT_ITEMS];      // pointers into a global list of items
} animScript_t;

typedef struct
{
	char animationGroup[MAX_QPATH];
	char animationScript[MAX_QPATH];

	// parsed from the start of the cfg file (this is basically obsolete now - need to get rid of it)
	gender_t gender;
	footstep_t footsteps;
	vec3_t headOffset;
	int version;
	qboolean isSkeletal;

	// parsed from animgroup file
	animation_t *animations[MAX_MODEL_ANIMATIONS];              // anim names, frame ranges, etc
	headAnimation_t headAnims[MAX_HEAD_ANIMS];
	int numAnimations, numHeadAnims;

	// parsed from script file
	animScript_t scriptAnims[MAX_AISTATES][NUM_ANIM_MOVETYPES];                 // locomotive anims, etc
	animScript_t scriptCannedAnims[NUM_ANIM_MOVETYPES];                         // played randomly
	animScript_t scriptEvents[NUM_ANIM_EVENTTYPES];                             // events that trigger special anims

	// global list of script items for this model
	animScriptItem_t scriptItems[MAX_ANIMSCRIPT_ITEMS_PER_MODEL];
	int numScriptItems;

} animModelInfo_t;

// this is the main structure that is duplicated on the client and server
typedef struct
{
	animModelInfo_t modelInfo[MAX_ANIMSCRIPT_MODELS];
	int clientConditions[MAX_CLIENTS][NUM_ANIM_CONDITIONS][2];

	// pointers to functions from the owning module
	// constify the arg
	int (*soundIndex)(const char *name);
	void (*playSound)(int soundIndex, vec3_t org, int clientNum);
} animScriptData_t;

//------------------------------------------------------------------
// Conditional Constants

typedef enum
{
	POSITION_UNUSED = 0,
	POSITION_BEHIND,
	POSITION_INFRONT,
	POSITION_RIGHT,
	POSITION_LEFT,

	NUM_ANIM_COND_POSITIONS
} animScriptPosition_t;

typedef enum
{
	MOUNTED_UNUSED = 0,
	MOUNTED_MG42,
	MOUNTED_AAGUN,

	NUM_ANIM_COND_MOUNTED
} animScriptMounted_t;

typedef enum
{
	LEANING_UNUSED = 0,
	LEANING_RIGHT,
	LEANING_LEFT,

	NUM_ANIM_COND_LEANING
} animScriptLeaning_t;

typedef enum
{
	IMPACTPOINT_UNUSED = 0,
	IMPACTPOINT_HEAD,
	IMPACTPOINT_CHEST,
	IMPACTPOINT_GUT,
	IMPACTPOINT_GROIN,
	IMPACTPOINT_SHOULDER_RIGHT,
	IMPACTPOINT_SHOULDER_LEFT,
	IMPACTPOINT_KNEE_RIGHT,
	IMPACTPOINT_KNEE_LEFT,

	NUM_ANIM_COND_IMPACTPOINT
} animScriptImpactPoint_t;

typedef enum
{
	FLAILING_UNUSED = 0,
	FLAILING_INAIR,
	FLAILING_VCRASH,
	FLAILING_HCRASH,

	NUM_ANIM_COND_FLAILING
} animScriptFlailingType_t;

typedef enum
{
	//ANIM_BITFLAG_SNEAKING,
	//ANIM_BITFLAG_AFTERBATTLE,
	ANIM_BITFLAG_ZOOMING = 0,

	NUM_ANIM_COND_BITFLAG
} animScriptGenBitFlag_t;

typedef enum
{
	ACC_BELT_LEFT = 0, // belt left (lower)
	ACC_BELT_RIGHT,    // belt right (lower)
	ACC_BELT,          // belt (upper)
	ACC_BACK,          // back (upper)
	ACC_WEAPON,        // weapon (upper)
	ACC_WEAPON2,       // weapon2 (upper)
	ACC_HAT,           // hat (head)
	ACC_MOUTH2,        //
	ACC_MOUTH3,        //
	ACC_RANK,          //
	ACC_MAX            // this is bound by network limits, must change network stream to increase this
} accType_t;

#define MAX_GIB_MODELS      16

#define MAX_WEAPS_PER_CLASS 10

typedef struct
{
	int classNum;
	const char *characterFile;
	const char *iconName;
	const char *iconArrow;

	weapon_t classWeapons[MAX_WEAPS_PER_CLASS];

	qhandle_t icon;
	qhandle_t arrow;

} bg_playerclass_t;

typedef struct bg_character_s
{
	char characterFile[MAX_QPATH];

#ifdef USE_MDXFILE
	qhandle_t mesh;
	qhandle_t skin;

	qhandle_t headModel;
	qhandle_t headSkin;

	qhandle_t accModels[ACC_MAX];
	qhandle_t accSkins[ACC_MAX];

	qhandle_t gibModels[MAX_GIB_MODELS];

	qhandle_t undressedCorpseModel;
	qhandle_t undressedCorpseSkin;

	qhandle_t hudhead;
	qhandle_t hudheadskin;
	animation_t hudheadanimations[MAX_HD_ANIMATIONS];
#endif // CGAMEDLL

	animModelInfo_t *animModelInfo;
} bg_character_t;

//------------------------------------------------------------------
// Global Function Decs

void BG_InitWeaponStrings(void);
void BG_AnimParseAnimScript(animModelInfo_t *modelInfo, animScriptData_t *scriptData, const char *filename, char *input);
int BG_AnimScriptAnimation(playerState_t *ps, animModelInfo_t *modelInfo, scriptAnimMoveTypes_t movetype, qboolean isContinue);
int BG_AnimScriptCannedAnimation(playerState_t *ps, animModelInfo_t *modelInfo);
int BG_AnimScriptEvent(playerState_t *ps, animModelInfo_t *modelInfo, scriptAnimEventTypes_t event, qboolean isContinue, qboolean force);
int BG_IndexForString(char *token, animStringItem_t *strings, qboolean allowFail);
int BG_PlayAnimName(playerState_t *ps, animModelInfo_t *animModelInfo, char *animName, animBodyPart_t bodyPart, qboolean setTimer, qboolean isContinue, qboolean force);
void BG_ClearAnimTimer(playerState_t *ps, animBodyPart_t bodyPart);

char *BG_GetAnimString(animModelInfo_t *animModelInfo, int anim);
void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion);
int BG_GetConditionValue(int client, int condition, qboolean checkConversion);
qboolean BG_GetConditionBitFlag(int client, int condition, int bitNumber);
void BG_SetConditionBitFlag(int client, int condition, int bitNumber);
void BG_ClearConditionBitFlag(int client, int condition, int bitNumber);
int BG_GetAnimScriptAnimation(int client, animModelInfo_t *animModelInfo, aistateEnum_t aistate, scriptAnimMoveTypes_t movetype);
void BG_AnimUpdatePlayerStateConditions(pmove_t *pmove);
animation_t *BG_AnimationForString(char *string, animModelInfo_t *animModelInfo);
animation_t *BG_GetAnimationForIndex(animModelInfo_t *animModelInfo, int index);
int BG_GetAnimScriptEvent(playerState_t *ps, scriptAnimEventTypes_t event);
int PM_IdleAnimForWeapon(int weapon);
int PM_RaiseAnimForWeapon(int weapon);
void PM_ContinueWeaponAnim(int anim);

extern animStringItem_t animStateStr[];
extern animStringItem_t animBodyPartsStr[];

bg_playerclass_t *BG_GetPlayerClassInfo(int team, int cls);
bg_playerclass_t *BG_PlayerClassForPlayerState(playerState_t *ps);
qboolean BG_ClassHasWeapon(bg_playerclass_t *classInfo, weapon_t weap);
qboolean BG_WeaponIsPrimaryForClassAndTeam(int classnum, team_t team, weapon_t weapon);
int BG_ClassWeaponCount(bg_playerclass_t *classInfo, team_t team);
const char *BG_ShortClassnameForNumber(int classNum);
const char *BG_ClassnameForNumber(int classNum);
const char *BG_ClassLetterForNumber(int classNum);

extern bg_playerclass_t bg_allies_playerclasses[NUM_PLAYER_CLASSES];
extern bg_playerclass_t bg_axis_playerclasses[NUM_PLAYER_CLASSES];

#define MAX_PATH_CORNERS        512

typedef struct
{
	char name[64];
	vec3_t origin;
} pathCorner_t;

extern int          numPathCorners;
extern pathCorner_t pathCorners[MAX_PATH_CORNERS];

#define NUM_EXPERIENCE_LEVELS 11

typedef enum
{
	ME_PLAYER = 0,
	ME_PLAYER_REVIVE,
	ME_PLAYER_DISGUISED,
	ME_PLAYER_OBJECTIVE,
	ME_CONSTRUCT,
	ME_DESTRUCT,
	ME_DESTRUCT_2,
	ME_LANDMINE,
	ME_TANK,
	ME_TANK_DEAD,
	ME_COMMANDMAP_MARKER,
} mapEntityType_t;

extern const char *rankNames_Axis[NUM_EXPERIENCE_LEVELS];
extern const char *rankNames_Allies[NUM_EXPERIENCE_LEVELS];
extern const char *miniRankNames_Axis[NUM_EXPERIENCE_LEVELS];
extern const char *miniRankNames_Allies[NUM_EXPERIENCE_LEVELS];
extern const char *rankSoundNames_Axis[NUM_EXPERIENCE_LEVELS];
extern const char *rankSoundNames_Allies[NUM_EXPERIENCE_LEVELS];

#define MAX_SPLINE_PATHS        512
#define MAX_SPLINE_CONTROLS     4
#define MAX_SPLINE_SEGMENTS     16

typedef struct splinePath_s splinePath_t;

typedef struct
{
	vec3_t start;
	vec3_t v_norm;
	float length;
} splineSegment_t;

struct splinePath_s
{
	pathCorner_t point;

	char strTarget[64];

	splinePath_t *next;
	splinePath_t *prev;

	pathCorner_t controls[MAX_SPLINE_CONTROLS];
	int numControls;
	splineSegment_t segments[MAX_SPLINE_SEGMENTS];

	float length;

	qboolean isStart;
	qboolean isEnd;
};

extern int          numSplinePaths;
extern splinePath_t splinePaths[MAX_SPLINE_PATHS];

pathCorner_t *BG_Find_PathCorner(const char *match);
splinePath_t *BG_GetSplineData(int number, qboolean *backwards);
void BG_AddPathCorner(const char *name, vec3_t origin);
splinePath_t *BG_AddSplinePath(const char *name, const char *target, vec3_t origin);
void BG_BuildSplinePaths(void);
splinePath_t *BG_Find_Spline(const char *match);
float BG_SplineLength(splinePath_t *pSpline);
void BG_AddSplineControl(splinePath_t *spline, const char *name);
void BG_LinearPathOrigin2(float radius, splinePath_t **pSpline, float *deltaTime, vec3_t result, qboolean backwards);

int BG_MaxAmmoForWeapon(weapon_t weaponNum, int *skill);

void BG_InitLocations(vec2_t world_mins, vec2_t world_maxs);
char *BG_GetLocationString(vec_t *pos);

extern const char *bg_fireteamNames[MAX_FIRETEAMS / 2];

typedef struct
{
	int ident;
	char joinOrder[MAX_CLIENTS];        // order in which clients joined the fire team (server), client uses to store if a client is on this fireteam
	int leader;         // leader = joinOrder[0] on server, stored here on client
	qboolean inuse;
	qboolean priv;
} fireteamData_t;

long BG_StringHashValue(const char *fname);
long BG_StringHashValue_Lwr(const char *fname);

int trap_PC_AddGlobalDefine(char *define);
int trap_PC_LoadSource(const char *filename);
int trap_PC_FreeSource(int handle);
int trap_PC_ReadToken(int handle, pc_token_t *pc_token);
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line);
int trap_PC_UnReadToken(int handle);

void PC_SourceError(int handle, char *format, ...);
void PC_SourceWarning(int handle, char *format, ...);

#ifdef GAMEDLL
const char *PC_String_Parse(int handle);
const char *PC_Line_Parse(int handle);
#else
const char *String_Alloc(const char *p);
qboolean PC_String_Parse(int handle, const char **out);
#endif
qboolean PC_String_ParseNoAlloc(int handle, char *out, size_t size);
qboolean PC_Int_Parse(int handle, int *i);
qboolean PC_Color_Parse(int handle, vec4_t *c);
qboolean PC_Vec_Parse(int handle, vec3_t *c);
qboolean PC_Float_Parse(int handle, float *f);

typedef enum
{
	UIMENU_NONE = 0,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_NEED_CD,     // Obsolete
	UIMENU_BAD_CD_KEY,  // Obsolete
	UIMENU_TEAM,
	UIMENU_POSTGAME,
	UIMENU_HELP,

	UIMENU_WM_QUICKMESSAGE,
	UIMENU_WM_QUICKMESSAGEALT,

	UIMENU_WM_FTQUICKMESSAGE,
	UIMENU_WM_FTQUICKMESSAGEALT,

	UIMENU_WM_TAPOUT,
	UIMENU_WM_TAPOUT_LMS,

	UIMENU_WM_AUTOUPDATE,

	// say, team say, etc
	UIMENU_INGAME_MESSAGEMODE,
} uiMenuCommand_t;

void BG_AdjustAAGunMuzzleForBarrel(vec_t *origin, vec_t *forward, vec_t *right, vec_t *up, int barrel);

int BG_ClassTextToClass(char *token);
skillType_t BG_ClassSkillForClass(int classnum);

qboolean BG_isLightWeaponSupportingFastReload(int weapon);

int BG_FootstepForSurface(int surfaceFlags);

#define MATCH_MINPLAYERS "4" //"1"	// Minimum # of players needed to start a match

#ifdef FEATURE_MULTIVIEW
// Multiview support
int BG_simpleHintsCollapse(int hint, int val);
int BG_simpleHintsExpand(int hint, int val);
#endif
int BG_simpleWeaponState(int ws);

// Crosshair support
void BG_setCrosshair(char *colString, float *col, float alpha, char *cvarName);

// Voting
#define VOTING_DISABLED     ((1 << numVotesAvailable) - 1)

typedef struct
{
	const char *pszCvar;
	int flag;
} voteType_t;

extern const voteType_t voteToggles[];
extern int              numVotesAvailable;

// Tracemap
#ifdef CGAMEDLL
void CG_GenerateTracemap(void);
#endif // CGAMEDLL
qboolean BG_LoadTraceMap(char *rawmapname, vec2_t world_mins, vec2_t world_maxs);
float BG_GetSkyHeightAtPoint(vec3_t pos);
float BG_GetSkyGroundHeightAtPoint(vec3_t pos);
float BG_GetGroundHeightAtPoint(vec3_t pos);
int BG_GetTracemapGroundFloor(void);
int BG_GetTracemapGroundCeil(void);

// bg_animgroup.c

void BG_ClearAnimationPool(void);
qboolean BG_R_RegisterAnimationGroup(const char *filename, animModelInfo_t *animModelInfo);

// bg_character.c

typedef struct bg_characterDef_s
{
	char mesh[MAX_QPATH];
	char animationGroup[MAX_QPATH];
	char animationScript[MAX_QPATH];
	char skin[MAX_QPATH];
	char undressedCorpseModel[MAX_QPATH];
	char undressedCorpseSkin[MAX_QPATH];
	char hudhead[MAX_QPATH];
	char hudheadanims[MAX_QPATH];
	char hudheadskin[MAX_QPATH];
} bg_characterDef_t;

qboolean BG_ParseCharacterFile(const char *filename, bg_characterDef_t *characterDef);
bg_character_t *BG_GetCharacter(int team, int cls);
bg_character_t *BG_GetCharacterForPlayerstate(playerState_t *ps);
void BG_ClearCharacterPool(void);
bg_character_t *BG_FindFreeCharacter(const char *characterFile);
bg_character_t *BG_FindCharacter(const char *characterFile);

// bg_sscript.c

typedef enum
{
	S_LT_NOT_LOOPED = 0,
	S_LT_LOOPED_ON,
	S_LT_LOOPED_OFF
} speakerLoopType_t;

typedef enum
{
	S_BT_LOCAL = 0,
	S_BT_GLOBAL,
	S_BT_NOPVS
} speakerBroadcastType_t;

typedef struct bg_speaker_s
{
	char filename[MAX_QPATH];
	qhandle_t noise;
	vec3_t origin;
	char targetname[32];
	long targetnamehash;

	speakerLoopType_t loop;
	speakerBroadcastType_t broadcast;
	int wait;
	int random;
	int volume;
	int range;

	qboolean activated;
	int nextActivateTime;
	int soundTime;
} bg_speaker_t;

void BG_ClearScriptSpeakerPool(void);
int BG_NumScriptSpeakers(void);
int BG_GetIndexForSpeaker(bg_speaker_t *speaker);
bg_speaker_t *BG_GetScriptSpeaker(int index);
qboolean BG_SS_DeleteSpeaker(int index);
qboolean BG_SS_StoreSpeaker(bg_speaker_t *speaker);
qboolean BG_LoadSpeakerScript(const char *filename);

// Lookup table to find ammo table entry
extern ammotable_t ammoTableMP[WP_NUM_WEAPONS];
#define GetAmmoTableData(ammoIndex) ((ammotable_t *)(&ammoTableMP[ammoIndex]))

// Lookup table to find weapon table entry
extern weaponTable_t weaponTable[WP_NUM_WEAPONS];
#define GetWeaponTableData(weaponIndex) ((weaponTable_t *)(&weaponTable[weaponIndex]))

// Lookup table to find mod properties
extern modTable_t modTable[MOD_NUM_MODS];

#define MAX_MAP_SIZE 65536

qboolean BG_BBoxCollision(vec3_t min1, vec3_t max1, vec3_t min2, vec3_t max2);

//#define VISIBLE_TRIGGERS

// bg_stats.c

typedef struct weap_ws_convert_s
{
	weapon_t iWeapon;
	extWeaponStats_t iWS;
} weap_ws_convert_t;

extWeaponStats_t BG_WeapStatForWeapon(weapon_t iWeaponID);

typedef enum popupMessageType_e
{
	PM_DYNAMITE = 0,
	PM_CONSTRUCTION,
	PM_MINES,
	PM_DEATH,
	PM_MESSAGE,
	PM_OBJECTIVE,
	PM_DESTRUCTION,
	PM_TEAM,
	PM_AMMOPICKUP,
	PM_HEALTHPICKUP,
	PM_NUM_TYPES
} popupMessageType_t;

typedef enum popupMessageBigType_e
{
	PM_SKILL = 0,
	PM_RANK,
	PM_DISGUISE,
	PM_BIG_NUM_TYPES
} popupMessageBigType_t;

int PM_AltSwitchFromForWeapon(int weapon);
int PM_AltSwitchToForWeapon(int weapon);

void PM_TraceLegs(trace_t * trace, float *legsOffset, vec3_t start, vec3_t end, trace_t * bodytrace, vec3_t viewangles, void (tracefunc)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int ignoreent, int tracemask);
void PM_TraceHead(trace_t * trace, vec3_t start, vec3_t end, trace_t * bodytrace, vec3_t viewangles, void (tracefunc)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int ignoreent, int tracemask);
void PM_TraceAllParts(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end);
void PM_TraceAll(trace_t *trace, vec3_t start, vec3_t end);

// Store all sounds used in server engine and send them to client in events only as Enums
typedef enum
{
	GAMESOUND_BLANK = 0,
	GAMESOUND_PLAYER_GURP1,         // "sound/player/gurp1.wav"                         Player takes damage from drowning
	GAMESOUND_PLAYER_GURP2,         // "sound/player/gurp2.wav"
	GAMESOUND_PLAYER_BUBBLE,
	GAMESOUND_WPN_AIRSTRIKE_PLANE,  // "sound/weapons/airstrike/airstrike_plane.wav"    Used by Airstrike marker after it triggers
	GAMESOUND_WPN_ARTILLERY_FLY_1,  // "sound/weapons/artillery/artillery_fly_1.wav"    Used by Artillery before impact
	GAMESOUND_WPN_ARTILLERY_FLY_2,  // "sound/weapons/artillery/artillery_fly_2.wav"
	GAMESOUND_WPN_ARTILLERY_FLY_3,  // "sound/weapons/artillery/artillery_fly_3.wav"

	GAMESOUND_MISC_REVIVE,          // "sound/misc/vo_revive.wav"                       Used by revival Needle
	GAMESOUND_MISC_REFEREE,         // "sound/misc/referee.wav"                         Game Referee performs action
	GAMESOUND_MISC_VOTE,            // "sound/misc/vote.wav"                            Vote is issued

	//GAMESOUND_MISC_BANNED,        // "sound/osp/banned.wav"                           Player is banned
	//GAMESOUND_MISC_KICKED,        // "sound/osp/kicked.wav"                           Player is kicked

	GAMESOUND_WORLD_BUILD,          // "sound/world/build.wav"
	GAMESOUND_WORLD_CHAIRCREAK,     // "sound/world/chaircreak.wav"                     Common code
	GAMESOUND_WORLD_MG_CONSTRUCTED, // "sound/world/mg_constructed.wav"

	GAMESOUND_MAX
} gameSounds;

// defines for viewlocking (mg/medics etc)
#define VIEWLOCK_NONE               0   // disabled, let them look around
#define VIEWLOCK_JITTER             2   // this enable screen jitter when firing
#define VIEWLOCK_MG42               3   // tell the client to lock the view in the direction of the gun
#define VIEWLOCK_MEDIC              7   // look at the nearest medic

// cursor hint & trace distances
#define CH_NONE_DIST        0
#define CH_LADDER_DIST      100
#define CH_WATER_DIST       100
#define CH_BREAKABLE_DIST   64
#define CH_DOOR_DIST        96
#define CH_ACTIVATE_DIST    96
#define CH_REVIVE_DIST      64
#define CH_KNIFE_DIST       48
#define CH_DIST             100 //128       // use the largest value from above
#define CH_MAX_DIST         1024    // use the largest value from above
#define CH_MAX_DIST_ZOOM    8192    // max dist for zooming hints

// FLAME & FLAMER constants
#define FLAMETHROWER_RANGE  2500    // multiplayer range, was 850 in SP

// these define how the flame looks and flamer acts
#define FLAME_START_SIZE        1.0     // bg
#define FLAME_START_MAX_SIZE    140.0   // bg
#define FLAME_MAX_SIZE          200.0   // cg flame sprites cannot be larger than this
#define FLAME_START_SPEED       1200.0  // cg speed of flame as it leaves the nozzle
#define FLAME_MIN_SPEED         60.0    // bg 200.0
#define FLAME_CHUNK_DIST        8.0     // cg space in between chunks when fired

#define FLAME_BLUE_LENGTH       130.0   // cg
#define FLAME_BLUE_MAX_ALPHA    1.0     // cg

// these are calculated (don't change)
#define FLAME_LENGTH            (FLAMETHROWER_RANGE + 50.0)   // NOTE: only modify the range, since this should always reflect that range

#define FLAME_LIFETIME          (int)((FLAME_LENGTH / FLAME_START_SPEED) * 1000)        // life duration in milliseconds
#define FLAME_FRICTION_PER_SEC  (2.0 * FLAME_START_SPEED) // bg
#define FLAME_BLUE_LIFE         (int)((FLAME_BLUE_LENGTH / FLAME_START_SPEED) * 1000) // cg

#define FLAME_BLUE_FADEIN_TIME(x)       (0.2 * x)  // cg
#define FLAME_BLUE_FADEOUT_TIME(x)      (0.05 * x) // cg
#define GET_FLAME_BLUE_SIZE_SPEED(x)    (((float)x / FLAME_LIFETIME) / 1.0)       // cg x is the current sizeMax
#define GET_FLAME_SIZE_SPEED(x)         (((float)x / FLAME_LIFETIME) / 0.3)       // cg x is the current sizeMax

#endif // #ifndef INCLUDE_BG_PUBLIC_H
