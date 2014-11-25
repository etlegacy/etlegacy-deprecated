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
 * @file g_antilag.c
 */

#include "g_local.h"

void G_StoreClientPosition(gentity_t *ent)
{
	int top;

	if (!(ent->inuse &&
	      (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES) &&
	      ent->r.linked &&
	      (ent->health > 0) &&
	      !(ent->client->ps.pm_flags & PMF_LIMBO) &&
	      (ent->client->ps.pm_type == PM_NORMAL)
	      ))
	{
		return;
	}

	ent->client->topMarker++;
	if (ent->client->topMarker >= MAX_CLIENT_MARKERS)
	{
		ent->client->topMarker = 0;
	}

	top = ent->client->topMarker;

	VectorCopy(ent->r.mins, ent->client->clientMarkers[top].mins);
	VectorCopy(ent->r.maxs, ent->client->clientMarkers[top].maxs);
	VectorCopy(ent->s.pos.trBase, ent->client->clientMarkers[top].origin);
	ent->client->clientMarkers[top].time = level.time;
}

static void G_AdjustSingleClientPosition(gentity_t *ent, int time)
{
	int i, j;

	if (time > level.time)
	{
		time = level.time;
	} // no lerping forward....

	// find a pair of markers which bound the requested time
	i = j = ent->client->topMarker;
	do
	{
		if (ent->client->clientMarkers[i].time <= time)
		{
			break;
		}

		j = i;
		i--;
		if (i < 0)
		{
			i = MAX_CLIENT_MARKERS - 1;
		}
	}
	while (i != ent->client->topMarker);

	if (i == j)     // oops, no valid stored markers
	{
		return;
	}

	// save current position to backup
	if (ent->client->backupMarker.time != level.time)
	{
		VectorCopy(ent->r.currentOrigin, ent->client->backupMarker.origin);
		VectorCopy(ent->r.mins, ent->client->backupMarker.mins);
		VectorCopy(ent->r.maxs, ent->client->backupMarker.maxs);
		ent->client->backupMarker.time = level.time;
	}

	if (i != ent->client->topMarker)
	{
		float frac = (float)(time - ent->client->clientMarkers[i].time) /
		             (float)(ent->client->clientMarkers[j].time - ent->client->clientMarkers[i].time);

		LerpPosition(ent->client->clientMarkers[i].origin, ent->client->clientMarkers[j].origin, frac, ent->r.currentOrigin);
		LerpPosition(ent->client->clientMarkers[i].mins, ent->client->clientMarkers[j].mins, frac, ent->r.mins);
		LerpPosition(ent->client->clientMarkers[i].maxs, ent->client->clientMarkers[j].maxs, frac, ent->r.maxs);
	}
	else
	{
		VectorCopy(ent->client->clientMarkers[j].origin, ent->r.currentOrigin);
		VectorCopy(ent->client->clientMarkers[j].mins, ent->r.mins);
		VectorCopy(ent->client->clientMarkers[j].maxs, ent->r.maxs);
	}

	trap_LinkEntity(ent);
}

static void G_ReAdjustSingleClientPosition(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	// restore from backup
	if (ent->client->backupMarker.time == level.time)
	{
		VectorCopy(ent->client->backupMarker.origin, ent->r.currentOrigin);
		VectorCopy(ent->client->backupMarker.mins, ent->r.mins);
		VectorCopy(ent->client->backupMarker.maxs, ent->r.maxs);
		ent->client->backupMarker.time = 0;

		trap_LinkEntity(ent);
	}
}

void G_AdjustClientPositions(gentity_t *ent, int time, qboolean forward)
{
	int       i;
	gentity_t *list;

	for (i = 0; i < level.numConnectedClients; i++, list++)
	{
		list = g_entities + level.sortedClients[i];
		// ok lets test everything under the sun
		if (list->client &&
		    list->inuse &&
		    (list->client->sess.sessionTeam == TEAM_AXIS || list->client->sess.sessionTeam == TEAM_ALLIES) &&
		    (list != ent) &&
		    list->r.linked &&
		    (list->health > 0) &&
		    !(list->client->ps.pm_flags & PMF_LIMBO) &&
		    (list->client->ps.pm_type == PM_NORMAL)
		    )
		{
			if (forward)
			{
				G_AdjustSingleClientPosition(list, time);
			}
			else
			{
				G_ReAdjustSingleClientPosition(list);
			}
		}
	}
}

void G_ResetMarkers(gentity_t *ent)
{
	int   i, time;
	char  buffer[MAX_CVAR_VALUE_STRING];
	float period;

	trap_Cvar_VariableStringBuffer("sv_fps", buffer, sizeof(buffer) - 1);

	period = atoi(buffer);
	if (!period)
	{
		period = 50;
	}
	else
	{
		period = 1000.f / period;
	}

	ent->client->topMarker = MAX_CLIENT_MARKERS - 1;
	for (i = MAX_CLIENT_MARKERS - 1, time = level.time; i >= 0; i--, time -= period)
	{
		VectorCopy(ent->r.mins, ent->client->clientMarkers[i].mins);
		VectorCopy(ent->r.maxs, ent->client->clientMarkers[i].maxs);
		VectorCopy(ent->r.currentOrigin, ent->client->clientMarkers[i].origin);
		ent->client->clientMarkers[i].time = time;
	}
}

