/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2017 ET:Legacy team <mail@etlegacy.com>
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
 * @file g_weapon.c
 * @brief Perform the server side effects of a weapon firing
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

vec3_t forward, right, up;
vec3_t muzzleEffect;
vec3_t muzzleTrace;

// forward dec
void Bullet_Fire(gentity_t *ent, float spread, int damage, qboolean distance_falloff);
qboolean Bullet_Fire_Extended(gentity_t *source, gentity_t *attacker, vec3_t start, vec3_t end, int damage, qboolean distance_falloff);

/**
======================================================================
KNIFE
======================================================================
*/

/**
 * @brief Weapon_Knife
 * @param[in] ent
 * @param[in] modnum
 */
void Weapon_Knife(gentity_t *ent, meansOfDeath_t modnum)
{
	trace_t        tr;
	gentity_t      *traceEnt, *tent;
	int            damage;
	meansOfDeath_t mod = modnum;
	vec3_t         pforward, end;

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, ent->s.weapon, forward, right, up, muzzleTrace);
	VectorMA(muzzleTrace, CH_KNIFE_DIST, forward, end);
	G_HistoricalTrace(ent, &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT);

	// ignore hits on NOIMPACT surfaces or no contact
	if ((tr.surfaceFlags & SURF_NOIMPACT) || tr.fraction == 1.0f)
	{
		return;
	}

	if (tr.entityNum >= MAX_CLIENTS)       // world brush or non-player entity (no blood)
	{
		tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
	}
	else                                // other player
	{
		tent = G_TempEntity(tr.endpos, EV_MISSILE_HIT);
	}

	tent->s.otherEntityNum = tr.entityNum;
	tent->s.eventParm      = DirToByte(tr.plane.normal);
	tent->s.weapon         = ent->s.weapon;
	tent->s.clientNum      = ent->r.ownerNum;

	if (tr.entityNum == ENTITYNUM_WORLD)     // don't worry about doing any damage
	{
		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	if (!(traceEnt->takedamage))
	{
		return;
	}

	damage = GetWeaponTableData(ent->s.weapon)->damage;   // default knife damage for frontal attacks (10)

	// no damage
	if (!damage)
	{
		return;
	}

	// Covert ops deal double damage with a knife
	if (ent->client->sess.playerType == PC_COVERTOPS)
	{
		damage <<= 1; // damage *= 2;    // Watch it - you could hurt someone with that thing!

	}
	// Only do backstabs if the body is standing up (i.e. alive)
	if (traceEnt->client && traceEnt->health > 0)
	{
		vec3_t eforward;

		AngleVectors(ent->client->ps.viewangles, pforward, NULL, NULL);
		AngleVectors(traceEnt->client->ps.viewangles, eforward, NULL, NULL);

		if (DotProduct(eforward, pforward) > 0.6f)           // from behind(-ish)
		{
			damage = 100;   // enough to drop a 'normal' (100 health) human with one jab
			mod    = MOD_BACKSTAB;

			if (ent->client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 4)
			{
				damage = traceEnt->health;
			}
		}
	}

	G_Damage(traceEnt, ent, ent, vec3_origin, tr.endpos, (damage + rand() % 5), 0, mod);
}

/**
 * @brief Make it TR_LINEAR so it doesnt chew bandwidth...
 * @param[in,out] self
 */
void MagicSink(gentity_t *self)
{
	self->clipmask   = 0;
	self->r.contents = 0;

	self->nextthink = level.time + 4000;
	self->think     = G_FreeEntity;

	self->s.pos.trType = TR_LINEAR;
	self->s.pos.trTime = level.time;
	VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
	VectorSet(self->s.pos.trDelta, 0, 0, -5);
}

/**
 * @brief Class-specific in multiplayer
 * @param[in] ent
 */
void Weapon_Medic(gentity_t *ent)
{
	vec3_t velocity, offset, angles, tosspos, viewpos;

	VectorCopy(ent->client->ps.viewangles, angles);

	// clamp pitch
	if (angles[PITCH] < -30)
	{
		angles[PITCH] = -30;
	}
	else if (angles[PITCH] > 30)
	{
		angles[PITCH] = 30;
	}

	AngleVectors(angles, velocity, NULL, NULL);
	VectorScale(velocity, 64, offset);
	offset[2] += ent->client->ps.viewheight / 2;
	VectorScale(velocity, 75, velocity);
	velocity[2] += 50 + crandom() * 25;

	VectorCopy(muzzleEffect, tosspos);
	VectorMA(tosspos, 48, forward, tosspos);
	VectorCopy(ent->client->ps.origin, viewpos);

	Weapon_Medic_Ext(ent, viewpos, tosspos, velocity);
}

/**
 * @brief Weapon_Medic_Ext
 * @param[in,out] ent
 * @param[in,out] viewpos
 * @param[in,out] tosspos
 * @param[in] velocity
 */
void Weapon_Medic_Ext(gentity_t *ent, vec3_t viewpos, vec3_t tosspos, vec3_t velocity)
{
	gentity_t *ent2;
	vec3_t    mins, maxs;
	trace_t   tr;

	if (level.time - ent->client->ps.classWeaponTime > level.medicChargeTime[ent->client->sess.sessionTeam - 1])
	{
		ent->client->ps.classWeaponTime = level.time - level.medicChargeTime[ent->client->sess.sessionTeam - 1];
	}

	if (ent->client->sess.skill[SK_FIRST_AID] >= 2)
	{
		ent->client->ps.classWeaponTime += level.medicChargeTime[ent->client->sess.sessionTeam - 1] * 0.15;
	}
	else
	{
		ent->client->ps.classWeaponTime += level.medicChargeTime[ent->client->sess.sessionTeam - 1] * 0.25;
	}

	VectorSet(mins, -(ITEM_RADIUS + 8), -(ITEM_RADIUS + 8), 0);
	VectorSet(maxs, (ITEM_RADIUS + 8), (ITEM_RADIUS + 8), 2 * (ITEM_RADIUS + 8));

	trap_EngineerTrace(&tr, viewpos, mins, maxs, tosspos, ent->s.number, MASK_MISSILESHOT);
	if (tr.startsolid)
	{
		VectorCopy(forward, viewpos);
		VectorNormalizeFast(viewpos);
		VectorMA(ent->r.currentOrigin, -24.f, viewpos, viewpos);

		trap_EngineerTrace(&tr, viewpos, mins, maxs, tosspos, ent->s.number, MASK_MISSILESHOT);

		VectorCopy(tr.endpos, tosspos);
	}
	else if (tr.fraction < 1)       // oops, bad launch spot
	{
		VectorCopy(tr.endpos, tosspos);
		SnapVectorTowards(tosspos, viewpos);
	}

	ent2            = LaunchItem(BG_GetItem(ITEM_HEALTH), tosspos, velocity, ent->s.number);
	ent2->think     = MagicSink;
	ent2->nextthink = level.time + 30000;

	ent2->parent = ent; // so we can score properly later

#ifdef FEATURE_OMNIBOT
	// Omni-bot - Send a fire event.
	Bot_Event_FireWeapon(ent - g_entities, Bot_WeaponGameToBot(ent->s.weapon), ent2);
#endif
}

/**
 * @brief Weapon_MagicAmmo
 * @param[in] ent
 */
void Weapon_MagicAmmo(gentity_t *ent)
{
	vec3_t velocity, offset, tosspos, viewpos, angles;

	VectorCopy(ent->client->ps.viewangles, angles);

	// clamp pitch
	if (angles[PITCH] < -30)
	{
		angles[PITCH] = -30;
	}
	else if (angles[PITCH] > 30)
	{
		angles[PITCH] = 30;
	}

	AngleVectors(angles, velocity, NULL, NULL);
	VectorScale(velocity, 64, offset);
	offset[2] += ent->client->ps.viewheight / 2;
	VectorScale(velocity, 75, velocity);
	velocity[2] += 50 + crandom() * 25;

	VectorCopy(muzzleEffect, tosspos);
	VectorMA(tosspos, 48, forward, tosspos);
	VectorCopy(ent->client->ps.origin, viewpos);

	Weapon_MagicAmmo_Ext(ent, viewpos, tosspos, velocity);
}

/**
 * @brief Weapon_MagicAmmo_Ext
 * @param[in,out] ent
 * @param[in,out] viewpos
 * @param[in,out] tosspos
 * @param[in] velocity
 */
void Weapon_MagicAmmo_Ext(gentity_t *ent, vec3_t viewpos, vec3_t tosspos, vec3_t velocity)
{
	vec3_t    mins, maxs;
	trace_t   tr;
	gentity_t *ent2;

	if (level.time - ent->client->ps.classWeaponTime > level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1])
	{
		ent->client->ps.classWeaponTime = level.time - level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1];
	}

	if (ent->client->sess.skill[SK_SIGNALS] >= 1)
	{
		ent->client->ps.classWeaponTime += level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1] * 0.15;
	}
	else
	{
		ent->client->ps.classWeaponTime += level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1] * 0.25;
	}

	VectorSet(mins, -(ITEM_RADIUS + 8), -(ITEM_RADIUS + 8), 0);
	VectorSet(maxs, (ITEM_RADIUS + 8), (ITEM_RADIUS + 8), 2 * (ITEM_RADIUS + 8));

	trap_EngineerTrace(&tr, viewpos, mins, maxs, tosspos, ent->s.number, MASK_MISSILESHOT);
	if (tr.startsolid)
	{
		VectorCopy(forward, viewpos);
		VectorNormalizeFast(viewpos);
		VectorMA(ent->r.currentOrigin, -24.f, viewpos, viewpos);

		trap_EngineerTrace(&tr, viewpos, mins, maxs, tosspos, ent->s.number, MASK_MISSILESHOT);

		VectorCopy(tr.endpos, tosspos);
	}
	else if (tr.fraction < 1)       // oops, bad launch spot
	{
		VectorCopy(tr.endpos, tosspos);
		SnapVectorTowards(tosspos, viewpos);
	}

	ent2            = LaunchItem(BG_GetItem(ent->client->sess.skill[SK_SIGNALS] >= 1 ? ITEM_MEGA_AMMO_PACK : ITEM_AMMO_PACK), tosspos, velocity, ent->s.number);
	ent2->think     = MagicSink;
	ent2->nextthink = level.time + 30000;

	ent2->parent = ent;

	if (ent->client->sess.skill[SK_SIGNALS] >= 1)
	{
		ent2->count     = 2;
		ent2->s.density = 2;
	}
	else
	{
		ent2->count     = 1;
		ent2->s.density = 1;
	}

#ifdef FEATURE_OMNIBOT
	// Omni-bot - Send a fire event.
	Bot_Event_FireWeapon(ent - g_entities, Bot_WeaponGameToBot(ent->s.weapon), ent2);
#endif
}

/**
 * @brief Took this out of Weapon_Syringe so we can use it from other places
 * @param[in] ent
 * @param[in,out] traceEnt
 * @return
 */
