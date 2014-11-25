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
 * @file g_missile.c
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#define MISSILE_PRESTEP_TIME    50

void M_think(gentity_t *ent);
void G_ExplodeMissile(gentity_t *ent);

/*
================
G_BounceMissile
================
*/
void G_BounceMissile(gentity_t *ent, trace_t *trace)
{
	vec3_t    velocity;
	float     dot;
	int       hitTime;
	gentity_t *ground;

	// boom after 750 msecs
	if (ent->s.weapon == WP_M7 || ent->s.weapon == WP_GPG40)
	{
		ent->s.effect1Time = qtrue; // has bounced

		if ((ent->nextthink - level.time) < 3250)
		{
			G_ExplodeMissile(ent);
			return;
		}
	}

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity, qfalse, ent->s.effect2Time);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	// record this for mover pushing
	if (trace->plane.normal[2] > 0.2 /*&& VectorLengthSquared( ent->s.pos.trDelta ) < Square(40)*/)
	{
		ent->s.groundEntityNum = trace->entityNum;
	}

	// set ground entity
	if (ent->s.groundEntityNum != -1)
	{
		ground = &g_entities[ent->s.groundEntityNum];
	}
	else
	{
		ground = NULL;
	}

	// allow ground entity to push missle
	if (ent->s.groundEntityNum != ENTITYNUM_WORLD && ground)
	{
		VectorMA(ent->s.pos.trDelta, 0.85f, ground->instantVelocity, ent->s.pos.trDelta);
	}

	if (ent->s.eFlags & EF_BOUNCE_HALF)
	{
		vec3_t relativeDelta;

		if (ent->s.eFlags & EF_BOUNCE)         // both flags marked, do a third type of bounce
		{
			VectorScale(ent->s.pos.trDelta, 0.35, ent->s.pos.trDelta);
		}
		else
		{
			VectorScale(ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta);
		}

		// grenades on movers get scaled back much earlier
		if (ent->s.groundEntityNum != ENTITYNUM_WORLD)
		{
			VectorScale(ent->s.pos.trDelta, 0.5, ent->s.pos.trDelta);
		}

		// calculate relative delta for stop calcs
		if (ent->s.groundEntityNum == ENTITYNUM_WORLD || 1)
		{
			VectorCopy(ent->s.pos.trDelta, relativeDelta);
		}
		else
		{
			VectorSubtract(ent->s.pos.trDelta, ground->instantVelocity, relativeDelta);
		}

		// check for stop
		//if ( trace->plane.normal[2] > 0.2 && VectorLengthSquared( ent->s.pos.trDelta ) < Square(40) )
		if (trace->plane.normal[2] > 0.2 && VectorLengthSquared(relativeDelta) < Square(40))
		{
			// make the world the owner of the dynamite, so the player can shoot it after it stops moving
			if (ent->s.weapon == WP_DYNAMITE || ent->s.weapon == WP_LANDMINE || ent->s.weapon == WP_SATCHEL || ent->s.weapon == WP_SMOKE_BOMB)
			{
				ent->r.ownerNum = ENTITYNUM_WORLD;
			}

			G_SetOrigin(ent, trace->endpos);
			ent->s.time = level.time; // final rotation value
			if (ent->s.weapon == WP_M7 || ent->s.weapon == WP_GPG40)
			{
				// explode one 750msecs after launchtime
				ent->nextthink = level.time + (750 - (level.time + 4000 - ent->nextthink));
			}

			return;
		}
	}

	SnapVector(ent->s.pos.trDelta);

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);

	SnapVector(ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

/*
================
G_MissileImpact
    impactDamage is how much damage the impact will do to func_explosives
================
*/
void G_MissileImpact(gentity_t *ent, trace_t *trace, int impactDamage)
{
	gentity_t *other = &g_entities[trace->entityNum];
	gentity_t *temp;
	vec3_t    velocity;
	int       event = 0, param = 0, otherentnum = 0;

	// handle func_explosives
	if (other->classname && Q_stricmp(other->classname, "func_explosive") == 0)
	{
		// the damage is sufficient to "break" the ent (health == 0 is non-breakable)
		if (other->health && impactDamage >= other->health)
		{
			// check for other->takedamage needs to be inside the health check since it is
			// likely that, if successfully destroyed by the missile, in the next runmissile()
			// update takedamage would be set to '0' and the func_explosive would not be
			// removed yet, causing a bounce.
			if (other->takedamage)
			{
				BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity, qfalse, ent->s.effect2Time);
				G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->s.origin, impactDamage, 0, ent->methodOfDeath);
			}

			// its possible of the func_explosive not to die from this and it
			// should reflect the missile or explode it not vanish into oblivion
			if (other->health <= 0)
			{
				return;
			}
		}
	}

	// check for bounce
	if ((!other->takedamage || !ent->damage) && (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF)))
	{
		G_BounceMissile(ent, trace);
		// spotter White Phosphorous rounds shouldn't bounce noise
		if (!Q_stricmp(ent->classname, "WP"))
		{
			return;
		}

		G_AddEvent(ent, EV_GRENADE_BOUNCE, BG_FootstepForSurface(trace->surfaceFlags));

		return;
	}

	// impact damage
	if (other->takedamage || other->dmgparent)
	{
		if (ent->damage)
		{
			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity, qfalse, ent->s.effect2Time);
			if (!VectorLengthSquared(velocity))
			{
				velocity[2] = 1;    // stepped on a grenade
			}

			G_Damage(other->dmgparent ? other->dmgparent : other, ent, &g_entities[ent->r.ownerNum], velocity, ent->s.origin, ent->damage, 0, ent->methodOfDeath);
		}
		else     // if no damage value, then this is a splash damage grenade only
		{
			G_BounceMissile(ent, trace);
			return;
		}
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if (other->takedamage && other->client)
	{
		event       = EV_MISSILE_HIT;
		param       = DirToByte(trace->plane.normal);
		otherentnum = other->s.number;
	}
	else
	{
		// try projecting it in the direction it came from, for better decals
		vec3_t dir;
		BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, dir, qfalse, ent->s.effect2Time);
		BG_GetMarkDir(dir, trace->plane.normal, dir);

		event = EV_MISSILE_MISS;
		param = DirToByte(dir);
	}

	temp                   = G_TempEntity(trace->endpos, event);
	temp->s.otherEntityNum = otherentnum;
	//temp->r.svFlags |= SVF_BROADCAST;
	temp->s.eventParm = param;
	temp->s.weapon    = ent->s.weapon;
	temp->s.clientNum = ent->r.ownerNum;

	if (IS_MORTAR_WEAPON_SET(ent->s.weapon))
	{
		temp->s.legsAnim = ent->s.legsAnim; // need this one as well
		temp->r.svFlags |= SVF_BROADCAST;
	}

	// splash damage (doesn't apply to person directly hit)
	if (ent->splashDamage)
	{
		G_RadiusDamage(trace->endpos, ent, ent->parent, ent->splashDamage, ent->splashRadius, other, ent->splashMethodOfDeath);
	}

	//trap_LinkEntity( ent );

	G_FreeEntity(ent);
}

/*
==============
Concussive_think
==============
*/