void G_AttachBodyParts(gentity_t *ent)
{
	int       i;
	gentity_t *list;

	for (i = 0; i < level.numConnectedClients; i++, list++)
	{
		list = g_entities + level.sortedClients[i];
		// ok lets test everything under the sun
		if (list->inuse &&
		    (list->client->sess.sessionTeam == TEAM_AXIS || list->client->sess.sessionTeam == TEAM_ALLIES) &&
		    (list != ent) &&
		    list->r.linked &&
		    (list->health > 0) &&
		    !(list->client->ps.pm_flags & PMF_LIMBO) &&
		    (list->client->ps.pm_type == PM_NORMAL)
		    )
		{
			list->client->tempHead = G_BuildHead(list);
			list->client->tempLeg  = G_BuildLeg(list);
		}
		else
		{
			list->client->tempHead = NULL;
			list->client->tempLeg  = NULL;
		}
	}
}

void G_DettachBodyParts(void)
{
	int       i;
	gentity_t *list;

	for (i = 0; i < level.numConnectedClients; i++, list++)
	{
		list = g_entities + level.sortedClients[i];
		if (list->client->tempHead)
		{
			G_FreeEntity(list->client->tempHead);
		}
		if (list->client->tempLeg)
		{
			G_FreeEntity(list->client->tempLeg);
		}
	}
}

int G_SwitchBodyPartEntity(gentity_t *ent)
{
	if (ent->s.eType == ET_TEMPHEAD)
	{
		return ent->parent - g_entities;
	}
	if (ent->s.eType == ET_TEMPLEGS)
	{
		return ent->parent - g_entities;
	}

	return ent - g_entities;
}

#define POSITION_READJUST                       \
	if (res != results->entityNum) {               \
		VectorSubtract(end, start, dir);          \
		VectorNormalizeFast(dir);             \
                                    \
		VectorMA(results->endpos, -1, dir, results->endpos);  \
		results->entityNum = res;               \
	}

// Run a trace with players in historical positions.
void G_HistoricalTrace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	float  maxsBackup[MAX_CLIENTS];
	vec3_t dir;
	int    res, clientNum, i;

	memset(&maxsBackup, 0, sizeof(maxsBackup));

	if (!g_antilag.integer || !ent->client)
	{
		G_AttachBodyParts(ent);

		trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);

		res = G_SwitchBodyPartEntity(&g_entities[results->entityNum]);
		POSITION_READJUST

		G_DettachBodyParts();
		return;
	}

	G_AdjustClientPositions(ent, ent->client->pers.cmd.serverTime, qtrue);

	G_AttachBodyParts(ent) ;

	for (i = 0; i < level.numConnectedClients; ++i)
	{
		clientNum = level.sortedClients[i];
		if (&g_entities[clientNum] && g_entities[clientNum].client && g_entities[clientNum].takedamage)
		{
			maxsBackup[clientNum]           = g_entities[clientNum].r.maxs[2];
			g_entities[clientNum].r.maxs[2] = ClientHitboxMaxZ(&g_entities[clientNum]);
		}
	}

	trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);

	for (i = 0; i < level.numConnectedClients; ++i)
	{
		clientNum = level.sortedClients[i];
		if (&g_entities[clientNum] && g_entities[clientNum].client && g_entities[clientNum].takedamage)
		{
			g_entities[clientNum].r.maxs[2] = maxsBackup[clientNum];
		}
	}

	res = G_SwitchBodyPartEntity(&g_entities[results->entityNum]);
	POSITION_READJUST

	G_DettachBodyParts();

	G_AdjustClientPositions(ent, 0, qfalse);
}

void G_HistoricalTraceBegin(gentity_t *ent)
{
	// don't do this with antilag off
	if (!g_antilag.integer)
	{
		return;
	}

	G_AdjustClientPositions(ent, ent->client->pers.cmd.serverTime, qtrue);
}

void G_HistoricalTraceEnd(gentity_t *ent)
{
	// don't do this with antilag off
	if (!g_antilag.integer)
	{
		return;
	}

	G_AdjustClientPositions(ent, 0, qfalse);
}

/**
 * @brief Run a trace without fixups (historical fixups will be done externally)
 * @param ignoreCorpses Skip corpses for bullet tracing (=non gibbing weapons)
 */
void G_Trace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, qboolean ignoreCorpses)
{
	float  maxsBackup[MAX_CLIENTS];
	vec3_t dir;
	int    res, clientNum, i;

	G_AttachBodyParts(ent);

	// ignore bodies for bullet tracing
	if (ignoreCorpses)
	{
		if (g_corpses.integer == 0)
		{
			for (i = 0; i < BODY_QUEUE_SIZE; i++)
			{
				G_TempTraceIgnoreEntity(level.bodyQue[i]);
			}
		}
		else
		{
			for (i = 0; i < level.num_entities; i++)  // // slower way, improve by time
			{
				if (g_entities[i].s.eType == ET_CORPSE)
				{
					G_TempTraceIgnoreEntity(&g_entities[i]);
				}
			}
		}
	}

	for (i = 0; i < level.numConnectedClients; ++i)
	{
		clientNum = level.sortedClients[i];
		if (&g_entities[clientNum] && g_entities[clientNum].client && g_entities[clientNum].takedamage)
		{
			maxsBackup[clientNum]           = g_entities[clientNum].r.maxs[2];
			g_entities[clientNum].r.maxs[2] = ClientHitboxMaxZ(&g_entities[clientNum]);
		}
	}

	trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);

	for (i = 0; i < level.numConnectedClients; ++i)
	{
		clientNum = level.sortedClients[i];
		if (&g_entities[clientNum] && g_entities[clientNum].client && g_entities[clientNum].takedamage)
		{
			g_entities[clientNum].r.maxs[2] = maxsBackup[clientNum];
		}
	}

	res = G_SwitchBodyPartEntity(&g_entities[results->entityNum]);
	POSITION_READJUST

	G_DettachBodyParts();
	// ok let the bodies be traced again
	if (ignoreCorpses)
	{
		G_ResetTempTraceIgnoreEnts();
	}
}