qboolean ReviveEntity(gentity_t *ent, gentity_t *traceEnt)
{
	vec3_t   org;
	trace_t  tr;
	int      healamt, headshot, oldweapon, oldweaponstate, oldclasstime = 0;
	qboolean usedSyringe = qfalse;
	int      ammo[MAX_WEAPONS];         // total amount of ammo
	int      ammoclip[MAX_WEAPONS];     // ammo in clip
	int      weapons[MAX_WEAPONS / (sizeof(int) * 8)];  // 64 bits for weapons held

	// heal the dude
	// copy some stuff out that we'll wanna restore
	VectorCopy(traceEnt->client->ps.origin, org);

	headshot = traceEnt->client->ps.eFlags & EF_HEADSHOT;

	if (ent->client->sess.skill[SK_FIRST_AID] >= 3)
	{
		healamt = traceEnt->client->ps.stats[STAT_MAX_HEALTH];
	}
	else
	{
		healamt = (int)(traceEnt->client->ps.stats[STAT_MAX_HEALTH] * 0.5);
	}
	oldweapon      = traceEnt->client->ps.weapon;
	oldweaponstate = traceEnt->client->ps.weaponstate;

	// keep class special weapon time to keep them from exploiting revives
	oldclasstime = traceEnt->client->ps.classWeaponTime;

	memcpy(ammo, traceEnt->client->ps.ammo, sizeof(int) * MAX_WEAPONS);
	memcpy(ammoclip, traceEnt->client->ps.ammoclip, sizeof(int) * MAX_WEAPONS);
	memcpy(weapons, traceEnt->client->ps.weapons, sizeof(int) * (MAX_WEAPONS / (sizeof(int) * 8)));

	ClientSpawn(traceEnt, qtrue, qfalse, qtrue);

#ifdef FEATURE_OMNIBOT
	Bot_Event_Revived(traceEnt - g_entities, ent);
#endif

	traceEnt->client->ps.stats[STAT_PLAYER_CLASS] = traceEnt->client->sess.playerType;
	memcpy(traceEnt->client->ps.ammo, ammo, sizeof(int) * MAX_WEAPONS);
	memcpy(traceEnt->client->ps.ammoclip, ammoclip, sizeof(int) * MAX_WEAPONS);
	memcpy(traceEnt->client->ps.weapons, weapons, sizeof(int) * (MAX_WEAPONS / (sizeof(int) * 8)));

	if (headshot)
	{
		traceEnt->client->ps.eFlags |= EF_HEADSHOT;
	}
	traceEnt->client->ps.weapon      = oldweapon;
	traceEnt->client->ps.weaponstate = oldweaponstate;

	// set idle animation on weapon
	traceEnt->client->ps.weapAnim = ((traceEnt->client->ps.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | PM_IdleAnimForWeapon(traceEnt->client->ps.weapon);

	traceEnt->client->ps.classWeaponTime = oldclasstime;

	traceEnt->health = healamt;
	VectorCopy(org, traceEnt->s.origin);
	VectorCopy(org, traceEnt->r.currentOrigin);
	VectorCopy(org, traceEnt->client->ps.origin);

	trap_Trace(&tr, traceEnt->client->ps.origin, traceEnt->client->ps.mins, traceEnt->client->ps.maxs, traceEnt->client->ps.origin, traceEnt->s.number, MASK_PLAYERSOLID);
	if (tr.allsolid)
	{
		traceEnt->client->ps.pm_flags |= PMF_DUCKED;
	}

	traceEnt->r.contents = CONTENTS_CORPSE;
	trap_LinkEntity(ent);

	// Let the person being revived know about it
	trap_SendServerCommand(traceEnt - g_entities, va("cp \"You have been revived by [lof]%s[lon] [lof]%s^7!\"", ent->client->sess.sessionTeam == TEAM_ALLIES ? rankNames_Allies[ent->client->sess.rank] : rankNames_Axis[ent->client->sess.rank], ent->client->pers.netname));

	traceEnt->props_frame_state = ent->s.number;

	// Mark that the medicine was indeed dispensed
	usedSyringe = qtrue;

	// sound
	G_Sound(traceEnt, GAMESOUND_MISC_REVIVE);

	traceEnt->client->pers.lastrevive_client = ent->s.clientNum;
	traceEnt->client->pers.lasthealth_client = ent->s.clientNum;

	if (g_fastres.integer > 0)
	{
		BG_AnimScriptEvent(&traceEnt->client->ps, traceEnt->client->pers.character->animModelInfo, ANIM_ET_JUMP, qfalse, qtrue);
	}
	else
	{
		// Play revive animation
		BG_AnimScriptEvent(&traceEnt->client->ps, traceEnt->client->pers.character->animModelInfo, ANIM_ET_REVIVE, qfalse, qtrue);
		traceEnt->client->ps.pm_flags |= PMF_TIME_LOCKPLAYER;
		traceEnt->client->ps.pm_time   = 2100;
	}

	// Tell the caller if we actually used a syringe
	return usedSyringe;
}

/**
* @brief Shoot the syringe, do the old lazarus bit
*
* @param[in,out] ent
*
* @note Currently medic player can get out of syringe ammo when G_MISC_MEDIC_SYRINGE_HEAL is set
*/
void Weapon_Syringe(gentity_t *ent)
{
	vec3_t    end;
	trace_t   tr;
	gentity_t *traceEnt;

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePointForActivate(ent, forward, right, up, muzzleTrace);
	VectorMA(muzzleTrace, CH_REVIVE_DIST, forward, end);
	// right on top of intended revivee.
	G_HistoricalTrace(ent, &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT);

	if (tr.startsolid)
	{
		VectorMA(muzzleTrace, 8, forward, end);
		trap_Trace(&tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT);
	}

	if (tr.fraction == 1.0f) // no hit
	{
		// give back ammo
		ent->client->ps.ammoclip[BG_FindClipForWeapon(WP_MEDIC_SYRINGE)] += 1;
		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	if (!traceEnt->client)
	{
		// give back ammo
		ent->client->ps.ammoclip[BG_FindClipForWeapon(WP_MEDIC_SYRINGE)] += 1;
		return;
	}

	if (traceEnt->client->ps.pm_type == PM_DEAD)
	{
		qboolean usedSyringe;

		if (traceEnt->client->sess.sessionTeam != ent->client->sess.sessionTeam)
		{
			return;
		}

		// moved all the revive stuff into its own function
		usedSyringe = ReviveEntity(ent, traceEnt);

		// syringe "hit"
		// we no longer track the syringe - it's no real weapon and messes up the total weapon stats (see acc)
		// FIXME: add a new award 'most revives' instead?
		//if (g_gamestate.integer == GS_PLAYING)
		//{
		//ent->client->sess.aWeaponStats[WS_SYRINGE].hits++;
		//}

		G_LogPrintf("Medic_Revive: %d %d\n", (int)(ent - g_entities), (int)(traceEnt - g_entities));

		if (!traceEnt->isProp)     // flag for if they were teamkilled or not
		{
			G_AddSkillPoints(ent, SK_FIRST_AID, 4.f);
			G_DebugAddSkillPoints(ent, SK_FIRST_AID, 4.f, "reviving a player");
		}

		// calculate ranks to update numFinalDead arrays. Have to do it manually as addscore has an early out
		if (g_gametype.integer == GT_WOLF_LMS)
		{
			CalculateRanks();
		}

		// If the medicine wasn't used, give back the ammo
		if (!usedSyringe)
		{
			ent->client->ps.ammoclip[BG_FindClipForWeapon(WP_MEDIC_SYRINGE)] += 1;
		}
	}
	else if (g_misc.integer & G_MISC_MEDIC_SYRINGE_HEAL) //  FIXME: clarify ammo restore
	{
		if (traceEnt->client->sess.sessionTeam != ent->client->sess.sessionTeam)
		{
			// this doesn't heal enemy but ammo is gone :D FIXME?
			return;
		}

		if (traceEnt->health > (traceEnt->client->ps.stats[STAT_MAX_HEALTH] * 0.25f))
		{
			// this doesn't heal enemy but ammo is gone :D FIXME?
			return;
		}

		{
			int healamt;

			// check the skill of the needle-bearer
			if (ent->client->sess.skill[SK_FIRST_AID] >= 3) // full revive
			{
				healamt = traceEnt->client->ps.stats[STAT_MAX_HEALTH];
			}
			else
			{
				healamt = (int)(traceEnt->client->ps.stats[STAT_MAX_HEALTH] * 0.5);
			}

			traceEnt->health = healamt;
		}
		G_Sound(traceEnt, GAMESOUND_MISC_REVIVE);

		//ent->client->sess.team_hits -= 2.f;

		// set lasthealth_client also when a player was healed with syringe
		traceEnt->client->pers.lasthealth_client = ent->s.clientNum;

		if (!traceEnt->isProp)      // flag for if they were teamkilled or not
		{
			G_AddSkillPoints(ent, SK_FIRST_AID, 2.f);
			G_DebugAddSkillPoints(ent, SK_FIRST_AID, 2.f, "syringe heal a player");
		}
	}
}

/**
 * @brief Hmmmm. Needles. With stuff in it. Woooo.
 * @param ent
 */
void Weapon_AdrenalineSyringe(gentity_t *ent)
{
	ent->client->ps.powerups[PW_ADRENALINE] = level.time + 10000;
}

void G_ExplodeMissile(gentity_t *ent);

/**
 * @brief Crude version of G_RadiusDamage to see if the dynamite can damage a func_constructible
 * @param[in] origin
 * @param[in] radius
 * @param[out] damagedList
 * @return
 */
int EntsThatRadiusCanDamage(vec3_t origin, float radius, int *damagedList)
{
	float     dist;
	gentity_t *ent;
	int       entityList[MAX_GENTITIES];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
	int       i, e;
	float     boxradius;
	vec3_t    dest;
	trace_t   tr;
	vec3_t    midpoint;
	int       numDamaged = 0;

	if (radius < 1)
	{
		radius = 1;
	}

	boxradius = M_SQRT2 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for (i = 0 ; i < 3 ; i++)
	{
		mins[i] = origin[i] - boxradius;
		maxs[i] = origin[i] + boxradius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (e = 0 ; e < numListedEntities ; e++)
	{
		ent = &g_entities[entityList[e]];

		if (!ent->r.bmodel)
		{
			VectorSubtract(ent->r.currentOrigin, origin, v);
		}
		else
		{
			for (i = 0 ; i < 3 ; i++)
			{
				if (origin[i] < ent->r.absmin[i])
				{
					v[i] = ent->r.absmin[i] - origin[i];
				}
				else if (origin[i] > ent->r.absmax[i])
				{
					v[i] = origin[i] - ent->r.absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}
		}

		dist = VectorLength(v);
		if (dist >= radius)
		{
			continue;
		}

		if (CanDamage(ent, origin))
		{
			damagedList[numDamaged++] = entityList[e];
		}
		else
		{
			VectorAdd(ent->r.absmin, ent->r.absmax, midpoint);
			VectorScale(midpoint, 0.5f, midpoint);
			VectorCopy(midpoint, dest);

			trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
			if (tr.fraction < 1.0f)
			{
				VectorSubtract(dest, origin, dest);
				dist = VectorLength(dest);
				if (dist < radius * 0.2f)     // closer than 1/4 dist
				{
					damagedList[numDamaged++] = entityList[e];
				}
			}
		}
	}

	return numDamaged;
}

extern void G_LandminePrime(gentity_t *self);
extern void explosive_indicator_think(gentity_t *ent);

#define MIN_BLOCKINGWARNING_INTERVAL 3000

/**
 * @brief MakeTemporarySolid
 * @param[in,out] ent
 */
static void MakeTemporarySolid(gentity_t *ent)
{
	if (ent->entstate == STATE_UNDERCONSTRUCTION)
	{
		ent->clipmask   = ent->realClipmask;
		ent->r.contents = ent->realContents;
		if (!ent->realNonSolidBModel)
		{
			ent->s.eFlags &= ~EF_NONSOLID_BMODEL;
		}
	}

	trap_LinkEntity(ent);
}

/**
 * @brief UndoTemporarySolid
 * @param[in,out] ent
 */
static void UndoTemporarySolid(gentity_t *ent)
{
	ent->entstate     = STATE_UNDERCONSTRUCTION;
	ent->s.powerups   = STATE_UNDERCONSTRUCTION;
	ent->realClipmask = ent->clipmask;
	ent->clipmask     = 0;
	ent->realContents = ent->r.contents;
	ent->r.contents   = 0;
	if (ent->s.eFlags & EF_NONSOLID_BMODEL)
	{
		ent->realNonSolidBModel = qtrue;
	}
	else
	{
		ent->s.eFlags |= EF_NONSOLID_BMODEL;
	}

	trap_LinkEntity(ent);
}

/**
 * @brief HandleEntsThatBlockConstructible
 * @param[in] constructor
 * @param[in,out] constructible
 * @param[in] handleBlockingEnts kill players, return flags, remove entities
 * @param[in] warnBlockingPlayers warn any players that are in the constructible area
 */
static void HandleEntsThatBlockConstructible(gentity_t *constructor, gentity_t *constructible, qboolean handleBlockingEnts, qboolean warnBlockingPlayers)
{
	// check if something blocks us
	int       constructibleList[MAX_GENTITIES];
	int       entityList[MAX_GENTITIES];
	int       blockingList[MAX_GENTITIES];
	int       constructibleEntities = 0;
	int       listedEntities, e;
	int       blockingEntities = 0;
	gentity_t *check, *block;
	// backup...
	int constructibleModelindex     = constructible->s.modelindex;
	int constructibleClipmask       = constructible->clipmask;
	int constructibleContents       = constructible->r.contents;
	int constructibleNonSolidBModel = (constructible->s.eFlags & EF_NONSOLID_BMODEL);

	trap_SetBrushModel(constructible, va("*%i", constructible->s.modelindex2));

	// ...and restore
	constructible->clipmask   = constructibleClipmask;
	constructible->r.contents = constructibleContents;
	if (!constructibleNonSolidBModel)
	{
		constructible->s.eFlags &= ~EF_NONSOLID_BMODEL;
	}
	trap_LinkEntity(constructible);

	// store our origin
	VectorCopy(constructible->r.absmin, constructible->s.origin2);
	VectorAdd(constructible->r.absmax, constructible->s.origin2, constructible->s.origin2);
	VectorScale(constructible->s.origin2, 0.5f, constructible->s.origin2);

	// get all the entities that make up the constructible
	if (constructible->track && constructible->track[0])
	{
		vec3_t mins, maxs;

		VectorCopy(constructible->r.absmin, mins);
		VectorCopy(constructible->r.absmax, maxs);

		check = NULL;

		while (1)
		{
			check = G_Find(check, FOFS(track), constructible->track);

			if (check == constructible)
			{
				continue;
			}

			if (!check)
			{
				break;
			}

			if (constructible->count2)
			{
				if (check->partofstage != constructible->grenadeFired)
				{
					continue;
				}
			}

			// get the bounding box of all entities in the constructible together
			AddPointToBounds(check->r.absmin, mins, maxs);
			AddPointToBounds(check->r.absmax, mins, maxs);

			constructibleList[constructibleEntities++] = check->s.number;
		}

		listedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		// make our constructible entities solid so we can check against them
		//trap_LinkEntity( constructible );
		MakeTemporarySolid(constructible);
		for (e = 0; e < constructibleEntities; e++)
		{
			check = &g_entities[constructibleList[e]];

			//trap_LinkEntity( check );
			MakeTemporarySolid(check);
		}
	}
	else
	{
		// changed * to abs*
		listedEntities = trap_EntitiesInBox(constructible->r.absmin, constructible->r.absmax, entityList, MAX_GENTITIES);

		// make our constructible solid so we can check against it
		//trap_LinkEntity( constructible );
		MakeTemporarySolid(constructible);
	}

	for (e = 0; e < listedEntities; e++)
	{
		check = &g_entities[entityList[e]];

		// ignore everything but items, players and missiles (grenades too)
		if (check->s.eType != ET_MISSILE && check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER && !check->physicsObject)
		{
			continue;
		}

		// remove any corpses, this includes dynamite
		if (check->r.contents == CONTENTS_CORPSE)
		{
			blockingList[blockingEntities++] = entityList[e];
			continue;
		}

		// FIXME: dynamite seems to test out of position?
		// see if the entity is in a solid now
		if ((block = G_TestEntityPosition(check)) == NULL)
		{
			continue;
		}

		// the entity is blocked and it is a player, then warn the player
		if (warnBlockingPlayers && check->s.eType == ET_PLAYER)
		{
			if ((level.time - check->client->lastConstructibleBlockingWarnTime) >= MIN_BLOCKINGWARNING_INTERVAL)
			{
				trap_SendServerCommand(check->s.number, "cp \"Warning, leave the construction area\" 1");
				check->client->lastConstructibleBlockingWarnTime = level.time;
			}
		}

		blockingList[blockingEntities++] = entityList[e];
	}

	// undo the temporary solid for our entities
	UndoTemporarySolid(constructible);
	if (constructible->track && constructible->track[0])
	{
		for (e = 0; e < constructibleEntities; e++)
		{
			check = &g_entities[constructibleList[e]];

			//trap_UnlinkEntity( check );
			UndoTemporarySolid(check);
		}
	}

	if (handleBlockingEnts)
	{
		for (e = 0; e < blockingEntities; e++)
		{
			block = &g_entities[blockingList[e]];

			if (block->client || block->s.eType == ET_CORPSE)
			{
				G_Damage(block, constructible, constructor, NULL, NULL, 9999, DAMAGE_NO_PROTECTION, MOD_CRUSH_CONSTRUCTION);
			}
			else if (block->s.eType == ET_ITEM && block->item->giType == IT_TEAM)
			{
				// see if it's a critical entity, one that we can't just simply kill (basically flags)
				Team_DroppedFlagThink(block);
			}
			else
			{
				// remove the landmine from both teamlists
				if (block->s.eType == ET_MISSILE && block->methodOfDeath == MOD_LANDMINE)
				{
					mapEntityData_t *mEnt;

					if ((mEnt = G_FindMapEntityData(&mapEntityData[0], block - g_entities)) != NULL)
					{
						G_FreeMapEntityData(&mapEntityData[0], mEnt);
					}

					if ((mEnt = G_FindMapEntityData(&mapEntityData[1], block - g_entities)) != NULL)
					{
						G_FreeMapEntityData(&mapEntityData[1], mEnt);
					}
				}

				// just get rid of it
				G_FreeEntity(block);
			}
		}
	}

	if (constructibleModelindex)
	{
		trap_SetBrushModel(constructible, va("*%i", constructibleModelindex));
		// ...and restore
		constructible->clipmask   = constructibleClipmask;
		constructible->r.contents = constructibleContents;
		if (!constructibleNonSolidBModel)
		{
			constructible->s.eFlags &= ~EF_NONSOLID_BMODEL;
		}
		trap_LinkEntity(constructible);
	}
	else
	{
		constructible->s.modelindex = 0;
		//constructible->clipmask = constructibleClipmask;
		//constructible->r.contents = constructibleContents;
		//if( !constructibleNonSolidBModel )
		//  constructible->s.eFlags &= ~EF_NONSOLID_BMODEL;
		trap_LinkEntity(constructible);
	}
}

#define CONSTRUCT_POSTDECAY_TIME 500

/**
 * @brief TryConstructing
 *
 * @param ent
 *
 * @return qfalse when it couldn't build
 *
 * @note !! If the conditions here of a buildable constructible change, then BotIsConstructible() must reflect those changes !!
 */
static qboolean TryConstructing(gentity_t *ent)
{
	gentity_t *constructible = ent->client->touchingTOI->target_ent;

	// no construction during prematch
	if (level.warmupTime)
	{
		return qfalse;
	}

	// see if we are in a trigger_objective_info targetting multiple func_constructibles
	if (constructible->s.eType == ET_CONSTRUCTIBLE && ent->client->touchingTOI->chain)
	{
		gentity_t *otherconstructible = NULL;

		// use the target that has the same team as the player
		if (constructible->s.teamNum != ent->client->sess.sessionTeam)
		{
			constructible = ent->client->touchingTOI->chain;
		}

		otherconstructible = constructible->chain;

		// make sure the other constructible isn't built/underconstruction/something
		if (otherconstructible->s.angles2[0] != 0.f ||
		    otherconstructible->s.angles2[1] != 0.f ||
		    (otherconstructible->count2 && otherconstructible->grenadeFired))
		{
			return qfalse;
		}
	}

	// see if we are in a trigger_objective_info targetting a func_constructible
	if (constructible->s.eType == ET_CONSTRUCTIBLE &&
	    constructible->s.teamNum == ent->client->sess.sessionTeam)
	{
		// constructible xp sharing
		float addhealth;
		float xpperround;

		if (constructible->s.angles2[0] >= 250)     // have to do this so we don't score multiple times
		{
			return qfalse;
		}

		if (constructible->s.angles2[1] != 0.f)
		{
			return qfalse;
		}

		// Check if we can construct - updates the classWeaponTime as well
		if (!ReadyToConstruct(ent, constructible, qtrue))
		{
			return qtrue;
		}

		// try to start building
		if (constructible->s.angles2[0] <= 0)
		{
			// wait a bit, this prevents network spam
			if (level.time - constructible->lastHintCheckTime < CONSTRUCT_POSTDECAY_TIME)
			{
				return qtrue;      // likely will come back soon - so override other plier bits anyway
			}

			// swap brushmodels if staged
			if (constructible->count2)
			{
				constructible->grenadeFired++;
				constructible->s.modelindex2 = constructible->conbmodels[constructible->grenadeFired - 1];
				//trap_SetBrushModel( constructible, va( "*%i", constructible->conbmodels[constructible->grenadeFired-1] ) );
			}

			G_SetEntState(constructible, STATE_UNDERCONSTRUCTION);

			if (!constructible->count2)
			{
				// call script
				G_Script_ScriptEvent(constructible, "buildstart", "final");
				constructible->s.frame = 1;
			}
			else
			{
				if (constructible->grenadeFired == constructible->count2)
				{
					G_Script_ScriptEvent(constructible, "buildstart", "final");
					constructible->s.frame = constructible->grenadeFired;
				}
				else
				{
					switch (constructible->grenadeFired)
					{
					case 1:
						G_Script_ScriptEvent(constructible, "buildstart", "stage1");
						constructible->s.frame = 1;
						break;
					case 2:
						G_Script_ScriptEvent(constructible, "buildstart", "stage2");
						constructible->s.frame = 2;
						break;
					case 3:
						G_Script_ScriptEvent(constructible, "buildstart", "stage3");
						constructible->s.frame = 3;
						break;
					default:
						break;
					}
				}
			}

			if (ent->client->touchingTOI->chain && ent->client->touchingTOI->count2)
			{
				// find the constructible indicator and change team
				mapEntityData_t      *mEnt;
				mapEntityData_Team_t *teamList;
				gentity_t            *indicator = &g_entities[ent->client->touchingTOI->count2];

				indicator->s.teamNum = constructible->s.teamNum;

				// update the map for the other team
				teamList = indicator->s.teamNum == TEAM_AXIS ? &mapEntityData[1] : &mapEntityData[0]; // inversed
				if ((mEnt = G_FindMapEntityData(teamList, indicator - g_entities)) != NULL)
				{
					G_FreeMapEntityData(teamList, mEnt);
				}
			}

			if (!constructible->count2 || constructible->grenadeFired == 1)
			{
				// link in if we just started building
				G_UseEntity(constructible, ent->client->touchingTOI, ent);
			}

			// setup our think function for decaying
			constructible->think     = func_constructible_underconstructionthink;
			constructible->nextthink = level.time + FRAMETIME;

			G_PrintClientSpammyCenterPrint(ent - g_entities, "Constructing...");
		}

		if (!ent->client->constructSoundTime || level.time > ent->client->constructSoundTime)
		{
			// construction sound sent as event (was temp entity)
			G_AddEvent(ent, EV_GENERAL_SOUND, GAMESOUND_WORLD_BUILD);
			ent->client->constructSoundTime = level.time + 4000; // duration of sound
		}

		// constructible xp sharing
		addhealth  = (255.f / (constructible->constructibleStats.duration / (float)FRAMETIME));
		xpperround = constructible->constructibleStats.constructxpbonus / (255.f / addhealth) + 0.01f;
		G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, xpperround);
		G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, xpperround, "construction sharing.");

		// Give health until it is full, don't continue
		constructible->s.angles2[0] += addhealth;

		if (constructible->s.angles2[0] >= 250)
		{
			constructible->s.angles2[0] = 0;
			HandleEntsThatBlockConstructible(ent, constructible, qtrue, qfalse);
		}
		else
		{
			constructible->lastHintCheckTime = level.time;
			HandleEntsThatBlockConstructible(ent, constructible, qfalse, qtrue);
			return qtrue;      // properly constructed
		}

		if (constructible->count2)
		{
			// backup...
			int constructibleClipmask       = constructible->clipmask;
			int constructibleContents       = constructible->r.contents;
			int constructibleNonSolidBModel = (constructible->s.eFlags & EF_NONSOLID_BMODEL);

			constructible->s.modelindex2 = 0;
			trap_SetBrushModel(constructible, va("*%i", constructible->conbmodels[constructible->grenadeFired - 1]));

			// ...and restore
			constructible->clipmask   = constructibleClipmask;
			constructible->r.contents = constructibleContents;
			if (!constructibleNonSolidBModel)
			{
				constructible->s.eFlags &= ~EF_NONSOLID_BMODEL;
			}

			if (constructible->grenadeFired == constructible->count2)
			{
				constructible->s.angles2[1] = 1;
			}
		}
		else
		{
			// backup...
			int constructibleClipmask       = constructible->clipmask;
			int constructibleContents       = constructible->r.contents;
			int constructibleNonSolidBModel = (constructible->s.eFlags & EF_NONSOLID_BMODEL);

			constructible->s.modelindex2 = 0;
			trap_SetBrushModel(constructible, constructible->model);

			// ...and restore
			constructible->clipmask   = constructibleClipmask;
			constructible->r.contents = constructibleContents;
			if (!constructibleNonSolidBModel)
			{
				constructible->s.eFlags &= ~EF_NONSOLID_BMODEL;
			}

			constructible->s.angles2[1] = 1;
		}

		// unlink the objective info to get rid of the indicator for now
		// don't unlink, we still want the location popup. Instead, constructible_indicator_think got changed to free
		// the indicator when the constructible is constructed
		//if( constructible->parent )
		//  trap_UnlinkEntity( constructible->parent );

		G_SetEntState(constructible, STATE_DEFAULT);

		// make destructable
		if (!(constructible->spawnflags & 2))
		{
			constructible->takedamage = qtrue;
			constructible->health     = constructible->sound1to2;
		}

		// Stop thinking
		constructible->think     = NULL;
		constructible->nextthink = 0;

		if (!constructible->count2)
		{
			// call script
			G_Script_ScriptEvent(constructible, "built", "final");
		}
		else
		{
			if (constructible->grenadeFired == constructible->count2)
			{
				G_Script_ScriptEvent(constructible, "built", "final");
			}
			else
			{
				switch (constructible->grenadeFired)
				{
				case 1:
					G_Script_ScriptEvent(constructible, "built", "stage1");
					break;
				case 2:
					G_Script_ScriptEvent(constructible, "built", "stage2");
					break;
				case 3:
					G_Script_ScriptEvent(constructible, "built", "stage3");
					break;
				default:
					break;
				}
			}
		}

		// Stop sound
		if (constructible->parent->spawnflags & 8)
		{
			constructible->parent->s.loopSound = 0;
		}
		else
		{
			constructible->s.loopSound = 0;
		}

		//ent->client->ps.classWeaponTime = level.time; // Out of "ammo"

		// if not invulnerable and dynamite-able, create a 'destructable' marker for the other team
		if (!(constructible->spawnflags & CONSTRUCTIBLE_INVULNERABLE) && (constructible->constructibleStats.weaponclass >= 1))
		{
			if (!constructible->count2 || constructible->grenadeFired == 1)
			{
				gentity_t *tent = NULL;
				gentity_t *e;

				e               = G_Spawn();
				e->r.svFlags    = SVF_BROADCAST;
				e->classname    = "explosive_indicator";
				e->s.pos.trType = TR_STATIONARY;
				e->s.eType      = ET_EXPLOSIVE_INDICATOR;

				// Find the trigger_objective_info that targets us (if not set before)
				while ((tent = G_Find(tent, FOFS(target), constructible->targetname)) != NULL)
				{
					if (tent->s.eType == ET_OID_TRIGGER)
					{
						if (tent->spawnflags & 8)
						{
							e->s.eType = ET_TANK_INDICATOR;
						}
						e->parent = tent;
					}
				}

				if (constructible->spawnflags & AXIS_CONSTRUCTIBLE)
				{
					e->s.teamNum = TEAM_AXIS;
				}
				else if (constructible->spawnflags & ALLIED_CONSTRUCTIBLE)
				{
					e->s.teamNum = TEAM_ALLIES;
				}

				e->s.modelindex2 = ent->client->touchingTOI->s.teamNum;
				e->r.ownerNum    = constructible->s.number;
				e->think         = explosive_indicator_think;
				e->nextthink     = level.time + FRAMETIME;

				e->s.effect1Time = constructible->constructibleStats.weaponclass;

				if (constructible->parent->tagParent)
				{
					e->tagParent = constructible->parent->tagParent;
					Q_strncpyz(e->tagName, constructible->parent->tagName, MAX_QPATH);
				}
				else
				{
					VectorCopy(constructible->r.absmin, e->s.pos.trBase);
					VectorAdd(constructible->r.absmax, e->s.pos.trBase, e->s.pos.trBase);
					VectorScale(e->s.pos.trBase, 0.5f, e->s.pos.trBase);
				}

				SnapVector(e->s.pos.trBase);

				trap_LinkEntity(e);
			}
			else
			{
				gentity_t *check;
				int       i;

				// find our marker and update it's coordinates
				for (i = 0, check = g_entities; i < level.num_entities; i++, check++)
				{
					if (check->s.eType != ET_EXPLOSIVE_INDICATOR && check->s.eType != ET_TANK_INDICATOR && check->s.eType != ET_TANK_INDICATOR_DEAD)
					{
						continue;
					}

					if (check->r.ownerNum == constructible->s.number)
					{
						// found it!
						if (constructible->parent->tagParent)
						{
							check->tagParent = constructible->parent->tagParent;
							Q_strncpyz(check->tagName, constructible->parent->tagName, MAX_QPATH);
						}
						else
						{
							VectorCopy(constructible->r.absmin, check->s.pos.trBase);
							VectorAdd(constructible->r.absmax, check->s.pos.trBase, check->s.pos.trBase);
							VectorScale(check->s.pos.trBase, 0.5f, check->s.pos.trBase);

							SnapVector(check->s.pos.trBase);
						}

						trap_LinkEntity(check);
						break;
					}
				}
			}
		}

		return qtrue;      // building
	}

	return qfalse;
}

/**
 * @brief AutoBuildConstruction
 * @param[in,out] constructible
 */
void AutoBuildConstruction(gentity_t *constructible)
{
	HandleEntsThatBlockConstructible(NULL, constructible, qtrue, qfalse);
	if (constructible->count2)
	{
		// backup...
		int constructibleClipmask       = constructible->clipmask;
		int constructibleContents       = constructible->r.contents;
		int constructibleNonSolidBModel = (constructible->s.eFlags & EF_NONSOLID_BMODEL);

		constructible->s.modelindex2 = 0;
		trap_SetBrushModel(constructible, va("*%i", constructible->conbmodels[constructible->grenadeFired - 1]));

		// ...and restore
		constructible->clipmask   = constructibleClipmask;
		constructible->r.contents = constructibleContents;
		if (!constructibleNonSolidBModel)
		{
			constructible->s.eFlags &= ~EF_NONSOLID_BMODEL;
		}

		if (constructible->grenadeFired == constructible->count2)
		{
			constructible->s.angles2[1] = 1;
		}
	}
	else
	{
		// backup...
		int constructibleClipmask       = constructible->clipmask;
		int constructibleContents       = constructible->r.contents;
		int constructibleNonSolidBModel = (constructible->s.eFlags & EF_NONSOLID_BMODEL);

		constructible->s.modelindex2 = 0;
		trap_SetBrushModel(constructible, constructible->model);

		// ...and restore
		constructible->clipmask   = constructibleClipmask;
		constructible->r.contents = constructibleContents;
		if (!constructibleNonSolidBModel)
		{
			constructible->s.eFlags &= ~EF_NONSOLID_BMODEL;
		}

		constructible->s.angles2[1] = 1;
	}

	// unlink the objective info to get rid of the indicator for now
	// don't unlink, we still want the location popup. Instead, constructible_indicator_think got changed to free
	// the indicator when the constructible is constructed
	//          if( constructible->parent )
	//              trap_UnlinkEntity( constructible->parent );

	G_SetEntState(constructible, STATE_DEFAULT);

	// make destructable
	if (!(constructible->spawnflags & CONSTRUCTIBLE_INVULNERABLE))
	{
		constructible->takedamage = qtrue;
		constructible->health     = constructible->constructibleStats.health;
	}

	// Stop thinking
	constructible->think     = NULL;
	constructible->nextthink = 0;

	if (!constructible->count2)
	{
		// call script
		G_Script_ScriptEvent(constructible, "built", "final");
	}
	else
	{
		if (constructible->grenadeFired == constructible->count2)
		{
			G_Script_ScriptEvent(constructible, "built", "final");
		}
		else
		{
			switch (constructible->grenadeFired)
			{
			case 1:
				G_Script_ScriptEvent(constructible, "built", "stage1");
				break;
			case 2:
				G_Script_ScriptEvent(constructible, "built", "stage2");
				break;
			case 3:
				G_Script_ScriptEvent(constructible, "built", "stage3");
				break;
			default:
				break;
			}
		}
	}

	// Stop sound
	if (constructible->parent->spawnflags & 8)
	{
		constructible->parent->s.loopSound = 0;
	}
	else
	{
		constructible->s.loopSound = 0;
	}

	//ent->client->ps.classWeaponTime = level.time; // Out of "ammo"

	//if not invulnerable and dynamite-able, create a 'destructable' marker for the other team
	if (!(constructible->spawnflags & CONSTRUCTIBLE_INVULNERABLE) && (constructible->constructibleStats.weaponclass >= 1))
	{
		if (!constructible->count2 || constructible->grenadeFired == 1)
		{
			gentity_t *tent = NULL;
			gentity_t *e;

			e               = G_Spawn();
			e->r.svFlags    = SVF_BROADCAST;
			e->classname    = "explosive_indicator";
			e->s.pos.trType = TR_STATIONARY;
			e->s.eType      = ET_EXPLOSIVE_INDICATOR;

			// Find the trigger_objective_info that targets us (if not set before)
			while ((tent = G_Find(tent, FOFS(target), constructible->targetname)) != NULL)
			{
				if (tent->s.eType == ET_OID_TRIGGER)
				{
					if (tent->spawnflags & 8)
					{
						e->s.eType = ET_TANK_INDICATOR;
					}
					e->parent = tent;
				}
			}

			if (constructible->spawnflags & AXIS_CONSTRUCTIBLE)
			{
				e->s.teamNum = TEAM_AXIS;
			}
			else if (constructible->spawnflags & ALLIED_CONSTRUCTIBLE)
			{
				e->s.teamNum = TEAM_ALLIES;
			}

			e->s.modelindex2 = constructible->parent->s.teamNum == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
			e->r.ownerNum    = constructible->s.number;
			e->think         = explosive_indicator_think;
			e->nextthink     = level.time + FRAMETIME;

			e->s.effect1Time = constructible->constructibleStats.weaponclass;

			if (constructible->parent->tagParent)
			{
				e->tagParent = constructible->parent->tagParent;
				Q_strncpyz(e->tagName, constructible->parent->tagName, MAX_QPATH);
			}
			else
			{
				VectorCopy(constructible->r.absmin, e->s.pos.trBase);
				VectorAdd(constructible->r.absmax, e->s.pos.trBase, e->s.pos.trBase);
				VectorScale(e->s.pos.trBase, 0.5f, e->s.pos.trBase);
			}

			SnapVector(e->s.pos.trBase);

			trap_LinkEntity(e);
		}
		else
		{
			gentity_t *check;
			int       i;

			// find our marker and update it's coordinates
			for (i = 0, check = g_entities; i < level.num_entities; i++, check++)
			{
				if (check->s.eType != ET_EXPLOSIVE_INDICATOR && check->s.eType != ET_TANK_INDICATOR && check->s.eType != ET_TANK_INDICATOR_DEAD)
				{
					continue;
				}

				if (check->r.ownerNum == constructible->s.number)
				{
					// found it!
					if (constructible->parent->tagParent)
					{
						check->tagParent = constructible->parent->tagParent;
						Q_strncpyz(check->tagName, constructible->parent->tagName, MAX_QPATH);
					}
					else
					{
						VectorCopy(constructible->r.absmin, check->s.pos.trBase);
						VectorAdd(constructible->r.absmax, check->s.pos.trBase, check->s.pos.trBase);
						VectorScale(check->s.pos.trBase, 0.5f, check->s.pos.trBase);

						SnapVector(check->s.pos.trBase);
					}

					trap_LinkEntity(check);
					break;
				}
			}
		}
	}
}

/**
 * @brief G_LandmineTriggered
 * @param[in] ent
 * @return
 */
qboolean G_LandmineTriggered(gentity_t *ent)
{
	if (ent->s.teamNum == (TEAM_AXIS + 8) || ent->s.teamNum == (TEAM_ALLIES + 8))
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/**
 * @brief G_LandmineArmed
 * @param[in] ent
 * @return
 */
qboolean G_LandmineArmed(gentity_t *ent)
{
	if (ent->s.teamNum == TEAM_AXIS || ent->s.teamNum == TEAM_ALLIES)
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/**
 * @brief G_LandmineUnarmed
 * @param[in] ent
 * @return
 */
qboolean G_LandmineUnarmed(gentity_t *ent)
{
	return (!G_LandmineArmed(ent) && !G_LandmineTriggered(ent));
}

/**
 * @brief G_LandmineTeam
 * @param[in] ent
 * @return
 */
team_t G_LandmineTeam(gentity_t *ent)
{
	return (ent->s.teamNum % 4);
}

/**
 * @brief G_LandmineSpotted
 * @param[in] ent
 * @return
 */
qboolean G_LandmineSpotted(gentity_t *ent)
{
	return ent->s.modelindex2 ? qtrue : qfalse;
}

/**
 * @brief trap_EngineerTrace
 * @param[out] results
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param[in] passEntityNum
 * @param[in] contentmask
 */
void trap_EngineerTrace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	G_TempTraceIgnorePlayersAndBodies();
	trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
	G_ResetTempTraceIgnoreEnts();
}

/**
 * @brief Weapon_Engineer
 * @param[in,out] ent
 */
void Weapon_Engineer(gentity_t *ent)
{
	trace_t   tr;
	gentity_t *traceEnt;
	vec3_t    mins, end, origin;
	int       touch[MAX_GENTITIES];

	// Can't heal an MG42 if you're using one!
	if (ent->client->ps.persistant[PERS_HWEAPON_USE])
	{
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	VectorCopy(ent->client->ps.origin, muzzleTrace);
	muzzleTrace[2] += ent->client->ps.viewheight;

	VectorMA(muzzleTrace, 64, forward, end);             // CH_BREAKABLE_DIST
	trap_EngineerTrace(&tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT | CONTENTS_TRIGGER);

	if (tr.surfaceFlags & SURF_NOIMPACT || tr.fraction == 1.0f || tr.entityNum == ENTITYNUM_NONE || tr.entityNum == ENTITYNUM_WORLD)
	{
		// might be constructible
		if (!ent->client->touchingTOI)
		{
			goto weapengineergoto1;
		}
		else
		{
			if (TryConstructing(ent))
			{
				return;
			}
		}
		return;
	}

weapengineergoto1:

	traceEnt = &g_entities[tr.entityNum];

	if (G_EmplacedGunIsRepairable(traceEnt, ent))
	{
		// "Ammo" for this weapon is time based
		if (ent->client->ps.classWeaponTime + level.engineerChargeTime[ent->client->sess.sessionTeam - 1] < level.time)
		{
			ent->client->ps.classWeaponTime = level.time - level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		}

		if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3)
		{
			ent->client->ps.classWeaponTime += .66f * 150;
		}
		else
		{
			ent->client->ps.classWeaponTime += 150;
		}

		if (ent->client->ps.classWeaponTime > level.time)
		{
			ent->client->ps.classWeaponTime = level.time;
			return;     // Out of "ammo"
		}

		if (traceEnt->health >= 255)
		{
			traceEnt->s.frame = 0;

			if (traceEnt->mg42BaseEnt > 0)
			{
				g_entities[traceEnt->mg42BaseEnt].health     = MG42_MULTIPLAYER_HEALTH;
				g_entities[traceEnt->mg42BaseEnt].takedamage = qtrue;
				traceEnt->health                             = 0;
			}
			else
			{
				traceEnt->health = MG42_MULTIPLAYER_HEALTH;
			}

			G_LogPrintf("Repair: %d\n", (int)(ent - g_entities));

			if (traceEnt->sound3to2 != ent->client->sess.sessionTeam)
			{
				// constructible xp sharing - some lucky dood is going to get the last 0.00035 points and the repair bonus
				G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 0.00035f);
				G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 0.00035f, "repairing a MG 42");
			}

			traceEnt->takedamage = qtrue;
			traceEnt->s.eFlags  &= ~EF_SMOKING;

			trap_SendServerCommand(ent - g_entities, "cp \"You have repaired the MG 42\" 1");

			G_AddEvent(ent, EV_MG42_FIXED, 0);
		}
		else
		{
			float xpperround = 0.03529f;

			traceEnt->health += 3;

			G_PrintClientSpammyCenterPrint(ent - g_entities, "Repairing MG 42...");

			// constructible xp sharing - repairing an emplaced mg42
			G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, xpperround);
			G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, xpperround, "repairing a MG 42");
		}
	}
	else
	{
		trap_EngineerTrace(&tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT);

		if (tr.surfaceFlags & SURF_NOIMPACT || tr.fraction == 1.0f || tr.entityNum == ENTITYNUM_NONE || tr.entityNum == ENTITYNUM_WORLD)
		{
			// might be constructible
			if (!ent->client->touchingTOI)
			{
				goto weapengineergoto2;
			}
			else
			{
				if (TryConstructing(ent))
				{
					return;
				}
			}
			return;
		}

weapengineergoto2:

		traceEnt = &g_entities[tr.entityNum];

		if (traceEnt->methodOfDeath == MOD_LANDMINE)
		{
			trace_t tr2;
			vec3_t  base;
			vec3_t  tr_down = { 0, 0, 16 };

			VectorSubtract(traceEnt->s.pos.trBase, tr_down, base);

			trap_EngineerTrace(&tr2, traceEnt->s.pos.trBase, NULL, NULL, base, traceEnt->s.number, MASK_SHOT);

			if (!(tr2.surfaceFlags & SURF_LANDMINE) || (tr2.entityNum != ENTITYNUM_WORLD && (!g_entities[tr2.entityNum].inuse || g_entities[tr2.entityNum].s.eType != ET_CONSTRUCTIBLE)))
			{
				trap_SendServerCommand(ent - g_entities, "cp \"Landmine cannot be armed here\" 1");

				G_FreeEntity(traceEnt);

				Add_Ammo(ent, WP_LANDMINE, 1, qfalse);

				// give back the correct charge amount
				if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3)
				{
					ent->client->ps.classWeaponTime -= .33f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
				}
				else
				{
					ent->client->ps.classWeaponTime -= .5f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
				}
				ent->client->sess.aWeaponStats[WS_LANDMINE].atts--;
				return;

				// check landmine team so that enemy mines can be disarmed
				// even if you're using all of yours :x
			}
			else if (G_CountTeamLandmines(ent->client->sess.sessionTeam) >= team_maxLandmines.integer && G_LandmineTeam(traceEnt) == ent->client->sess.sessionTeam)
			{
				if (G_LandmineUnarmed(traceEnt))
				{
					// should be impossible now
					//if ( G_LandmineTeam( traceEnt ) != ent->client->sess.sessionTeam )
					//return;

					trap_SendServerCommand(ent - g_entities, "cp \"Your team has too many landmines placed\" 1");

					G_FreeEntity(traceEnt);

					Add_Ammo(ent, WP_LANDMINE, 1, qfalse);
					// give back the correct charge amount
					if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3)
					{
						ent->client->ps.classWeaponTime -= .33f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
					}
					else
					{
						ent->client->ps.classWeaponTime -= .5f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
					}

					ent->client->sess.aWeaponStats[WS_LANDMINE].atts--;
					return;
				}
				else
				{
					goto weapengineergoto3;
				}
			}
			else
			{
				if (G_LandmineUnarmed(traceEnt))
				{
					// opposing team cannot accidentally arm it
					if (G_LandmineTeam(traceEnt) != ent->client->sess.sessionTeam)
					{
						return;
					}

					G_PrintClientSpammyCenterPrint(ent - g_entities, "Arming landmine...");

					// give health until it is full, don't continue
					if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 2)
					{
						traceEnt->health += 24;
					}
					else
					{
						traceEnt->health += 12;
					}

					if (traceEnt->health >= 250)
					{
						//traceEnt->health = 255;
						trap_SendServerCommand(ent - g_entities, "cp \"Landmine armed\" 1");
					}
					else
					{
						return;
					}

					// crosshair mine owner id
					if (g_misc.integer & G_MISC_CROSSHAIR_LANDMINE)
					{
						traceEnt->s.otherEntityNum = ent->s.number;
					}
					else
					{
						traceEnt->s.otherEntityNum = MAX_CLIENTS + 1;
					}

					traceEnt->r.contents = 0;   // (player can walk through)
					trap_LinkEntity(traceEnt);

					// don't allow disarming for sec (so guy that WAS arming doesn't start disarming it!
					traceEnt->timestamp = level.time + 1000;
					traceEnt->health    = 0;

					traceEnt->s.teamNum     = ent->client->sess.sessionTeam;
					traceEnt->s.modelindex2 = 0;

					traceEnt->nextthink = level.time + 2000;
					traceEnt->think     = G_LandminePrime;
				}
				else
				{
weapengineergoto3:
					if (traceEnt->timestamp > level.time)
					{
						return;
					}
					if (traceEnt->health >= 250)     // have to do this so we don't score multiple times
					{
						return;
					}

					if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 2)
					{
						traceEnt->health += 6;
					}
					else
					{
						traceEnt->health += 3;
					}

					G_PrintClientSpammyCenterPrint(ent - g_entities, "Defusing landmine...");

					if (traceEnt->health >= 250)
					{
						mapEntityData_t *mEnt;

						//traceEnt->health = 255;
						//traceEnt->think = G_FreeEntity;
						//traceEnt->nextthink = level.time + FRAMETIME;

						trap_SendServerCommand(ent - g_entities, "cp \"Landmine defused\" 1");

						Add_Ammo(ent, WP_LANDMINE, 1, qfalse);

						if (G_LandmineTeam(traceEnt) != ent->client->sess.sessionTeam)
						{
							G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 4.f);
							G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 4.f, "defusing an enemy landmine");
						}

						// update our map
						if ((mEnt = G_FindMapEntityData(&mapEntityData[0], traceEnt - g_entities)) != NULL)
						{
							G_FreeMapEntityData(&mapEntityData[0], mEnt);
						}

						if ((mEnt = G_FindMapEntityData(&mapEntityData[1], traceEnt - g_entities)) != NULL)
						{
							G_FreeMapEntityData(&mapEntityData[1], mEnt);
						}

						G_FreeEntity(traceEnt);
					}
					else
					{
						return;
					}
				}
			}
		}
		else if (traceEnt->methodOfDeath == MOD_SATCHEL)
		{
			if (traceEnt->health >= 250)     // have to do this so we don't score multiple times
			{
				return;
			}

			// give health until it is full, don't continue
			traceEnt->health += 3;

			G_PrintClientSpammyCenterPrint(ent - g_entities, "Disarming satchel charge...");

			if (traceEnt->health >= 250)
			{
				traceEnt->health    = 255;
				traceEnt->think     = G_FreeEntity;
				traceEnt->nextthink = level.time + FRAMETIME;

				// consistency with dynamite defusing
				G_PrintClientSpammyCenterPrint(ent - g_entities, "Satchel charge disarmed");

				G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f);
				G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f, "disarming satchel charge");
			}
			else
			{
				return;
			}
		}
		else if (traceEnt->methodOfDeath == MOD_DYNAMITE)
		{
			gentity_t *hit;
			vec3_t    maxs;
			int       i, num;

			// not armed
			if (traceEnt->s.teamNum >= 4)
			{
				qboolean friendlyObj = qfalse;
				qboolean enemyObj    = qfalse;

				// Opposing team cannot accidentally arm it
				if ((traceEnt->s.teamNum - 4) != ent->client->sess.sessionTeam)
				{
					return;
				}

				G_PrintClientSpammyCenterPrint(ent - g_entities, "Arming dynamite...");

				// Give health until it is full, don't continue
				if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 2)
				{
					traceEnt->health += 14;
				}
				else
				{
					traceEnt->health += 7;
				}

				{
					int    entityList[MAX_GENTITIES];
					int    numListedEntities;
					int    e;
					vec3_t org;

					VectorCopy(traceEnt->r.currentOrigin, org);
					org[2] += 4;        // move out of ground

					G_TempTraceIgnorePlayersAndBodies();
					numListedEntities = EntsThatRadiusCanDamage(org, traceEnt->splashRadius, entityList);
					G_ResetTempTraceIgnoreEnts();

					for (e = 0; e < numListedEntities; e++)
					{
						hit = &g_entities[entityList[e]];

						if (hit->s.eType != ET_CONSTRUCTIBLE)
						{
							continue;
						}

						// invulnerable
						if ((hit->spawnflags & CONSTRUCTIBLE_INVULNERABLE) || (hit->parent && (hit->parent->spawnflags & 8)))
						{
							continue;
						}

						if (!G_ConstructionIsPartlyBuilt(hit))
						{
							continue;
						}

						// is it a friendly constructible
						if (hit->s.teamNum == traceEnt->s.teamNum - 4)
						{
							// G_FreeEntity( traceEnt );
							// trap_SendServerCommand( ent-g_entities, "cp \"You cannot arm dynamite near a friendly construction!\" 1");
							// return;
							friendlyObj = qtrue;
						}
					}
				}

				VectorCopy(traceEnt->r.currentOrigin, origin);
				SnapVector(origin);
				VectorAdd(origin, traceEnt->r.mins, mins);
				VectorAdd(origin, traceEnt->r.maxs, maxs);
				num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);
				VectorAdd(origin, traceEnt->r.mins, mins);
				VectorAdd(origin, traceEnt->r.maxs, maxs);

				for (i = 0 ; i < num ; i++)
				{
					hit = &g_entities[touch[i]];

					if (!(hit->r.contents & CONTENTS_TRIGGER))
					{
						continue;
					}

					if (hit->s.eType == ET_OID_TRIGGER)
					{
						if (!(hit->spawnflags & (AXIS_OBJECTIVE | ALLIED_OBJECTIVE)))
						{
							continue;
						}

						// only if it targets a func_explosive
						if (hit->target_ent && Q_stricmp(hit->target_ent->classname, "func_explosive"))
						{
							continue;
						}

						if (((hit->spawnflags & AXIS_OBJECTIVE) && (ent->client->sess.sessionTeam == TEAM_AXIS)) ||
						    ((hit->spawnflags & ALLIED_OBJECTIVE) && (ent->client->sess.sessionTeam == TEAM_ALLIES)))
						{
							//G_FreeEntity( traceEnt );
							//trap_SendServerCommand( ent-g_entities, "cp \"You cannot arm dynamite near a friendly objective!\" 1");
							//return;
							friendlyObj = qtrue;
						}

						if (((hit->spawnflags & AXIS_OBJECTIVE) && (ent->client->sess.sessionTeam == TEAM_ALLIES)) ||
						    ((hit->spawnflags & ALLIED_OBJECTIVE) && (ent->client->sess.sessionTeam == TEAM_AXIS)))
						{
							enemyObj = qtrue;
						}
					}
				}

				if (friendlyObj && !enemyObj)
				{
					G_FreeEntity(traceEnt);
					trap_SendServerCommand(ent - g_entities, "cp \"You cannot arm dynamite near a friendly objective!\" 1");
					return;
				}

				if (traceEnt->health >= 250)
				{
					traceEnt->health = 255;
				}
				else
				{
					return;
				}

				// don't allow disarming for sec (so guy that WAS arming doesn't start disarming it!
				traceEnt->timestamp = level.time + 1000;
				traceEnt->health    = 5;

				// set teamnum so we can check it for drop/defuse exploit
				traceEnt->s.teamNum = ent->client->sess.sessionTeam;
				// for dynamic light pulsing
				traceEnt->s.effect1Time = level.time;

				// dynamite crosshair ID
				if (g_misc.integer & G_MISC_CROSSHAIR_DYNAMITE)
				{
					traceEnt->s.otherEntityNum = ent->s.number;
				}
				else
				{
					traceEnt->s.otherEntityNum = MAX_CLIENTS + 1;
				}

				// arm it
				traceEnt->nextthink = level.time + 30000;
				traceEnt->think     = G_ExplodeMissile;

				// moved down here to prevent two prints when dynamite IS near objective
				trap_SendServerCommand(ent - g_entities, "cp \"Dynamite is now armed with a 30 second timer!\" 1");

				// check if player is in trigger objective field
				// made this the actual bounding box of dynamite instead of range, also must snap origin to line up properly
				VectorCopy(traceEnt->r.currentOrigin, origin);
				SnapVector(origin);
				VectorAdd(origin, traceEnt->r.mins, mins);
				VectorAdd(origin, traceEnt->r.maxs, maxs);
				num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

				for (i = 0 ; i < num ; i++)
				{
					hit = &g_entities[touch[i]];

					if (!(hit->r.contents & CONTENTS_TRIGGER))
					{
						continue;
					}
					if (hit->s.eType == ET_OID_TRIGGER)
					{
						if (!(hit->spawnflags & (AXIS_OBJECTIVE | ALLIED_OBJECTIVE)))
						{
							continue;
						}

						// only if it targets a func_explosive
						if (hit->target_ent && Q_stricmp(hit->target_ent->classname, "func_explosive"))
						{
							continue;
						}

						if (hit->spawnflags & AXIS_OBJECTIVE)
						{
							if (ent->client->sess.sessionTeam == TEAM_ALLIES)         // transfer score info if this is a bomb scoring objective
							{
								traceEnt->accuracy = hit->accuracy;
							}
						}
						else if (hit->spawnflags & ALLIED_OBJECTIVE)
						{
							if (ent->client->sess.sessionTeam == TEAM_AXIS)         // dito other team
							{
								traceEnt->accuracy = hit->accuracy;
							}
						}

						// spawnflags 128 = disabled
						if (!(hit->spawnflags & 128) && (((hit->spawnflags & AXIS_OBJECTIVE) && (ent->client->sess.sessionTeam == TEAM_ALLIES)) ||
						                                 ((hit->spawnflags & ALLIED_OBJECTIVE) && (ent->client->sess.sessionTeam == TEAM_AXIS))))
						{
#ifdef FEATURE_OMNIBOT
							const char *Goalname = _GetEntityName(hit);
#endif
							gentity_t *pm;

							pm = G_PopupMessage(PM_DYNAMITE);

							pm->s.effect2Time = 0;
							pm->s.effect3Time = hit->s.teamNum;
							pm->s.teamNum     = ent->client->sess.sessionTeam;

							G_Script_ScriptEvent(hit, "dynamited", ent->client->sess.sessionTeam == TEAM_AXIS ? "axis" : "allies");

#ifdef FEATURE_OMNIBOT
							// notify omni-bot framework of planted dynamite
							hit->numPlanted += 1;
							Bot_AddDynamiteGoal(traceEnt, traceEnt->s.teamNum, va("%s_%i", Goalname, hit->numPlanted));
#endif

							if (!(hit->spawnflags & OBJECTIVE_DESTROYED))
							{
								if (traceEnt->parent && traceEnt->parent->client)
								{
									G_LogPrintf("Dynamite_Plant: %d\n", (int)(traceEnt->parent - g_entities));
								}
								traceEnt->parent = ent;     // give explode score to guy who armed it
							}
							traceEnt->etpro_misc_1 |= 1;
							traceEnt->etpro_misc_2  = hit->s.number;
						}
						// i = num;
						return;     // bail out here because primary obj's take precendence over constructibles
					}
				}

				// reordered this check so its AFTER the primary obj check
				// - first see if the dynamite is planted near a constructable object that can be destroyed
				{
					int    entityList[MAX_GENTITIES];
					int    numListedEntities;
					int    e;
					vec3_t org;

					VectorCopy(traceEnt->r.currentOrigin, org);
					org[2] += 4;        // move out of ground

					G_TempTraceIgnorePlayersAndBodies();
					numListedEntities = EntsThatRadiusCanDamage(org, traceEnt->splashRadius, entityList);
					G_ResetTempTraceIgnoreEnts();

					for (e = 0; e < numListedEntities; e++)
					{
						hit = &g_entities[entityList[e]];

						if (hit->s.eType != ET_CONSTRUCTIBLE)
						{
							continue;
						}

						// invulnerable
						if (hit->spawnflags & CONSTRUCTIBLE_INVULNERABLE)
						{
							continue;
						}

						if (!G_ConstructionIsPartlyBuilt(hit))
						{
							continue;
						}

						// is it a friendly constructible
						if (hit->s.teamNum == traceEnt->s.teamNum)
						{
							// er, didnt we just pass this check earlier?
							//G_FreeEntity( traceEnt );
							//trap_SendServerCommand( ent-g_entities, "cp \"You cannot arm dynamite near a friendly construction!\" 1");
							//return;
							continue;
						}

						// not dynamite-able
						if (hit->constructibleStats.weaponclass < 1)
						{
							continue;
						}

						if (hit->parent)
						{
#ifdef FEATURE_OMNIBOT
							const char *Goalname = _GetEntityName(hit->parent);
#endif
							gentity_t *pm;

							pm = G_PopupMessage(PM_DYNAMITE);

							pm->s.effect2Time = 0;     // 0 = planted
							pm->s.effect3Time = hit->parent->s.teamNum;
							pm->s.teamNum     = ent->client->sess.sessionTeam;

							G_Script_ScriptEvent(hit, "dynamited", ent->client->sess.sessionTeam == TEAM_AXIS ? "axis" : "allies");

#ifdef FEATURE_OMNIBOT
							// notify omni-bot framework of planted dynamite
							hit->numPlanted += 1;
							Bot_AddDynamiteGoal(traceEnt, traceEnt->s.teamNum, va("%s_%i", Goalname, hit->numPlanted));
#endif

							if ((!(hit->parent->spawnflags & OBJECTIVE_DESTROYED)) &&
							    hit->s.teamNum && (hit->s.teamNum == ent->client->sess.sessionTeam))              // ==, as it's inverse
							{
								if (traceEnt->parent && traceEnt->parent->client)
								{
									G_LogPrintf("Dynamite_Plant: %d\n", (int)(traceEnt->parent - g_entities));
								}
								traceEnt->parent = ent;     // give explode score to guy who armed it
							}
							traceEnt->etpro_misc_1 |= 1;
						}
						return;
					}
				}
			}
			else
			{
				int dynamiteDropTeam;

				if (traceEnt->timestamp > level.time)
				{
					return;
				}
				if (traceEnt->health >= 248)         // have to do this so we don't score multiple times
				{
					return;
				}

				dynamiteDropTeam = traceEnt->s.teamNum;     // set this here since we wack traceent later but want teamnum for scoring

				if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 2)
				{
					traceEnt->health += 6;
				}
				else
				{
					traceEnt->health += 3;
				}

				G_PrintClientSpammyCenterPrint(ent - g_entities, "Defusing dynamite...");

				if (traceEnt->health >= 248)
				{
					int      scored     = 0;
					qboolean defusedObj = qfalse;

					traceEnt->health = 255;
					// Need some kind of event/announcement here

					//Add_Ammo( ent, WP_DYNAMITE, 1, qtrue );

					traceEnt->think     = G_FreeEntity;
					traceEnt->nextthink = level.time + FRAMETIME;

					VectorCopy(traceEnt->r.currentOrigin, origin);
					SnapVector(origin);
					VectorAdd(origin, traceEnt->r.mins, mins);
					VectorAdd(origin, traceEnt->r.maxs, maxs);
					num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

					// don't report if not disarming *enemy* dynamite in field
					/*                  if (dynamiteDropTeam == ent->client->sess.sessionTeam)
					                        return;*/

					// eh, why was this commented out? it makes sense, and prevents a sploit.
					if (dynamiteDropTeam == ent->client->sess.sessionTeam)
					{
						return;
					}

					for (i = 0 ; i < num ; i++)
					{
						hit = &g_entities[touch[i]];

						if (!(hit->r.contents & CONTENTS_TRIGGER))
						{
							continue;
						}
						if (hit->s.eType == ET_OID_TRIGGER)
						{

							if (!(hit->spawnflags & (AXIS_OBJECTIVE | ALLIED_OBJECTIVE)))
							{
								continue;
							}

							// spawnflags 128 = disabled (#309)
							if (hit->spawnflags & 128)
							{
								continue;
							}

							// prevent plant/defuse exploit near a/h cabinets or non-destroyable locations (bank doors on goldrush)
							if (!hit->target_ent || hit->target_ent->s.eType != ET_EXPLOSIVE)
							{
								continue;
							}

							if (ent->client->sess.sessionTeam == TEAM_AXIS)
							{
								if ((hit->spawnflags & AXIS_OBJECTIVE) && (!scored))
								{
									G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f);
									G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f, "defusing enemy dynamite");
									scored++;
								}
								if (hit->target_ent)
								{
									G_Script_ScriptEvent(hit->target_ent, "defused", "");
								}

								{
									gentity_t *pm;

									pm = G_PopupMessage(PM_DYNAMITE);

									pm->s.effect2Time = 1;     // 1 = defused
									pm->s.effect3Time = hit->s.teamNum;
									pm->s.teamNum     = ent->client->sess.sessionTeam;
								}

								//trap_SendServerCommand(-1, "cp \"Axis engineer disarmed the Dynamite!\n\"");
								defusedObj = qtrue;
							}
							else         // TEAM_ALLIES
							{
								if ((hit->spawnflags & ALLIED_OBJECTIVE) && (!scored))
								{
									G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f);
									G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f, "defusing enemy dynamite");
									scored++;
									hit->spawnflags &= ~OBJECTIVE_DESTROYED;     // "re-activate" objective since it wasn't destroyed
								}
								if (hit->target_ent)
								{
									G_Script_ScriptEvent(hit->target_ent, "defused", "");
								}

								{
									gentity_t *pm;

									pm = G_PopupMessage(PM_DYNAMITE);

									pm->s.effect2Time = 1;     // 1 = defused
									pm->s.effect3Time = hit->s.teamNum;
									pm->s.teamNum     = ent->client->sess.sessionTeam;
								}

								//trap_SendServerCommand(-1, "cp \"Allied engineer disarmed the Dynamite!\n\"");

								defusedObj = qtrue;
							}
						}
					}
					// prevent multiple messages here
					if (defusedObj)
					{
						return;
					}

					// reordered this check so its AFTER the primary obj check
					// - first see if the dynamite was planted near a constructable object that would have been destroyed
					{
						int    entityList[MAX_GENTITIES];
						int    numListedEntities;
						int    e;
						vec3_t org;

						VectorCopy(traceEnt->r.currentOrigin, org);
						org[2] += 4;        // move out of ground

						G_TempTraceIgnorePlayersAndBodies();
						numListedEntities = EntsThatRadiusCanDamage(org, traceEnt->splashRadius, entityList);
						G_ResetTempTraceIgnoreEnts();

						for (e = 0; e < numListedEntities; e++)
						{
							hit = &g_entities[entityList[e]];

							if (hit->s.eType != ET_CONSTRUCTIBLE)
							{
								continue;
							}

							// not completely build yet - NOTE: don't do this, in case someone places dynamite before construction is complete
							//if( hit->s.angles2[0] < 255 )
							//continue;

							// invulnerable
							if (hit->spawnflags & CONSTRUCTIBLE_INVULNERABLE)
							{
								continue;
							}

							// not dynamite-able
							if (hit->constructibleStats.weaponclass < 1)
							{
								continue;
							}

							// we got somthing to destroy
							if (ent->client->sess.sessionTeam == TEAM_AXIS)
							{
								if (hit->s.teamNum == TEAM_AXIS && (!scored))
								{
									G_LogPrintf("Dynamite_Diffuse: %d\n", (int)(ent - g_entities));
									G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f);
									G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f, "defusing enemy dynamite");
									scored++;
								}
								G_Script_ScriptEvent(hit, "defused", "");

								{
									gentity_t *pm;

									pm = G_PopupMessage(PM_DYNAMITE);

									pm->s.effect2Time = 1;     // 1 = defused
									pm->s.effect3Time = hit->parent->s.teamNum;
									pm->s.teamNum     = ent->client->sess.sessionTeam;
								}
								//trap_SendServerCommand(-1, "cp \"Axis engineer disarmed the Dynamite!\" 2");
							}
							else         // TEAM_ALLIES
							{
								if (hit->s.teamNum == TEAM_ALLIES && (!scored))
								{
									if (ent->client)
									{
										G_LogPrintf("Dynamite_Diffuse: %d\n", (int)(ent - g_entities));
									}
									G_AddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f);
									G_DebugAddSkillPoints(ent, SK_EXPLOSIVES_AND_CONSTRUCTION, 6.f, "defusing enemy dynamite");
									scored++;
								}
								G_Script_ScriptEvent(hit, "defused", "");

								{
									gentity_t *pm;

									pm = G_PopupMessage(PM_DYNAMITE);

									pm->s.effect2Time = 1;     // 1 = defused
									pm->s.effect3Time = hit->parent->s.teamNum;
									pm->s.teamNum     = ent->client->sess.sessionTeam;
								}
								//trap_SendServerCommand(-1, "cp \"Allied engineer disarmed the Dynamite!\" 2");
							}

							return;
						}
					}
				}
			}
		}
		else if (ent->client->touchingTOI)
		{
			if (TryConstructing(ent))
			{
				return;
			}
		}
	}
}