void M_think(gentity_t *ent)
{
	gentity_t *tent;

	ent->count++;

	if (ent->count == ent->health)
	{
		ent->think = G_FreeEntity;
	}

	tent = G_TempEntity(ent->s.origin, EV_SMOKE);
	VectorCopy(ent->s.origin, tent->s.origin);
	if (ent->s.density == 1)
	{
		tent->s.origin[2] += 16;

		tent->s.angles2[0] = 16;
	}
	else
	{
		//tent->s.origin[2]+=32;
		// Note to self Maxx said to lower the spawn loc for the smoke 16 units
		tent->s.origin[2] += 16;

		// Note to self Maxx changed this to 24
		tent->s.angles2[0] = 24;
	}

	tent->s.time    = 3000;
	tent->s.time2   = 100;
	tent->s.density = 0;

	tent->s.angles2[1] = 96;
	tent->s.angles2[2] = 50;

	ent->nextthink = level.time + FRAMETIME;
}

/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile(gentity_t *ent)
{
	vec3_t dir;
	vec3_t origin;
	int    etype;

	if (ent->s.weapon == WP_SMOKE_MARKER && ent->active)
	{
		level.numActiveAirstrikes[ent->s.teamNum - 1]--;
	}

	etype        = ent->s.eType;
	ent->s.eType = ET_GENERAL;

	// splash damage
	if (ent->splashDamage)
	{
		vec3_t origin;

		VectorCopy(ent->r.currentOrigin, origin);

		if (ent->s.weapon == WP_DYNAMITE)
		{
			origin[2] += 4;
		}

		if ((ent->s.weapon == WP_DYNAMITE && (ent->etpro_misc_1 & 1)) || ent->s.weapon == WP_SATCHEL)
		{
			etpro_RadiusDamage(origin, ent, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath, qtrue);
			G_TempTraceIgnorePlayersAndBodies();
			etpro_RadiusDamage(origin, ent, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath, qfalse);
			G_ResetTempTraceIgnoreEnts();
		}
		else
		{
			G_RadiusDamage(origin, ent, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath);
		}
	}

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin, qfalse, ent->s.effect2Time);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	if (ent->accuracy == 1)
	{
		G_AddEvent(ent, EV_MISSILE_MISS_SMALL, DirToByte(dir));
	}
	else if (ent->accuracy == 2)
	{
		G_AddEvent(ent, EV_MISSILE_MISS_LARGE, DirToByte(dir));
	}
	else if (ent->accuracy == 3)
	{
		ent->freeAfterEvent = qtrue;
		trap_LinkEntity(ent);
		return;
	}
	else
	{
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));
		ent->s.clientNum = ent->r.ownerNum;
	}

	ent->freeAfterEvent = qtrue;

	trap_LinkEntity(ent);

	if (etype == ET_MISSILE)
	{
		if (ent->s.weapon == WP_LANDMINE)
		{
			mapEntityData_t *mEnt;

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[0], mEnt);
			}

			if ((mEnt = G_FindMapEntityData(&mapEntityData[1], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[1], mEnt);
			}
		}
		else if (ent->s.weapon == WP_DYNAMITE && (ent->etpro_misc_1 & 1))         // do some scoring
		{   // check if dynamite is in trigger_objective_info field
			vec3_t    mins, maxs;
			int       i, num, touch[MAX_GENTITIES];
			gentity_t *hit;

			ent->free = NULL; // no defused tidy up if we exploded

			// made this the actual bounding box of dynamite instead of range
			VectorAdd(ent->r.currentOrigin, ent->r.mins, mins);
			VectorAdd(ent->r.currentOrigin, ent->r.maxs, maxs);
			num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

			for (i = 0; i < num; i++)
			{
				hit = &g_entities[touch[i]];
				if (!hit->target)
				{
					continue;
				}

				if ((hit->s.eType != ET_OID_TRIGGER))
				{
					continue;
				}

				if (!(hit->spawnflags & (AXIS_OBJECTIVE | ALLIED_OBJECTIVE)))
				{
					continue;
				}

				if (hit->target_ent)
				{
					// only if it targets a func_explosive
					if (hit->target_ent->s.eType != ET_EXPLOSIVE)
					{
						continue;
					}

					if (hit->target_ent->constructibleStats.weaponclass < 1)
					{
						continue;
					}
				}

				if (((hit->spawnflags & AXIS_OBJECTIVE) && (ent->s.teamNum == TEAM_ALLIES)) || ((hit->spawnflags & ALLIED_OBJECTIVE) && (ent->s.teamNum == TEAM_AXIS)))
				{
					if (ent->parent->client && G_GetWeaponClassForMOD(MOD_DYNAMITE) >= hit->target_ent->constructibleStats.weaponclass)
					{
						G_AddKillSkillPointsForDestruction(ent->parent, MOD_DYNAMITE, &hit->target_ent->constructibleStats);
					}

					G_UseTargets(hit, ent);
					hit->think     = G_FreeEntity;
					hit->nextthink = level.time + FRAMETIME;

					G_Script_ScriptEvent(hit, "destroyed", "");
				}
			}
		}

		// give big weapons the shakey shakey // FIXME: WP_MORTAR/SET?
		// FIXME: weapon table
		switch (ent->s.weapon)
		{
		case WP_DYNAMITE:
		case WP_PANZERFAUST:
		case WP_BAZOOKA:
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
		case WP_MAPMORTAR:
		case WP_ARTY:
		case WP_SMOKE_MARKER:
		case WP_LANDMINE:
		case WP_SATCHEL:
		{
			gentity_t *tent = G_TempEntity(ent->r.currentOrigin, EV_SHAKE);

			tent->s.onFireStart = ent->splashDamage * 4;
			tent->r.svFlags    |= SVF_BROADCAST;
		}
		break;
		default:
			break;
		}
	}
}

/*
=================
Landmine_Check_Ground
=================
*/
void Landmine_Check_Ground(gentity_t *self)
{
	vec3_t  mins, maxs;
	vec3_t  start, end;
	trace_t tr;

	VectorCopy(self->r.currentOrigin, start);
	VectorCopy(self->r.currentOrigin, end);

	end[2] -= 4;

	VectorCopy(self->r.mins, mins);
	VectorCopy(self->r.maxs, maxs);

	trap_Trace(&tr, start, mins, maxs, end, self->s.number, MASK_MISSILESHOT);

	if (tr.fraction == 1)
	{
		self->s.groundEntityNum = -1;
	}
}