/**
 * Launch airstrike as line of bombs mostly-perpendicular to line of grenade travel
 * (close air support should *always* drop parallel to friendly lines, tho accidents do happen)
*/
extern void G_ExplodeMissile(gentity_t *ent);

/**
 * @brief G_AirStrikeExplode
 * @param[in,out] self
 */
void G_AirStrikeExplode(gentity_t *self)
{
	self->r.svFlags &= ~SVF_NOCLIENT;
	self->r.svFlags |= SVF_BROADCAST;

	self->think     = G_ExplodeMissile;
	self->nextthink = level.time + 50;
}

/**
 * @brief G_AvailableAirstrikes
 * @param[in] ent
 * @return
 */
qboolean G_AvailableAirstrikes(gentity_t *ent)
{
	if ((g_misc.integer & G_MISC_ARTY_STRIKE_COMBINE) && !G_AvailableArtillery(ent))
	{
		return qfalse;
	}

	if (ent->client->sess.sessionTeam == TEAM_AXIS)
	{
		if (level.axisBombCounter > 0)
		{
			return qfalse;
		}
	}
	else
	{
		if (level.alliedBombCounter > 0)
		{
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief Arty/airstrike rate limiting
 * @param[in] ent
 * @return
 */
qboolean G_AvailableArtillery(gentity_t *ent)
{
	if (ent->client->sess.sessionTeam == TEAM_AXIS)
	{
		if (level.axisArtyCounter > 0)
		{
			return qfalse;
		}
	}
	else
	{
		if (level.alliedArtyCounter > 0)
		{
			return qfalse;
		}
	}
	return qtrue;
}

/**
 * @brief G_AddAirstrikeToCounters
 * @param[in] ent
 */
void G_AddAirstrikeToCounters(gentity_t *ent)
{
	if (g_misc.integer & G_MISC_ARTY_STRIKE_COMBINE)
	{
		G_AddArtilleryToCounters(ent);
	}

	if (ent->client->sess.sessionTeam == TEAM_AXIS)
	{
		level.axisBombCounter += team_airstrikeTime.integer * 1000;
	}
	else
	{
		level.alliedBombCounter += team_airstrikeTime.integer * 1000;
	}
}

/**
 * @brief arty/airstrike rate limiting
 * @param[in] ent
 */
void G_AddArtilleryToCounters(gentity_t *ent)
{
	if (ent->client->sess.sessionTeam == TEAM_AXIS)
	{
		level.axisArtyCounter += team_artyTime.integer * 1000;
	}
	else
	{
		level.alliedArtyCounter += team_artyTime.integer * 1000;
	}
}

#define NUMBOMBS 10
#define BOMBSPREAD 150

/**
 * @brief weapon_checkAirStrikeThink1
 * @param[in,out] ent
 */
void weapon_checkAirStrikeThink1(gentity_t *ent)
{
	if (!weapon_checkAirStrike(ent))
	{
		ent->think     = G_ExplodeMissile;
		ent->nextthink = level.time + 1000;
		return;
	}

	ent->think     = weapon_callAirStrike;
	ent->nextthink = level.time + 1500;
}

/**
 * @brief weapon_checkAirStrikeThink2
 * @param[in,out] ent
 */
void weapon_checkAirStrikeThink2(gentity_t *ent)
{
	if (!weapon_checkAirStrike(ent))
	{
		ent->think     = G_ExplodeMissile;
		ent->nextthink = level.time + 1000;
		return;
	}

	ent->think     = weapon_callSecondPlane;
	ent->nextthink = level.time + 500;
}

/**
 * @brief weapon_callSecondPlane
 * @param[out] ent
 */
void weapon_callSecondPlane(gentity_t *ent)
{
	gentity_t *te;

	te = G_TempEntityNotLinked(EV_GLOBAL_SOUND);

	te->s.eventParm = GAMESOUND_WPN_AIRSTRIKE_PLANE;
	te->r.svFlags  |= SVF_BROADCAST;

	ent->nextthink = level.time + 1000;
	ent->think     = weapon_callAirStrike;
}

/**
 * @brief weapon_checkAirStrike
 * @param[in,out] ent
 * @return
 */
qboolean weapon_checkAirStrike(gentity_t *ent)
{
	if (ent->s.teamNum == TEAM_AXIS)
	{
		level.numActiveAirstrikes[0]++;
	}
	else
	{
		level.numActiveAirstrikes[1]++;
	}

	// cancel the airstrike if FF off and player joined spec
	// FIXME: this is a stupid workaround. Just store the parent team in the enitity itself and use that - no need to look up the parent
	if (!g_friendlyFire.integer && ent->parent->client && ent->parent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		ent->splashDamage = 0;  // no damage
		ent->think        = G_ExplodeMissile;
		ent->nextthink    = (int)(level.time + crandom() * 50);

		ent->active = qfalse;
		if (ent->s.teamNum == TEAM_AXIS)
		{
			level.numActiveAirstrikes[0]--;
		}
		else
		{
			level.numActiveAirstrikes[1]--;
		}
		return qfalse; // do nothing, don't hurt anyone
	}

	if (ent->s.teamNum == TEAM_AXIS || ent->s.teamNum == TEAM_ALLIES)
	{
		if (level.numActiveAirstrikes[ent->s.teamNum - 1] > 6 || !G_AvailableAirstrikes(ent->parent))
		{
			G_HQSay(ent->parent, COLOR_YELLOW, "HQ: ", "All available planes are already en-route.");

			G_GlobalClientEvent(EV_AIRSTRIKEMESSAGE, 0, ent->parent - g_entities);

			ent->active = qfalse;
			level.numActiveAirstrikes[ent->s.teamNum - 1]--;

			return qfalse;
		}
	}
	return qtrue;
}

/**
 * @brief weapon_callAirStrike
 * @param[in,out] ent
 */
void weapon_callAirStrike(gentity_t *ent)
{
	int       i, j;
	vec3_t    bombaxis, lookaxis, pos, bomboffset, fallaxis, temp, dir, skypoint;
	gentity_t *bomb;
	trace_t   tr;
	float     traceheight, bottomtraceheight;

	VectorCopy(ent->s.pos.trBase, bomboffset);
	bomboffset[2] += 4096.f;

	// turn off smoke grenade
	ent->think     = G_ExplodeMissile;
	ent->nextthink = (int)(level.time + 950 + NUMBOMBS * 100 + crandom() * 50); // 950 offset is for aircraft flyby

	ent->active = qtrue;

	G_AddAirstrikeToCounters(ent->parent);

	{
		gentity_t *te;

		te = G_TempEntityNotLinked(EV_GLOBAL_SOUND);

		te->s.eventParm = GAMESOUND_WPN_AIRSTRIKE_PLANE;
		te->r.svFlags  |= SVF_BROADCAST;
	}

	trap_Trace(&tr, ent->s.pos.trBase, NULL, NULL, bomboffset, ent->s.number, MASK_SHOT);
	if ((tr.fraction < 1.0f) && (!(tr.surfaceFlags & SURF_NOIMPACT)))           //SURF_SKY)) ) { // changed for trenchtoast foggie prollem
	{
		G_HQSay(ent->parent, COLOR_YELLOW, "Pilot: ", "Aborting, can't see target.");

		G_GlobalClientEvent(EV_AIRSTRIKEMESSAGE, 1, ent->parent - g_entities);

		if (ent->s.teamNum == TEAM_AXIS)
		{
			level.numActiveAirstrikes[0]--;
			level.axisBombCounter -= team_airstrikeTime.integer * 1000;
			if (level.axisBombCounter < 0)
			{
				level.axisBombCounter = 0;
			}
		}
		else
		{
			level.numActiveAirstrikes[1]--;
			level.alliedBombCounter -= team_airstrikeTime.integer * 1000;
			if (level.alliedBombCounter < 0)
			{
				level.alliedBombCounter = 0;
			}
		}
		ent->active = qfalse;
		return;
	}

	G_HQSay(ent->parent, COLOR_YELLOW, "Pilot: ", "Affirmative, on my way!");

	G_GlobalClientEvent(EV_AIRSTRIKEMESSAGE, 2, ent->parent - g_entities);

	VectorCopy(tr.endpos, bomboffset);
	VectorCopy(tr.endpos, skypoint);
	traceheight       = bomboffset[2];
	bottomtraceheight = traceheight - MAX_TRACE;

	VectorSubtract(ent->s.pos.trBase, ent->parent->client->ps.origin, lookaxis);
	lookaxis[2] = 0;
	VectorNormalize(lookaxis);

	dir[0] = 0;
	dir[1] = 0;
	dir[2] = crandom(); // generate either up or down vector
	VectorNormalize(dir);   // which adds randomness to pass direction below

	for (j = 0; j < ent->count; j++)
	{
		RotatePointAroundVector(bombaxis, dir, lookaxis, 90 + crandom() * 30);   // munge the axis line a bit so it's not totally perpendicular
		VectorNormalize(bombaxis);

		VectorCopy(bombaxis, pos);
		VectorScale(pos, (-.5f * BOMBSPREAD * NUMBOMBS), pos);
		VectorAdd(ent->s.pos.trBase, pos, pos);   // first bomb position
		VectorScale(bombaxis, BOMBSPREAD, bombaxis);   // bomb drop direction offset

		for (i = 0; i < NUMBOMBS; i++)
		{
			bomb               = G_Spawn();
			bomb->nextthink    = (int)(level.time + i * 100 + crandom() * 50 + 1000 + (j * 2000));    // 1000 for aircraft flyby, other term for tumble stagger
			bomb->think        = G_AirStrikeExplode;
			bomb->s.eType      = ET_MISSILE;
			bomb->r.svFlags    = SVF_NOCLIENT;
			bomb->s.weapon     = WP_SMOKE_MARKER;  // might wanna change this
			bomb->r.ownerNum   = ent->s.number;
			bomb->parent       = ent->parent;
			bomb->s.teamNum    = ent->s.teamNum;
			bomb->damage       = 400;  // maybe should un-hard-code these?
			bomb->splashDamage = 400;
			bomb->s.eFlags     = EF_SMOKINGBLACK; // add some client side smoke

			// for explosion type
			bomb->accuracy            = 2;
			bomb->classname           = "air strike";
			bomb->splashRadius        = 400;
			bomb->methodOfDeath       = MOD_AIRSTRIKE;
			bomb->splashMethodOfDeath = MOD_AIRSTRIKE;
			bomb->clipmask            = MASK_MISSILESHOT;
			bomb->s.pos.trType        = TR_STATIONARY; // was TR_GRAVITY,  might wanna go back to this and drop from height
			//bomb->s.pos.trTime = level.time;      // move a bit on the very first frame
			bomboffset[0] = crandom() * .5f * BOMBSPREAD;
			bomboffset[1] = crandom() * .5f * BOMBSPREAD;
			bomboffset[2] = 0.f;
			VectorAdd(pos, bomboffset, bomb->s.pos.trBase);

			VectorCopy(bomb->s.pos.trBase, bomboffset);   // make sure bombs fall "on top of" nonuniform scenery
			bomboffset[2] = traceheight;

			VectorCopy(bomboffset, fallaxis);
			fallaxis[2] = bottomtraceheight;

			trap_Trace(&tr, bomboffset, NULL, NULL, fallaxis, ent - g_entities, bomb->clipmask);
			if (tr.fraction != 1.0f)
			{
				VectorCopy(tr.endpos, bomb->s.pos.trBase);

				// Snap origin!
				VectorMA(bomb->s.pos.trBase, 2.f, tr.plane.normal, temp);
				SnapVectorTowards(bomb->s.pos.trBase, temp);            // save net bandwidth

				// G_RailTrail( skypoint, bomb->s.pos.trBase );
				trap_TraceNoEnts(&tr, skypoint, NULL, NULL, bomb->s.pos.trBase, 0, CONTENTS_SOLID);
				if (tr.fraction < 1.f)
				{
					G_FreeEntity(bomb);
					// move pos for next bomb
					VectorAdd(pos, bombaxis, pos);
					continue;
				}
			}

			VectorCopy(bomb->s.pos.trBase, bomb->r.currentOrigin);

			// move pos for next bomb
			VectorAdd(pos, bombaxis, pos);
		}
	}
}

/**
 * @brief Sound effect for spotter round, had to do this as half-second bomb warning
 * @param ent
 */
void artilleryThink_real(gentity_t *ent)
{
	ent->freeAfterEvent = qtrue;
	trap_LinkEntity(ent);

	switch (rand() % 3)
	{
	case 0:
		G_AddEvent(ent, EV_GENERAL_SOUND_VOLUME, GAMESOUND_WPN_ARTILLERY_FLY_1);
		ent->s.onFireStart = 255;
		break;
	case 1:
		G_AddEvent(ent, EV_GENERAL_SOUND_VOLUME, GAMESOUND_WPN_ARTILLERY_FLY_2);
		ent->s.onFireStart = 255;
		break;
	case 2:
		G_AddEvent(ent, EV_GENERAL_SOUND_VOLUME, GAMESOUND_WPN_ARTILLERY_FLY_3);
		ent->s.onFireStart = 255;
		break;
	default:
		break;
	}
}

/**
 * @brief artilleryThink
 * @param[out] ent
 */
void artilleryThink(gentity_t *ent)
{
	ent->think     = artilleryThink_real;
	ent->nextthink = level.time + FRAMETIME;

	ent->r.svFlags = SVF_BROADCAST;
}

/**
 * @brief Makes smoke disappear after a bit (just unregisters stuff)
 * @param[out] ent
 */
void artilleryGoAway(gentity_t *ent)
{
	ent->freeAfterEvent = qtrue;
	trap_LinkEntity(ent);
}

/**
 * @brief Generates some smoke debris
 * @param[in,out] ent
 */
void artillerySpotterThink(gentity_t *ent)
{
	gentity_t *bomb;
	vec3_t    tmpdir;
	int       i;

	ent->think     = G_ExplodeMissile;
	ent->nextthink = level.time + 1;
	SnapVector(ent->s.pos.trBase);

	for (i = 0; i < 7; i++)
	{
		bomb                    = G_Spawn();
		bomb->s.eType           = ET_MISSILE;
		bomb->r.svFlags         = 0;
		bomb->r.ownerNum        = ent->s.number;
		bomb->parent            = ent;
		bomb->s.teamNum         = ent->s.teamNum;
		bomb->nextthink         = (int)(level.time + 1000 + random() * 300);
		bomb->classname         = "WP";         // WP == White Phosphorous, so we can check for bounce noise in grenade bounce routine
		bomb->damage            = 0;          // maybe should un-hard-code these?
		bomb->splashDamage      = 0;
		bomb->splashRadius      = 0;
		bomb->s.weapon          = WP_SMOKETRAIL;
		bomb->think             = artilleryGoAway;
		bomb->s.eFlags         |= EF_BOUNCE;
		bomb->clipmask          = MASK_MISSILESHOT;
		bomb->s.pos.trType      = TR_GRAVITY;   // was TR_GRAVITY,  might wanna go back to this and drop from height
		bomb->s.pos.trTime      = level.time;   // move a bit on the very first frame
		bomb->s.otherEntityNum2 = ent->s.otherEntityNum2;
		VectorCopy(ent->s.pos.trBase, bomb->s.pos.trBase);
		tmpdir[0] = crandom();
		tmpdir[1] = crandom();
		tmpdir[2] = 1;
		VectorNormalize(tmpdir);
		tmpdir[2] = 1;           // extra up
		VectorScale(tmpdir, 500 + random() * 500, tmpdir);
		VectorCopy(tmpdir, bomb->s.pos.trDelta);
		SnapVector(bomb->s.pos.trDelta);            // save net bandwidth
		VectorCopy(ent->s.pos.trBase, bomb->s.pos.trBase);
		VectorCopy(ent->s.pos.trBase, bomb->r.currentOrigin);
	}
}

/**
 * @brief G_GlobalClientEvent
 * @param[in] event
 * @param[in] param
 * @param[in] client
 */
void G_GlobalClientEvent(entity_event_t event, int param, int client)
{
	gentity_t *tent;

	tent = G_TempEntity(vec3_origin, event);

	tent->s.density      = param;
	tent->r.singleClient = client;
	tent->r.svFlags      = SVF_SINGLECLIENT | SVF_BROADCAST;

	// Calling for a lot of artillery or airstrikes can result voice over spam
	tent->s.effect1Time = 1; // don't buffer
}

/**
 * @brief Weapon_Artillery
 * @param[in,out] ent
 */
void Weapon_Artillery(gentity_t *ent)
{
	trace_t   trace;
	int       i, count;
	vec3_t    muzzlePoint, end, bomboffset, pos, fallaxis;
	float     traceheight, bottomtraceheight;
	gentity_t *bomb, *bomb2;

	if (ent->client->ps.stats[STAT_PLAYER_CLASS] != PC_FIELDOPS)
	{
		G_Printf("not a fieldops, you can't shoot this!\n");
		return;
	}

	// FIXME: decide if we want to do charge costs for 'Insufficient fire support' calls
	//        and remove ReadyToCallArtillery() function
	// FIXME: check omnibot legacy mod interface to deal with this
	if (!ReadyToCallArtillery(ent))
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES)
	{
		if (!G_AvailableArtillery(ent))
		{
			G_HQSay(ent, COLOR_YELLOW, "Fire Mission: ", "Insufficient fire support.");
			ent->active = qfalse;

			G_GlobalClientEvent(EV_ARTYMESSAGE, 0, ent - g_entities);

			return;
		}
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	VectorCopy(ent->r.currentOrigin, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;

	VectorMA(muzzlePoint, 8192, forward, end);
	trap_Trace(&trace, muzzlePoint, NULL, NULL, end, ent->s.number, MASK_SHOT);

	if (trace.surfaceFlags & SURF_NOIMPACT)
	{
		return;
	}

	VectorCopy(trace.endpos, pos);
	VectorCopy(pos, bomboffset);
	bomboffset[2] += 4096;

	trap_Trace(&trace, pos, NULL, NULL, bomboffset, ent->s.number, MASK_SHOT);
	if (trace.fraction < 1.0f && !(trace.surfaceFlags & SURF_NOIMPACT)) // was SURF_SKY
	{
		G_HQSay(ent, COLOR_YELLOW, "Fire Mission: ", "Aborting, can't see target.");

		G_GlobalClientEvent(EV_ARTYMESSAGE, 1, ent - g_entities);

		return;
	}

	// arty/airstrike rate limiting.
	G_AddArtilleryToCounters(ent);

	G_HQSay(ent, COLOR_YELLOW, "Fire Mission: ", "Firing for effect!");

	G_GlobalClientEvent(EV_ARTYMESSAGE, 2, ent - g_entities);

	VectorCopy(trace.endpos, bomboffset);
	traceheight       = bomboffset[2];
	bottomtraceheight = traceheight - MAX_TRACE;

	// "spotter" round (i == 0)
	// i == 1->4 is regular explosives
	if (ent->client->sess.skill[SK_SIGNALS] >= 3)
	{
		count = 9;
	}
	else
	{
		count = 5;
	}

	for (i = 0; i < count; i++)
	{
		bomb                      = G_Spawn();
		bomb->s.eType             = ET_MISSILE;
		bomb->s.weapon            = WP_ARTY;   // might wanna change this
		bomb->r.ownerNum          = ent->s.number;
		bomb->s.clientNum         = ent->s.number;
		bomb->parent              = ent;
		bomb->s.teamNum           = ent->client->sess.sessionTeam;
		bomb->damage              = 0; // arty itself has no damage
		bomb->methodOfDeath       = MOD_ARTY;
		bomb->splashMethodOfDeath = MOD_ARTY;
		bomb->clipmask            = MASK_MISSILESHOT;
		bomb->s.pos.trType        = TR_STATIONARY;   // was TR_GRAVITY,  might wanna go back to this and drop from height
		bomb->s.pos.trTime        = level.time;      // move a bit on the very first frame

		if (i == 0)
		{
			bomb->think             = artillerySpotterThink;
			bomb->nextthink         = level.time + 5000;
			bomb->r.svFlags         = SVF_BROADCAST;
			bomb->classname         = "props_explosion"; // was "air strike"
			bomb->splashDamage      = 90;
			bomb->splashRadius      = 50;
			bomb->count             = 7;
			bomb->count2            = 1000;
			bomb->delay             = 300;
			bomb->s.otherEntityNum2 = 1; // first bomb

			// spotter round is always dead on (OK, unrealistic but more fun)
			bomboffset[0] = crandom() * 50; // was 0; changed per id request to prevent spotter round assassinations
			bomboffset[1] = crandom() * 50; // was 0;
			bomboffset[2] = 0;
		}
		else
		{
			bomb->think     = G_AirStrikeExplode;
			bomb->nextthink = (int)(level.time + 8950 + 2000 * i + crandom() * 800);
			bomb->r.svFlags = SVF_NOCLIENT;
			// for explosion type
			bomb->accuracy     = 2;
			bomb->classname    = "air strike";
			bomb->splashDamage = 400;
			bomb->splashRadius = 400;

			bomboffset[0] = crandom() * 250;
			bomboffset[1] = crandom() * 250;

			bomb->s.eFlags = EF_SMOKINGBLACK; // add some client side smoke, don't do this for bomb 0 (no all time smoke for spotter)
		}
		bomboffset[2] = 0;

		VectorAdd(pos, bomboffset, bomb->s.pos.trBase);

		VectorCopy(bomb->s.pos.trBase, bomboffset);  // make sure bombs fall "on top of" nonuniform scenery
		bomboffset[2] = traceheight;

		VectorCopy(bomboffset, fallaxis);
		fallaxis[2] = bottomtraceheight;

		trap_Trace(&trace, bomboffset, NULL, NULL, fallaxis, ent->s.number, MASK_SHOT);
		if (trace.fraction != 1.0f)
		{
			VectorCopy(trace.endpos, bomb->s.pos.trBase);
		}

		bomb->s.pos.trDelta[0] = 0; // might need to change this
		bomb->s.pos.trDelta[1] = 0;
		bomb->s.pos.trDelta[2] = 0;
		SnapVector(bomb->s.pos.trDelta);            // save net bandwidth
		VectorCopy(bomb->s.pos.trBase, bomb->r.currentOrigin);

		// build arty falling sound effect in front of bomb drop
		bomb2               = G_Spawn();
		bomb2->think        = artilleryThink;
		bomb2->s.eType      = ET_MISSILE;
		bomb2->r.svFlags    = SVF_NOCLIENT;
		bomb2->r.ownerNum   = ent->s.number;
		bomb2->parent       = ent;
		bomb2->s.teamNum    = ent->s.teamNum;
		bomb2->damage       = 0;
		bomb2->nextthink    = bomb->nextthink - 600;
		bomb2->classname    = "air strike";
		bomb2->clipmask     = MASK_MISSILESHOT;
		bomb2->s.pos.trType = TR_STATIONARY; // was TR_GRAVITY,  might wanna go back to this and drop from height
		bomb2->s.pos.trTime = level.time;    // move a bit on the very first frame
		VectorCopy(bomb->s.pos.trBase, bomb2->s.pos.trBase);
		VectorCopy(bomb->s.pos.trDelta, bomb2->s.pos.trDelta);
		VectorCopy(bomb->s.pos.trBase, bomb2->r.currentOrigin);
	}

	if (ent->client->sess.skill[SK_SIGNALS] >= 2)
	{
		if (level.time - ent->client->ps.classWeaponTime > level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1];
		}

		ent->client->ps.classWeaponTime += 0.66f * level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1];
	}
	else
	{
		ent->client->ps.classWeaponTime = level.time;
	}

	// weapon stats
#ifndef DEBUG_STATS
	if (g_gamestate.integer == GS_PLAYING)
#endif
	ent->client->sess.aWeaponStats[WS_ARTILLERY].atts++;
#ifdef FEATURE_OMNIBOT
	// Omni-bot - Send a fire event.
	Bot_Event_FireWeapon(ent - g_entities, Bot_WeaponGameToBot(WP_ARTY), 0);
#endif
}

#define SMOKEBOMB_GROWTIME 1000
#define SMOKEBOMB_SMOKETIME 15000
#define SMOKEBOMB_POSTSMOKETIME 2000

/**
 * @brief Increases postsmoke time from 2000->32000, this way, the entity
 * is still around while the smoke is around, so we can check if it blocks bot's vision
 * - eeeeeh this is wrong. 32 seconds is way too long. Also - we shouldn't be
 * rendering the grenade anymore after the smoke stops and definately not send it to the client
 * - back to the old value 2000, now that it looks like smoke disappears more
 * quickly
 *
 * @param[in,out] ent
 */
void weapon_smokeBombExplode(gentity_t *ent)
{
	int lived = 0;

	if (!ent->grenadeExplodeTime)
	{
		ent->grenadeExplodeTime = level.time;
	}

	lived          = level.time - ent->grenadeExplodeTime;
	ent->nextthink = level.time + FRAMETIME;

	if (lived < SMOKEBOMB_GROWTIME)
	{
		// Just been thrown, increase radius
		ent->s.effect1Time = (int)(16 + lived * ((640 - 16) / (float)SMOKEBOMB_GROWTIME));
	}
	else if (lived < SMOKEBOMB_SMOKETIME + SMOKEBOMB_GROWTIME)
	{
		// Smoking
		ent->s.effect1Time = 640;
	}
	else if (lived < SMOKEBOMB_SMOKETIME + SMOKEBOMB_GROWTIME + SMOKEBOMB_POSTSMOKETIME)
	{
		// Dying out
		ent->s.effect1Time = -1;
	}
	else
	{
		// Poof and it's gone
		G_FreeEntity(ent);
	}
}

/*
======================================================================
MACHINEGUN
======================================================================
*/

/*
======================
SnapVectorTowards


======================
*/



/**
 * @brief Round a vector to integers for more efficient network
 * transmission, but make sure that it rounds towards a given point
 * rather than blindly truncating.  This prevents it from truncating
 * into a wall.
 *
 * @param[in,out] v
 * @param[out] to
 *
 * @note Modified so it doesn't have trouble with negative locations (quadrant problems)
 * (this was causing some problems with bullet marks appearing since snapping
 * too far off the target surface causes the the distance between the transmitted impact
 * point and the actual hit surface larger than the mark radius.  (so nothing shows) )
 */
void SnapVectorTowards(vec3_t v, vec3_t to)
{
	int i;

	for (i = 0 ; i < 3 ; i++)
	{
		if (to[i] <= v[i])
		{
			//          v[i] = (int)v[i];
			v[i] = (float)floor(v[i]);
		}
		else
		{
			//          v[i] = (int)v[i] + 1;
			v[i] = (float)ceil(v[i]);
		}
	}
}

/**
 * @brief See if a new particle emitter should be created at the bullet impact point
 * @param[in] ent
 * @param[in] attacker
 * @param[in] tr
 */
void EmitterCheck(gentity_t *ent, gentity_t *attacker, trace_t *tr)
{
	vec3_t origin;

	VectorCopy(tr->endpos, origin);
	SnapVectorTowards(tr->endpos, attacker->s.origin);

	if (Q_stricmp(ent->classname, "func_leaky") == 0)
	{
		gentity_t *tent;

		tent = G_TempEntity(origin, EV_EMITTER);

		VectorCopy(origin, tent->s.origin);
		tent->s.time    = 1234;
		tent->s.density = 9876;
		VectorCopy(tr->plane.normal, tent->s.origin2);
	}
}

/**
 * @brief Find target end position for bullet trace based on entities weapon and accuracy
 *
 * @param[in] ent
 * @param[in] spread
 * @param[out] end
 */
void Bullet_Endpos(gentity_t *ent, float spread, vec3_t *end)
{
	if (weaponTable[ent->s.weapon].isScoped)
	{
		// aim dir already accounted for sway of scoped weapons in CalcMuzzlePoints()
		VectorMA(muzzleTrace, 2 * MAX_TRACE, forward, *end);
	}
	else
	{
		VectorMA(muzzleTrace, MAX_TRACE, forward, *end);

		// random spread
		VectorMA(*end, (crandom() * spread), right, *end);
		VectorMA(*end, (crandom() * spread), up, *end);
	}
}

/**
 * @brief Bullet_Fire
 * @param[in] ent
 * @param[in] spread
 * @param[in] damage
 * @param[in] distance_falloff
 */
void Bullet_Fire(gentity_t *ent, float spread, int damage, qboolean distance_falloff)
{
	vec3_t end;

	switch (ent->s.weapon)
	{
	// light weapons
	case WP_LUGER:
	case WP_COLT:
	case WP_MP40:
	case WP_THOMPSON:
	case WP_STEN:
	case WP_SILENCER:
	case WP_SILENCED_COLT:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDLUGER:
	case WP_AKIMBO_SILENCEDCOLT:
		// increase in accuracy (spread reduction) at level 3
		if (ent->client->sess.skill[SK_LIGHT_WEAPONS] >= 3)
		{
			spread *= .65f;
		}
		break;
	default:
		break;
	}

	Bullet_Endpos(ent, spread, &end);

	G_HistoricalTraceBegin(ent);

	Bullet_Fire_Extended(ent, ent, muzzleTrace, end, damage, distance_falloff);

	G_HistoricalTraceEnd(ent);
}

/**
 * @brief A modified Bullet_Fire with more parameters.
 *
 * @details The original Bullet_Fire still passes through here and functions as it always has.
 *
 * Uses for this include shooting through entities (windows, doors, other players, etc.) and reflecting bullets
 *
 * @param[in] source
 * @param[in] attacker
 * @param[in] start
 * @param[in] end
 * @param[in] damage
 * @param[in] distance_falloff
 * @return
 */
qboolean Bullet_Fire_Extended(gentity_t *source, gentity_t *attacker, vec3_t start, vec3_t end, int damage, qboolean distance_falloff)
{
	trace_t   tr;
	gentity_t *tent;
	gentity_t *traceEnt;
	qboolean  hitClient = qfalse;
	qboolean  waslinked = qfalse;

	// prevent shooting ourselves in the head when prone, firing through a breakable
	if (g_entities[attacker->s.number].client && g_entities[attacker->s.number].r.linked == qtrue)
	{
		g_entities[attacker->s.number].r.linked = qfalse;
		waslinked                               = qtrue;
	}

	G_Trace(source, &tr, start, NULL, NULL, end, source->s.number, MASK_SHOT, !GetWeaponTableData(attacker->s.weapon)->canGib);

	// prevent shooting ourselves in the head when prone, firing through a breakable
	if (waslinked == qtrue)
	{
		g_entities[attacker->s.number].r.linked = qtrue;
	}

	// bullet debugging using Q3A's railtrail
	if (g_debugBullets.integer & 1)
	{
		tent = G_TempEntity(start, EV_RAILTRAIL);
		VectorCopy(tr.endpos, tent->s.origin2);
		tent->s.otherEntityNum2 = attacker->s.number;
	}

	traceEnt = &g_entities[tr.entityNum];

	EmitterCheck(traceEnt, attacker, &tr);

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards(tr.endpos, start);

	if (distance_falloff)
	{
		vec_t  dist;
		vec3_t shotvec;
		float  scale;

		//VectorSubtract( tr.endpos, start, shotvec );
		VectorSubtract(tr.endpos, muzzleTrace, shotvec);

		dist = VectorLength(shotvec);
		// ~~~---______
		// start at 100% at 1500 units (and before),
		// and go to 50% at 2500 units (and after)

		// 1500 to 2500 -> 0.0 to 1.0
		scale = (dist - 1500.f) / (2500.f - 1500.f);
		// 0.0 to 1.0 -> 0.0 to 0.5
		scale *= 0.5f;
		// 0.0 to 0.5 -> 1.0 to 0.5
		scale = 1.0f - scale;

		// And, finally, cap it.
		if (scale >= 1.0f)
		{
			scale = 1.0f;
		}
		else if (scale < 0.5f)
		{
			scale = 0.5f;
		}

		damage *= scale;
	}

	// send bullet impact
	if (traceEnt->takedamage && traceEnt->client)
	{
		tent              = G_TempEntity(tr.endpos, EV_BULLET_HIT_FLESH);
		tent->s.eventParm = traceEnt->s.number;

		if (AccuracyHit(traceEnt, attacker))
		{
			hitClient = qtrue;
		}

		if (g_debugBullets.integer >= 2)       // show hit player bb
		{
			gentity_t *bboxEnt;
			vec3_t    b1, b2;

			VectorCopy(traceEnt->r.currentOrigin, b1);
			VectorCopy(traceEnt->r.currentOrigin, b2);
			VectorAdd(b1, traceEnt->r.mins, b1);
			VectorAdd(b2, traceEnt->r.maxs, b2);
			bboxEnt = G_TempEntity(b1, EV_RAILTRAIL);
			VectorCopy(b2, bboxEnt->s.origin2);
			bboxEnt->s.dmgFlags = 1;    // ("type")
		}
	}
	else
	{
		trace_t tr2;
		// bullet impact should reflect off surface
		vec3_t reflect;
		float  dot;

		if (g_debugBullets.integer <= -2)      // show hit thing bb
		{
			gentity_t *bboxEnt;
			vec3_t    b1, b2;
			vec3_t    maxs;

			VectorCopy(traceEnt->r.currentOrigin, b1);
			VectorCopy(traceEnt->r.currentOrigin, b2);
			VectorAdd(b1, traceEnt->r.mins, b1);
			VectorCopy(traceEnt->r.maxs, maxs);
			maxs[2] = ClientHitboxMaxZ(traceEnt);
			VectorAdd(b2, maxs, b2);
			bboxEnt = G_TempEntity(b1, EV_RAILTRAIL);
			VectorCopy(b2, bboxEnt->s.origin2);
			bboxEnt->s.dmgFlags = 1;    // ("type")
		}

		tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_WALL);

		G_Trace(source, &tr2, start, NULL, NULL, end, source->s.number, MASK_WATER | MASK_SHOT, !GetWeaponTableData(attacker->s.weapon)->canGib);

		if ((tr.entityNum != tr2.entityNum && tr2.fraction != 1.f))
		{
			vec3_t v;

			VectorSubtract(tr.endpos, start, v);

			tent->s.origin2[0] = (8192 * tr2.fraction) / VectorLength(v);
		}
		else
		{
			tent->s.origin2[0] = 0;
		}

		dot = DotProduct(forward, tr.plane.normal);
		VectorMA(forward, -2 * dot, tr.plane.normal, reflect);
		VectorNormalize(reflect);

		tent->s.eventParm       = DirToByte(reflect);
		tent->s.otherEntityNum2 = ENTITYNUM_NONE;
	}
	tent->s.otherEntityNum = attacker->s.number;

	if (traceEnt->takedamage)
	{
		G_Damage(traceEnt, attacker, attacker, forward, tr.endpos, damage, (distance_falloff ? DAMAGE_DISTANCEFALLOFF : 0), GetAmmoTableData(attacker->s.weapon)->mod);

		// allow bullets to "pass through" func_explosives if they break by taking another simultanious shot
		if (traceEnt->s.eType == ET_EXPLOSIVE)
		{
			if (traceEnt->health <= damage)
			{
				// start new bullet at position this hit the bmodel and continue to the end position (ignoring shot-through bmodel in next trace)
				// spread = 0 as this is an extension of an already spread shot
				return Bullet_Fire_Extended(traceEnt, attacker, tr.endpos, end, damage, distance_falloff);
			}
		}
	}
	return hitClient;
}

/**
======================================================================
GRENADE LAUNCHER
  700 has been the standard direction multiplier in fire_grenade()
======================================================================
*/

/**
 * @brief weapon_gpg40_fire
 * @param[in] ent
 * @param[in] grenType
 * @return
 */
gentity_t *weapon_gpg40_fire(gentity_t *ent, int grenType)
{
	trace_t tr;
	vec3_t  viewpos;
	vec3_t  tosspos;
	vec3_t  orig_viewpos; // to prevent nade-through-teamdoor sploit

	AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);

	VectorCopy(muzzleEffect, tosspos);

	// check for valid start spot (so you don't throw through or get stuck in a wall)
	VectorCopy(ent->s.pos.trBase, viewpos);
	viewpos[2] += ent->client->ps.viewheight;
	VectorCopy(viewpos, orig_viewpos);      // to prevent nade-through-teamdoor sploit
	VectorMA(viewpos, 32, forward, viewpos);

	// to prevent nade-through-teamdoor sploit
	trap_Trace(&tr, orig_viewpos, tv(-4.f, -4.f, 0.f), tv(4.f, 4.f, 6.f), viewpos, ent->s.number, MASK_MISSILESHOT);
	if (tr.fraction < 1)     // oops, bad launch spot
	{
		VectorCopy(tr.endpos, tosspos);
		SnapVectorTowards(tosspos, orig_viewpos);
	}
	else
	{
		trap_Trace(&tr, viewpos, tv(-4.f, -4.f, 0.f), tv(4.f, 4.f, 6.f), tosspos, ent->s.number, MASK_MISSILESHOT);
		if (tr.fraction < 1)     // oops, bad launch spot
		{
			VectorCopy(tr.endpos, tosspos);
			SnapVectorTowards(tosspos, viewpos);
		}
	}

	VectorScale(forward, 2000, forward);

	// return the grenade so we can do some prediction before deciding if we really want to throw it or not
	return fire_grenade(ent, tosspos, forward, grenType);
}

/**
 * @brief weapon_mortar_fire
 * @param[in] ent
 * @param[in] grenType
 * @return
 */
gentity_t *weapon_mortar_fire(gentity_t *ent, int grenType)
{
	trace_t tr;
	vec3_t  launchPos, testPos;
	vec3_t  angles;

	VectorCopy(ent->client->ps.viewangles, angles);
	angles[PITCH] -= 60.f;
	//if( angles[PITCH] < -89.f )
	//  angles[PITCH] = -89.f;
	AngleVectors(angles, forward, NULL, NULL);

	VectorCopy(muzzleEffect, launchPos);

	// check for valid start spot (so you don't throw through or get stuck in a wall)
	VectorMA(launchPos, 32, forward, testPos);

	forward[0] *= 3000 * 1.1f;
	forward[1] *= 3000 * 1.1f;
	forward[2] *= 1500 * 1.1f;

	trap_Trace(&tr, testPos, tv(-4.f, -4.f, 0.f), tv(4.f, 4.f, 6.f), launchPos, ent->s.number, MASK_MISSILESHOT);

	if (tr.fraction < 1)     // oops, bad launch spot
	{
		VectorCopy(tr.endpos, launchPos);
		SnapVectorTowards(launchPos, testPos);
	}

	return fire_grenade(ent, launchPos, forward, grenType);
}