/*
================
G_RunMissile
================
*/
void G_RunMissile(gentity_t *ent)
{
	vec3_t  origin;
	trace_t tr;

	if (ent->s.weapon == WP_LANDMINE || ent->s.weapon == WP_DYNAMITE || ent->s.weapon == WP_SATCHEL)
	{
		Landmine_Check_Ground(ent);

		if (ent->s.groundEntityNum == -1)
		{
			if (ent->s.pos.trType != TR_GRAVITY)
			{
				ent->s.pos.trType = TR_GRAVITY;
				ent->s.pos.trTime = level.time;
			}
		}
	}

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin, qfalse, ent->s.effect2Time);

	if ((ent->clipmask & CONTENTS_BODY) && (ent->s.weapon == WP_DYNAMITE || ent->s.weapon == WP_ARTY || ent->s.weapon == WP_SMOKE_MARKER
	                                        || ent->s.weapon == WP_GRENADE_LAUNCHER || ent->s.weapon == WP_GRENADE_PINEAPPLE
	                                        || ent->s.weapon == WP_LANDMINE || ent->s.weapon == WP_SATCHEL || ent->s.weapon == WP_SMOKE_BOMB
	                                        ))
	{
		if (!ent->s.pos.trDelta[0] && !ent->s.pos.trDelta[1] && !ent->s.pos.trDelta[2])
		{
			ent->clipmask &= ~CONTENTS_BODY;
		}
	}

	if (level.tracemapLoaded &&
	    (IS_MORTAR_WEAPON_SET(ent->s.weapon) ||
	     ent->s.weapon == WP_GPG40 ||
	     ent->s.weapon == WP_M7 ||
	     ent->s.weapon == WP_GRENADE_LAUNCHER ||
	     ent->s.weapon == WP_GRENADE_PINEAPPLE))
	{
		if (ent->count)
		{
			if (ent->r.currentOrigin[0] < level.mapcoordsMins[0] ||
			    ent->r.currentOrigin[1] > level.mapcoordsMins[1] ||
			    ent->r.currentOrigin[0] > level.mapcoordsMaxs[0] ||
			    ent->r.currentOrigin[1] < level.mapcoordsMaxs[1])
			{
				gentity_t *tent;

				tent              = G_TempEntity(ent->r.currentOrigin, EV_MORTAR_MISS);
				tent->s.clientNum = ent->r.ownerNum;
				tent->r.svFlags  |= SVF_BROADCAST;
				tent->s.density   = 1;  // angular

				G_FreeEntity(ent);
				return;
			}
			else
			{
				float skyHeight = BG_GetSkyHeightAtPoint(origin);

				if (origin[2] < BG_GetTracemapGroundFloor())
				{
					gentity_t *tent;

					tent              = G_TempEntity(ent->r.currentOrigin, EV_MORTAR_MISS);
					tent->s.clientNum = ent->r.ownerNum;
					tent->r.svFlags  |= SVF_BROADCAST;
					tent->s.density   = 0;  // direct

					G_FreeEntity(ent);
					return;
				}

				// are we in worldspace again - or did we hit a ceiling from the outside of the world
				if (skyHeight == MAX_MAP_SIZE)
				{
					G_RunThink(ent);
					VectorCopy(origin, ent->r.currentOrigin);

					return;     // keep flying
				}

				if (skyHeight <= origin[2])
				{
					G_RunThink(ent);
					return; // keep flying
				}

				// back in the world, keep going like normal
				VectorCopy(origin, ent->r.currentOrigin);
				ent->count  = 0;
				ent->count2 = 1;
			}
		}
		else if (!ent->count2 && BG_GetSkyHeightAtPoint(origin) - BG_GetGroundHeightAtPoint(origin) > 512)
		{
			vec3_t delta;

			VectorSubtract(origin, ent->r.currentOrigin, delta);
			if (delta[2] < 0)
			{
				ent->count2 = 1;
			}
		}
	}

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, ent->r.ownerNum, ent->clipmask);

	if (IS_MORTAR_WEAPON_SET(ent->s.weapon) && ent->count2 == 1)
	{
		if (ent->r.currentOrigin[2] > origin[2] && origin[2] - BG_GetGroundHeightAtPoint(origin) < 512)
		{
			vec3_t  impactpos;
			trace_t mortar_tr;

			VectorSubtract(origin, ent->r.currentOrigin, impactpos);
			VectorMA(origin, 8, impactpos, impactpos);

			trap_Trace(&mortar_tr, origin, ent->r.mins, ent->r.maxs, impactpos,
			           ent->r.ownerNum, ent->clipmask);

			if (mortar_tr.fraction != 1)
			{
				gentity_t *tent;

				impactpos[2] = BG_GetGroundHeightAtPoint(impactpos);

				tent              = G_TempEntity(impactpos, EV_MORTAR_IMPACT);
				tent->s.clientNum = ent->r.ownerNum;
				tent->r.svFlags  |= SVF_BROADCAST;

				ent->count2     = 2;
				ent->s.legsAnim = 1;

				/*
				{
				    gentity_t *tent;

				    tent = G_TempEntity( origin, EV_RAILTRAIL );
				    VectorCopy( impactpos, tent->s.origin2 );
				    tent->s.dmgFlags = 0;

				    tent = G_TempEntity( origin, EV_RAILTRAIL );
				    VectorCopy( ent->r.currentOrigin, tent->s.origin2 );
				    tent->s.dmgFlags = 0;
				}
				*/
			}
		}
	}

	VectorCopy(tr.endpos, ent->r.currentOrigin);

	if (tr.startsolid)
	{
		tr.fraction = 0;
	}

	trap_LinkEntity(ent);

	if (tr.fraction != 1)
	{
		int impactDamage;

		if (level.tracemapLoaded &&
		    (IS_MORTAR_WEAPON_SET(ent->s.weapon) ||
		     ent->s.weapon == WP_GPG40 ||
		     ent->s.weapon == WP_M7 ||
		     ent->s.weapon == WP_GRENADE_LAUNCHER ||
		     ent->s.weapon == WP_GRENADE_PINEAPPLE)
		    && (tr.surfaceFlags & SURF_SKY))
		{
			// goes through sky
			ent->count = 1;
			trap_UnlinkEntity(ent);
			G_RunThink(ent);
			return; // keep flying
		}
		else if (tr.surfaceFlags & SURF_NOIMPACT) // never explode or bounce on sky
		{
			G_FreeEntity(ent);
			return;
		}

		//      G_SetOrigin( ent, tr.endpos );

		if (IS_PANZER_WEAPON(ent->s.weapon) || IS_MORTAR_WEAPON_SET(ent->s.weapon))
		{
			impactDamage = 999; // goes through pretty much any func_explosives
		}
		else
		{
			impactDamage = 20;  // "grenade"/"dynamite"     // probably adjust this based on velocity

		}

		if (ent->s.weapon == WP_DYNAMITE || ent->s.weapon == WP_LANDMINE || ent->s.weapon == WP_SATCHEL)
		{
			if (ent->s.pos.trType != TR_STATIONARY)
			{
				G_MissileImpact(ent, &tr, impactDamage);
			}
		}
		else
		{
			G_MissileImpact(ent, &tr, impactDamage);
		}

		if (ent->s.eType != ET_MISSILE)
		{
			gentity_t *tent = G_TempEntity(ent->r.currentOrigin, EV_SHAKE);

			tent->s.onFireStart = ent->splashDamage * 4;
			tent->r.svFlags    |= SVF_BROADCAST;
			return;     // exploded
		}
	}
	else if (VectorLengthSquared(ent->s.pos.trDelta))           // free fall/no intersection
	{
		ent->s.groundEntityNum = ENTITYNUM_NONE;
	}

	// check think function after bouncing
	G_RunThink(ent);
}

/*
================
G_PredictBounceMissile
================
*/
void G_PredictBounceMissile(gentity_t *ent, trajectory_t *pos, trace_t *trace, int time)
{
	vec3_t velocity, origin;
	float  dot;
	int    hitTime;

	BG_EvaluateTrajectory(pos, time, origin, qfalse, ent->s.effect2Time);

	// reflect the velocity on the trace plane
	hitTime = time;
	BG_EvaluateTrajectoryDelta(pos, hitTime, velocity, qfalse, ent->s.effect2Time);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, pos->trDelta);

	if (ent->s.eFlags & EF_BOUNCE_HALF)
	{
		if (ent->s.eFlags & EF_BOUNCE)         // both flags marked, do a third type of bounce
		{
			VectorScale(pos->trDelta, 0.35, pos->trDelta);
		}
		else
		{
			VectorScale(pos->trDelta, 0.65, pos->trDelta);
		}

		// check for stop
		if (trace->plane.normal[2] > 0.2 && VectorLengthSquared(pos->trDelta) < Square(40))
		{
			VectorCopy(trace->endpos, pos->trBase);
			return;
		}
	}

	VectorAdd(origin, trace->plane.normal, pos->trBase);
	pos->trTime = time;
}