/**
 * @brief weapon_grenadelauncher_fire
 * @param[in] ent
 * @param[in] grenType
 * @return
 */
gentity_t *weapon_grenadelauncher_fire(gentity_t *ent, int grenType)
{
	trace_t  tr;
	vec3_t   viewpos;
	float    upangle = 0, pitch = ent->s.apos.trBase[0];   // start with level throwing and adjust based on angle
	vec3_t   tosspos;
	qboolean underhand = qtrue;

	// smoke grenades always overhand
	if (pitch >= 0)
	{
		forward[2] += 0.5f;
		// Used later in underhand boost
		pitch = 1.3f;
	}
	else
	{
		pitch       = -pitch;
		pitch       = MIN(pitch, 30);
		pitch      /= 30.f;
		pitch       = 1 - pitch;
		forward[2] += (pitch * 0.5f);

		// Used later in underhand boost
		pitch *= 0.3f;
		pitch += 1.f;
	}

	VectorNormalizeFast(forward);         // make sure forward is normalized

	upangle  = -(ent->s.apos.trBase[0]);  // this will give between  -90 / 90
	upangle  = MIN(upangle, 50);
	upangle  = MAX(upangle, -50);         //    now clamped to  -50 / 50    (don't allow firing straight up/down)
	upangle  = upangle / 100.0f;          //                   -0.5 / 0.5
	upangle += 0.5f;                      //                    0.0 / 1.0

	if (upangle < .1f)
	{
		upangle = .1f;
	}

	switch (grenType)
	{
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_SMOKE_MARKER:
	case WP_SMOKE_BOMB:
		upangle *= 900;
		break;
	default:  // WP_DYNAMITE / WP_LANDMINE / WP_SATCHEL
		upangle *= 400;
		break;
	}

	VectorCopy(muzzleEffect, tosspos);

	if (underhand)
	{
		// move a little bit more away from the player (so underhand tosses don't get caught on nearby lips)
		VectorMA(muzzleEffect, 8, forward, tosspos);
		tosspos[2] -= 8;    // lower origin for the underhand throw
		upangle    *= pitch;
		SnapVector(tosspos);
	}

	VectorScale(forward, upangle, forward);

	// check for valid start spot (so you don't throw through or get stuck in a wall)
	VectorCopy(ent->s.pos.trBase, viewpos);
	viewpos[2] += ent->client->ps.viewheight;

	if (grenType == WP_DYNAMITE || grenType == WP_SATCHEL)
	{
		trap_Trace(&tr, viewpos, tv(-12.f, -12.f, 0.f), tv(12.f, 12.f, 20.f), tosspos, ent->s.number, MASK_MISSILESHOT);
	}
	else if (grenType == WP_LANDMINE)
	{
		trap_Trace(&tr, viewpos, tv(-16.f, -16.f, 0.f), tv(16.f, 16.f, 16.f), tosspos, ent->s.number, MASK_MISSILESHOT);
	}
	else
	{
		trap_Trace(&tr, viewpos, tv(-4.f, -4.f, 0.f), tv(4.f, 4.f, 6.f), tosspos, ent->s.number, MASK_MISSILESHOT);
	}

	if (tr.startsolid)
	{
		// this code is a bit more solid than the previous code
		VectorCopy(forward, viewpos);
		VectorNormalizeFast(viewpos);
		VectorMA(ent->r.currentOrigin, -24.f, viewpos, viewpos);

		if (grenType == WP_DYNAMITE || grenType == WP_SATCHEL)
		{
			trap_Trace(&tr, viewpos, tv(-12.f, -12.f, 0.f), tv(12.f, 12.f, 20.f), tosspos, ent->s.number, MASK_MISSILESHOT);
		}
		else if (grenType == WP_LANDMINE)
		{
			trap_Trace(&tr, viewpos, tv(-16.f, -16.f, 0.f), tv(16.f, 16.f, 16.f), tosspos, ent->s.number, MASK_MISSILESHOT);
		}
		else
		{
			trap_Trace(&tr, viewpos, tv(-4.f, -4.f, 0.f), tv(4.f, 4.f, 6.f), tosspos, ent->s.number, MASK_MISSILESHOT);
		}

		VectorCopy(tr.endpos, tosspos);
	}
	else if (tr.fraction < 1)        // oops, bad launch spot
	{
		VectorCopy(tr.endpos, tosspos);
		SnapVectorTowards(tosspos, viewpos);
	}

	// return the grenade so we can do some prediction before deciding if we really want to throw it or not
	return fire_grenade(ent, tosspos, forward, grenType);
}

/**
======================================================================
ANTI TANK ROCKETS
======================================================================
*/

/**
 * @brief fires a rocket of type WP_BAZOOKA or WP_PANZERFAUST
 * @param[in] ent
 * @return
 */
gentity_t *weapon_antitank_fire(gentity_t *ent)
{
	//VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
	return fire_rocket(ent, muzzleEffect, forward, ent->s.weapon);
}

/**
======================================================================
FLAMETHROWER
======================================================================
*/

/**
 * @brief BurnMeGood now takes the flamechunk separately, because
 * the old 'set-self-in-flames' method doesn't have a flamechunk to
 * pass, and deaths were getting blamed on the world/player 0
 *
 * @param[in] self
 * @param[in,out] body
 * @param[in] chunk
 */
void G_BurnMeGood(gentity_t *self, gentity_t *body, gentity_t *chunk)
{
	vec3_t origin;

	// add the new damage
	body->flameQuota    += 5;
	body->flameQuotaTime = level.time;

	// fill in our own origin if we have no flamechunk
	if (chunk != NULL)
	{
		VectorCopy(chunk->r.currentOrigin, origin);
	}
	else
	{
		VectorCopy(self->r.currentOrigin, origin);
	}

	// yet another flamethrower damage model, trying to find a feels-good damage combo that isn't overpowered
	if (body->lastBurnedFrameNumber != level.framenum)
	{
		G_Damage(body, self, self, vec3_origin, origin, GetWeaponTableData(WP_FLAMETHROWER)->damage, 0, MOD_FLAMETHROWER);
		body->lastBurnedFrameNumber = level.framenum;
	}

	// make em burn
	if (body->client && (body->health <= 0 || body->flameQuota > 0)) // was > FLAME_THRESHOLD
	{
		if (body->s.onFireEnd < level.time)
		{
			body->s.onFireStart = level.time;
		}
		body->s.onFireEnd = level.time + FIRE_FLASH_TIME;
		// use ourself as the attacker if we have no flamechunk
		body->flameBurnEnt = (chunk != NULL) ? chunk->r.ownerNum : self->s.number;
		// add to playerState for client-side effect
		body->client->ps.onFireStart = level.time;
	}
}