/*
================
G_PredictMissile

  selfNum is the character that is checking to see what the missile is going to do

  returns qfalse if the missile won't explode, otherwise it'll return the time is it expected to explode
================
*/
int G_PredictMissile(gentity_t *ent, int duration, vec3_t endPos, qboolean allowBounce)
{
	vec3_t       origin;
	trace_t      tr;
	int          time;
	trajectory_t pos = ent->s.pos;
	vec3_t       org;
	gentity_t    backupEnt;

	BG_EvaluateTrajectory(&pos, level.time, org, qfalse, ent->s.effect2Time);

	backupEnt = *ent;

	for (time = level.time + FRAMETIME; time < level.time + duration; time += FRAMETIME)
	{
		// get current position
		BG_EvaluateTrajectory(&pos, time, origin, qfalse, ent->s.effect2Time);

		// trace a line from the previous position to the current position,
		// ignoring interactions with the missile owner
		trap_Trace(&tr, org, ent->r.mins, ent->r.maxs, origin,
		           ent->r.ownerNum, ent->clipmask);

		VectorCopy(tr.endpos, org);

		if (tr.startsolid)
		{
			*ent = backupEnt;
			return qfalse;
		}

		if (tr.fraction != 1)
		{
			// never explode or bounce on sky
			if  (tr.surfaceFlags & SURF_NOIMPACT)
			{
				*ent = backupEnt;
				return qfalse;
			}

			if (allowBounce && (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF)))
			{
				G_PredictBounceMissile(ent, &pos, &tr, time - FRAMETIME + (int)((float)FRAMETIME * tr.fraction));
				pos.trTime = time;
				continue;
			}

			// exploded, so drop out of loop
			break;
		}
	}
	/*
	    if (!allowBounce && tr.fraction < 1 && tr.entityNum > level.maxclients)
	    {
	        // go back a bit in time, so we can catch it in the air
	        time -= 200;
	        if (time < level.time + FRAMETIME)
	            time = level.time + FRAMETIME;
	        BG_EvaluateTrajectory( &pos, time, org );
	    }
	*/

	// get current position
	VectorCopy(org, endPos);
	// set the entity data back
	*ent = backupEnt;

	if (allowBounce && (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF)))
	{
		return ent->nextthink;
	}
	else        // it will probably explode before it times out
	{
		return time;
	}
}

//=============================================================================
// Server side Flamethrower
//=============================================================================

// copied from cg_flamethrower.c - FIXME: move same definitions of cg_flamethrower.c to bg_public.h and remove FLAME_* defs here
#define FLAME_START_SIZE        1.0
#define FLAME_START_MAX_SIZE    100.0   // when the flame is spawned, it should endevour to reach this size
#define FLAME_START_SPEED       1200.0  // speed of flame as it leaves the nozzle
#define FLAME_MIN_SPEED         60.0

// these are calculated (don't change)
#define FLAME_LENGTH            (FLAMETHROWER_RANGE + 50.0)   // NOTE: only modify the range, since this should always reflect that range

#define FLAME_LIFETIME          (int)((FLAME_LENGTH / FLAME_START_SPEED) * 1000)        // life duration in milliseconds
#define FLAME_FRICTION_PER_SEC  (2.0f * FLAME_START_SPEED)

void G_BurnTarget(gentity_t *self, gentity_t *body, qboolean directhit)
{
	float   radius, dist;
	vec3_t  point, v;
	trace_t tr;

	if (!body->takedamage)
	{
		return;
	}

	// don't catch fire if invulnerable or same team in no FF
	if (body->client)
	{
		if (body->client->ps.powerups[PW_INVULNERABLE] >= level.time)
		{
			body->flameQuota  = 0;
			body->s.onFireEnd = level.time - 1;
			return;
		}

		//if( !self->count2 && body == self->parent )
		//  return;

		if (!(g_friendlyFire.integer) && OnSameTeam(body, self->parent))
		{
			return;
		}
	}

	// don't catch fire if under water or invulnerable
	if (body->waterlevel >= 3)
	{
		body->flameQuota  = 0;
		body->s.onFireEnd = level.time - 1;
		return;
	}

	if (!body->r.bmodel)
	{
		VectorCopy(body->r.currentOrigin, point);
		if (body->client)
		{
			point[2] += body->client->ps.viewheight;
		}

		VectorSubtract(point, self->r.currentOrigin, v);
	}
	else
	{
		int i;

		for (i = 0 ; i < 3 ; i++)
		{
			if (self->s.origin[i] < body->r.absmin[i])
			{
				v[i] = body->r.absmin[i] - self->r.currentOrigin[i];
			}
			else if (self->r.currentOrigin[i] > body->r.absmax[i])
			{
				v[i] = self->r.currentOrigin[i] - body->r.absmax[i];
			}
			else
			{
				v[i] = 0;
			}
		}
	}

	radius = self->speed;

	dist = VectorLength(v);

	// The person who shot the flame only burns when within 1/2 the radius
	if (body->s.number == self->r.ownerNum && dist >= (radius * 0.5))
	{
		return;
	}
	if (!directhit && dist >= radius)
	{
		return;
	}

	// Non-clients that take damage get damaged here
	if (!body->client)
	{
		if (body->health > 0)
		{
			G_Damage(body, self->parent, self->parent, vec3_origin, self->r.currentOrigin, 2, 0, MOD_FLAMETHROWER);
		}

		return;
	}

	// do a trace to see if there's a wall btwn. body & flame centroid -- prevents damage through walls
	trap_Trace(&tr, self->r.currentOrigin, NULL, NULL, point, body->s.number, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		return;
	}

	// now check the damageQuota to see if we should play a pain animation
	// first reduce the current damageQuota with time
	if (body->flameQuotaTime && body->flameQuota > 0)
	{
		body->flameQuota -= (int)(((float)(level.time - body->flameQuotaTime) / 1000) * 2.5f);
		if (body->flameQuota < 0)
		{
			body->flameQuota = 0;
		}
	}

	G_BurnMeGood(self, body, self);
}