// for traces calls
static vec3_t flameChunkMins = { -4, -4, -4 };
static vec3_t flameChunkMaxs = { 4, 4, 4 };

/**
 * @brief Weapon_FlamethrowerFire
 * @param[in,out] ent
 * @return
 */
gentity_t *Weapon_FlamethrowerFire(gentity_t *ent)
{
	vec3_t    start;
	vec3_t    trace_start;
	vec3_t    trace_end;
	trace_t   trace;
	gentity_t *traceEnt;

	VectorCopy(ent->r.currentOrigin, start);
	start[2] += ent->client->ps.viewheight;
	VectorCopy(start, trace_start);

	VectorMA(start, -8, forward, start);
	VectorMA(start, 10, right, start);
	VectorMA(start, -6, up, start);

	// prevent flame thrower cheat, run & fire while aiming at the ground, don't get hurt
	// 72 total box height, 18 xy -> 77 trace radius (from view point towards the ground) is enough to cover the area around the feet
	VectorMA(trace_start, 77.0f, forward, trace_end);
	trap_Trace(&trace, trace_start, flameChunkMins, flameChunkMaxs, trace_end, ent->s.number, MASK_SHOT | MASK_WATER);
	if (trace.fraction != 1.0f)
	{
		// additional checks to filter out false positives
		if (trace.endpos[2] > (ent->r.currentOrigin[2] + ent->r.mins[2] - 8) && trace.endpos[2] < ent->r.currentOrigin[2])
		{
			// trigger in a 21 radius around origin
			trace_start[0] -= trace.endpos[0];
			trace_start[1] -= trace.endpos[1];
			if (trace_start[0] * trace_start[0] + trace_start[1] * trace_start[1] < 441)
			{
				// set self in flames
				G_BurnMeGood(ent, ent, NULL);
			}
		}
	}

	traceEnt = fire_flamechunk(ent, start, forward);

	// flamethrower exploit fix
	ent->r.svFlags        |= SVF_BROADCAST;
	ent->client->flametime = level.time + 2500;
	return traceEnt;
}