void G_FlameDamage(gentity_t *self, gentity_t *ignoreent)
{
	gentity_t *body;
	vec3_t    mins, maxs;
	float     radius    = self->speed;
	float     boxradius = M_SQRT2 * radius; // radius * sqrt(2) for bounding box enlargement
	int       entityList[MAX_GENTITIES];
	int       i, e, numListedEntities;

	for (i = 0 ; i < 3 ; i++)
	{
		mins[i] = self->r.currentOrigin[i] - boxradius;
		maxs[i] = self->r.currentOrigin[i] + boxradius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (e = 0 ; e < numListedEntities ; e++)
	{
		body = &g_entities[entityList[e]];

		if (body == ignoreent)
		{
			continue;
		}

		G_BurnTarget(self, body, qfalse);
	}
}

void G_RunFlamechunk(gentity_t *ent)
{
	vec3_t    vel, add;
	vec3_t    neworg;
	trace_t   tr;
	float     speed;
	gentity_t *ignoreent = NULL;

	// vel was only being set if (level.time - ent->timestamp > 50
	// However, below, it was being used when we hit something and it was uninitialized
	VectorCopy(ent->s.pos.trDelta, vel);
	speed = VectorNormalize(vel);

	// Adust the current speed of the chunk
	if (level.time - ent->timestamp > 50)
	{
		speed -= (50.f / 1000.f) * FLAME_FRICTION_PER_SEC;

		if (speed < FLAME_MIN_SPEED)
		{
			speed = FLAME_MIN_SPEED;
		}

		VectorScale(vel, speed, ent->s.pos.trDelta);
	}
	else
	{
		speed = FLAME_START_SPEED;
	}

	// Move the chunk
	VectorScale(ent->s.pos.trDelta, 50.f / 1000.f, add);
	VectorAdd(ent->r.currentOrigin, add, neworg);

	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, neworg, ent->r.ownerNum, MASK_SHOT | MASK_WATER);

	if (tr.startsolid)
	{
		VectorClear(ent->s.pos.trDelta);
		ent->count2++;
	}
	else if (tr.fraction != 1.0f && !(tr.surfaceFlags & SURF_NOIMPACT))
	{
		float dot;

		VectorCopy(tr.endpos, ent->r.currentOrigin);

		dot = DotProduct(vel, tr.plane.normal);
		VectorMA(vel, -2 * dot, tr.plane.normal, vel);
		VectorNormalize(vel);
		speed *= 0.5 * (0.25 + 0.75 * ((dot + 1.0) * 0.5));
		VectorScale(vel, speed, ent->s.pos.trDelta);

		if (tr.entityNum != ENTITYNUM_WORLD && tr.entityNum != ENTITYNUM_NONE)
		{
			ignoreent = &g_entities[tr.entityNum];
			G_BurnTarget(ent, ignoreent, qtrue);
		}

		ent->count2++;
	}
	else
	{
		VectorCopy(neworg, ent->r.currentOrigin);
	}

	// Do damage to nearby entities, every 100ms
	if (ent->flameQuotaTime <= level.time)
	{
		ent->flameQuotaTime = level.time + 100;
		G_FlameDamage(ent, ignoreent);
	}

	// Show debugging bbox
	if (g_debugBullets.integer > 3)
	{
		gentity_t *bboxEnt;
		float     size = ent->speed / 2;
		vec3_t    b1, b2;
		vec3_t    temp;

		VectorSet(temp, -size, -size, -size);
		VectorCopy(ent->r.currentOrigin, b1);
		VectorCopy(ent->r.currentOrigin, b2);
		VectorAdd(b1, temp, b1);
		VectorSet(temp, size, size, size);
		VectorAdd(b2, temp, b2);
		bboxEnt = G_TempEntity(b1, EV_RAILTRAIL);
		VectorCopy(b2, bboxEnt->s.origin2);
		bboxEnt->s.dmgFlags = 1;    // ("type")
	}

	// Adjust the size
	if (ent->speed < FLAME_START_MAX_SIZE)
	{
		ent->speed += 10.f;

		if (ent->speed > FLAME_START_MAX_SIZE)
		{
			ent->speed = FLAME_START_MAX_SIZE;
		}
	}

	// Remove after 2 seconds
	if (level.time - ent->timestamp > (FLAME_LIFETIME - 150))       // increased to 350 from 250 to match visuals better
	{
		G_FreeEntity(ent);
		return;
	}

	G_RunThink(ent);
}

/*
=================
fire_flamechunk
=================
*/
gentity_t *fire_flamechunk(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt;

	// Only spawn every other frame
	if (self->count2)
	{
		self->count2--;
		return NULL;
	}

	self->count2 = 1;
	VectorNormalize(dir);

	bolt            = G_Spawn();
	bolt->classname = "flamechunk";

	bolt->timestamp      = level.time;
	bolt->flameQuotaTime = level.time + 50;
	bolt->s.eType        = ET_FLAMETHROWER_CHUNK;
	bolt->r.svFlags      = SVF_NOCLIENT;
	bolt->s.weapon       = self->s.weapon;
	bolt->r.ownerNum     = self->s.number;
	bolt->parent         = self;
	bolt->methodOfDeath  = MOD_FLAMETHROWER;
	bolt->clipmask       = MASK_MISSILESHOT;
	bolt->count2         = 0; // how often it bounced off of something

	bolt->s.pos.trType     = TR_DECCELERATE;
	bolt->s.pos.trTime     = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
	bolt->s.pos.trDuration = 800;

	// 'speed' will be the current size radius of the chunk
	bolt->speed = FLAME_START_SIZE;
	VectorSet(bolt->r.mins, -4, -4, -4);
	VectorSet(bolt->r.maxs, 4, 4, 4);
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, FLAME_START_SPEED, bolt->s.pos.trDelta);

	SnapVector(bolt->s.pos.trDelta);            // save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