//======================================================================

/**
 * @brief Add leaning offset
 * @param ent
 * @param point
 */
void AddLean(gentity_t *ent, vec3_t point)
{
	if (ent->client)
	{
		if (ent->client->ps.leanf != 0.f)
		{
			vec3_t right;

			AngleVectors(ent->client->ps.viewangles, NULL, right, NULL);
			VectorMA(point, ent->client->ps.leanf, right, point);
		}
	}
}

/**
 * @brief AccuracyHit
 * @param[in] target
 * @param[in] attacker
 * @return
 */
qboolean AccuracyHit(gentity_t *target, gentity_t *attacker)
{
	if (!target->takedamage)
	{
		return qfalse;
	}

	if (!attacker)
	{
		return qfalse;
	}

	if (target == attacker)
	{
		return qfalse;
	}

	if (!target->client)
	{
		return qfalse;
	}

	if (!attacker->client)
	{
		return qfalse;
	}

	if (target->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return qfalse;
	}

	if (OnSameTeam(target, attacker))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Set muzzle location relative to pivoting eye
 * @param[in] ent
 * @param[in] weapon
 * @param forward - unused
 * @param[in] right
 * @param[in] up
 * @param[out] muzzlePoint
 */
void CalcMuzzlePoint(gentity_t *ent, int weapon, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	VectorCopy(ent->r.currentOrigin, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	// this puts the start point outside the bounding box, isn't necessary
	//VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// offset for more realistic firing from actual gun position
	switch (weapon)    // changed this so I can predict weapons
	{
	case WP_PANZERFAUST:
	case WP_BAZOOKA:
		VectorMA(muzzlePoint, 10, right, muzzlePoint);
		break;
	case WP_DYNAMITE:
	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
	case WP_SATCHEL:
	case WP_SMOKE_BOMB:
		VectorMA(muzzlePoint, 20, right, muzzlePoint);
		break;
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
		VectorMA(muzzlePoint, -6, right, muzzlePoint);
		VectorMA(muzzlePoint, -4, up, muzzlePoint);
		break;
	default:
		VectorMA(muzzlePoint, 6, right, muzzlePoint);
		VectorMA(muzzlePoint, -4, up, muzzlePoint);
		break;
	}

	// actually, this is sort of moot right now since
	// you're not allowed to fire when leaning.  Leave in
	// in case we decide to enable some lean-firing.
	// - works with gl now
	//AddLean(ent, muzzlePoint);

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

/**
 * @brief For activate
 * @param[in] ent
 * @param forward - unused
 * @param right - unused
 * @param up - unused
 * @param[out] muzzlePoint
 */
void CalcMuzzlePointForActivate(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	VectorCopy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;

	AddLean(ent, muzzlePoint);

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

/**
 * @brief CalcMuzzlePoints
 * @param[in] ent
 * @param[in] weapon
 */
void CalcMuzzlePoints(gentity_t *ent, int weapon)
{
	vec3_t viewang;

	VectorCopy(ent->client->ps.viewangles, viewang);

	// non ai's take into account scoped weapon 'sway' (just another way aimspread is visualized/utilized)

	if (weaponTable[weapon].isScoped)
	{
		float pitchMinAmp, yawMinAmp, phase;

		if (weapon == WP_FG42SCOPE)
		{
			pitchMinAmp = 4 * ZOOM_PITCH_MIN_AMPLITUDE;
			yawMinAmp   = 4 * ZOOM_YAW_MIN_AMPLITUDE;
		}
		else
		{
			pitchMinAmp = ZOOM_PITCH_MIN_AMPLITUDE;
			yawMinAmp   = ZOOM_YAW_MIN_AMPLITUDE;
		}

		// rotate 'forward' vector by the sway
		phase           = level.time / 1000.0f * ZOOM_PITCH_FREQUENCY * (float)M_PI * 2;
		viewang[PITCH] += ZOOM_PITCH_AMPLITUDE * (float)sin((double)phase) * (ent->client->currentAimSpreadScale + pitchMinAmp);

		phase         = level.time / 1000.0f * ZOOM_YAW_FREQUENCY * (float)M_PI * 2;
		viewang[YAW] += ZOOM_YAW_AMPLITUDE * (float)sin((double)phase) * (ent->client->currentAimSpreadScale + yawMinAmp);
	}

	// set aiming directions
	AngleVectors(viewang, forward, right, up);

	// modified the muzzle stuff so that weapons that need to fire down a perfect trace
	// straight out of the camera (SP5, Mauser right now) can have that accuracy, but
	// weapons that need an offset effect (bazooka/grenade/etc.) can still look like
	// they came out of the weap.
	CalcMuzzlePointForActivate(ent, forward, right, up, muzzleTrace);
	CalcMuzzlePoint(ent, weapon, forward, right, up, muzzleEffect);
}

/**
 * @brief G_PlayerCanBeSeenByOthers
 * @param[in] ent
 * @return
 */
qboolean G_PlayerCanBeSeenByOthers(gentity_t *ent)
{
	int       i;
	gentity_t *ent2;
	vec3_t    pos[3];

	VectorCopy(ent->client->ps.origin, pos[0]);
	VectorCopy(ent->client->ps.mins, pos[1]);
	VectorCopy(ent->client->ps.maxs, pos[2]);

	for (i = 0, ent2 = g_entities; i < level.maxclients; i++, ent2++)
	{
		if (!ent2->inuse || ent2 == ent)
		{
			continue;
		}

		if (ent2->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		if (ent2->health <= 0 ||
		    ent2->client->sess.sessionTeam == ent->client->sess.sessionTeam)
		{
			continue;
		}

		if (ent2->client->ps.eFlags & EF_ZOOMING)
		{
			G_SetupFrustum_ForBinoculars(ent2);
		}
		else
		{
			G_SetupFrustum(ent2);
		}

		if (G_VisibleFromBinoculars_Box(ent2, ent, pos[0], pos[1], pos[2]))
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief FireWeapon
 * @param[in,out] ent
 */
void FireWeapon(gentity_t *ent)
{
	gentity_t *pFiredShot = 0;   // Omni-bot To tell bots about projectiles
	float     aimSpreadScale;
#ifdef FEATURE_OMNIBOT
	qboolean callEvent = qtrue;
#endif

	// dead guys don't fire guns
	if (ent->client->ps.pm_type == PM_DEAD)
	{
		return;
	}

	// mg42
	if (ent->client->ps.persistant[PERS_HWEAPON_USE] && ent->active)
	{
		return;
	}

	// need to call this for AI prediction also
	CalcMuzzlePoints(ent, ent->s.weapon);

	if (g_userAim.integer)
	{
		aimSpreadScale = ent->client->currentAimSpreadScale;
		// add accuracy factor for AI
		aimSpreadScale += 0.15f; // just adding a temp /maximum/ accuracy for player (this will be re-visited in greater detail :)
		if (aimSpreadScale > 1)
		{
			aimSpreadScale = 1.0f;  // still cap at 1.0
		}
	}
	else
	{
		aimSpreadScale = 1.0f;
	}

	if ((ent->client->ps.eFlags & EF_ZOOMING) && (ent->client->ps.stats[STAT_KEYS] & (1 << INV_BINOCS)))
	{
		if (ent->client->sess.playerType == PC_FIELDOPS)
		{
			if (ent->client->ps.leanf == 0.f)
			{
				Weapon_Artillery(ent);
			}
			return;
		}
	}

	if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		aimSpreadScale = 2.0f;
	}

	// covert ops disguise handling
	if (ent->client->ps.powerups[PW_OPS_DISGUISED])
	{
		switch (ent->s.weapon)
		{
		case WP_SMOKE_BOMB:     // never loose disguise
		case WP_SATCHEL:
		case WP_SATCHEL_DET:
		case WP_BINOCULARS:
			break;
		case WP_KNIFE:           // in case of covert ops weapon disguise is lost when seen by others AND 'low noise' weapon is used
		case WP_KNIFE_KABAR:
		case WP_STEN:
		case WP_SILENCER:
		case WP_SILENCED_COLT:
		case WP_AKIMBO_SILENCEDCOLT:
		case WP_AKIMBO_SILENCEDLUGER:
		case WP_K43:
		case WP_K43_SCOPE:
		case WP_GARAND:
		case WP_GARAND_SCOPE:
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
			if (G_PlayerCanBeSeenByOthers(ent))
			{
				ent->client->ps.powerups[PW_OPS_DISGUISED] = 0;
				ent->client->disguiseClientNum             = -1;
			}
			break;
		default:     // luger, akimbo luger, colt, akimbo colt, fg42, fg42_scoped
			ent->client->ps.powerups[PW_OPS_DISGUISED] = 0;
			ent->client->disguiseClientNum             = -1;
			break;
		}
	}

	// fire the specific weapon
	switch (ent->s.weapon)
	{
	case WP_KNIFE:
		Weapon_Knife(ent, MOD_KNIFE);
		break;
	case WP_KNIFE_KABAR:
		Weapon_Knife(ent, MOD_KNIFE_KABAR);
		break;
	case WP_MEDKIT:
#ifdef FEATURE_OMNIBOT
		callEvent = qfalse;
#endif
		Weapon_Medic(ent);
		break;
	case WP_PLIERS:
		Weapon_Engineer(ent);
		break;
	case WP_SMOKE_MARKER:
		if (level.time - ent->client->ps.classWeaponTime > level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1];
		}

		if (ent->client->sess.skill[SK_SIGNALS] >= 2)
		{
			ent->client->ps.classWeaponTime += .66f * level.fieldopsChargeTime[ent->client->sess.sessionTeam - 1];
		}
		else
		{
			ent->client->ps.classWeaponTime = level.time;
		}
		pFiredShot = weapon_grenadelauncher_fire(ent, WP_SMOKE_MARKER);
		break;
	case WP_MEDIC_SYRINGE:
		Weapon_Syringe(ent);
		break;
	case WP_MEDIC_ADRENALINE:
		ent->client->ps.classWeaponTime = level.time;
		Weapon_AdrenalineSyringe(ent);
		break;
	case WP_AMMO:
#ifdef FEATURE_OMNIBOT
		callEvent = qfalse;
#endif
		Weapon_MagicAmmo(ent);
		break;
	case WP_LUGER:
	case WP_SILENCER:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
	case WP_COLT:
	case WP_SILENCED_COLT:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
	case WP_FG42:
	case WP_STEN:
	case WP_MP40:
	case WP_THOMPSON:
		Bullet_Fire(ent, GetWeaponTableData(ent->s.weapon)->spread * aimSpreadScale, GetWeaponTableData(ent->s.weapon)->damage, qtrue);
		break;
	case WP_KAR98:
	case WP_CARBINE:
	case WP_GARAND:
	case WP_K43:
		aimSpreadScale = 1.0f;
		Bullet_Fire(ent, GetWeaponTableData(ent->s.weapon)->spread * aimSpreadScale, GetWeaponTableData(ent->s.weapon)->damage, qfalse);
		break;
	case WP_FG42SCOPE:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		Bullet_Fire(ent, GetWeaponTableData(ent->s.weapon)->spread * aimSpreadScale, GetWeaponTableData(ent->s.weapon)->damage, qfalse);
		break;
	case WP_SATCHEL_DET:
		if (G_ExplodeSatchels(ent))
		{
			ent->client->ps.ammo[WP_SATCHEL_DET]     = 0;
			ent->client->ps.ammoclip[WP_SATCHEL_DET] = 0;
			ent->client->ps.ammoclip[WP_SATCHEL]     = 1;
			G_AddEvent(ent, EV_NOAMMO, 0);
		}
		break;
	case WP_MOBILE_MG42_SET:
	case WP_MOBILE_BROWNING_SET:
		Bullet_Fire(ent, GetWeaponTableData(ent->s.weapon)->spread * 0.05f * aimSpreadScale, GetWeaponTableData(ent->s.weapon)->damage, qfalse);
		break;
	case WP_MOBILE_MG42:
	case WP_MOBILE_BROWNING:
		if ((ent->client->ps.pm_flags & PMF_DUCKED) || (ent->client->ps.eFlags & EF_PRONE))
		{
			Bullet_Fire(ent, GetWeaponTableData(ent->s.weapon)->spread * 0.6f * aimSpreadScale, GetWeaponTableData(ent->s.weapon)->damage, qfalse);
		}
		else
		{
			Bullet_Fire(ent, GetWeaponTableData(ent->s.weapon)->spread * aimSpreadScale, GetWeaponTableData(ent->s.weapon)->damage, qfalse);
		}
		break;
	case WP_PANZERFAUST:
	case WP_BAZOOKA:
		if (level.time - ent->client->ps.classWeaponTime > level.soldierChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.soldierChargeTime[ent->client->sess.sessionTeam - 1];
		}

		if (ent->client->sess.skill[SK_HEAVY_WEAPONS] >= 1)
		{
			ent->client->ps.classWeaponTime += .66f * level.soldierChargeTime[ent->client->sess.sessionTeam - 1];
		}
		else
		{
			ent->client->ps.classWeaponTime = level.time;
		}

		pFiredShot = weapon_antitank_fire(ent);
		if (ent->client)
		{
			vec3_t forward;

			AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
			VectorMA(ent->client->ps.velocity, -64, forward, ent->client->ps.velocity);
		}
		break;
	case WP_GPG40:
	case WP_M7:
		if (level.time - ent->client->ps.classWeaponTime > level.engineerChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		}

		ent->client->ps.classWeaponTime += .5f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		pFiredShot                       = weapon_gpg40_fire(ent, ent->s.weapon);
		break;
	case WP_MORTAR_SET:
	case WP_MORTAR2_SET:
		if (level.time - ent->client->ps.classWeaponTime > level.soldierChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.soldierChargeTime[ent->client->sess.sessionTeam - 1];
		}

		if (ent->client->sess.skill[SK_HEAVY_WEAPONS] >= 1)
		{
			ent->client->ps.classWeaponTime += .33f * level.soldierChargeTime[ent->client->sess.sessionTeam - 1];
		}
		else
		{
			ent->client->ps.classWeaponTime += .5f * level.soldierChargeTime[ent->client->sess.sessionTeam - 1];
		}
		pFiredShot = weapon_mortar_fire(ent, ent->s.weapon);
		break;
	case WP_SATCHEL:
	case WP_SMOKE_BOMB:
		if (level.time - ent->client->ps.classWeaponTime > level.covertopsChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.covertopsChargeTime[ent->client->sess.sessionTeam - 1];
		}

		if (ent->client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 2)
		{
			ent->client->ps.classWeaponTime += .66f * level.covertopsChargeTime[ent->client->sess.sessionTeam - 1];
		}
		else
		{
			ent->client->ps.classWeaponTime = level.time;
		}
		pFiredShot = weapon_grenadelauncher_fire(ent, ent->s.weapon);
		break;
	case WP_LANDMINE:
		if (level.time - ent->client->ps.classWeaponTime > level.engineerChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		}

		if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3)
		{
			// use 33%, not 66%, when upgraded.
			// do not penalize the happy fun engineer.
			ent->client->ps.classWeaponTime += .33f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		}
		else
		{
			ent->client->ps.classWeaponTime += .5f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		}
		pFiredShot = weapon_grenadelauncher_fire(ent, ent->s.weapon);
		break;
	case WP_DYNAMITE:
		if (level.time - ent->client->ps.classWeaponTime > level.engineerChargeTime[ent->client->sess.sessionTeam - 1])
		{
			ent->client->ps.classWeaponTime = level.time - level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		}

		if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3)
		{
			ent->client->ps.classWeaponTime += .66f * level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
		}
		else
		{
			ent->client->ps.classWeaponTime = level.time;
		}
		pFiredShot = weapon_grenadelauncher_fire(ent, ent->s.weapon);
		break;
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
		pFiredShot = weapon_grenadelauncher_fire(ent, ent->s.weapon);
		break;
	case WP_FLAMETHROWER:
		// this is done client-side only now
		// - um, no it isnt? FIXME
		pFiredShot = Weapon_FlamethrowerFire(ent);
		break;
	case WP_MAPMORTAR:
		break;
	default:
		break;
	}

#ifdef FEATURE_OMNIBOT
	// Omni-bot - Send a fire event.
	if (callEvent)
	{
		Bot_Event_FireWeapon(ent - g_entities, Bot_WeaponGameToBot(ent->s.weapon), pFiredShot);
	}
#endif

#ifndef DEBUG_STATS
	if (g_gamestate.integer == GS_PLAYING)
#endif
	ent->client->sess.aWeaponStats[BG_WeapStatForWeapon(ent->s.weapon)].atts++;
}