void DynaSink(gentity_t *self)
{
	self->clipmask   = 0;
	self->r.contents = 0;

	if (self->timestamp < level.time)
	{
		self->think     = G_FreeEntity;
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	self->s.pos.trBase[2] -= 0.5f;
	self->nextthink        = level.time + 50;
}

void DynaFree(gentity_t *self)
{
	// see if the dynamite was planted near a constructable object that would have been destroyed
	int       entityList[MAX_GENTITIES];
	int       numListedEntities;
	int       e;
	vec3_t    org;
	gentity_t *hit;

	self->free = NULL;

	if (self->think != G_ExplodeMissile)
	{
		return; // we weren't armed, so no defused event
	}

	VectorCopy(self->r.currentOrigin, org);
	org[2] += 4;    // move out of ground

	numListedEntities = EntsThatRadiusCanDamage(org, self->splashRadius, entityList);

	for (e = 0; e < numListedEntities; e++)
	{
		hit = &g_entities[entityList[e]];

		if (hit->s.eType != ET_CONSTRUCTIBLE)
		{
			continue;
		}

		// invulnerable
		if (hit->spawnflags & 2)
		{
			continue;
		}

		// not dynamite-able
		if (!(hit->spawnflags & 32))
		{
			continue;
		}

		G_Script_ScriptEvent(hit, "defused", "");
	}
}

/*
==========
G_FadeItems

remove any items that the player should no longer have, on disconnect/class change etc
changed to just set the parent to NULL
==========
*/
void G_FadeItems(gentity_t *ent, int modType)
{
	gentity_t *e = &g_entities[MAX_CLIENTS];
	int       i;

	for (i = MAX_CLIENTS ; i < level.num_entities ; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (e->s.eType != ET_MISSILE)
		{
			continue;
		}

		if (e->methodOfDeath != modType)
		{
			continue;
		}

		if (e->parent != ent)
		{
			continue;
		}

		e->parent     = NULL;
		e->r.ownerNum = ENTITYNUM_NONE;

		G_FreeEntity(e);
	}
}

int G_CountTeamLandmines(team_t team)
{
	gentity_t *e = &g_entities[MAX_CLIENTS];
	int       i;
	int       cnt = 0;

	for (i = MAX_CLIENTS ; i < level.num_entities ; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (e->s.eType != ET_MISSILE)
		{
			continue;
		}

		if (e->methodOfDeath != MOD_LANDMINE)
		{
			continue;
		}

		if (e->s.teamNum % 4 == team && e->s.teamNum < 4)
		{
			cnt++;
		}
	}

	return cnt;
}

qboolean G_SweepForLandmines(vec3_t origin, float radius, int team)
{
	gentity_t *e = &g_entities[MAX_CLIENTS];
	int       i;
	vec3_t    dist;

	radius *= radius;

	for (i = MAX_CLIENTS; i < level.num_entities; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (e->s.eType != ET_MISSILE)
		{
			continue;
		}

		if (e->methodOfDeath != MOD_LANDMINE)
		{
			continue;
		}

		if (e->s.teamNum % 4 != team && e->s.teamNum < 4)
		{
			VectorSubtract(origin, e->r.currentOrigin, dist);
			if (VectorLengthSquared(dist) > radius)
			{
				continue;
			}

			return qtrue;   // found one
		}
	}

	return qfalse;
}

gentity_t *G_FindSatchel(gentity_t *ent)
{
	gentity_t *e = &g_entities[MAX_CLIENTS];
	int       i;

	for (i = MAX_CLIENTS ; i < level.num_entities ; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (e->s.eType != ET_MISSILE)
		{
			continue;
		}

		if (e->methodOfDeath != MOD_SATCHEL)
		{
			continue;
		}

		if (e->parent != ent)
		{
			continue;
		}

		return e;
	}

	return NULL;
}

/*
==========
G_ExplodeMines
==========
*/
// removes any weapon objects lying around in the map when they disconnect/switch team
void G_ExplodeMines(gentity_t *ent)
{
	G_FadeItems(ent, MOD_LANDMINE);
}

/*
==========
G_ExplodeSatchels
==========
*/
qboolean G_ExplodeSatchels(gentity_t *ent)
{
	gentity_t *e = &g_entities[MAX_CLIENTS];
	vec3_t    dist;
	int       i;
	qboolean  blown = qfalse;

	for (i = MAX_CLIENTS ; i < level.num_entities; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (e->s.eType != ET_MISSILE)
		{
			continue;
		}

		if (e->methodOfDeath != MOD_SATCHEL)
		{
			continue;
		}

		VectorSubtract(e->r.currentOrigin, ent->r.currentOrigin, dist);
		if (VectorLengthSquared(dist) > Square(2000))
		{
			continue;
		}

		if (e->parent != ent)
		{
			continue;
		}

		G_ExplodeMissile(e);
		blown = qtrue;
	}

	return blown;
}

void G_FreeSatchel(gentity_t *ent)
{
	gentity_t *other;

	ent->free = NULL;

	if (ent->s.eType != ET_MISSILE)
	{
		return;
	}

	other = &g_entities[ent->s.clientNum];

	if (!other->client || other->client->pers.connected != CON_CONNECTED)
	{
		return;
	}

	if (other->client->sess.playerType != PC_COVERTOPS)
	{
		return;
	}

	other->client->ps.ammo[WP_SATCHEL_DET]     = 0;
	other->client->ps.ammoclip[WP_SATCHEL_DET] = 0;
	other->client->ps.ammoclip[WP_SATCHEL]     = 1;
	if (other->client->ps.weapon == WP_SATCHEL_DET)
	{
		G_AddEvent(other, EV_NOAMMO, 0);
	}
}

/*
==========
LandMineTrigger
==========
*/
void LandminePostThink(gentity_t *self);

void LandMineTrigger(gentity_t *self)
{
	self->r.contents = CONTENTS_CORPSE;
	trap_LinkEntity(self);
	self->nextthink  = level.time + FRAMETIME;
	self->think      = LandminePostThink;
	self->s.teamNum += 8;
	// communicate trigger time to client
	self->s.time = level.time;
}

void LandMinePostTrigger(gentity_t *self)
{
	self->nextthink = level.time + 300;
	self->think     = G_ExplodeMissile;
}

/*107     11      20      0       0       0       0       //fire gren

==========
G_LandmineThink
==========
*/

// Function to check if an entity will set off a landmine
#define LANDMINE_TRIGGER_DIST 64.0f

qboolean sEntWillTriggerMine(gentity_t *ent, gentity_t *mine)
{
	// player types are the only things that set off mines (human and bot)
	if (ent->s.eType == ET_PLAYER && ent->client)
	{
		vec3_t dist;

		VectorSubtract(mine->r.currentOrigin, ent->r.currentOrigin, dist);
		// have to be within the trigger distance AND on the ground -- if we jump over a mine, we don't set it off
		//      (or if we fly by after setting one off)
		if ((VectorLengthSquared(dist) <= Square(LANDMINE_TRIGGER_DIST)) && (fabs(dist[2]) < 45.f))
		{
			return qtrue;
		}
	}

	return qfalse;
}

// Landmine waits for 2 seconds then primes, which sets think to checking for "enemies"
void G_LandmineThink(gentity_t *self)
{
	int       entityList[MAX_GENTITIES];
	int       i, cnt;
	vec3_t    range = { LANDMINE_TRIGGER_DIST, LANDMINE_TRIGGER_DIST, LANDMINE_TRIGGER_DIST };
	vec3_t    mins, maxs;
	qboolean  trigger = qfalse;
	gentity_t *ent;

	self->nextthink = level.time + FRAMETIME;

	if (level.time - self->missionLevel > 200)
	{
		self->s.density = 0; // time out the covert ops visibile thing, or we could get other clients being able to see mine later, etc
	}

	VectorSubtract(self->r.currentOrigin, range, mins);
	VectorAdd(self->r.currentOrigin, range, maxs);

	cnt = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (i = 0; i < cnt; i++)
	{
		ent = &g_entities[entityList[i]];

		if (!ent->client)
		{
			continue;
		}

		//if( !g_friendlyFire.integer && G_LandmineTeam( self ) == ent->client->sess.sessionTeam ) {
		//   continue;
		//}

#ifdef FEATURE_OMNIBOT
		if (!(g_OmniBotFlags.integer & OBF_TRIGGER_MINES) && ent->r.svFlags & SVF_BOT)
		{
			if (G_LandmineTeam(self) == ent->client->sess.sessionTeam)
			{
				continue;
			}
			if (G_LandmineSpotted(self))
			{
				continue;
			}
		}
#endif

		// use the unified trigger check to see if we are close enough to prime the mine
		if (sEntWillTriggerMine(ent, self))
		{
			trigger = qtrue;
			break;
		}
	}

	if (trigger)
	{
#ifdef FEATURE_OMNIBOT
		Bot_Event_PreTriggerMine(ent - g_entities, self);
#endif
		LandMineTrigger(self);
	}
}

void LandminePostThink(gentity_t *self)
{
	int       entityList[MAX_GENTITIES];
	int       i, cnt;
	vec3_t    range = { LANDMINE_TRIGGER_DIST, LANDMINE_TRIGGER_DIST, LANDMINE_TRIGGER_DIST };
	vec3_t    mins, maxs;
	qboolean  trigger = qfalse;
	gentity_t *ent;

	self->nextthink = level.time + FRAMETIME;

	if (level.time - self->missionLevel > 5000)
	{
		self->s.density = 0; // time out the covert ops visibile thing, or we could get other clients being able to see mine later, etc
	}

	VectorSubtract(self->r.currentOrigin, range, mins);
	VectorAdd(self->r.currentOrigin, range, maxs);

	cnt = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (i = 0; i < cnt; i++)
	{
		ent = &g_entities[entityList[i]];

		// use the unifed trigger check to see if we're still standing on the mine, so we don't set it off
		if (sEntWillTriggerMine(ent, self))
		{
			trigger = qtrue;
			break;
		}
	}

	if (!trigger)
	{
#ifdef FEATURE_OMNIBOT
		Bot_Event_PostTriggerMine(ent - g_entities, self);
#endif
		LandMinePostTrigger(self);
	}
}

/*
==========
G_LandminePrime
==========
*/
void G_LandminePrime(gentity_t *self)
{
	self->nextthink = level.time + FRAMETIME;
	self->think     = G_LandmineThink;
}

qboolean G_LandmineSnapshotCallback(int entityNum, int clientNum)
{
	gentity_t *ent   = &g_entities[entityNum];
	gentity_t *clEnt = &g_entities[clientNum];
	team_t    team;

	if (clEnt->client->sess.skill[SK_BATTLE_SENSE] >= 4)
	{
		return qtrue;
	}

	if (!G_LandmineArmed(ent))
	{
		return qtrue;
	}

	if (G_LandmineSpotted(ent))
	{
		return qtrue;
	}

	team = G_LandmineTeam(ent);
	if (team == clEnt->client->sess.sessionTeam)
	{
		return qtrue;
	}

	// fix for covops spotting
	if (clEnt->client->sess.playerType == PC_COVERTOPS && (clEnt->client->ps.eFlags & EF_ZOOMING) && (clEnt->client->ps.stats[STAT_KEYS] & (1 << INV_BINOCS)))
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
fire_grenade

    NOTE!!!! NOTE!!!!!

    This accepts a /non-normalized/ direction vector to allow specification
    of how hard it's thrown.  Please scale the vector before calling.
=================
*/
gentity_t *fire_grenade(gentity_t *self, vec3_t start, vec3_t dir, int grenadeWPID)
{
	gentity_t *bolt     = G_Spawn();
	qboolean  noExplode = qfalse;

	// no self->client for shooter_grenade's
	if (self->client && self->client->ps.grenadeTimeLeft)
	{
		bolt->nextthink = level.time + self->client->ps.grenadeTimeLeft;
	}
	else
	{
		bolt->nextthink = level.time + 2500;
	}

	switch (grenadeWPID)
	{
	case WP_DYNAMITE:
		noExplode       = qtrue;
		bolt->nextthink = level.time + 15000;
		bolt->think     = DynaSink;
		bolt->timestamp = level.time + 16500;
		bolt->free      = DynaFree;
		break;
	case WP_LANDMINE:
		noExplode       = qtrue;
		bolt->nextthink = level.time + 15000;
		bolt->think     = DynaSink;
		bolt->timestamp = level.time + 16500;
		break;
	case WP_SATCHEL:
		noExplode         = qtrue;
		bolt->nextthink   = 0;
		bolt->s.clientNum = self->s.clientNum;
		bolt->free        = G_FreeSatchel;
		break;
	case WP_MORTAR_SET: // only on impact
	case WP_MORTAR2_SET:
		noExplode       = qtrue;
		bolt->nextthink = 0;
		break;
	default:
		break;
	}

	// no self->client for shooter_grenade's
	if (self->client)
	{
		self->client->ps.grenadeTimeLeft = 0;       // reset grenade timer

	}
	if (!noExplode)
	{
		bolt->think = G_ExplodeMissile;
	}

	bolt->s.eType    = ET_MISSILE;
	bolt->r.svFlags  = SVF_BROADCAST;
	bolt->s.weapon   = grenadeWPID;
	bolt->r.ownerNum = self->s.number;
	bolt->parent     = self;
	bolt->s.teamNum  = self->client->sess.sessionTeam;

	// commented out bolt->damage and bolt->splashdamage, override with GetWeaponTableData(WP_X)->damage()
	// so it works with different netgame balance.  didn't uncomment bolt->damage on dynamite 'cause its so *special*
	bolt->damage       = GetWeaponTableData(grenadeWPID)->damage; // overridden for dynamite, satchel, landmine
	bolt->splashDamage = GetWeaponTableData(grenadeWPID)->damage;

	switch (grenadeWPID)
	{
	case WP_GPG40:
		bolt->classname           = "gpg40_grenade";
		bolt->splashRadius        = 300;
		bolt->methodOfDeath       = MOD_GPG40;
		bolt->splashMethodOfDeath = MOD_GPG40;
		bolt->s.eFlags            = EF_BOUNCE_HALF | EF_BOUNCE;
		bolt->nextthink           = level.time + 4000;
		break;
	case WP_M7:
		bolt->classname           = "m7_grenade";
		bolt->splashRadius        = 300;
		bolt->methodOfDeath       = MOD_M7;
		bolt->splashMethodOfDeath = MOD_M7;
		bolt->s.eFlags            = EF_BOUNCE_HALF | EF_BOUNCE;
		bolt->nextthink           = level.time + 4000;
		break;
	case WP_SMOKE_BOMB:
		bolt->classname     = "smoke_bomb";
		bolt->s.eFlags      = EF_BOUNCE_HALF | EF_BOUNCE;
		bolt->methodOfDeath = MOD_SMOKEBOMB;
		break;
	case WP_GRENADE_LAUNCHER:
		bolt->classname           = "grenade";
		bolt->splashRadius        = 300;
		bolt->methodOfDeath       = MOD_GRENADE_LAUNCHER;
		bolt->splashMethodOfDeath = MOD_GRENADE_LAUNCHER;
		bolt->s.eFlags            = EF_BOUNCE_HALF | EF_BOUNCE;
		break;
	case WP_GRENADE_PINEAPPLE:
		bolt->classname           = "grenade";
		bolt->splashRadius        = 300;
		bolt->methodOfDeath       = MOD_GRENADE_PINEAPPLE;
		bolt->splashMethodOfDeath = MOD_GRENADE_PINEAPPLE;
		bolt->s.eFlags            = EF_BOUNCE_HALF | EF_BOUNCE;
		break;
	case WP_SMOKE_MARKER:
		bolt->classname = "grenade";
		bolt->s.eFlags  = EF_BOUNCE_HALF | EF_BOUNCE;
		// properly set MOD
		bolt->methodOfDeath       = MOD_SMOKEGRENADE;
		bolt->splashMethodOfDeath = MOD_SMOKEGRENADE;
		break;
	case WP_MORTAR_SET:
		bolt->classname           = "mortar_grenade";
		bolt->splashRadius        = 800;
		bolt->methodOfDeath       = MOD_MORTAR;
		bolt->splashMethodOfDeath = MOD_MORTAR;
		bolt->s.eFlags            = 0;
		break;
	case WP_MORTAR2_SET:
		bolt->classname           = "mortar_grenade";
		bolt->splashRadius        = 800;
		bolt->methodOfDeath       = MOD_MORTAR2;
		bolt->splashMethodOfDeath = MOD_MORTAR2;
		bolt->s.eFlags            = 0;
		break;
	case WP_LANDMINE:
		bolt->accuracy            = 0;
		bolt->s.teamNum           = self->client->sess.sessionTeam + 4;
		bolt->classname           = "landmine";
		bolt->damage              = 0;
		bolt->splashRadius        = 225;
		bolt->methodOfDeath       = MOD_LANDMINE;
		bolt->splashMethodOfDeath = MOD_LANDMINE;
		bolt->s.eFlags            = (EF_BOUNCE | EF_BOUNCE_HALF);
		bolt->health              = 5;
		bolt->takedamage          = qtrue;
		bolt->r.contents          = CONTENTS_CORPSE;        // (player can walk through)

		bolt->r.snapshotCallback = qtrue;

		VectorSet(bolt->r.mins, -16, -16, 0);
		VectorCopy(bolt->r.mins, bolt->r.absmin);
		VectorSet(bolt->r.maxs, 16, 16, 16);
		VectorCopy(bolt->r.maxs, bolt->r.absmax);
		break;
	case WP_SATCHEL:
		bolt->accuracy            = 0;
		bolt->classname           = "satchel_charge";
		bolt->damage              = 0;
		bolt->splashRadius        = 300;
		bolt->methodOfDeath       = MOD_SATCHEL;
		bolt->splashMethodOfDeath = MOD_SATCHEL;
		bolt->s.eFlags            = (EF_BOUNCE | EF_BOUNCE_HALF);
		bolt->health              = 5;
		bolt->takedamage          = qfalse;
		bolt->r.contents          = CONTENTS_CORPSE;        // (player can walk through)

		VectorSet(bolt->r.mins, -12, -12, 0);
		VectorCopy(bolt->r.mins, bolt->r.absmin);
		VectorSet(bolt->r.maxs, 12, 12, 20);
		VectorCopy(bolt->r.maxs, bolt->r.absmax);
		break;
	case WP_DYNAMITE:
		bolt->accuracy = 0;     // sets to score below if dynamite is in trigger_objective_info & it's an objective
		trap_SendServerCommand(self - g_entities, "cp \"Dynamite is set, but NOT armed!\"");
		// differentiate non-armed dynamite with non-pulsing dlight
		bolt->s.teamNum           = self->client->sess.sessionTeam + 4;
		bolt->classname           = "dynamite";
		bolt->damage              = 0;
		bolt->splashRadius        = 400;
		bolt->methodOfDeath       = MOD_DYNAMITE;
		bolt->splashMethodOfDeath = MOD_DYNAMITE;
		bolt->s.eFlags            = (EF_BOUNCE | EF_BOUNCE_HALF);

		// dynamite is shootable
		bolt->health     = 5;
		bolt->takedamage = qfalse;

		bolt->r.contents = CONTENTS_CORPSE;                 // (player can walk through)

		// nope - this causes the dynamite to impact on the players bb when he throws it.
		// will try setting it when it settles
		//bolt->r.ownerNum            = ENTITYNUM_WORLD;  // make the world the owner of the dynamite, so the player can shoot it without modifying the bullet code to ignore players id for hits

		// small target cube
		VectorSet(bolt->r.mins, -12, -12, 0);
		VectorCopy(bolt->r.mins, bolt->r.absmin);
		VectorSet(bolt->r.maxs, 12, 12, 20);
		VectorCopy(bolt->r.maxs, bolt->r.absmax);
		break;
	}

	// blast radius proportional to damage
	bolt->splashRadius = GetWeaponTableData(grenadeWPID)->damage;

	bolt->clipmask = MASK_MISSILESHOT;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);
	VectorCopy(dir, bolt->s.pos.trDelta);

	// add velocity of player (:sigh: guess people don't like it)
	//VectorAdd( bolt->s.pos.trDelta, self->s.pos.trDelta, bolt->s.pos.trDelta );

	// add velocity of ground entity
	if (self->s.groundEntityNum != ENTITYNUM_NONE && self->s.groundEntityNum != ENTITYNUM_WORLD)
	{
		VectorAdd(bolt->s.pos.trDelta, g_entities[self->s.groundEntityNum].instantVelocity, bolt->s.pos.trDelta);
	}

	SnapVector(bolt->s.pos.trDelta);            // save net bandwidth

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket(gentity_t *self, vec3_t start, vec3_t dir, int rocketType)
{
	gentity_t *bolt = G_Spawn();
	int       mod;

	// FIXME: weapon table
	switch (rocketType)
	{
	case WP_BAZOOKA:
		mod = MOD_BAZOOKA;
		break;
	default:
		rocketType = WP_PANZERFAUST;
		mod        = MOD_PANZERFAUST;
		break;
	}

	VectorNormalize(dir);

	bolt->classname = "rocket";
	bolt->nextthink = level.time + 20000;   // push it out a little
	bolt->think     = G_ExplodeMissile;
	bolt->accuracy  = 4;
	bolt->s.eType   = ET_MISSILE;
	bolt->r.svFlags = SVF_BROADCAST;

	// Use the correct weapon in multiplayer
	bolt->s.weapon = self->s.weapon;

	bolt->r.ownerNum          = self->s.number;
	bolt->parent              = self;
	bolt->damage              = GetWeaponTableData(WP_PANZERFAUST)->damage;
	bolt->splashDamage        = GetWeaponTableData(WP_PANZERFAUST)->damage;
	bolt->splashRadius        = 300; //G_GetWeaponDamage(WP_PANZERFAUST);  // hardcoded bleh hack FIXME: weapon table
	bolt->methodOfDeath       = mod;
	bolt->splashMethodOfDeath = mod;
	bolt->clipmask            = MASK_MISSILESHOT;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	VectorScale(dir, 2500, bolt->s.pos.trDelta);

	SnapVector(bolt->s.pos.trDelta);            // save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	if (self->client)
	{
		bolt->s.teamNum = self->client->sess.sessionTeam;
	}

	return bolt;
}

/*
======================
fire_flamebarrel
======================
*/
gentity_t *fire_flamebarrel(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt = G_Spawn();

	VectorNormalize(dir);

	// for explosion type
	bolt->accuracy = 3;

	bolt->classname    = "flamebarrel";
	bolt->nextthink    = level.time + 3000;
	bolt->think        = G_ExplodeMissile;
	bolt->s.eType      = ET_FLAMEBARREL;
	bolt->s.eFlags     = EF_BOUNCE_HALF;
	bolt->r.svFlags    = SVF_BLANK;
	bolt->s.weapon     = WP_PANZERFAUST;
	bolt->r.ownerNum   = self->s.number;
	bolt->parent       = self;
	bolt->damage       = 100;
	bolt->splashDamage = 20;
	bolt->splashRadius = 60;

	bolt->methodOfDeath       = MOD_EXPLOSIVE;
	bolt->splashMethodOfDeath = MOD_EXPLOSIVE;

	bolt->clipmask = MASK_MISSILESHOT;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, 900 + (crandom() * 100), bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);            // save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

// sniper

/*
==============
visible
==============
*/
qboolean visible(gentity_t *self, gentity_t *other)
{
	trace_t   tr;
	gentity_t *traceEnt;

	trap_Trace(&tr, self->r.currentOrigin, NULL, NULL, other->r.currentOrigin, self->s.number, MASK_SHOT);

	traceEnt = &g_entities[tr.entityNum];

	if (traceEnt == other)
	{
		return qtrue;
	}

	return qfalse;
}

/*
==============
fire_mortar
    dir is a non-normalized direction/power vector
==============
*/
gentity_t *fire_mortar(gentity_t *self, vec3_t start, vec3_t dir)
{
	gentity_t *bolt = G_Spawn();

	//  VectorNormalize (dir);

	if (self->spawnflags)
	{
		gentity_t *tent;

		tent            = G_TempEntity(self->s.pos.trBase, EV_MORTAREFX);
		tent->s.density = self->spawnflags; // send smoke and muzzle flash flags
		VectorCopy(self->s.pos.trBase, tent->s.origin);
		VectorCopy(self->s.apos.trBase, tent->s.angles);
	}

	bolt->classname = "mortar";
	bolt->nextthink = level.time + 20000;   // push it out a little
	bolt->think     = G_ExplodeMissile;

	// for explosion type
	bolt->accuracy = 4;

	bolt->s.eType = ET_MISSILE;

	bolt->r.svFlags           = SVF_BROADCAST; // broadcast sound.  not multiplayer friendly, but for mortars it should be okay
	bolt->s.weapon            = WP_MAPMORTAR;
	bolt->r.ownerNum          = self->s.number;
	bolt->parent              = self;
	bolt->damage              = GetWeaponTableData(WP_MAPMORTAR)->damage;
	bolt->splashDamage        = GetWeaponTableData(WP_MAPMORTAR)->damage;
	bolt->splashRadius        = 120;
	bolt->methodOfDeath       = MOD_MAPMORTAR;
	bolt->splashMethodOfDeath = MOD_MAPMORTAR_SPLASH;
	bolt->clipmask            = MASK_MISSILESHOT;

	if (self->client)
	{
		bolt->s.clientNum = self->client->ps.clientNum;
	}
	else
	{
		bolt->s.clientNum = -1;
	}

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	VectorCopy(dir, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);            // save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}
