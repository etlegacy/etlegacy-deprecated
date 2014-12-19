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
 * @file cg_weapons.c
 * @brief Events and effects dealing with weapons
 */

#include "cg_local.h"

vec3_t ejectBrassCasingOrigin;

// forward decs
int CG_WeaponIndex(int weapnum, int *bank, int *cycle);
extern int weapBanksMultiPlayer[MAX_WEAP_BANKS_MP][MAX_WEAPS_IN_BANK_MP]; // moved to bg_misc.c so I can get a droplist

/*
==============
CG_StartWeaponAnim
==============
*/
static void CG_StartWeaponAnim(int anim)
{
	if (cg.predictedPlayerState.pm_type >= PM_DEAD)
	{
		return;
	}

	if (cg.pmext.weapAnimTimer > 0)
	{
		return;
	}

	if (cg.predictedPlayerState.weapon == WP_NONE)
	{
		return;
	}

	cg.predictedPlayerState.weapAnim = ((cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

static void CG_ContinueWeaponAnim(int anim)
{
	if (cg.predictedPlayerState.weapon == WP_NONE)
	{
		return;
	}

	if ((cg.predictedPlayerState.weapAnim & ~ANIM_TOGGLEBIT) == anim)
	{
		return;
	}
	if (cg.pmext.weapAnimTimer > 0)
	{
		return;     // a high priority animation is running
	}
	CG_StartWeaponAnim(anim);
}

/*
==============
AddLean
    add leaning offset
==============
*/
void AddLean(vec3_t viewAngle, vec3_t point, float ammount)
{
	if (ammount)
	{
		vec3_t up, right;

		AngleVectors(viewAngle, up, right, NULL);
		VectorMA(point, ammount, right, point);
		// to match client's view
		point[2] -= fabs(ammount / 3.5);
	}
}

/*
==============
CG_MachineGunEjectBrassNew
==============
*/
void CG_MachineGunEjectBrassNew(centity_t *cent)
{
	localEntity_t *le;
	refEntity_t   *re;
	vec3_t        velocity, xvelocity;
	float         waterScale = 1.0f;
	vec3_t        v[3], end;

	if (cg_brassTime.integer <= 0)
	{
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = -50 + 25 * crandom();
	velocity[1] = -100 + 40 * crandom();
	velocity[2] = 200 + 50 * random();

	le->leType    = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime   = le->startTime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);

	AnglesToAxis(cent->lerpAngles, v);

	VectorCopy(ejectBrassCasingOrigin, re->origin);

	VectorCopy(re->origin, le->pos.trBase);

	if (CG_PointContents(re->origin, -1) & (CONTENTS_WATER | CONTENTS_SLIME))         // modified since slime is no longer deadly
	{
		waterScale = 0.1f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->hModel = cgs.media.smallgunBrassModel;

	le->bounceFactor = 0.4f * waterScale;

	le->angles.trType     = TR_LINEAR;
	le->angles.trTime     = cg.time;
	le->angles.trBase[0]  = (rand() & 31) + 60;  // bullets should come out horizontal not vertical JPW NERVE
	le->angles.trBase[1]  = rand() & 255; // random spin from extractor
	le->angles.trBase[2]  = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	if (CG_PointContents(end, 0) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		le->leBounceSoundType = LEBS_NONE;
	}
	else
	{
		le->leBounceSoundType = LEBS_BRASS;
	}

	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_MachineGunEjectBrass
==========================
*/
void CG_MachineGunEjectBrass(centity_t *cent)
{
	localEntity_t *le;
	refEntity_t   *re;
	vec3_t        velocity, xvelocity;
	vec3_t        offset, xoffset;
	float         waterScale = 1.0f;
	vec3_t        v[3];

	if (cg_brassTime.integer <= 0)
	{
		return;
	}

	if (!(cg.snap->ps.persistant[PERS_HWEAPON_USE]) && (cent->currentState.clientNum == cg.snap->ps.clientNum) && (!((cent->currentState.eFlags & EF_MG42_ACTIVE) || (cent->currentState.eFlags & EF_AAGUN_ACTIVE))))
	{
		CG_MachineGunEjectBrassNew(cent);
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType    = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime   = le->startTime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);

	AnglesToAxis(cent->lerpAngles, v);

	// new brass handling behavior because the SP stuff just doesn't cut it for MP
	if ((cent->currentState.eFlags & EF_MG42_ACTIVE) || (cent->currentState.eFlags & EF_AAGUN_ACTIVE))
	{
		offset[0]             = 25;
		offset[1]             = -4;
		offset[2]             = 28;
		velocity[0]           = -20 + 40 * crandom(); // more reasonable brass ballistics for a machinegun
		velocity[1]           = -150 + 40 * crandom();
		velocity[2]           = 100 + 50 * crandom();
		re->hModel            = cgs.media.machinegunBrassModel;
		le->angles.trBase[0]  = 90; //rand()&31; // belt-fed rounds should come out horizontal
		le->angles.trBase[1]  = rand() & 255;
		le->angles.trBase[2]  = rand() & 31;
		le->angles.trDelta[0] = 2;
		le->angles.trDelta[1] = 1;
		le->angles.trDelta[2] = 0;
	}
	else
	{
		switch (cent->currentState.weapon)
		{
		case WP_LUGER:
		case WP_COLT:
		case WP_SILENCER:
		case WP_SILENCED_COLT:
			offset[0]  = 24;
			offset[1]  = -4;
			offset[2]  = 36;
			re->hModel = cgs.media.smallgunBrassModel;
			break;
		case WP_MOBILE_MG42:
		case WP_MOBILE_MG42_SET:
		case WP_MOBILE_BROWNING:
		case WP_MOBILE_BROWNING_SET:
			offset[0]  = 12;
			offset[1]  = -4;
			offset[2]  = 24;
			re->hModel = cgs.media.machinegunBrassModel;
			break;
		case WP_KAR98:
		case WP_CARBINE:
		case WP_K43:
			offset[0]  = 16;
			offset[1]  = -4;
			offset[2]  = 24;
			re->hModel = cgs.media.machinegunBrassModel;
			break;
		case WP_MP40:
		case WP_THOMPSON:
		case WP_STEN:
		default:
			offset[0]  = 16;
			offset[1]  = -4;
			offset[2]  = 24;
			re->hModel = cgs.media.smallgunBrassModel;
			break;
		}
		velocity[0] = -50 + 25 * crandom();
		velocity[1] = -100 + 40 * crandom();
		velocity[2] = 200 + 50 * random();

		le->angles.trBase[0]  = (rand() & 15) + 82;  // bullets should come out horizontal not vertical JPW NERVE
		le->angles.trBase[1]  = rand() & 255; // random spin from extractor
		le->angles.trBase[2]  = rand() & 31;
		le->angles.trDelta[0] = 2;
		le->angles.trDelta[1] = 1;
		le->angles.trDelta[2] = 0;
	}

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd(cent->lerpOrigin, xoffset, re->origin);

	VectorCopy(re->origin, le->pos.trBase);

	if (CG_PointContents(re->origin, -1) & (CONTENTS_WATER | CONTENTS_SLIME)) // modified since slime is no longer deadly
	{
		waterScale = 0.10;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;

	le->leFlags = LEF_TUMBLE;

	{
		vec3_t end;

		VectorCopy(cent->lerpOrigin, end);
		end[2] -= 24;

		if (CG_PointContents(end, 0) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
		{
			le->leBounceSoundType = LEBS_NONE;
		}
		else
		{
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}

/*
==============
CG_PanzerFaustEjectBrass
    toss the 'used' panzerfaust casing (unit is one-shot, disposable)
==============
*/
static void CG_PanzerFaustEjectBrass(centity_t *cent)
{
	localEntity_t *le      = CG_AllocLocalEntity();
	refEntity_t   *re      = &le->refEntity;
	vec3_t        velocity = { 16, -200, 0 };
	vec3_t        offset   = { -24, -4, 24 }; // forward, left, up
	vec3_t        xoffset, xvelocity;
	float         waterScale = 1.0f;
	vec3_t        v[3];

	le->leType    = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime   = le->startTime + (cg_brassTime.integer * 8) + (cg_brassTime.integer * random());

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);
	//le->pos.trTime = cg.time - 2000;

	AnglesToAxis(cent->lerpAngles, v);

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd(cent->lerpOrigin, xoffset, re->origin);

	VectorCopy(re->origin, le->pos.trBase);

	if (CG_PointContents(re->origin, -1) & (CONTENTS_WATER | CONTENTS_SLIME))
	{
		waterScale = 0.1f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);

	// make it bigger
	le->sizeScale = 3.0f;

	re->hModel = cgs.media.panzerfaustBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;

	le->angles.trBase[0] = 0;
	le->angles.trBase[1] = cent->currentState.apos.trBase[1];   // rotate to match the player
	le->angles.trBase[2] = 0;

	le->angles.trDelta[0] = 0;
	le->angles.trDelta[1] = 0;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE | LEF_SMOKING;   // probably doesn't need to be 'tumble' since it doesn't really rotate much when flying

	le->leBounceSoundType = LEBS_NONE;

	le->leMarkType = LEMT_NONE;
}

/*
==============
CG_SpearTrail
    simple bubble trail behind a missile
==============
*/
/*void CG_SpearTrail(centity_t *ent, const weaponInfo_t *wi )
{
    int     contents, lastContents;
    vec3_t  origin, lastPos;
    entityState_t   *es;

    es = &ent->currentState;
    BG_EvaluateTrajectory( &es->pos, cg.time, origin );
    contents = CG_PointContents( origin, -1 );

    BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
    lastContents = CG_PointContents( lastPos, -1 );

    ent->trailTime = cg.time;

    if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
        if ( contents & lastContents & CONTENTS_WATER ) {
            CG_BubbleTrail( lastPos, origin, 1, 8 );
        }
    }
}*/


/*
==========================
CG_GetWindVector

  compute random wind vector for smoke puff
==========================
*/
void CG_GetWindVector(vec3_t dir)
{
	dir[0] = random() * 0.25f;
	dir[1] = sin(0.00001f * cg.time); // simulate a little wind so it looks natural
	dir[2] = random(); // one direction (so smoke goes side-like)
	VectorNormalize(dir);
}

// LT/FOPS pyro for marking air strikes
/*
==========================
CG_PyroSmokeTrail
==========================
*/
void CG_PyroSmokeTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int    step;
	vec3_t origin, lastPos, dir;
	//int           contents;
	int           startTime;
	entityState_t *es;
	int           t;
	float         rnd;
	team_t        team;

	if (ent->currentState.weapon == WP_LANDMINE)
	{
		if (ent->currentState.teamNum < 8)
		{
			ent->miscTime = 0;
			return;
		}
		else if (ent->currentState.teamNum < 12)
		{
			if (!ent->miscTime)
			{
				ent->trailTime = cg.time;
				ent->miscTime  = cg.time;

				// play the armed sound - weird place to do it but saves us sending an event
				trap_S_StartSound(NULL, ent->currentState.number, CHAN_WEAPON, cgs.media.minePrimedSound);
			}
		}

		if (cg.time - ent->miscTime > 1000)
		{
			return;
		}

		if (ent->currentState.otherEntityNum2)
		{
			team = TEAM_AXIS;
		}
		else
		{
			team = TEAM_ALLIES;
		}
	}
	else
	{
		team = ent->currentState.teamNum;
	}

	step      = 50;
	es        = &ent->currentState;
	startTime = ent->trailTime;
	t         = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin, qfalse, es->effect2Time);
	CG_PointContents(origin, -1);

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos, qfalse, es->effect2Time);
	CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	/* smoke pyro works fine in water (well, it's dye in real life, might wanna change this in-game)
	    if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	        return;
	*/

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step)
	{
		BG_EvaluateTrajectory(&es->pos, t, lastPos, qfalse, es->effect2Time);

		if (ent->currentState.density)     // corkscrew effect
		{
			vec3_t right;
			vec3_t angles;

			VectorCopy(ent->currentState.apos.trBase, angles);
			angles[ROLL] += cg.time % 360;
			AngleVectors(angles, NULL, right, NULL);
			VectorMA(lastPos, ent->currentState.density, right, lastPos);
		}

		dir[0] = crandom() * 5; // compute offset from flare base
		dir[1] = crandom() * 5;
		dir[2] = 0;
		VectorAdd(lastPos, dir, origin); // store in origin

		CG_GetWindVector(dir);
		if (ent->currentState.weapon == WP_LANDMINE)
		{
			VectorScale(dir, 45, dir);
		}
		else
		{
			VectorScale(dir, 65, dir);
		}

		rnd = random();

		if (team == TEAM_ALLIES)     // allied team, generate blue smoke
		{
			CG_SmokePuff(origin, dir,
			             25 + rnd * 110,       // width
			             rnd * 0.5 + 0.5, rnd * 0.5 + 0.5, 1, 0.5,
			             4800 + (rand() % 2800),         // duration was 2800+
			             t,
			             0,
			             0,
			             cgs.media.smokePuffShader);
		}
		else
		{
			CG_SmokePuff(origin, dir,
			             25 + rnd * 110,       // width
			             1.0, rnd * 0.5 + 0.5, rnd * 0.5 + 0.5, 0.5,
			             4800 + (rand() % 2800),         // duration was 2800+
			             t,
			             0,
			             0,
			             cgs.media.smokePuffShader);
		}
	}
}

// new trail effects

/*
==========================
CG_RocketTrail
==========================
*/
void CG_RocketTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int           step;
	vec3_t        origin, lastPos;
	int           contents;
	int           lastContents, startTime = ent->trailTime;
	entityState_t *es = &ent->currentState;
	int           t;

	if (ent->currentState.eType == ET_FLAMEBARREL)
	{
		step = 30;
	}
	else if (ent->currentState.eType == ET_FP_PARTS)
	{
		step = 50;
	}
	else // ent->currentState.eType == ET_RAMJET & others
	{
		step = 10;
	}

	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin, qfalse, es->effect2Time);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ((ent->currentState.eType != ET_RAMJET) && es->pos.trType == TR_STATIONARY)
	{
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos, qfalse, es->effect2Time);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		if (contents & lastContents & CONTENTS_WATER)
		{
			CG_BubbleTrail(lastPos, origin, 3, 8);
		}
		return;
	}

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step)
	{
		float rnd;

		BG_EvaluateTrajectory(&es->pos, t, lastPos, qfalse, es->effect2Time);
		rnd = random();

		switch (ent->currentState.eType)
		{
		case ET_FLAMEBARREL:
			if ((rand() % 100) > 50)
			{
				CG_ParticleExplosion("twiltb2", lastPos, vec3_origin, 100 + (int)(rnd * 400), 5, 7 + (int)(rnd * 10), qfalse);       // fire

			}
			CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 800 + (int)(rnd * 1500), 5, 12 + (int)(rnd * 30), qfalse);          // smoke
			break;
		case ET_FP_PARTS:
			if ((rand() % 100) > 50)
			{
				CG_ParticleExplosion("twiltb2", lastPos, vec3_origin, 100 + (int)(rnd * 400), 5, 7 + (int)(rnd * 10), qfalse);       // fire

			}
			CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 800 + (int)(rnd * 1500), 5, 12 + (int)(rnd * 30), qfalse);          // smoke
			break;
		case ET_RAMJET:
		{
			int duration;

			VectorCopy(ent->lerpOrigin, lastPos);
			duration = 100;
			CG_ParticleExplosion("twiltb2", lastPos, vec3_origin, duration + (int)(rnd * 100), 5, 5 + (int)(rnd * 10), qfalse);       // fire
			CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 400 + (int)(rnd * 750), 12, 24 + (int)(rnd * 30), qfalse);          // smoke
		}
		break;
		case ET_FIRE_COLUMN:
		case ET_FIRE_COLUMN_SMOKE:
		{
			int duration;
			int sizeStart;
			int sizeEnd;

			//VectorCopy (ent->lerpOrigin, lastPos);

			if (ent->currentState.density)     // corkscrew effect
			{
				vec3_t right;
				vec3_t angles;

				VectorCopy(ent->currentState.apos.trBase, angles);
				angles[ROLL] += cg.time % 360;
				AngleVectors(angles, NULL, right, NULL);
				VectorMA(lastPos, ent->currentState.density, right, lastPos);
			}

			duration  = ent->currentState.angles[0];
			sizeStart = ent->currentState.angles[1];
			sizeEnd   = ent->currentState.angles[2];

			if (!duration)
			{
				duration = 100;
			}

			if (!sizeStart)
			{
				sizeStart = 5;
			}

			if (!sizeEnd)
			{
				sizeEnd = 7;
			}

			CG_ParticleExplosion("twiltb2", lastPos, vec3_origin, duration + (int)(rnd * 400), sizeStart, sizeEnd + (int)(rnd * 10), qfalse);         // fire

			if (ent->currentState.eType == ET_FIRE_COLUMN_SMOKE && (rand() % 100) > 50)
			{
				CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 800 + (int)(rnd * 1500), 5, 12 + (int)(rnd * 30), qfalse);          // smoke
			}
		}
		break;
		default:
			//CG_ParticleExplosion( "twiltb", lastPos, vec3_origin, 300+(int)(rnd*100), 4, 14+(int)(rnd*8) );   // fire
			CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 800 + (int)(rnd * 1500), 5, 12 + (int)(rnd * 30), qfalse);          // smoke
			break;
		}
	}
	/*
	    // spawn a smoke junction
	    if ((cg.time - ent->lastTrailTime) >= 50 + rand()%50) {
	        ent->headJuncIndex = CG_AddSmokeJunc( ent->headJuncIndex,
	                                                ent, // rain - zinx's trail fix
	                                                cgs.media.smokeTrailShader,
	                                                origin,
	                                                4500, 0.4, 20, 80 );
	        ent->lastTrailTime = cg.time;
	    }
	*/
}

/*
==========================
CG_DynamiteTrail
==========================
*/
static void CG_DynamiteTrail(centity_t *ent, const weaponInfo_t *wi)
{
	vec3_t origin;
	float  mult;

	BG_EvaluateTrajectory(&ent->currentState.pos, cg.time, origin, qfalse, ent->currentState.effect2Time);

	if (ent->currentState.teamNum < 4)
	{
		mult = 0.004f * (cg.time - ent->currentState.effect1Time) / 30000.0f;
		trap_R_AddLightToScene(origin, 320, fabs(sin((cg.time - ent->currentState.effect1Time) * mult)), 1.0f, 0, 0, 0, REF_FORCE_DLIGHT);
	}
	else
	{
		mult = 1 - ((cg.time - ent->trailTime) / 15500.0f);
		trap_R_AddLightToScene(origin, 320, mult, 1.0f, 1.0f, 0, 0, REF_FORCE_DLIGHT);
	}
}

/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int           step = 15;  // nice and smooth curves
	vec3_t        origin, lastPos;
	int           contents;
	int           lastContents;
	int           startTime = ent->trailTime;
	entityState_t *es       = &ent->currentState;
	int           t         = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin, qfalse, es->effect2Time);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if (es->pos.trType == TR_STATIONARY)
	{
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos, qfalse, es->effect2Time);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		if (contents & lastContents & CONTENTS_WATER)
		{
			CG_BubbleTrail(lastPos, origin, 2, 8);
		}
		return;
	}

	// spawn smoke junctions
	for ( ; t <= ent->trailTime ; t += step)
	{
		BG_EvaluateTrajectory(&es->pos, t, origin, qfalse, es->effect2Time);
		ent->headJuncIndex = CG_AddSmokeJunc(ent->headJuncIndex,
		                                     ent,    // trail fix
		                                     cgs.media.smokeTrailShader,
		                                     origin,
		                                     1000, 0.3f, 2, 20);
		ent->lastTrailTime = cg.time;
	}
}

/*
==========================
CG_RailTrail
    re-inserted this as a debug mechanism for bullets
==========================
*/
void CG_RailTrail2(vec3_t color, clientInfo_t *ci, vec3_t start, vec3_t end)
{
	localEntity_t *le = CG_AllocLocalEntity();
	refEntity_t   *re = &le->refEntity;

	le->leType    = LE_CONST_RGB;
	le->startTime = cg.time;
	le->endTime   = cg.time + cg_railTrailTime.value;
	le->lifeRate  = 1.0 / (le->endTime - le->startTime);

	re->shaderTime   = cg.time / 1000.0f;
	re->reType       = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);

	le->color[0] = color[0];
	le->color[1] = color[1];
	le->color[2] = color[2];
	le->color[3] = 1.0f;

	AxisClear(re->axis);
}

/**
 * @brief draw boxes for debugging
 */
void CG_RailTrail(vec3_t color, clientInfo_t *ci, vec3_t start, vec3_t end, int type)      // added 'type'
{
	vec3_t diff, v1, v2, v3, v4, v5, v6;

	if (!type)     // just a line
	{
		CG_RailTrail2(color, ci, start, end);
		return;
	}

	// type '1' (box)

	VectorSubtract(start, end, diff);

	VectorCopy(start, v1);
	VectorCopy(start, v2);
	VectorCopy(start, v3);
	v1[0] -= diff[0];
	v2[1] -= diff[1];
	v3[2] -= diff[2];
	CG_RailTrail2(color, ci, start, v1);
	CG_RailTrail2(color, ci, start, v2);
	CG_RailTrail2(color, ci, start, v3);

	VectorCopy(end, v4);
	VectorCopy(end, v5);
	VectorCopy(end, v6);
	v4[0] += diff[0];
	v5[1] += diff[1];
	v6[2] += diff[2];
	CG_RailTrail2(color, ci, end, v4);
	CG_RailTrail2(color, ci, end, v5);
	CG_RailTrail2(color, ci, end, v6);

	CG_RailTrail2(color, ci, v2, v6);
	CG_RailTrail2(color, ci, v6, v1);
	CG_RailTrail2(color, ci, v1, v5);

	CG_RailTrail2(color, ci, v2, v4);
	CG_RailTrail2(color, ci, v4, v3);
	CG_RailTrail2(color, ci, v3, v5);
}

///////////////////////////////////////////////////////////////////////////////

/*
======================
CG_ParseWeaponConfig
    read information for weapon animations (first/length/fps)
======================
*/
static qboolean CG_ParseWeaponConfig(const char *filename, weaponInfo_t *wi)
{
	char         *text_p, *prev;
	int          len;
	int          i;
	float        fps;
	char         *token;
	qboolean     newfmt = qfalse;
	char         text[20000];
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0)
	{
		CG_Printf("CG_ParseWeaponConfig: File not found: %s\n", filename);
		return qfalse;
	}

	if (len >= sizeof(text) - 1)
	{
		CG_Printf("CG_ParseWeaponConfig: File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;

	// read optional parameters
	while (1)
	{
		prev  = text_p; // so we can unget
		token = COM_Parse(&text_p);
		if (!token)                         // get the variable
		{
			break;
		}
		/*      if ( !Q_stricmp( token, "whatever_variable" ) ) {
		            token = COM_Parse( &text_p );   // get the value
		            if ( !token ) {
		                break;
		            }
		            continue;
		        }*/

		if (!Q_stricmp(token, "newfmt"))
		{
			newfmt = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if (token[0] >= '0' && token[0] <= '9')
		{
			text_p = prev;  // unget the token
			break;
		}
		Com_Printf("CG_ParseWeaponConfig: Unknown token in weapon cfg '%s' in %s\n", token, filename);
	}

	for (i = 0 ; i < MAX_WP_ANIMATIONS  ; i++)
	{
		token = COM_Parse(&text_p);     // first frame
		if (!token)
		{
			break;
		}
		wi->weapAnimations[i].firstFrame = atoi(token);

		token = COM_Parse(&text_p);     // length
		if (!token)
		{
			break;
		}
		wi->weapAnimations[i].numFrames = atoi(token);

		token = COM_Parse(&text_p);     // fps
		if (!token)
		{
			break;
		}
		fps = atof(token);
		if (fps == 0)
		{
			fps = 1;
		}

		wi->weapAnimations[i].frameLerp   = 1000 / fps;
		wi->weapAnimations[i].initialLerp = 1000 / fps;

		token = COM_Parse(&text_p);     // looping frames
		if (!token)
		{
			break;
		}
		wi->weapAnimations[i].loopFrames = atoi(token);
		if (wi->weapAnimations[i].loopFrames > wi->weapAnimations[i].numFrames)
		{
			wi->weapAnimations[i].loopFrames = wi->weapAnimations[i].numFrames;
		}
		else if (wi->weapAnimations[i].loopFrames < 0)
		{
			wi->weapAnimations[i].loopFrames = 0;
		}

		// store animation/draw bits in '.moveSpeed'

		wi->weapAnimations[i].moveSpeed = 0;

		if (newfmt)
		{
			token = COM_Parse(&text_p);     // barrel anim bits
			if (!token)
			{
				break;
			}
			wi->weapAnimations[i].moveSpeed = atoi(token);

			token = COM_Parse(&text_p);     // animated weapon
			if (!token)
			{
				break;
			}
			if (atoi(token))
			{
				wi->weapAnimations[i].moveSpeed |= (1 << W_MAX_PARTS);      // set the bit one higher than can be set by the barrel bits

			}
			token = COM_Parse(&text_p);     // barrel hide bits (so objects can be flagged to not be drawn during all sequences (a reloading hand that comes in from off screen for that one animation for example)
			if (!token)
			{
				break;
			}
			wi->weapAnimations[i].moveSpeed |= ((atoi(token)) << 8);       // use 2nd byte for draw bits
		}

	}

	if (i != MAX_WP_ANIMATIONS)
	{
		CG_Printf("CG_ParseWeaponConfig: Error parsing weapon animation file: %s", filename);
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_RW_ParseError(int handle, char *format, ...)
{
	int         line;
	char        filename[MAX_QPATH];
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	filename[0] = '\0';
	line        = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

	trap_PC_FreeSource(handle);

	return qfalse;
}

static qboolean CG_RW_ParseWeaponLinkPart(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t  token;
	char        filename[MAX_QPATH];
	int         part;
	partModel_t *partModel;

	if (!PC_Int_Parse(handle, &part))
	{
		return CG_RW_ParseError(handle, "expected part index");
	}

	if (part < 0 || part >= W_MAX_PARTS)
	{
		return CG_RW_ParseError(handle, "part index out of bounds");
	}

	partModel = &weaponInfo->partModels[viewType][part];

	memset(partModel, 0, sizeof(*partModel));

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "tag"))
		{
			if (!PC_String_ParseNoAlloc(handle, partModel->tagName, sizeof(partModel->tagName)))
			{
				return CG_RW_ParseError(handle, "expected tag name");
			}
		}
		else if (!Q_stricmp(token.string, "model"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected model filename");
			}
			else
			{
				partModel->model = trap_R_RegisterModel(filename);
			}
		}
		else if (!Q_stricmp(token.string, "skin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				partModel->skin[0] = trap_R_RegisterSkin(filename);
			}
		}
		else if (!Q_stricmp(token.string, "axisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				partModel->skin[TEAM_AXIS] = trap_R_RegisterSkin(filename);
			}
		}
		else if (!Q_stricmp(token.string, "alliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				partModel->skin[TEAM_ALLIES] = trap_R_RegisterSkin(filename);
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseWeaponLink(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "part"))
		{
			if (!CG_RW_ParseWeaponLinkPart(handle, weaponInfo, viewType))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseViewType(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t token;
	char       filename[MAX_QPATH];

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "model"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected model filename");
			}
			else
			{
				weaponInfo->weaponModel[viewType].model = trap_R_RegisterModel(filename);
			}
		}
		else if (!Q_stricmp(token.string, "skin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				weaponInfo->weaponModel[viewType].skin[0] = trap_R_RegisterSkin(filename);
			}
		}
		else if (!Q_stricmp(token.string, "axisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				weaponInfo->weaponModel[viewType].skin[TEAM_AXIS] = trap_R_RegisterSkin(filename);
			}
		}
		else if (!Q_stricmp(token.string, "alliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				weaponInfo->weaponModel[viewType].skin[TEAM_ALLIES] = trap_R_RegisterSkin(filename);
			}
		}
		else if (!Q_stricmp(token.string, "flashModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected flashModel filename");
			}
			else
			{
				weaponInfo->flashModel[viewType] = trap_R_RegisterModel(filename);
			}
		}
		else if (!Q_stricmp(token.string, "weaponLink"))
		{
			if (!CG_RW_ParseWeaponLink(handle, weaponInfo, viewType))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseModModel(int handle, weaponInfo_t *weaponInfo)
{
	char filename[MAX_QPATH];
	int  mod;

	if (!PC_Int_Parse(handle, &mod))
	{
		return CG_RW_ParseError(handle, "expected mod index");
	}

	if (mod < 0 || mod >= 6)
	{
		return CG_RW_ParseError(handle, "mod index out of bounds");
	}

	if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
	{
		return CG_RW_ParseError(handle, "expected model filename");
	}
	else
	{
		// maybe it's a shader ...
		// check extensions of file name and register shader OR model
		if (!strstr(filename, ".md3") && !strstr(filename, ".mdc")) // FIXME: add more model formats?
		{   // we assume it's a shader
			weaponInfo->modModels[mod] = trap_R_RegisterShader(filename);
		}
		else
		{
			weaponInfo->modModels[mod] = trap_R_RegisterModel(filename);
		}

		// still no q_handle_t
		if (!weaponInfo->modModels[mod])
		{
			Com_Printf(S_COLOR_RED "ERROR: CG_RW_ParseModModel() no model or shader for %s registered.\n", filename);
			return qfalse; // this isn't vanilla behaviour
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseClient(int handle, weaponInfo_t *weaponInfo)
{
	pc_token_t token;
	char       filename[MAX_QPATH];
	int        i;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "standModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected standModel filename");
			}
			else
			{
				weaponInfo->standModel = trap_R_RegisterModel(filename);
			}
		}
		else if (!Q_stricmp(token.string, "droppedAnglesHack"))
		{
			weaponInfo->droppedAnglesHack = qtrue;
		}
		else if (!Q_stricmp(token.string, "pickupModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected pickupModel filename");
			}
			else
			{
				weaponInfo->weaponModel[W_PU_MODEL].model = trap_R_RegisterModel(filename);
			}
		}
		else if (!Q_stricmp(token.string, "pickupSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected pickupSound filename");
			}
			else
			{
				//weaponInfo->pickupSound = trap_S_RegisterSound( filename, qfalse );
			}
		}
		else if (!Q_stricmp(token.string, "weaponConfig"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponConfig filename");
			}
			else
			{
				if (!CG_ParseWeaponConfig(filename, weaponInfo))
				{
					CG_Error("Couldn't register weapon (failed to parse %s)", filename);
				}
			}
		}
		else if (!Q_stricmp(token.string, "handsModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected handsModel filename");
			}
			else
			{
				weaponInfo->handsModel = trap_R_RegisterModel(filename);
			}
		}
		else if (!Q_stricmp(token.string, "flashDlightColor"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->flashDlightColor))
			{
				return CG_RW_ParseError(handle, "expected flashDlightColor as r g b");
			}
		}
		else if (!Q_stricmp(token.string, "flashSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected flashSound filename");
			}
			else
			{
				for (i = 0; i < 4; i++)
				{
					if (!weaponInfo->flashSound[i])
					{
						weaponInfo->flashSound[i] = trap_S_RegisterSound(filename, qfalse);
						break;
					}
				}
				if (i == 4)
				{
					CG_Printf(S_COLOR_YELLOW "WARNING: only up to 4 flashSounds supported per weapon\n");
				}
			}
		}
		else if (!Q_stricmp(token.string, "flashEchoSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected flashEchoSound filename");
			}
			else
			{
				for (i = 0; i < 4; i++)
				{
					if (!weaponInfo->flashEchoSound[i])
					{
						weaponInfo->flashEchoSound[i] = trap_S_RegisterSound(filename, qfalse);
						break;
					}
				}
				if (i == 4)
				{
					CG_Printf(S_COLOR_YELLOW "WARNING: only up to 4 flashEchoSounds supported per weapon\n");
				}
			}
		}
		else if (!Q_stricmp(token.string, "lastShotSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected lastShotSound filename");
			}
			else
			{
				for (i = 0; i < 4; i++)
				{
					if (!weaponInfo->lastShotSound[i])
					{
						weaponInfo->lastShotSound[i] = trap_S_RegisterSound(filename, qfalse);
						break;
					}
				}
				if (i == 4)
				{
					CG_Printf(S_COLOR_YELLOW "WARNING: only up to 4 lastShotSound supported per weapon\n");
				}
			}
		}
		else if (!Q_stricmp(token.string, "readySound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected readySound filename");
			}
			else
			{
				weaponInfo->readySound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "firingSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected firingSound filename");
			}
			else
			{
				weaponInfo->firingSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "overheatSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected overheatSound filename");
			}
			else
			{
				weaponInfo->overheatSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "reloadSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected reloadSound filename");
			}
			else
			{
				weaponInfo->reloadSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "reloadFastSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected reloadFastSound filename");
			}
			else
			{
				weaponInfo->reloadFastSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "spinupSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected spinupSound filename");
			}
			else
			{
				weaponInfo->spinupSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "spindownSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected spindownSound filename");
			}
			else
			{
				weaponInfo->spindownSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "switchSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected switchSound filename");
			}
			else
			{
				weaponInfo->switchSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "weaponIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponIcon filename");
			}
			else
			{
				weaponInfo->weaponIcon[0] = trap_R_RegisterShader(filename);
			}
		}
		else if (!Q_stricmp(token.string, "weaponSelectedIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponSelectedIcon filename");
			}
			else
			{
				weaponInfo->weaponIcon[1] = trap_R_RegisterShader(filename);
			}
		}
		else if (!Q_stricmp(token.string, "missileModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileModel filename");
			}
			else
			{
				weaponInfo->missileModel = trap_R_RegisterModel(filename);
			}
		}
		else if (!Q_stricmp(token.string, "missileAlliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				weaponInfo->missileAlliedSkin = trap_R_RegisterSkin(filename);
			}
		}
		else if (!Q_stricmp(token.string, "missileAxisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}
			else
			{
				weaponInfo->missileAxisSkin = trap_R_RegisterSkin(filename);
			}
		}
		else if (!Q_stricmp(token.string, "missileSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileSound filename");
			}
			else
			{
				weaponInfo->missileSound = trap_S_RegisterSound(filename, qfalse);
			}
		}
		else if (!Q_stricmp(token.string, "missileTrailFunc"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileTrailFunc");
			}
			else
			{
				if (!Q_stricmp(filename, "GrenadeTrail"))
				{
					weaponInfo->missileTrailFunc = CG_GrenadeTrail;
				}
				else if (!Q_stricmp(filename, "RocketTrail"))
				{
					weaponInfo->missileTrailFunc = CG_RocketTrail;
				}
				else if (!Q_stricmp(filename, "PyroSmokeTrail"))
				{
					weaponInfo->missileTrailFunc = CG_PyroSmokeTrail;
				}
				else if (!Q_stricmp(filename, "DynamiteTrail"))
				{
					weaponInfo->missileTrailFunc = CG_DynamiteTrail;
				}
			}
		}
		else if (!Q_stricmp(token.string, "missileDlight"))
		{
			if (!PC_Float_Parse(handle, &weaponInfo->missileDlight))
			{
				return CG_RW_ParseError(handle, "expected missileDlight value");
			}
		}
		else if (!Q_stricmp(token.string, "missileDlightColor"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->missileDlightColor))
			{
				return CG_RW_ParseError(handle, "expected missileDlightColor as r g b");
			}
		}
		else if (!Q_stricmp(token.string, "ejectBrassFunc"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected ejectBrassFunc");
			}
			else
			{
				if (!Q_stricmp(filename, "MachineGunEjectBrass"))
				{
					weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
				}
				else if (!Q_stricmp(filename, "PanzerFaustEjectBrass"))
				{
					weaponInfo->ejectBrassFunc = CG_PanzerFaustEjectBrass;
				}
			}
		}
		else if (!Q_stricmp(token.string, "modModel"))
		{
			if (!CG_RW_ParseModModel(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "firstPerson"))
		{
			if (!CG_RW_ParseViewType(handle, weaponInfo, W_FP_MODEL))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "thirdPerson"))
		{
			if (!CG_RW_ParseViewType(handle, weaponInfo, W_TP_MODEL))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

static qboolean CG_RegisterWeaponFromWeaponFile(const char *filename, weaponInfo_t *weaponInfo)
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "weaponDef"))
	{
		return CG_RW_ParseError(handle, "expected 'weaponDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "client"))
		{
			if (!CG_RW_ParseClient(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/*
=================
CG_RegisterWeapon
=================
*/
void CG_RegisterWeapon(int weaponNum, qboolean force)
{
	weaponInfo_t *weaponInfo;
	char         *filename;

	if (weaponNum <= 0 || weaponNum >= WP_NUM_WEAPONS)
	{
		return;
	}

	weaponInfo = &cg_weapons[weaponNum];

	if (weaponInfo->registered && !force)
	{
		return;
	}

	memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;

	/*for( item = bg_itemlist + 1 ; item->classname ; item++ ) {
	    if( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
	        weaponInfo->item = item;
	        break;
	    }
	}

	if( !item->classname ) {
	    CG_Error( "Couldn't find weapon %i", weaponNum );
	}*/

	switch (weaponNum)
	{
	case WP_KNIFE:
		filename = "knife.weap";
		break;
	case WP_LUGER:
		filename = "luger.weap";
		break;
	case WP_COLT:
		filename = "colt.weap";
		break;
	case WP_MP40:
		filename = "mp40.weap";
		break;
	case WP_THOMPSON:
		filename = "thompson.weap";
		break;
	case WP_STEN:
		filename = "sten.weap";
		break;
	case WP_GRENADE_LAUNCHER:
		filename = "grenade.weap";
		break;
	case WP_GRENADE_PINEAPPLE:
		filename = "pineapple.weap";
		break;
	case WP_PANZERFAUST:
		filename = "panzerfaust.weap";
		break;
	case WP_BAZOOKA:
		filename = "bazooka.weap";
		break;
	case WP_FLAMETHROWER:
		filename = "flamethrower.weap";
		break;
	case WP_AMMO:
		filename = "ammopack.weap";
		break;
	case WP_SMOKETRAIL:
		filename = "smoketrail.weap";
		break;
	case WP_MEDKIT:
		filename = "medpack.weap";
		break;
	case WP_PLIERS:
		filename = "pliers.weap";
		break;
	case WP_SMOKE_MARKER:
		filename = "smokemarker.weap";
		break;
	case WP_DYNAMITE:
		filename = "dynamite.weap";
		break;
	case WP_MEDIC_ADRENALINE:
		filename = "adrenaline.weap";
		break;
	case WP_MEDIC_SYRINGE:
		filename = "syringe.weap";
		break;
	case WP_BINOCULARS:
		filename = "binocs.weap";
		break;
	case WP_KAR98:
		filename = "kar98.weap";
		break;
	case WP_GPG40:
		filename = "gpg40.weap";
		break;
	case WP_CARBINE:
		filename = "m1_garand.weap";
		break;
	case WP_M7:
		filename = "m7.weap";
		break;
	case WP_GARAND:
	case WP_GARAND_SCOPE:
		filename = "m1_garand_s.weap";
		break;
	case WP_FG42:
	case WP_FG42SCOPE:
		filename = "fg42.weap";
		break;
	case WP_LANDMINE:
		filename = "landmine.weap";
		break;
	case WP_SATCHEL:
		filename = "satchel.weap";
		break;
	case WP_SATCHEL_DET:
		filename = "satchel_det.weap";
		break;
	case WP_SMOKE_BOMB:
		filename = "smokegrenade.weap";
		break;
	case WP_MOBILE_MG42_SET:     // do we need a seperate file for this?
	case WP_MOBILE_MG42:
		filename = "mg42.weap";
		break;
	case WP_MOBILE_BROWNING:
	case WP_MOBILE_BROWNING_SET:
		filename = "browning.weap";
		break;
	case WP_SILENCER:
		filename = "silenced_luger.weap";
		break;
	case WP_SILENCED_COLT:
		filename = "silenced_colt.weap";
		break;
	case WP_K43:
	case WP_K43_SCOPE:
		filename = "k43.weap";
		break;
	case WP_MORTAR:
		filename = "mortar.weap";
		break;
	case WP_MORTAR_SET:
		filename = "mortar_set.weap";
		break;
	case WP_MORTAR2:
		filename = "axis_mortar.weap";
		break;
	case WP_MORTAR2_SET:
		filename = "axis_mortar_set.weap";
		break;
	case WP_AKIMBO_LUGER:
		filename = "akimbo_luger.weap";
		break;
	case WP_AKIMBO_SILENCEDLUGER:
		filename = "akimbo_silenced_luger.weap";
		break;
	case WP_AKIMBO_COLT:
		filename = "akimbo_colt.weap";
		break;
	case WP_AKIMBO_SILENCEDCOLT:
		filename = "akimbo_silenced_colt.weap";
		break;
	case WP_MAPMORTAR:
		filename = "mapmortar.weap";
		break;     // do we really need this?
	case WP_KNIFE_KABAR:
		filename = "knife_kbar.weap";
		break;
	case WP_ARTY:
	case VERYBIGEXPLOSION:
	case WP_DUMMY_MG42:
		//CG_Printf(S_COLOR_YELLOW "WARNING: skipping weapon %i to register.\n", weaponNum);
		return;     // to shut the game up
	default:
		CG_Printf(S_COLOR_RED "WARNING: trying to register weapon %i but there is no weapon file entry for it.\n", weaponNum);
		return;
	}

	if (!CG_RegisterWeaponFromWeaponFile(va("weapons/%s", filename), weaponInfo))
	{
		CG_Printf(S_COLOR_RED "WARNING: failed to register media for weapon %i from %s\n", weaponNum, filename);
	}
}

/**
 * @brief Registers models and icons for items of bg_itemlist (except weapons which are registered from *.weap file data)
 *
 * @note Actually IT_AMMO & IT_TEAM have no visuals
 */
void CG_RegisterItemVisuals(int itemNum)
{
	itemInfo_t *itemInfo = &bg_itemlist[itemNum].itemInfo;
	gitem_t    *item;
	int        i;

	if (itemInfo->registered)
	{
		return;
	}

	item = &bg_itemlist[itemNum];


	if (item->giType == IT_WEAPON)
	{
		return;
	}

	memset(itemInfo, 0, sizeof(&itemInfo));

	for (i = 0; i < MAX_ITEM_MODELS; i++)
	{
		// some items don't have world models see (bg_itemlist) - we don't have to register
		if (!item->world_model[i] || !item->world_model[i][0])
		{
			CG_DPrintf("CG_RegisterItemVisuals: NULL or empty world_model[%i] for item classname %s\n", i, item->classname);
			itemInfo->models[i] = 0;
		}
		else
		{
			itemInfo->models[i] = trap_R_RegisterModel(item->world_model[i]);
		}
	}

	// some items have no icon shader - we don't have to register
	if (item->icon && item->icon[0])
	{
		itemInfo->icons[0] = trap_R_RegisterShader(item->icon);
	}
	else
	{
		CG_DPrintf("CG_RegisterItemVisuals: NULL or empty item icon shader [%s] for classname %s\n", item->icon, item->classname);
		itemInfo->icons[0] = 0;
	}

	itemInfo->registered = qtrue;   // moved this down after the registerweapon()
}

/*
========================================================================================
VIEW WEAPON
========================================================================================
*/

// weapon animations

/*
==============
CG_GetPartFramesFromWeap
    get animation info from the parent if necessary
==============
*/
qboolean CG_GetPartFramesFromWeap(centity_t *cent, refEntity_t *part, refEntity_t *parent, int partid, weaponInfo_t *wi)
{
	int         i;
	int         frameoffset = 0;
	animation_t *anim       = cent->pe.weap.animation;

	if (partid == W_MAX_PARTS)
	{
		return qtrue;   // primary weap model drawn for all frames right now
	}

	// check draw bit
	if (anim->moveSpeed & (1 << (partid + 8)))           // hide bits are in high byte
	{
		return qfalse;  // not drawn for current sequence
	}

	// find part's start frame for this animation sequence
	// rain - & out ANIM_TOGGLEBIT or we'll go way out of bounds
	for (i = 0; i < (cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT); i++)
	{
		if (wi->weapAnimations[i].moveSpeed & (1 << partid))         // this part has animation for this sequence
		{
			frameoffset += wi->weapAnimations[i].numFrames;
		}
	}

	// now set the correct frame into the part
	if (anim->moveSpeed & (1 << partid))
	{
		part->backlerp = parent->backlerp;
		part->oldframe = frameoffset + (parent->oldframe - anim->firstFrame);
		part->frame    = frameoffset + (parent->frame - anim->firstFrame);
	}

	return qtrue;
}

/*
===============
CG_SetWeapLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetWeapLerpFrameAnimation(weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation)
{
	animation_t *anim;

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= MAX_WP_ANIMATIONS)
	{
		CG_Error("Bad animation number (CG_SWLFA): %i\n", newAnimation);
	}

	anim = &wi->weapAnimations[newAnimation];

	lf->animation     = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (cg_debugAnim.integer & 2)
	{
		CG_Printf("Weap Anim: %d\n", newAnimation);
	}
}

/*
===============
CG_ClearWeapLerpFrame
===============
*/
void CG_ClearWeapLerpFrame(weaponInfo_t *wi, lerpFrame_t *lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetWeapLerpFrameAnimation(wi, lf, animationNumber);
	lf->oldFrame      = lf->frame = lf->animation->firstFrame;
	lf->oldFrameModel = lf->frameModel = lf->animation->mdxFile;
}

/*
===============
CG_RunWeapLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunWeapLerpFrame(clientInfo_t *ci, weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation, float speedScale)
{
	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0)
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (!lf->animation)
	{
		CG_ClearWeapLerpFrame(wi, lf, newAnimation);
	}
	else if (newAnimation != lf->animationNumber)
	{
		if ((newAnimation & ~ANIM_TOGGLEBIT) == PM_RaiseAnimForWeapon(cg.snap->ps.nextWeapon))
		{
			CG_ClearWeapLerpFrame(wi, lf, newAnimation);     // clear when switching to raise (since it should be out of view anyway)
		}
		else
		{
			CG_SetWeapLerpFrameAnimation(wi, lf, newAnimation);
		}
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime)
	{
		int         f;
		animation_t *anim;

		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim->frameLerp)
		{
			return;     // shouldn't happen
		}
		if (cg.time < lf->animationTime)
		{
			lf->frameTime = lf->animationTime;      // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f  = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc
		if (f >= anim->numFrames)
		{
			f -= anim->numFrames;
			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame      = anim->firstFrame + f;
		lf->frameModel = anim->mdxFile;
		if (cg.time > lf->frameTime)
		{
			lf->frameTime = cg.time;
			if (cg_debugAnim.integer)
			{
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/*
==============
CG_WeaponAnimation
==============
*/
// modified.  this is now client-side only (server does not dictate weapon animation info)
static void CG_WeaponAnimation(playerState_t *ps, weaponInfo_t *weapon, int *weapOld, int *weap, float *weapBackLerp)
{
	centity_t    *cent = &cg.predictedPlayerEntity;
	clientInfo_t *ci   = &cgs.clientinfo[ps->clientNum];

	if (cg_noPlayerAnims.integer)
	{
		*weapOld = *weap = 0;
		return;
	}

	CG_RunWeapLerpFrame(ci, weapon, &cent->pe.weap, ps->weapAnim, 1);

	*weapOld      = cent->pe.weap.oldFrame;
	*weap         = cent->pe.weap.frame;
	*weapBackLerp = cent->pe.weap.backlerp;

	if (cg_debugAnim.integer == 3)
	{
		CG_Printf("oldframe: %d   frame: %d   backlerp: %f\n", cent->pe.weap.oldFrame, cent->pe.weap.frame, cent->pe.weap.backlerp);
	}
}

////////////////////////////////////////////////////////////////////////

/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles)
{
	float scale;
	int   delta;

	VectorCopy(cg.refdef_current->vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	if (cg.predictedPlayerState.eFlags & EF_MOUNTEDTANK)
	{
		angles[PITCH] = cg.refdefViewAngles[PITCH] / 1.2;
	}

	if (!cg.renderingThirdPerson && (IS_MORTAR_WEAPON_SET(cg.predictedPlayerState.weapon) || IS_MG_WEAPON_SET(cg.predictedPlayerState.weapon)) &&
	    cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
	{
		angles[PITCH] = cg.pmext.mountedWeaponAngles[PITCH];
	}

	if (cg.predictedPlayerState.eFlags & EF_PRONE_MOVING)
	{
		int pronemovingtime = cg.time - cg.proneMovingTime;

		if (pronemovingtime > 0)     // div by 0
		{
			float factor = pronemovingtime > 200 ? 1.f : 1.f / (200.f / (float)pronemovingtime);

			VectorMA(origin, factor * -20, cg.refdef_current->viewaxis[0], origin);
			VectorMA(origin, factor * 3, cg.refdef_current->viewaxis[1], origin);
		}
	}
	else
	{
		int pronenomovingtime = cg.time - -cg.proneMovingTime;

		if (pronenomovingtime < 200)
		{
			float factor = pronenomovingtime == 0 ? 1.f : 1.f - (1.f / (200.f / (float)pronenomovingtime));

			VectorMA(origin, factor * -20, cg.refdef_current->viewaxis[0], origin);
			VectorMA(origin, factor * 3, cg.refdef_current->viewaxis[1], origin);
		}
	}

	// adjust 'lean' into weapon
	if (cg.predictedPlayerState.leanf != 0)
	{
		vec3_t right, up;
		float  myfrac = 1.0f;

		switch (cg.predictedPlayerState.weapon)
		{
		case WP_FLAMETHROWER:
		case WP_KAR98:
		case WP_CARBINE:
		case WP_GPG40:
		case WP_M7:
		case WP_K43:
			myfrac = 2.0f;
			break;
		case WP_GARAND:
			myfrac = 3.0f;
			break;
		default:
			break;
		}

		// reverse the roll on the weapon so it stays relatively level
		angles[ROLL] -= cg.predictedPlayerState.leanf / (myfrac * 2.0f);
		AngleVectors(angles, NULL, right, up);
		VectorMA(origin, angles[ROLL], right, origin);

		// pitch the gun down a bit to show that firing is not allowed when leaning
		angles[PITCH] += (fabsf(cg.predictedPlayerState.leanf) / 2.0f);

		// this gives you some impression that the weapon stays in relatively the same
		// position while you lean, so you appear to 'peek' over the weapon
		AngleVectors(cg.refdefViewAngles, NULL, right, NULL);
		VectorMA(origin, -cg.predictedPlayerState.leanf / 4.0f, right, origin);
	}

	// on odd legs, invert some angles
	if (cg.bobcycle & 1)
	{
		scale = -cg.xyspeed;
	}
	else
	{
		scale = cg.xyspeed;
	}

	// gun angles from bobbing

	angles[ROLL]  += scale        * cg.bobfracsin * 0.005;
	angles[YAW]   += scale        * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed   * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if (delta < LAND_DEFLECT_TIME)
	{
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	}
	else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
	{
		origin[2] += cg.landChange * 0.25 *
		             (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

	// idle drift
	if ((!(cg.predictedPlayerState.eFlags & EF_MOUNTEDTANK))
	    && (cg.predictedPlayerState.weapon != WP_MORTAR_SET && cg.predictedPlayerState.weapon != WP_MORTAR2_SET)
	    && (cg.predictedPlayerState.weapon != WP_MOBILE_MG42_SET && cg.predictedPlayerState.weapon != WP_MOBILE_BROWNING_SET))
	{
		float fracsin = sin(cg.time * 0.001);

		// adjustment for MAX KAUFMAN
		//scale = cg.xyspeed + 40;
		scale = 80;

		angles[ROLL]  += scale * fracsin * 0.01;
		angles[YAW]   += scale * fracsin * 0.01;
		angles[PITCH] += scale * fracsin * 0.01;
	}

	// subtract the kickAngles
	VectorMA(angles, -1.0, cg.kickAngles, angles);
}

/*
===============
CG_FlamethrowerFlame
===============
*/
static void CG_FlamethrowerFlame(centity_t *cent, vec3_t origin)
{
	if (cent->currentState.weapon != WP_FLAMETHROWER)
	{
		return;
	}

	CG_FireFlameChunks(cent, origin, cent->lerpAngles, 1.0, qtrue);
	return;
}

/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups(refEntity_t *gun, int powerups, playerState_t *ps, centity_t *cent)
{
	// add powerup effects
	// no powerup effects on weapons
	trap_R_AddRefEntityToScene(gun);
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
//#define DEBUG_WEAPON

#define BARREL_SMOKE_TIME 1000

void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent)
{
	refEntity_t  gun;
	refEntity_t  barrel;
	refEntity_t  flash;
	vec3_t       angles;
	weapon_t     weaponNum = cent->currentState.weapon;
	weaponInfo_t *weapon;
	centity_t    *nonPredictedCent;
	qboolean     firing;
	qboolean     akimboFire = qfalse;
	qboolean     drawpart;
	qboolean     isPlayer = (qboolean)(cent->currentState.clientNum == cg.snap->ps.clientNum); // might as well have this check consistant throughout the routine

	// don't draw any weapons when the binocs are up
	if (cent->currentState.eFlags & EF_ZOOMING)
	{
		return;
	}

	switch (weaponNum)
	{
	// don't draw weapon stuff when looking through a scope
	case WP_FG42SCOPE:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		if (isPlayer && !cg.renderingThirdPerson)
		{
			return;
		}
		break;
	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
		if (ps && !ps->ammoclip[weaponNum])
		{
			return;
		}
		break;
	default:
		break;
	}

	// no weapon when on mg_42
	if (cent->currentState.eFlags & EF_MOUNTEDTANK)
	{
		if (isPlayer && !cg.renderingThirdPerson)
		{
			return;
		}

		if (cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			memset(&flash, 0, sizeof(flash));
			flash.renderfx = RF_LIGHTING_ORIGIN;
			flash.hModel   = cgs.media.mg42muzzleflash;

			VectorCopy(cg_entities[cg_entities[cent->currentState.number].tagParent].mountedMG42Flash.origin, flash.origin);
			AxisCopy(cg_entities[cg_entities[cent->currentState.number].tagParent].mountedMG42Flash.axis, flash.axis);

			trap_R_AddRefEntityToScene(&flash);

			// add dynamic light
			trap_R_AddLightToScene(flash.origin, 320, 1.25 + (rand() & 31) / 128.0f, 1.0f, 0.6f, 0.23f, 0, 0);
		}
		return;
	}

	if ((cent->currentState.eFlags & EF_MG42_ACTIVE) || (cent->currentState.eFlags & EF_AAGUN_ACTIVE))
	{
		// MG42 Muzzle Flash
		if (cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			CG_MG42EFX(cent);
		}
		return;
	}

	if ((!ps || cg.renderingThirdPerson) && (cent->currentState.eFlags & EF_PRONE_MOVING))
	{
		return;
	}

	weapon = &cg_weapons[weaponNum];

	if (IS_AKIMBO_WEAPON(weaponNum))
	{
		if (isPlayer)
		{
			akimboFire = BG_AkimboFireSequence(weaponNum, cg.predictedPlayerState.ammoclip[BG_FindClipForWeapon(weaponNum)], cg.predictedPlayerState.ammoclip[BG_FindClipForWeapon(weaponTable[weaponNum].akimboSideram)]);
		}
		else if (ps)
		{
			akimboFire = BG_AkimboFireSequence(weaponNum, ps->ammoclip[BG_FindClipForWeapon(weaponNum)], ps->ammoclip[BG_FindClipForWeapon(weaponTable[weaponNum].akimboSideram)]);
		}
		// alternate for other clients, store flip-flop on cent or smuffin
	}

	// add the weapon
	memset(&gun, 0, sizeof(gun));
	VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx    = parent->renderfx;

	if (ps)
	{
		team_t team = ps->persistant[PERS_TEAM];

		if ((weaponNum != WP_SATCHEL) && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))
		{
			team = team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
		}

		gun.hModel = weapon->weaponModel[W_FP_MODEL].model;
		if ((team == TEAM_AXIS) &&
		    weapon->weaponModel[W_FP_MODEL].skin[TEAM_AXIS])
		{
			gun.customSkin = weapon->weaponModel[W_FP_MODEL].skin[TEAM_AXIS];
		}
		else if ((team == TEAM_ALLIES) &&
		         weapon->weaponModel[W_FP_MODEL].skin[TEAM_ALLIES])
		{
			gun.customSkin = weapon->weaponModel[W_FP_MODEL].skin[TEAM_ALLIES];
		}
		else
		{
			gun.customSkin = weapon->weaponModel[W_FP_MODEL].skin[0];   // if not loaded it's 0 so doesn't do any harm
		}
	}
	else
	{
		team_t team = cgs.clientinfo[cent->currentState.clientNum].team;

		if ((weaponNum != WP_SATCHEL) && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))
		{
			team = team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
		}

		gun.hModel = weapon->weaponModel[W_TP_MODEL].model;
		if ((team == TEAM_AXIS) &&
		    weapon->weaponModel[W_TP_MODEL].skin[TEAM_AXIS])
		{
			gun.customSkin = weapon->weaponModel[W_FP_MODEL].skin[TEAM_AXIS];
		}
		else if ((team == TEAM_ALLIES) &&
		         weapon->weaponModel[W_TP_MODEL].skin[TEAM_ALLIES])
		{
			gun.customSkin = weapon->weaponModel[W_TP_MODEL].skin[TEAM_ALLIES];
		}
		else
		{
			gun.customSkin = weapon->weaponModel[W_TP_MODEL].skin[0];   // if not loaded it's 0 so doesn't do any harm
		}
	}

	if (!gun.hModel)
	{
#ifdef DEBUG_WEAPON
		CG_Printf("returning due to: !gun.hModel\n");
#endif
		return;
	}

	if (!ps && (cg.snap->ps.pm_flags & PMF_LADDER) && isPlayer)          // player on ladder
	{
#ifdef DEBUG_WEAPON
		CG_Printf("returning due to: !ps && cg.snap->ps.pm_flags & PMF_LADDER\n");
#endif
		return;
	}

	if (!ps)
	{
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound)
		{
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, weapon->firingSound, 255, 0);
			cent->pe.lightningFiring = qtrue;
		}
		else if (weapon->readySound)
		{
			trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, weapon->readySound, 255, 0);
		}
	}

	firing = ((cent->currentState.eFlags & EF_FIRING) != 0);

	if (ps && !cg.renderingThirdPerson && IS_MORTAR_WEAPON_SET(cg.predictedPlayerState.weapon) && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
	{
		vec3_t angles;

		angles[YAW]   = angles[ROLL] = 0.f;
		angles[PITCH] = -.4f * AngleNormalize180(cg.pmext.mountedWeaponAngles[PITCH] - ps->viewangles[PITCH]);

		AnglesToAxis(angles, gun.axis);

		CG_PositionRotatedEntityOnTag(&gun, parent, "tag_weapon");
	}
	else if ((!ps || cg.renderingThirdPerson) && IS_MORTAR_WEAPON(weaponNum))
	{
		CG_PositionEntityOnTag(&gun, parent, "tag_weapon2", 0, NULL);
	}
	else
	{
		CG_PositionEntityOnTag(&gun, parent, "tag_weapon", 0, NULL);
	}

	//qboolean    playerScaled = (qboolean)(cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[0] != 0);
	//if(!ps && playerScaled) {   // don't "un-scale" weap up in 1st person
	// for(i=0;i<3;i++) {  // scale weapon back up so it doesn't pick up the adjusted scale of the character models.
	//   // this will affect any parts attached to the gun as well (barrel/bolt/flash/brass/etc.)
	//   VectorScale( gun.axis[i], 1.0/(cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[i]), gun.axis[i]);
	// }
	//}

	if (ps)
	{
		drawpart = CG_GetPartFramesFromWeap(cent, &gun, parent, W_MAX_PARTS, weapon);     // W_MAX_PARTS specifies this as the primary view model
	}
	else
	{
		drawpart = qtrue;
	}

	if (drawpart)
	{
		if (weaponNum == WP_AMMO)
		{
			if (ps)
			{
				if (cgs.clientinfo[ps->clientNum].skill[SK_SIGNALS] >= 1)
				{
					gun.customShader = weapon->modModels[0];
				}
			}
			else
			{
				if (cgs.clientinfo[cent->currentState.clientNum].skill[SK_SIGNALS] >= 1)
				{
					gun.customShader = weapon->modModels[0];
				}
			}
		}

		if (!ps)
		{
			if (weaponNum == WP_MEDIC_SYRINGE)
			{
				if (cgs.clientinfo[cent->currentState.clientNum].skill[SK_FIRST_AID] >= 3)
				{
					gun.customShader = weapon->modModels[0];
				}
			}
		}
		CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups, ps, cent);
	}

	if ((!ps || cg.renderingThirdPerson) &&
	    (weaponNum == WP_AKIMBO_COLT || weaponNum == WP_AKIMBO_SILENCEDCOLT || weaponNum == WP_AKIMBO_LUGER || weaponNum == WP_AKIMBO_SILENCEDLUGER))
	{
		// add to other hand as well
		CG_PositionEntityOnTag(&gun, parent, "tag_weapon2", 0, NULL);
		CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups, ps, cent);
	}

	// test hack
	//if( weaponNum == WP_KNIFE )
	//  trap_R_AddLightToScene( gun.origin, 512, 1.5, 1.0, 1.0, 1.0, 0, 0 );

	if (isPlayer)
	{
		refEntity_t brass;

		if (IS_AKIMBO_WEAPON(weaponNum) && akimboFire)
		{
			CG_PositionRotatedEntityOnTag(&brass, parent, "tag_brass2");
		}
		else
		{
			CG_PositionRotatedEntityOnTag(&brass, parent, "tag_brass");
		}
		VectorCopy(brass.origin, ejectBrassCasingOrigin);
	}

	memset(&barrel, 0, sizeof(barrel));
	VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
	barrel.shadowPlane = parent->shadowPlane;
	barrel.renderfx    = parent->renderfx;

	// add barrels
	// attach generic weapon parts to the first person weapon.
	// if a barrel should be attached for third person, add it in the (!ps) section below
	angles[YAW] = angles[PITCH] = 0;

	if (ps)
	{
		int      i;
		qboolean spunpart;

		for (i = W_PART_1; i < W_MAX_PARTS; i++)
		{
			if (IS_MORTAR_WEAPON_SET(weaponNum) && (i == W_PART_4 || i == W_PART_5))
			{
				if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
				{
					continue;
				}
			}

			spunpart      = qfalse;
			barrel.hModel = weapon->partModels[W_FP_MODEL][i].model;

			if (IS_MORTAR_WEAPON_SET(weaponNum))
			{
				if (i == W_PART_3)
				{
					if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
					{
						angles[PITCH] = angles[YAW] = 0.f;
						angles[ROLL]  = .8f * AngleNormalize180(cg.pmext.mountedWeaponAngles[YAW] - ps->viewangles[YAW]);
						spunpart      = qtrue;
					}
				}
				else if (i == W_PART_1 || i == W_PART_2)
				{
					if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
					{
						angles[YAW]   = angles[ROLL] = 0.f;
						angles[PITCH] = -.4f * AngleNormalize180(cg.pmext.mountedWeaponAngles[PITCH] - ps->viewangles[PITCH]);
						spunpart      = qtrue;
					}
				}
			}
			//else if (weaponNum == WP_MOBILE_MG42_SET || weaponNum == WP_MOBILE_BROWNING_SET)
			//{
			//}

			if (spunpart)
			{
				AnglesToAxis(angles, barrel.axis);
			}

			if (barrel.hModel)
			{
				if (spunpart)
				{
					CG_PositionRotatedEntityOnTag(&barrel, parent, weapon->partModels[W_FP_MODEL][i].tagName);
				}
				else
				{
					CG_PositionEntityOnTag(&barrel, parent, weapon->partModels[W_FP_MODEL][i].tagName, 0, NULL);
				}

				drawpart = CG_GetPartFramesFromWeap(cent, &barrel, parent, i, weapon);

				if (IS_MORTAR_WEAPON_SET(weaponNum) && (i == W_PART_1 || i == W_PART_2))
				{
					if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
					{
						VectorMA(barrel.origin, .5f * angles[PITCH], cg.refdef_current->viewaxis[0], barrel.origin);
					}
				}

				if (drawpart)
				{
					if ((ps->persistant[PERS_TEAM] == TEAM_AXIS ||
					     (ps->persistant[PERS_TEAM] == TEAM_ALLIES && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))) &&
					    weapon->partModels[W_FP_MODEL][i].skin[TEAM_AXIS])
					{
						barrel.customSkin = weapon->partModels[W_FP_MODEL][i].skin[TEAM_AXIS];
					}
					else if ((ps->persistant[PERS_TEAM] == TEAM_ALLIES ||
					          (ps->persistant[PERS_TEAM] == TEAM_AXIS && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))) &&
					         weapon->partModels[W_FP_MODEL][i].skin[TEAM_ALLIES])
					{
						barrel.customSkin = weapon->partModels[W_FP_MODEL][i].skin[TEAM_ALLIES];
					}
					else
					{
						barrel.customSkin = weapon->partModels[W_FP_MODEL][i].skin[0];  // if not loaded it's 0 so doesn't do any harm

					}
					if (weaponNum == WP_MEDIC_SYRINGE && i == W_PART_1)
					{
						if (cgs.clientinfo[ps->clientNum].skill[SK_FIRST_AID] >= 3)
						{
							barrel.customShader = weapon->modModels[0];
						}
					}

					CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);

					if (weaponNum == WP_SATCHEL_DET && i == W_PART_1)
					{
						float       rangeSquared;
						qboolean    inRange;
						refEntity_t satchelDetPart;

						if (cg.satchelCharge)
						{
							rangeSquared = DistanceSquared(cg.satchelCharge->lerpOrigin, cg.predictedPlayerEntity.lerpOrigin);
						}
						else
						{
							rangeSquared = Square(2001.f);
						}

						if (rangeSquared <= Square(2000))
						{
							inRange = qtrue;
						}
						else
						{
							inRange = qfalse;
						}

						memset(&satchelDetPart, 0, sizeof(satchelDetPart));
						VectorCopy(parent->lightingOrigin, satchelDetPart.lightingOrigin);
						satchelDetPart.shadowPlane = parent->shadowPlane;
						satchelDetPart.renderfx    = parent->renderfx;

						satchelDetPart.hModel = weapon->modModels[0];
						CG_PositionEntityOnTag(&satchelDetPart, &barrel, "tag_rlight", 0, NULL);
						satchelDetPart.customShader = inRange ? weapon->modModels[2] : weapon->modModels[3];
						CG_AddWeaponWithPowerups(&satchelDetPart, cent->currentState.powerups, ps, cent);

						CG_PositionEntityOnTag(&satchelDetPart, &barrel, "tag_glight", 0, NULL);
						satchelDetPart.customShader = inRange ? weapon->modModels[5] : weapon->modModels[4];
						CG_AddWeaponWithPowerups(&satchelDetPart, cent->currentState.powerups, ps, cent);

						satchelDetPart.hModel = weapon->modModels[1];
						angles[PITCH]         = angles[ROLL] = 0.f;
						if (inRange)
						{
							angles[YAW] = -30.f + (60.f * (rangeSquared / Square(2000)));
						}
						else
						{
							angles[YAW] = 30.f;
						}
						AnglesToAxis(angles, satchelDetPart.axis);
						CG_PositionRotatedEntityOnTag(&satchelDetPart, &barrel, "tag_needle");
						satchelDetPart.customShader = weapon->modModels[2];
						CG_AddWeaponWithPowerups(&satchelDetPart, cent->currentState.powerups, ps, cent);
					}
					else if (IS_MORTAR_WEAPON_SET(weaponNum) && i == W_PART_3)
					{
						if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
						{
							refEntity_t bipodLeg;

							memset(&bipodLeg, 0, sizeof(bipodLeg));
							VectorCopy(parent->lightingOrigin, bipodLeg.lightingOrigin);
							bipodLeg.shadowPlane = parent->shadowPlane;
							bipodLeg.renderfx    = parent->renderfx;

							bipodLeg.hModel = weapon->partModels[W_FP_MODEL][3].model;
							CG_PositionEntityOnTag(&bipodLeg, &barrel, "tag_barrel4", 0, NULL);
							CG_AddWeaponWithPowerups(&bipodLeg, cent->currentState.powerups, ps, cent);

							bipodLeg.hModel = weapon->partModels[W_FP_MODEL][4].model;
							CG_PositionEntityOnTag(&bipodLeg, &barrel, "tag_barrel5", 0, NULL);
							CG_AddWeaponWithPowerups(&bipodLeg, cent->currentState.powerups, ps, cent);
						}
					}
				}
			}
		}
	}

	// add the scope model to the rifle if you've got it
	if (isPlayer && !cg.renderingThirdPerson)  // for now just do it on the first person weapons
	{
		// FIXME: do a switch
		if (weaponNum == WP_CARBINE || weaponNum == WP_KAR98 || weaponNum == WP_GPG40 || weaponNum == WP_M7)
		{
			if ((cg.snap->ps.ammo[BG_FindAmmoForWeapon(WP_GPG40)] || cg.snap->ps.ammo[BG_FindAmmoForWeapon(WP_M7)] || cg.snap->ps.ammoclip[BG_FindAmmoForWeapon(WP_GPG40)] || cg.snap->ps.ammoclip[BG_FindAmmoForWeapon(WP_M7)]))
			{
				int anim = cg.snap->ps.weapAnim & ~ANIM_TOGGLEBIT;

				if (anim == PM_AltSwitchFromForWeapon(weaponNum) || anim == PM_AltSwitchToForWeapon(weaponNum) || anim == PM_IdleAnimForWeapon(weaponNum))
				{
					barrel.hModel = weapon->modModels[0];
					if (barrel.hModel)
					{
						CG_PositionEntityOnTag(&barrel, parent, "tag_scope", 0, NULL);
						CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
					}
				}
			}
		}
		else if (weaponNum == WP_GARAND)
		{
			barrel.hModel = weapon->modModels[0];
			if (barrel.hModel)
			{
				CG_PositionEntityOnTag(&barrel, &gun, "tag_scope2", 0, NULL);
				CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			}

			barrel.hModel = weapon->modModels[1];
			//if(barrel.hModel) {
			CG_PositionEntityOnTag(&barrel, &gun, "tag_flash", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			//}
		}
		else if (weaponNum == WP_K43)
		{
			barrel.hModel = weapon->modModels[0];
			if (barrel.hModel)
			{
				CG_PositionEntityOnTag(&barrel, &gun, "tag_scope", 0, NULL);
				CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			}

			barrel.hModel = weapon->modModels[1];
			//if(barrel.hModel) {
			CG_PositionEntityOnTag(&barrel, &gun, "tag_flash", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			//}
		}
	}
	// 3rd person attachements
	else
	{
		// FIXME: do a switch
		if (weaponNum == WP_M7 || weaponNum == WP_GPG40)
		{
			// the holder
			barrel.hModel = weapon->modModels[1];
			CG_PositionEntityOnTag(&barrel, &gun, "tag_flash", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);

			// the grenade - have to always enabled it, no means of telling if another person has a grenade loaded or not atm :/
			//if( cg.snap->ps.weaponstate != WEAPON_FIRING && cg.snap->ps.weaponstate != WEAPON_RELOADING ) {
			if (weaponNum == WP_M7 /*|| weaponNum == WP_CARBINE*/)
			{
				barrel.hModel = weapon->missileModel;
				CG_PositionEntityOnTag(&barrel, &barrel, "tag_prj", 0, NULL);
				CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			}
		}
		else if (weaponNum == WP_GARAND || weaponNum == WP_GARAND_SCOPE || weaponNum == WP_K43 || weaponNum == WP_K43_SCOPE)
		{
			// the holder
			barrel.hModel = weapon->modModels[2];
			CG_PositionEntityOnTag(&barrel, &gun, "tag_scope", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
		}
		else if (IS_MG_WEAPON(weaponNum))
		{
			barrel.hModel = weapon->modModels[0];
			barrel.frame  = 1;
			CG_PositionEntityOnTag(&barrel, &gun, "tag_bipod", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
		}
		else if (IS_MG_WEAPON_SET(weaponNum))
		{
			barrel.hModel = weapon->modModels[0];
			barrel.frame  = 0;
			CG_PositionEntityOnTag(&barrel, &gun, "tag_bipod", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
		}
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on the single player podiums), so
	// go ahead and use the cent
	if ((nonPredictedCent - cg_entities) != cent->currentState.clientNum)
	{
		nonPredictedCent = cent;
	}

	// add the flash
	memset(&flash, 0, sizeof(flash));
	VectorCopy(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx    = parent->renderfx;

	if (ps)
	{
		flash.hModel = weapon->flashModel[W_FP_MODEL];
	}
	else
	{
		flash.hModel = weapon->flashModel[W_TP_MODEL];
	}

	angles[YAW]   = 0;
	angles[PITCH] = 0;
	angles[ROLL]  = crandom() * 10;
	AnglesToAxis(angles, flash.axis);

	if (/*isPlayer &&*/ IS_AKIMBO_WEAPON(weaponNum))
	{
		if (!ps || cg.renderingThirdPerson)
		{
			if (!cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_weapon");
				VectorMA(flash.origin, 10, flash.axis[0], flash.origin);
			}
			else
			{
				CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
			}
		}
		else
		{
			if (!cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash2");

			}
			else
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash");
			}
		}
	}
	else
	{
		CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
	}

	// store this position for other cgame elements to access
	cent->pe.gunRefEnt      = gun;
	cent->pe.gunRefEntFrame = cg.clientFrame;

	if (weaponNum == WP_FLAMETHROWER && (nonPredictedCent->currentState.eFlags & EF_FIRING))
	{
		// continuous flash
	}
	else
	{
		// continuous smoke after firing
		if (ps || cg.renderingThirdPerson || !isPlayer)
		{
			if (weaponNum == WP_STEN || IS_MG_WEAPON(weaponNum) || IS_MG_WEAPON_SET(weaponNum))
			{
				// hot smoking gun
				if (cg.time - cent->overheatTime < 3000)
				{
					if (!(rand() % 3))
					{
						float alpha = 1.0f - ((float)(cg.time - cent->overheatTime) / 3000.0f);

						alpha *= 0.25f;     // .25 max alpha
						CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 1000, 8, 20, 30, alpha, 8.f);
					}
				}
			}
			else if (IS_PANZER_WEAPON(weaponNum))
			{
				if (cg.time - cent->muzzleFlashTime < BARREL_SMOKE_TIME)
				{
					if (!(rand() % 5))
					{
						float alpha = 1.0f - ((float)(cg.time - cent->muzzleFlashTime) / (float)BARREL_SMOKE_TIME);     // what fraction of BARREL_SMOKE_TIME are we at

						alpha *= 0.25f;     // .25 max alpha
						CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 1000, 8, 20, 30, alpha, 8.f);
					}
				}
			}
		}

		if (IS_MORTAR_WEAPON_SET(weaponNum))
		{
			if (ps && !cg.renderingThirdPerson && cg.time - cent->muzzleFlashTime < 800)
			{
				CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 700, 16, 20, 30, .12f, 4.f);
			}
		}

		// impulse flash
		if (cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME)
		{
			// blue ignition flame if not firing flamer
			if (weaponNum != WP_FLAMETHROWER)
			{
				return;
			}
		}
	}

	// weapons that don't need to go any further as they have no flash or light
	// FIXME: weapon table
	switch (weaponNum)
	{
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_KNIFE:
	case WP_KNIFE_KABAR:
	case WP_DYNAMITE:
	case WP_GPG40:
	case WP_M7:
	case WP_LANDMINE:
	case WP_SATCHEL:
	case WP_SATCHEL_DET:
	case WP_SMOKE_BOMB:
	case WP_MEDIC_SYRINGE:
	case WP_MEDIC_ADRENALINE:
		return;
	default:
		break;
	}

	// weaps with barrel smoke
	if (ps || cg.renderingThirdPerson || !isPlayer)
	{
		if (weaponNum == WP_STEN)
		{
			if (cg.time - cent->muzzleFlashTime < 100)
			{
				CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 500, 8, 20, 30, 0.25f, 8.f);
			}
		}
	}

	if (flash.hModel)
	{
		if (weaponNum != WP_FLAMETHROWER)     // hide the flash also for now
		{   // changed this so the muzzle flash stays onscreen for long enough to be seen
			if (cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME)
			{
				//if (firing) { // Ridah
				trap_R_AddRefEntityToScene(&flash);
			}
		}
	}

	if (ps || cg.renderingThirdPerson || !isPlayer)
	{
		// no flamethrower flame on prone moving or dead players
		if (firing && !(cent->currentState.eFlags & (EF_PRONE_MOVING | EF_DEAD)))
		{
			// Flamethrower effect
			CG_FlamethrowerFlame(cent, flash.origin);

			// make a dlight for the flash
			if (weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2])
			{
				trap_R_AddLightToScene(flash.origin, 320, 1.25 + (rand() & 31) / 128.0f, weapon->flashDlightColor[0],
				                       weapon->flashDlightColor[1], weapon->flashDlightColor[2], 0, 0);
			}
		}
		else
		{
			if (weaponNum == WP_FLAMETHROWER)
			{
				vec3_t angles;
				AxisToAngles(flash.axis, angles);

				if (ps)
				{
					if (ps->ammoclip[BG_FindAmmoForWeapon(WP_FLAMETHROWER)])
					{
						CG_FireFlameChunks(cent, flash.origin, angles, 1.0, qfalse);
					}
				}
				else
				{
					CG_FireFlameChunks(cent, flash.origin, angles, 1.0, qfalse);
				}
			}
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon(playerState_t *ps)
{
	refEntity_t  hand;
	float        fovOffset;
	vec3_t       angles;
	vec3_t       gunoff;
	weaponInfo_t *weapon;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (ps->pm_type == PM_INTERMISSION)
	{
		return;
	}

	// no gun if in third person view
	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (cg.editingSpeakers)
	{
		return;
	}

	// allow the gun to be completely removed
	if ((!cg_drawGun.integer))
	{
		if ((cg.predictedPlayerState.eFlags & EF_FIRING) && !(cg.predictedPlayerState.eFlags & (EF_MG42_ACTIVE | EF_MOUNTEDTANK)))
		{
			vec3_t origin;

			// special hack for flamethrower...
			VectorCopy(cg.refdef_current->vieworg, origin);

			VectorMA(origin, 18, cg.refdef_current->viewaxis[0], origin);
			VectorMA(origin, -7, cg.refdef_current->viewaxis[1], origin);
			VectorMA(origin, -4, cg.refdef_current->viewaxis[2], origin);

			// Flamethrower effect
			CG_FlamethrowerFlame(&cg.predictedPlayerEntity, origin);
		}

		if (cg.binocZoomTime)
		{
			if (cg.binocZoomTime < 0)
			{
				if (-cg.binocZoomTime + 700 < cg.time)
				{
					cg.binocZoomTime = 0;
				}
			}
			else
			{
				if (cg.binocZoomTime + 500 < cg.time)
				{
					trap_SendConsoleCommand("+zoom\n");
					cg.binocZoomTime = 0;
				}
				else
				{
				}
			}
		}

		return;
	}

	// don't draw if testing a gun model
	if (cg.testGun)
	{
		return;
	}

	if ((ps->eFlags & EF_MG42_ACTIVE) || (ps->eFlags & EF_AAGUN_ACTIVE))
	{
		return;
	}

	// drop gun lower at higher fov
	if (!cg_gun_fovscale.integer && cg_fov.integer > 90)
	{
		fovOffset = -0.2 * (cg_fov.integer - 90);
	}
	else
	{
		fovOffset = 0;
	}

	// mounted gun drawing
	if (ps->eFlags & EF_MOUNTEDTANK)
	{
		memset(&hand, 0, sizeof(hand));
		CG_CalculateWeaponPosition(hand.origin, angles);
		AnglesToAxis(angles, hand.axis);
		hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

		if (cg_entities[cg_entities[cg_entities[ps->clientNum].tagParent].tankparent].currentState.density & 8)       // should we use a browning?
		{
			hand.hModel = cgs.media.hMountedFPBrowning;
		}
		else
		{
			hand.hModel = cgs.media.hMountedFPMG42;
		}

		//gunoff[0] = cg_gun_x.value;
		//gunoff[1] = cg_gun_y.value;
		//gunoff[2] = cg_gun_z.value;

		gunoff[0] = 20;
		if (cg.time - cg.predictedPlayerEntity.muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			gunoff[0] += random() * 2.f;
		}
		VectorMA(hand.origin, gunoff[0], cg.refdef_current->viewaxis[0], hand.origin);
		VectorMA(hand.origin, -10, cg.refdef_current->viewaxis[1], hand.origin);
		VectorMA(hand.origin, (-8 + fovOffset), cg.refdef_current->viewaxis[2], hand.origin);

		CG_AddWeaponWithPowerups(&hand, cg.predictedPlayerEntity.currentState.powerups, ps, &cg.predictedPlayerEntity);

		if (cg.time - cg.predictedPlayerEntity.overheatTime < 3000)
		{
			if (!(rand() % 3))
			{
				float alpha = 1.0f - ((float)(cg.time - cg.predictedPlayerEntity.overheatTime) / 3000.0f);

				alpha *= 0.25f;     // .25 max alpha
				CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, cg.tankflashorg, 1000, 8, 20, 30, alpha, 8.f);
			}
		}

		{
			// FIXME: HACK dummy model to just draw _something_
			refEntity_t flash;

			memset(&flash, 0, sizeof(flash));
			flash.renderfx = (RF_LIGHTING_ORIGIN | RF_DEPTHHACK);
			flash.hModel   = cgs.media.mg42muzzleflash;

			angles[YAW]   = 0;
			angles[PITCH] = 0;
			angles[ROLL]  = crandom() * 10;
			AnglesToAxis(angles, flash.axis);

			CG_PositionRotatedEntityOnTag(&flash, &hand, "tag_flash");

			VectorMA(flash.origin, 22, flash.axis[0], flash.origin);

			VectorCopy(flash.origin, cg.tankflashorg);

			if (cg.time - cg.predictedPlayerEntity.muzzleFlashTime < MUZZLE_FLASH_TIME)
			{
				trap_R_AddRefEntityToScene(&flash);
			}
		}
		return;
	}

	if (ps->weapon > WP_NONE)
	{
		float lengthscale;

		weapon = &cg_weapons[ps->weapon];

		memset(&hand, 0, sizeof(hand));

		// set up gun position
		CG_CalculateWeaponPosition(hand.origin, angles);

		gunoff[0] = cg_gun_x.value;
		gunoff[1] = cg_gun_y.value;
		gunoff[2] = cg_gun_z.value;

		VectorMA(hand.origin, gunoff[0], cg.refdef_current->viewaxis[0], hand.origin);
		VectorMA(hand.origin, gunoff[1], cg.refdef_current->viewaxis[1], hand.origin);
		VectorMA(hand.origin, (gunoff[2] + fovOffset), cg.refdef_current->viewaxis[2], hand.origin);

		AnglesToAxis(angles, hand.axis);

		if (cg_gun_frame.integer)
		{
			hand.frame    = hand.oldframe = cg_gun_frame.integer;
			hand.backlerp = 0;
		}
		else      // get the animation state
		{
			if (cg.binocZoomTime)
			{
				if (cg.binocZoomTime < 0)
				{
					if (-cg.binocZoomTime + 500 + 200 < cg.time)
					{
						cg.binocZoomTime = 0;
					}
					else
					{
						if (-cg.binocZoomTime + 200 < cg.time)
						{
							CG_ContinueWeaponAnim(WEAP_ALTSWITCHFROM);
						}
						else
						{
							CG_ContinueWeaponAnim(WEAP_IDLE2);
						}
					}
				}
				else
				{
					if (cg.binocZoomTime + 500 < cg.time)
					{
						trap_SendConsoleCommand("+zoom\n");
						cg.binocZoomTime = 0;
						CG_ContinueWeaponAnim(WEAP_IDLE2);
					}
					else
					{
						CG_ContinueWeaponAnim(WEAP_ALTSWITCHTO);
					}
				}
			}
			CG_WeaponAnimation(ps, weapon, &hand.oldframe, &hand.frame, &hand.backlerp);
		}

		hand.hModel   = weapon->handsModel;
		hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

		// adjust bazooka so it has bigger distance to our crosshair
		if (ps->weapon == WP_BAZOOKA)
		{
			hand.axis[0][0]       *= .8f;
			hand.axis[0][1]       *= .8f;
			hand.axis[0][2]       *= .8f;
			hand.nonNormalizedAxes = qtrue;
		}

		if (cg_gun_fovscale.integer && cg_fov.integer != 0)
		{
			if (cg_gun_fovscale.integer > 1 && cg_fov.integer <= 90)
			{
				lengthscale = 1.0f;
			}
			else
			{
				lengthscale = 1.0f / tan(DEG2RAD(cg_fov.integer / 2.0f));
			}
		}
		else
		{
			lengthscale = 1.0f;
		}

		VectorScale(hand.axis[0], lengthscale, hand.axis[0]);

		// add everything onto the hand
		CG_AddPlayerWeapon(&hand, ps, &cg.predictedPlayerEntity);
	}
}

/*
==============================================================================
WEAPON SELECTION
==============================================================================
*/

#define WP_ICON_X       38  // new sizes per MK
#define WP_ICON_X_WIDE  72  // new sizes per MK
#define WP_ICON_Y       38
#define WP_ICON_SPACE_Y 10
#define WP_DRAW_X       SCREEN_WIDTH - WP_ICON_X - 4 // 4 is 'selected' border width
#define WP_DRAW_X_WIDE  SCREEN_WIDTH - WP_ICON_X_WIDE - 4
#define WP_DRAW_Y       4

// secondary fire icons
#define WP_ICON_SEC_X   18  // new sizes per MK
#define WP_ICON_SEC_Y   18

/*
==============
CG_WeaponHasAmmo
    check for ammo
==============
*/
static qboolean CG_WeaponHasAmmo(int i)
{
	// certain weapons don't have ammo
	switch (i)
	{
	case WP_KNIFE:
	case WP_KNIFE_KABAR:
	case WP_PLIERS:
		return qtrue;
	default:
		break;
	}

	if (!(cg.predictedPlayerState.ammo[BG_FindAmmoForWeapon(i)]) &&
	    !(cg.predictedPlayerState.ammoclip[BG_FindClipForWeapon(i)]))
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_WeaponSelectable
===============
*/
qboolean CG_WeaponSelectable(int i)
{

	// allow the player to unselect all weapons
	//if(i == WP_NONE)
	//  return qtrue;

	if (BG_PlayerMounted(cg.predictedPlayerState.eFlags))
	{
		return qfalse;
	}

	// check for weapon
	if (!(COM_BitCheck(cg.predictedPlayerState.weapons, i)))
	{
		return qfalse;
	}

	if (!CG_WeaponHasAmmo(i))
	{
		return qfalse;
	}

	return qtrue;
}

/*
==============
CG_WeaponIndex
==============
*/
int CG_WeaponIndex(int weapnum, int *bank, int *cycle)
{
	static int bnk, cyc;

	if (weapnum <= 0 || weapnum >= WP_NUM_WEAPONS)
	{
		if (bank)
		{
			*bank = 0;
		}
		if (cycle)
		{
			*cycle = 0;
		}
		return 0;
	}

	for (bnk = 0; bnk < MAX_WEAP_BANKS_MP; bnk++)
	{
		for (cyc = 0; cyc < MAX_WEAPS_IN_BANK_MP; cyc++)
		{
			if (!weapBanksMultiPlayer[bnk][cyc])
			{
				break;
			}

			// found the current weapon
			if (weapBanksMultiPlayer[bnk][cyc] == weapnum)
			{
				if (bank)
				{
					*bank = bnk;
				}
				if (cycle)
				{
					*cycle = cyc;
				}
				return 1;
			}
		}
	}

	// failed to find the weapon in the table
	// probably an alternate

	return 0;
}

/*
==============
getNextWeapInBank
    Pass in a bank and cycle and this will return the next valid weapon higher in the cycle.
    if the weap passed in is above highest in a cycle (MAX_WEAPS_IN_BANK), this will safely loop around
==============
*/
static int getNextWeapInBank(int bank, int cycle)
{
	cycle++;

	cycle = cycle % MAX_WEAPS_IN_BANK_MP;

	if (weapBanksMultiPlayer[bank][cycle])          // return next weapon in bank if there is one
	{
		return weapBanksMultiPlayer[bank][cycle];
	}
	else                                    // return first in bank
	{
		return weapBanksMultiPlayer[bank][0];
	}
}

static int getNextWeapInBankBynum(int weapnum)
{
	int bank, cycle;

	if (!CG_WeaponIndex(weapnum, &bank, &cycle))
	{
		return weapnum;
	}

	return getNextWeapInBank(bank, cycle);
}

/*
==============
getPrevWeapInBank
    Pass in a bank and cycle and this will return the next valid weapon lower in the cycle.
    if the weap passed in is the lowest in a cycle (0), this will loop around to the
    top (MAX_WEAPS_IN_BANK-1) and start down from there looking for a valid weapon position
==============
*/
static int getPrevWeapInBank(int bank, int cycle)
{
	cycle--;
	if (cycle < 0)
	{
		cycle = MAX_WEAPS_IN_BANK_MP - 1;
	}

	while (!weapBanksMultiPlayer[bank][cycle])
	{
		cycle--;

		if (cycle < 0)
		{
			cycle = MAX_WEAPS_IN_BANK_MP - 1;
		}
	}
	return weapBanksMultiPlayer[bank][cycle];
}

static int getPrevWeapInBankBynum(int weapnum)
{
	int bank, cycle;

	if (!CG_WeaponIndex(weapnum, &bank, &cycle))
	{
		return weapnum;
	}

	return getPrevWeapInBank(bank, cycle);
}

/*
==============
getNextBankWeap
    Pass in a bank and cycle and this will return the next valid weapon in a higher bank.
    sameBankPosition: if there's a weapon in the next bank at the same cycle,
    return that (colt returns thompson for example) rather than the lowest weapon
==============
*/
static int getNextBankWeap(int bank, int cycle, qboolean sameBankPosition)
{
	bank++;

	bank = bank % MAX_WEAP_BANKS_MP;

	if (sameBankPosition && weapBanksMultiPlayer[bank][cycle])
	{
		return weapBanksMultiPlayer[bank][cycle];
	}
	else
	{
		return weapBanksMultiPlayer[bank][0];
	}
}

/*
==============
getPrevBankWeap
    Pass in a bank and cycle and this will return the next valid weapon in a lower bank.
    sameBankPosition: if there's a weapon in the prev bank at the same cycle,
    return that (thompson returns colt for example) rather than the highest weapon
==============
*/
static int getPrevBankWeap(int bank, int cycle, qboolean sameBankPosition)
{
	bank--;

	if (bank < 0)          // don't go below 0, cycle up to top
	{
		bank += MAX_WEAP_BANKS_MP;

	}
	bank = bank % MAX_WEAP_BANKS_MP;

	if (sameBankPosition && weapBanksMultiPlayer[bank][cycle])
	{
		return weapBanksMultiPlayer[bank][cycle];
	}
	else    // find highest weap in bank
	{
		int i;

		for (i = MAX_WEAPS_IN_BANK_MP - 1; i >= 0; i--)
		{
			if (weapBanksMultiPlayer[bank][i])
			{
				return weapBanksMultiPlayer[bank][i];
			}
		}

		// if it gets to here, no valid weaps in this bank, go down another bank
		return getPrevBankWeap(bank, cycle, sameBankPosition);
	}
}

/*
==============
getAltWeapon
==============
*/
static int getAltWeapon(int weapnum)
{
	if (weaponTable[weapnum].weapAlts)
	{
		return weaponTable[weapnum].weapAlts;
	}

	return weapnum;
}

/**
 *   @brief Passing the weapnum of the mp40 returns the id of the thompson, and likewise
 *   passing the weapnum of the thompson returns the id of the mp40.
 *   No equivalent available will return the weapnum passed in.
 *
 *   @return the id of the opposite team's weapon (but not for WP_GPG40 <-> WP_M7 - see CG_OutOfAmmoChange).
 */
int getEquivWeapon(int weapnum)
{
	int num = weapnum;

	switch (weapnum)
	{
	// going from german to american
	case WP_LUGER:
		num = WP_COLT;
		break;
	case WP_MP40:
		num = WP_THOMPSON;
		break;
	case WP_GRENADE_LAUNCHER:
		num = WP_GRENADE_PINEAPPLE;
		break;
	case WP_KAR98:
		num = WP_CARBINE;
		break;
	case WP_SILENCER:
		num = WP_SILENCED_COLT;
		break;
	// going from american to german
	case WP_COLT:
		num = WP_LUGER;
		break;
	case WP_THOMPSON:
		num = WP_MP40;
		break;
	case WP_GRENADE_PINEAPPLE:
		num = WP_GRENADE_LAUNCHER;
		break;
	case WP_CARBINE:
		num = WP_KAR98;
		break;
	case WP_SILENCED_COLT:
		num = WP_SILENCER;
		break;
	}
	return num;
}

/*
==============
CG_SetSniperZoom
==============
*/
void CG_SetSniperZoom(int lastweap, int newweap)
{
	int zoomindex;

	if (lastweap == newweap)
	{
		return;
	}

	// Keep binocs swaying
	if (!(cg.predictedPlayerState.eFlags & EF_ZOOMING))
	{
		cg.zoomval = 0;
	}
	cg.zoomedScope = 0;

	switch (newweap)
	{
	default:
		return;     // no sniper zoom, get out.
	case WP_FG42SCOPE:
		cg.zoomval     = cg_zoomDefaultSniper.value; // changed from defaultFG per atvi req
		cg.zoomedScope = 1;         // TODO: add to zoomTable
		zoomindex      = ZOOM_SNIPER; //  was FG42SCOPE
		break;
	case WP_GARAND_SCOPE:
	// fall through
	case WP_K43_SCOPE:
		cg.zoomval     = cg_zoomDefaultSniper.value;
		cg.zoomedScope = 900;       // TODO: add to zoomTable
		zoomindex      = ZOOM_SNIPER;
		break;
	}

	// constrain user preferred fov to weapon limitations
	if (cg.zoomval > zoomTable[zoomindex][ZOOM_OUT])
	{
		cg.zoomval = zoomTable[zoomindex][ZOOM_OUT];
	}
	if (cg.zoomval < zoomTable[zoomindex][ZOOM_IN])
	{
		cg.zoomval = zoomTable[zoomindex][ZOOM_IN];
	}

	cg.zoomTime = cg.time;
}

/*
==============
CG_PlaySwitchSound
    Get special switching sounds if they're there
==============
*/
void CG_PlaySwitchSound(int lastweap, int newweap)
{
	sfxHandle_t switchsound = cgs.media.selectSound;

	if (getAltWeapon(lastweap) == newweap)       // alt switch
	{
		switch (newweap)
		{
		case WP_SILENCER:
		case WP_LUGER:
		case WP_SILENCED_COLT:
		case WP_COLT:
		case WP_GPG40:
		case WP_M7:
		case WP_MORTAR:
		case WP_MORTAR_SET:
		case WP_MORTAR2:
		case WP_MORTAR2_SET:
		case WP_MOBILE_MG42:
		case WP_MOBILE_MG42_SET:
		case WP_MOBILE_BROWNING:
		case WP_MOBILE_BROWNING_SET:
			switchsound = cg_weapons[newweap].switchSound;
			break;
		case WP_CARBINE:
		case WP_KAR98:
			if (cg.predictedPlayerState.ammoclip[lastweap])
			{
				switchsound = cg_weapons[newweap].switchSound;
			}
			break;
		default:
			return;
		}
	}
	else
	{
		return;
	}

	trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, switchsound);
}

/*
==============
CG_FinishWeaponChange
==============
*/
void CG_FinishWeaponChange(int lastweap, int newweap)
{
	int newbank;

	if (cg.binocZoomTime)
	{
		return;
	}

	cg.mortarImpactTime = -2;

	switch (newweap)
	{
	case WP_LUGER:
		if ((cg.pmext.silencedSideArm & 1) && lastweap != WP_SILENCER)
		{
			newweap         = WP_SILENCER;
			cg.weaponSelect = newweap;
		}
		break;
	case WP_SILENCER:
		if (!(cg.pmext.silencedSideArm & 1) && lastweap != WP_LUGER)
		{
			newweap         = WP_LUGER;
			cg.weaponSelect = newweap;
		}
		break;
	case WP_COLT:
		if ((cg.pmext.silencedSideArm & 1) && lastweap != WP_SILENCED_COLT)
		{
			newweap         = WP_SILENCED_COLT;
			cg.weaponSelect = newweap;
		}
		break;
	case WP_SILENCED_COLT:
		if (!(cg.pmext.silencedSideArm & 1) && lastweap != WP_COLT)
		{
			newweap         = WP_COLT;
			cg.weaponSelect = newweap;
		}
		break;
	case WP_CARBINE:
		if ((cg.pmext.silencedSideArm & 2) && lastweap != WP_M7)
		{
			newweap         = WP_M7;
			cg.weaponSelect = newweap;
		}
		break;
	case WP_M7:
		if (!(cg.pmext.silencedSideArm & 2) && lastweap != WP_CARBINE)
		{
			newweap         = WP_CARBINE;
			cg.weaponSelect = newweap;
		}
		break;
	case WP_KAR98:
		if ((cg.pmext.silencedSideArm & 2) && lastweap != WP_GPG40)
		{
			newweap         = WP_GPG40;
			cg.weaponSelect = newweap;
		}
		break;
	case WP_GPG40:
		if (!(cg.pmext.silencedSideArm & 2) && lastweap != WP_KAR98)
		{
			newweap         = WP_KAR98;
			cg.weaponSelect = newweap;
		}
		break;
//  case WP_MEDIC_SYRINGE:
//      if((cg.pmext.silencedSideArm & 4) && lastweap != WP_MEDIC_ADRENALINE) {
//          newweap = WP_MEDIC_ADRENALINE;
//          cg.weaponSelect = newweap;
//      }
//      break;
//  case WP_MEDIC_ADRENALINE:
//      if(!(cg.pmext.silencedSideArm & 4) && lastweap != WP_MEDIC_SYRINGE) {
//          newweap = WP_MEDIC_SYRINGE;
//          cg.weaponSelect = newweap;
//      }
//      break;
	default:
		break;
	}

	if (lastweap == WP_BINOCULARS && (cg.snap->ps.eFlags & EF_ZOOMING))
	{
		trap_SendConsoleCommand("-zoom\n");
	}

	cg.weaponSelectTime = cg.time;  // flash the weapon icon

	if (cg.newCrosshairIndex)
	{
		trap_Cvar_Set("cg_drawCrossHair", va("%d", cg.newCrosshairIndex - 1));
	}
	cg.newCrosshairIndex = 0;

	// remember which weapon in this bank was last selected so when cycling back
	// to this bank, that weap will be highlighted first
	if (CG_WeaponIndex(newweap, &newbank, NULL))
	{
		cg.lastWeapSelInBank[newbank] = newweap;
	}

	if (lastweap == newweap)       // no need to do any more than flash the icon
	{
		return;
	}

	CG_PlaySwitchSound(lastweap, newweap);    // grabbed from SP

	CG_SetSniperZoom(lastweap, newweap);

	// setup for a user call to CG_LastWeaponUsed_f()
	if (lastweap == cg.lastFiredWeapon)
	{
		// don't set switchback for some weaps...
		switch (lastweap)
		{
		case WP_FG42SCOPE:
		case WP_GARAND_SCOPE:
		case WP_K43_SCOPE:
			break;
		default:
			cg.switchbackWeapon = lastweap;
			break;
		}
	}
	else
	{
		// if this ended up having the switchback be the same
		// as the new weapon, set the switchback to the prev
		// selected weapon will become the switchback
		if (cg.switchbackWeapon == newweap)
		{
			cg.switchbackWeapon = lastweap;
		}
		// this fixes cg.switchbackWeapon=0 after very first spawn and switching weapon for the first time
		else if (cg.switchbackWeapon == WP_NONE && CG_WeaponSelectable(lastweap)) // ensure last weapon is available
		{
			cg.switchbackWeapon = lastweap;
		}
	}

	cg.weaponSelect = newweap;
}

extern pmove_t cg_pmove;

/*
==============
CG_AltfireWeapon_f
    for example, switching between WP_MAUSER and WP_SNIPERRIFLE
==============
*/
void CG_AltWeapon_f(void)
{
	int      original, num;
	qboolean reload = qfalse;

	if (!cg.snap)
	{
		return;
	}

	// Overload for spec mode when following
	if (((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
#if FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		trap_SendConsoleCommand("followprev");
		return;
	}

	// Need ground for this
	if (cg.weaponSelect == WP_MORTAR || cg.weaponSelect == WP_MORTAR2)
	{
		int    contents;
		vec3_t point;

		if (cg.predictedPlayerState.groundEntityNum == ENTITYNUM_NONE)
		{
			return;
		}

		if (cg.weaponSelect == WP_MORTAR && !cg.predictedPlayerState.ammoclip[WP_MORTAR])
		{
			return;
		}

		if (cg.weaponSelect == WP_MORTAR2 && !cg.predictedPlayerState.ammoclip[WP_MORTAR2])
		{
			return;
		}

		if (cg.predictedPlayerState.eFlags & EF_PRONE)
		{
			return;
		}

		if (cg_pmove.waterlevel == 3)
		{
			return;
		}

		// don't allow set if moving
		if (VectorLengthSquared(cg.snap->ps.velocity))
		{
			return;
		}

		// eurgh, need it here too else we play sounds :/
		point[0] = cg.snap->ps.origin[0];
		point[1] = cg.snap->ps.origin[1];
		point[2] = cg.snap->ps.origin[2] + cg.snap->ps.crouchViewHeight;
		contents = CG_PointContents(point, cg.snap->ps.clientNum);
		if (contents & MASK_WATER)
		{
			return;
		}
	}
	else if (IS_MG_WEAPON(cg.weaponSelect))
	{
		if (!(cg.predictedPlayerState.eFlags & EF_PRONE))
		{
			return;
		}
	}

	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return; // force pause so holding it down won't go too fast

	}
	// Don't try to switch when in the middle of reloading.
	if (cg.snap->ps.weaponstate == WEAPON_RELOADING)
	{
		return;
	}

	original = cg.weaponSelect;

	num = getAltWeapon(original);

	if (original == WP_BINOCULARS)
	{
		/*if(cg.snap->ps.eFlags & EF_ZOOMING) {
		    trap_SendConsoleCommand( "-zoom\n" );
		} else {
		    trap_SendConsoleCommand( "+zoom\n" );
		}*/
		if (cg.snap->ps.eFlags & EF_ZOOMING)
		{
			trap_SendConsoleCommand("-zoom\n");
			cg.binocZoomTime = -cg.time;
		}
		else
		{
			if (!cg.binocZoomTime)
			{
				cg.binocZoomTime = cg.time;
			}
		}
	}

	// don't allow another weapon switch when we're still swapping the gpg40, to prevent animation breaking
	if ((cg.snap->ps.weaponstate == WEAPON_RAISING || cg.snap->ps.weaponstate == WEAPON_DROPPING) &&
	    // FIXME: do a switch
	    ((original == WP_GPG40 || num == WP_GPG40 || original == WP_M7 || num == WP_M7) ||
	     (original == WP_SILENCER || num == WP_SILENCER || original == WP_SILENCED_COLT || num == WP_SILENCED_COLT) ||
	     (original == WP_AKIMBO_SILENCEDCOLT || num == WP_AKIMBO_SILENCEDCOLT || original == WP_AKIMBO_SILENCEDLUGER || num == WP_AKIMBO_SILENCEDLUGER) ||
	     (original == WP_MORTAR_SET || num == WP_MORTAR_SET) ||
	     (original == WP_MORTAR2_SET || num == WP_MORTAR2_SET) ||
	     (original == WP_MOBILE_MG42_SET || num == WP_MOBILE_MG42_SET) ||
	     (original == WP_MOBILE_BROWNING_SET || num == WP_MOBILE_BROWNING_SET)))
	{
		return;
	}

	if (CG_WeaponSelectable(num))        // new weapon is valid
	{
		CG_FinishWeaponChange(original, num);
		if (original == num)
		{
			reload = qtrue;
		}
	}
	else
	{
		reload = qtrue;
	}

	if (reload && cg_weapaltReloads.integer)
	{
		// FIXME: This is a horrible way of doing it but theres not other way atm.
		trap_SendConsoleCommand("+reload\n");
		trap_SendConsoleCommand("-reload\n");
	}
}

/*
==============
CG_NextWeap

  switchBanks - curweap is the last in a bank, 'qtrue' means go to the next available bank, 'qfalse' means loop to the head of the bank
==============
*/
void CG_NextWeap(qboolean switchBanks)
{
	int      bank     = 0, cycle = 0, newbank = 0, newcycle = 0;
	int      num      = cg.weaponSelect;
	int      curweap  = cg.weaponSelect;
	qboolean nextbank = qfalse;     // need to switch to the next bank of weapons?
	int      i;

	if (IS_MORTAR_WEAPON_SET(curweap) || IS_MG_WEAPON_SET(curweap))
	{
		return;
	}

	switch (num)
	{
	case WP_LUGER:
		curweap = num = WP_SILENCER;
		break;
	case WP_COLT:
		curweap = num = WP_SILENCED_COLT;
		break;
	case WP_KAR98:
		curweap = num = WP_GPG40;
		break;
	case WP_CARBINE:
		curweap = num = WP_M7;
		break;
	default:
		break;
	}

	CG_WeaponIndex(curweap, &bank, &cycle);       // get bank/cycle of current weapon

	if (cg_cycleAllWeaps.integer || !switchBanks)
	{
		for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
		{
			num = getNextWeapInBankBynum(num);

			CG_WeaponIndex(num, NULL, &newcycle);         // get cycle of new weapon.  if it's lower than the original, then it cycled around

			if (switchBanks)
			{
				if (newcycle <= cycle)
				{
					nextbank = qtrue;
					break;
				}
			}
			else        // don't switch banks if you get to the end
			{
				if (num == curweap)        // back to start, just leave it where it is
				{
					return;
				}
			}

			if (CG_WeaponSelectable(num))
			{
				break;
			}
			else
			{
				qboolean found = qfalse;

				switch (num)
				{
				case WP_CARBINE:
					if ((found = CG_WeaponSelectable(WP_M7)))
					{
						num = WP_M7;
					}
					break;
				case WP_KAR98:
					if ((found = CG_WeaponSelectable(WP_GPG40)))
					{
						num = WP_GPG40;
					}
					break;
				}

				if (found)
				{
					break;
				}
			}
		}
	}
	else
	{
		nextbank = qtrue;
	}

	if (nextbank)
	{
		int j;

		for (i = 0; i < MAX_WEAP_BANKS_MP; i++)
		{
			if (cg_cycleAllWeaps.integer)
			{
				num = getNextBankWeap(bank + i, cycle, qfalse);     // cycling all weaps always starts the next bank at the bottom
			}
			else
			{
				if (cg.lastWeapSelInBank[bank + i + 1])
				{
					num = cg.lastWeapSelInBank[bank + i + 1];
				}
				else
				{
					num = getNextBankWeap(bank + i, cycle, qtrue);
				}
			}

			if (num == 0)
			{
				continue;
			}

			//if(num == WP_BINOCULARS) {
			//  continue;
			//}

			if (CG_WeaponSelectable(num))       // first entry in bank was selectable, no need to scan the bank
			{
				break;
			}
			else
			{
				qboolean found = qfalse;

				switch (num)
				{
				case WP_CARBINE:
					if ((found = CG_WeaponSelectable(WP_M7)))
					{
						num = WP_M7;
					}
					break;
				case WP_KAR98:
					if ((found = CG_WeaponSelectable(WP_GPG40)))
					{
						num = WP_GPG40;
					}
					break;
				}

				if (found)
				{
					break;
				}
			}

			CG_WeaponIndex(num, &newbank, &newcycle);     // get the bank of the new weap

			for (j = newcycle; j < MAX_WEAPS_IN_BANK_MP; j++)
			{
				num = getNextWeapInBank(newbank, j);

				//if(num == WP_BINOCULARS) {
				// continue;
				//}

				if (CG_WeaponSelectable(num))       // found selectable weapon
				{
					break;
				}
				else
				{
					qboolean found = qfalse;

					switch (num)
					{
					case WP_CARBINE:
						if ((found = CG_WeaponSelectable(WP_M7)))
						{
							num = WP_M7;
						}
						break;
					case WP_KAR98:
						if ((found = CG_WeaponSelectable(WP_GPG40)))
						{
							num = WP_GPG40;
						}
						break;
					}

					if (found)
					{
						break;
					}
				}

				num = 0;
			}

			if (num)     // a selectable weapon was found in the current bank
			{
				break;
			}
		}
	}

	CG_FinishWeaponChange(curweap, num);
}

/*
==============
CG_PrevWeap

  switchBanks - curweap is the last in a bank
        'qtrue'  - go to the next available bank
        'qfalse' - loop to the head of the bank
==============
*/
void CG_PrevWeap(qboolean switchBanks)
{
	int      bank     = 0, cycle = 0, newbank = 0, newcycle = 0;
	int      num      = cg.weaponSelect;
	int      curweap  = cg.weaponSelect;
	qboolean prevbank = qfalse;     // need to switch to the next bank of weapons?
	int      i;

	if (IS_MORTAR_WEAPON_SET(curweap) || IS_MG_WEAPON_SET(curweap))
	{
		return;
	}

	switch (num)
	{
	case WP_SILENCER:
		curweap = num = WP_LUGER;
		break;
	case WP_SILENCED_COLT:
		curweap = num = WP_COLT;
		break;
	case WP_GPG40:
		curweap = num = WP_KAR98;
		break;
	case WP_M7:
		curweap = num = WP_CARBINE;
		break;
	default:
		break;
	}

	CG_WeaponIndex(curweap, &bank, &cycle);       // get bank/cycle of current weapon

	// if you're using an alt mode weapon, try switching back to the parent first
	if (IS_RIFLENADE_WEAPON(curweap))
	{
		num = getAltWeapon(curweap);      // base any further changes on the parent
		if (CG_WeaponSelectable(num))        // the parent was selectable, drop back to that
		{
			CG_FinishWeaponChange(curweap, num);
			return;
		}
	}

	// initially, just try to find a lower weapon in the current bank
	if (cg_cycleAllWeaps.integer || !switchBanks)
	{
		for (i = cycle; i >= 0; i--)
		{
			num = getPrevWeapInBankBynum(num);

			CG_WeaponIndex(num, NULL, &newcycle);         // get cycle of new weapon.  if it's greater than the original, then it cycled around

			if (switchBanks)
			{
				if (newcycle > (cycle - 1))
				{
					prevbank = qtrue;
					break;
				}
			}
			else        // don't switch banks if you get to the end
			{
				if (num == curweap)        // back to start, just leave it where it is
				{
					return;
				}
			}

			//if(num == WP_BINOCULARS) {
			//  continue;
			//}

			if (CG_WeaponSelectable(num))
			{
				break;
			}
			else
			{
				qboolean found = qfalse;
				switch (num)
				{
				case WP_CARBINE:
					if ((found = CG_WeaponSelectable(WP_M7)))
					{
						num = WP_M7;
					}
					break;
				case WP_KAR98:
					if ((found = CG_WeaponSelectable(WP_GPG40)))
					{
						num = WP_GPG40;
					}
					break;
				}

				if (found)
				{
					break;
				}
			}
		}
	}
	else
	{
		prevbank = qtrue;
	}

	// cycle to previous bank.
	//  if cycleAllWeaps: find highest weapon in bank
	//      else: try to find weap in bank that matches cycle position
	//          else: use base weap in bank

	if (prevbank)
	{
		int j;

		for (i = 0; i < MAX_WEAP_BANKS_MP; i++)
		{
			if (cg_cycleAllWeaps.integer)
			{
				num = getPrevBankWeap(bank - i, cycle, qfalse);     // cycling all weaps always starts the next bank at the bottom
			}
			else
			{
				num = getPrevBankWeap(bank - i, cycle, qtrue);
			}

			if (num == 0)
			{
				continue;
			}

			if (CG_WeaponSelectable(num))       // first entry in bank was selectable, no need to scan the bank
			{
				break;
			}
			else
			{
				qboolean found = qfalse;

				switch (num)
				{
				case WP_CARBINE:
					if ((found = CG_WeaponSelectable(WP_M7)))
					{
						num = WP_M7;
					}
					break;
				case WP_KAR98:
					if ((found = CG_WeaponSelectable(WP_GPG40)))
					{
						num = WP_GPG40;
					}
					break;
				}

				if (found)
				{
					break;
				}
			}

			CG_WeaponIndex(num, &newbank, &newcycle);     // get the bank of the new weap

			for (j = MAX_WEAPS_IN_BANK_MP; j > 0; j--)
			{
				num = getPrevWeapInBank(newbank, j);

				if (CG_WeaponSelectable(num))       // found selectable weapon
				{
					break;
				}
				else
				{
					qboolean found = qfalse;
					switch (num)
					{
					case WP_CARBINE:
						if ((found = CG_WeaponSelectable(WP_M7)))
						{
							num = WP_M7;
						}
						break;
					case WP_KAR98:
						if ((found = CG_WeaponSelectable(WP_GPG40)))
						{
							num = WP_GPG40;
						}
						break;
					}

					if (found)
					{
						break;
					}
				}

				num = 0;
			}

			if (num)     // a selectable weapon was found in the current bank
			{
				break;
			}
		}
	}

	CG_FinishWeaponChange(curweap, num);
}

/*
==============
CG_LastWeaponUsed_f
==============
*/
void CG_LastWeaponUsed_f(void)
{
	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return; // force pause so holding it down won't go too fast

	}

	if (IS_MORTAR_WEAPON_SET(cg.weaponSelect) || IS_MG_WEAPON_SET(cg.weaponSelect))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	// don't switchback if reloading (it nullifies the reload)
	if (cg.snap->ps.weaponstate == WEAPON_RELOADING)
	{
		return;
	}

	if (!cg.switchbackWeapon)
	{
		cg.switchbackWeapon = cg.weaponSelect;
		return;
	}

	if (CG_WeaponSelectable(cg.switchbackWeapon))
	{
		CG_FinishWeaponChange(cg.weaponSelect, cg.switchbackWeapon);
	}
	else        // switchback no longer selectable, reset cycle
	{
		cg.switchbackWeapon = WP_NONE;
	}
}

/*
==============
CG_NextWeaponInBank_f
==============
*/
void CG_NextWeaponInBank_f(void)
{
	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return; // force pause so holding it down won't go too fast

	}
	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if (cg.zoomval)
	{
		if (cg_useWeapsForZoom.integer == 1)
		{
			CG_ZoomIn_f();
			return;
		}
		else if (cg_useWeapsForZoom.integer == 2)
		{
			CG_ZoomOut_f();
			return;
		}
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_NextWeap(qfalse);
}

/*
==============
CG_PrevWeaponInBank_f
==============
*/
void CG_PrevWeaponInBank_f(void)
{
	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return; // force pause so holding it down won't go too fast

	}
	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if (cg.zoomval)
	{
		if (cg_useWeapsForZoom.integer == 2)
		{
			CG_ZoomIn_f();
			return;
		}
		else if (cg_useWeapsForZoom.integer == 1)
		{
			CG_ZoomOut_f();
			return;
		}
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_PrevWeap(qfalse);
}

/*
==============
CG_NextWeapon_f
==============
*/
void CG_NextWeapon_f(void)
{
	if (!cg.snap)
	{
		return;
	}

#ifdef FEATURE_MULTIVIEW
	// Overload for MV clients
	if (cg.mvTotalClients > 0)
	{
		CG_mvToggleView_f();
		return;
	}
#endif

	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if (cg.zoomval)
	{
		if (cg_useWeapsForZoom.integer == 1)
		{
			CG_ZoomIn_f();
			return;
		}
		else if (cg_useWeapsForZoom.integer == 2)
		{
			CG_ZoomOut_f();
			return;
		}
	}

	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	// Don't try to switch when in the middle of reloading.
	// cheatinfo:   The server actually would let you switch if this check were not
	//              present, but would discard the reload.  So the when you switched
	//              back you'd have to start the reload over.  This seems bad, however
	//              the delay for the current reload is already in effect, so you'd lose
	//              the reload time twice.  (the first pause for the current weapon reload,
	//              and the pause when you have to reload again 'cause you canceled this one)

	if (cg.snap->ps.weaponstate == WEAPON_RELOADING)
	{
		return;
	}

	CG_NextWeap(qtrue);
}

/*
==============
CG_PrevWeapon_f
==============
*/
void CG_PrevWeapon_f(void)
{
	if (!cg.snap)
	{
		return;
	}

#if FEATURE_MULTIVIEW
	// Overload for MV clients
	if (cg.mvTotalClients > 0)
	{
		CG_mvSwapViews_f();
		return;
	}
#endif

	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if (cg.zoomval)
	{
		if (cg_useWeapsForZoom.integer == 1)
		{
			CG_ZoomOut_f();
			return;
		}
		else if (cg_useWeapsForZoom.integer == 2)
		{
			CG_ZoomIn_f();
			return;
		}
	}

	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	// Don't try to switch when in the middle of reloading.
	if (cg.snap->ps.weaponstate == WEAPON_RELOADING)
	{
		return;
	}

	CG_PrevWeap(qtrue);
}

/*
==============
CG_WeaponBank_f
    weapon keys are not generally bound directly('bind 1 weapon 1'),
    rather the key is bound to a given bank ('bind 1 weaponbank 1')
==============
*/
void CG_WeaponBank_f(void)
{
	int num, i, curweap;
	int curbank = 0, curcycle = 0, bank = 0, cycle = 0;

	if (!cg.snap)
	{
		return;
	}

	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return; // force pause so holding it down won't go too fast

	}

	if (IS_MORTAR_WEAPON_SET(cg.weaponSelect) || IS_MG_WEAPON_SET(cg.weaponSelect))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	// Don't try to switch when in the middle of reloading.
	if (cg.snap->ps.weaponstate == WEAPON_RELOADING)
	{
		return;
	}

	bank = atoi(CG_Argv(1));

	if (bank <= 0 || bank > MAX_WEAP_BANKS_MP)
	{
		return;
	}

	curweap = cg.weaponSelect;
	CG_WeaponIndex(curweap, &curbank, &curcycle);         // get bank/cycle of current weapon

	if (!cg.lastWeapSelInBank[bank])
	{
		num    = weapBanksMultiPlayer[bank][0];
		cycle -= 1; // cycle up to first weap
	}
	else
	{
		num = cg.lastWeapSelInBank[bank];
		CG_WeaponIndex(num, &bank, &cycle);
		if (bank != curbank)
		{
			cycle -= 1;
		}
	}

	for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
		num = getNextWeapInBank(bank, cycle + i);

		if (CG_WeaponSelectable(num))
		{
			break;
		}
		else
		{
			qboolean found = qfalse;
			switch (num)
			{
			case WP_CARBINE:
				if ((found = CG_WeaponSelectable(WP_M7)))
				{
					num = WP_M7;
				}
				break;
			case WP_KAR98:
				if ((found = CG_WeaponSelectable(WP_GPG40)))
				{
					num = WP_GPG40;
				}
				break;
			}

			if (found)
			{
				break;
			}
		}
	}

	if (i == MAX_WEAPS_IN_BANK_MP)
	{
		return;
	}

	// don't allow another weapon switch when we're still swapping the gpg40, to prevent animation breaking
	// FIXME: do a switch
	if ((cg.snap->ps.weaponstate == WEAPON_RAISING || cg.snap->ps.weaponstate == WEAPON_DROPPING) &&
	    ((curweap == WP_GPG40 || num == WP_GPG40 || curweap == WP_M7 || num == WP_M7) ||
	     (curweap == WP_SILENCER || num == WP_SILENCER || curweap == WP_SILENCED_COLT || num == WP_SILENCED_COLT) ||
	     (curweap == WP_MORTAR_SET || num == WP_MORTAR_SET || curweap == WP_MORTAR2_SET || num == WP_MORTAR2_SET)))
	{
		return;
	}

	CG_FinishWeaponChange(curweap, num);
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f(void)
{
	int num;

	if (!cg.snap)
	{
		return;
	}

	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (IS_MORTAR_WEAPON_SET(cg.weaponSelect) || IS_MG_WEAPON_SET(cg.weaponSelect))
	{
		return;
	}

	num = atoi(CG_Argv(1));

	// weapon bind should execute weaponbank instead -- for splitting out class weapons, per Id request
	if (num < MAX_WEAP_BANKS_MP)
	{
		CG_WeaponBank_f();
	}
	return;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange(qboolean allowforceswitch)
{
	int i;
	int bank  = 0, cycle = 0;
	int equiv = WP_NONE;

	// trivial switching
	if (cg.weaponSelect == WP_PLIERS || (cg.weaponSelect == WP_SATCHEL_DET && cg.predictedPlayerState.ammo[WP_SATCHEL_DET]))
	{
		return;
	}

	if (allowforceswitch)
	{
		switch (cg.weaponSelect)
		{
		case WP_LANDMINE:
		case WP_DYNAMITE:
			if (CG_WeaponSelectable(WP_PLIERS))
			{
				cg.weaponSelect = WP_PLIERS;
				CG_FinishWeaponChange(cg.predictedPlayerState.weapon, WP_PLIERS);
				return;
			}
			break;
		case WP_SATCHEL:
			if (CG_WeaponSelectable(WP_SATCHEL_DET))
			{
				cg.weaponSelect = WP_SATCHEL_DET;
				return;
			}
			break;
		case WP_MORTAR_SET:
			cg.weaponSelect = WP_MORTAR;
			return;
		case WP_MORTAR2_SET:
			cg.weaponSelect = WP_MORTAR2;
			return;
		case WP_MOBILE_MG42_SET:
			cg.weaponSelect = WP_MOBILE_MG42;
			return;
		case WP_MOBILE_BROWNING_SET:
			cg.weaponSelect = WP_MOBILE_BROWNING;
			return;
		case WP_PANZERFAUST:
		case WP_BAZOOKA:
		case WP_SMOKE_BOMB:
		case WP_MEDIC_ADRENALINE:
			for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
			{
				// make sure we don't reselect the panzer or bazooka
				if (weapBanksMultiPlayer[3][i] != WP_PANZERFAUST && weapBanksMultiPlayer[3][i] != WP_BAZOOKA
				    && CG_WeaponSelectable(weapBanksMultiPlayer[3][i]))     // find a rifle
				{
					cg.weaponSelect = weapBanksMultiPlayer[3][i];
					CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
					return;
				}
			}
			for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
			{
				if (CG_WeaponSelectable(weapBanksMultiPlayer[2][i]))       // find a pistol
				{
					cg.weaponSelect = weapBanksMultiPlayer[2][i];
					CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
					return;
				}
			}
			for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
			{
				if (CG_WeaponSelectable(weapBanksMultiPlayer[4][i]))       // find a grenade
				{
					cg.weaponSelect = weapBanksMultiPlayer[4][i];
					CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
					return;
				}
			}
			for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
			{
				if (CG_WeaponSelectable(weapBanksMultiPlayer[1][i]))       // find a knife
				{
					cg.weaponSelect = weapBanksMultiPlayer[1][i];
					CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
					return;
				}
			}
			break;
		case WP_GPG40:
		case WP_M7:
			// if you're using an alt mode weapon, try switching back to the parent
			// otherwise, switch to the equivalent if you've got it
			cg.weaponSelect = equiv = getAltWeapon(cg.weaponSelect);      // base any further changes on the parent
			if (CG_WeaponSelectable(equiv))          // the parent was selectable, drop back to that
			{
				CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
				return;
			}
			break;
		default:
			break;
		}

		// now try the opposite team's equivalent weap
		equiv = getEquivWeapon(cg.weaponSelect);

		if (equiv != cg.weaponSelect && CG_WeaponSelectable(equiv))
		{
			cg.weaponSelect = equiv;
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
			return;
		}
	}

	// didn't have available alternative or equivalent, try another weap in the bank
	CG_WeaponIndex(cg.weaponSelect, &bank, &cycle);       // get bank/cycle of current weapon

	// more useful weapon changes -- check if rifle or pistol is still working, and use that if available
	for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
		if (CG_WeaponSelectable(weapBanksMultiPlayer[3][i]))       // find a rifle
		{
			cg.weaponSelect = weapBanksMultiPlayer[3][i];
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
			return;
		}
	}
	for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
		if (CG_WeaponSelectable(weapBanksMultiPlayer[2][i]))       // find a pistol
		{
			cg.weaponSelect = weapBanksMultiPlayer[2][i];
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
			return;
		}
	}
	for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
		if (CG_WeaponSelectable(weapBanksMultiPlayer[4][i]))       // find a grenade
		{
			cg.weaponSelect = weapBanksMultiPlayer[4][i];
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
			return;
		}
	}
	for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
		if (CG_WeaponSelectable(weapBanksMultiPlayer[1][i]))       // find a knife
		{
			cg.weaponSelect = weapBanksMultiPlayer[1][i];
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
			return;
		}
	}

	/* never reached, as knife is always found
	// otherwise just do something
	for (i = cycle; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
	    equiv = getNextWeapInBank(bank, i);
	    if (CG_WeaponSelectable(equiv))        // found a reasonable replacement
	    {
	        cg.weaponSelect = equiv;
	        CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
	        return;
	    }
	}

	// still nothing available, just go to the next
	// available weap using the regular selection scheme
	CG_NextWeap(qtrue);
	*/
}

/*
===================================================================================================
WEAPON EVENTS
===================================================================================================
*/

void CG_MG42EFX(centity_t *cent)
{
	// complete overhaul of this one
	centity_t   *mg42;
	int         num;
	vec3_t      forward, point;
	refEntity_t flash;

	// find the mg42 we're attached to
	for (num = 0 ; num < cg.snap->numEntities ; num++)
	{
		mg42 = &cg_entities[cg.snap->entities[num].number];
		if (mg42->currentState.eType == ET_MG42_BARREL &&
		    mg42->currentState.otherEntityNum == cent->currentState.number)
		{
			// found it, clamp behind gun

			VectorCopy(mg42->currentState.pos.trBase, point);
			//AngleVectors (mg42->s.apos.trBase, forward, NULL, NULL);
			AngleVectors(cent->lerpAngles, forward, NULL, NULL);
			VectorMA(point, 40, forward, point);

			memset(&flash, 0, sizeof(flash));
			flash.renderfx = RF_LIGHTING_ORIGIN;
			flash.hModel   = cgs.media.mg42muzzleflash;

			VectorCopy(point, flash.origin);
			AnglesToAxis(cent->lerpAngles, flash.axis);

			trap_R_AddRefEntityToScene(&flash);

			// add dynamic light
			trap_R_AddLightToScene(flash.origin, 320, 1.25 + (rand() & 31) / 128.0f,
			                       1.0f, 0.6f, 0.23f, 0, 0);
			return;
		}
	}
}

/*
==============
CG_MortarEFX
    Right now mostly copied directly from Raf's MG42 FX, but with the optional addtion of smoke
==============
*/
void CG_MortarEFX(centity_t *cent)
{
	if (cent->currentState.density & 1)
	{
		// smoke
		CG_ParticleImpactSmokePuff(cgs.media.smokePuffShader, cent->currentState.origin);
	}

	if (cent->currentState.density & 2)
	{
		refEntity_t flash;

		// light
		trap_R_AddLightToScene(cent->currentState.origin, 256, 0.75 + 8.0 / (rand() & 31), 1.0f, 1.0f, 1.0f, 0, 0);

		// muzzle flash
		memset(&flash, 0, sizeof(flash));
		flash.renderfx = RF_LIGHTING_ORIGIN;
		flash.hModel   = cgs.media.mg42muzzleflash;
		VectorCopy(cent->currentState.origin, flash.origin);
		AnglesToAxis(cg.refdefViewAngles, flash.axis);
		trap_R_AddRefEntityToScene(&flash);
	}
}

/*
==============
CG_WeaponFireRecoil
==============
*/
void CG_WeaponFireRecoil(int weapon)
{
	float  pitchAdd  = 0;
	float  yawRandom = 0;
	vec3_t recoil;

	switch (weapon)
	{
	case WP_LUGER:
	case WP_SILENCER:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
	case WP_COLT:
	case WP_SILENCED_COLT:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
		// FIXME: add recoil for secondary weapons?
		//pitchAdd = 2 + rand() % 3;
		//yawRandom = 2;
		break;
	case WP_PANZERFAUST: // push the player back instead
	case WP_BAZOOKA:
		break;
	case WP_GARAND:
	case WP_KAR98:
	case WP_CARBINE:
	case WP_K43:
		pitchAdd  = 2;
		yawRandom = 1;
		break;
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		pitchAdd = 0.3f;
		break;
	case WP_FG42SCOPE:
	case WP_FG42:
	case WP_MOBILE_MG42:
	case WP_MOBILE_MG42_SET:
	case WP_MOBILE_BROWNING:
	case WP_MOBILE_BROWNING_SET:
	case WP_MP40:
	case WP_THOMPSON:
	case WP_STEN:
		pitchAdd  = (1 + rand() % 3) * 0.3;
		yawRandom = 0.6f;
		break;
	default:
		return;
	}
	// calc the recoil
	recoil[YAW]   = crandom() * yawRandom;
	recoil[ROLL]  = -recoil[YAW];   // why not
	recoil[PITCH] = -pitchAdd;

	// scale it up a bit (easier to modify this while tweaking)
	VectorScale(recoil, 30, recoil);

	// set the recoil
	VectorCopy(recoil, cg.kickAVel);
}

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon(centity_t *cent)
{
	entityState_t *ent = &cent->currentState;
	int           c;
	weaponInfo_t  *weap;
	sfxHandle_t   *firesound;
	sfxHandle_t   *fireEchosound;

	// quick hack for EF_MOUNTEDTANK, need to change this - likely it needs to use viewlocked as well
	if (cent->currentState.eFlags & EF_MOUNTEDTANK)
	{
		if (cg_entities[cg_entities[cg_entities[cent->currentState.number].tagParent].tankparent].currentState.density & 8)       // should we use a browning?
		{
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponSnd_2);
		}
		else
		{
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponSnd);
		}

		if (cg_muzzleFlash.integer)
		{
			cent->muzzleFlashTime = cg.time;
		}
		else
		{
			cent->muzzleFlashTime = 0;
		}

		return;
	}

	// mg42
	if (BG_PlayerMounted(cent->currentState.eFlags))
	{
		if (cent->currentState.eFlags & EF_AAGUN_ACTIVE)
		{
			//trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hflakWeaponSnd );
		}
		else
		{
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponSnd);
		}

		if (cg_brassTime.integer > 0)
		{
			CG_MachineGunEjectBrass(cent);
		}

		if (cg_muzzleFlash.integer)
		{
			cent->muzzleFlashTime = cg.time;
		}
		else
		{
			cent->muzzleFlashTime = 0;
		}

		return;
	}

	if (ent->weapon == WP_NONE)
	{
		return;
	}
	if (ent->weapon >= WP_NUM_WEAPONS)
	{
		CG_Error("CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS\n");
		return;
	}
	weap = &cg_weapons[ent->weapon];

	if (cent->currentState.clientNum == cg.snap->ps.clientNum)
	{
		cg.lastFiredWeapon = ent->weapon;
	}

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	/*
	if(cg_muzzleFlash.integer)
	{
	    cent->muzzleFlashTime = cg.time;
	}
	*/
	if (cg_muzzleFlash.integer)
	{
		cent->muzzleFlashTime = cg.time;
	}
	else
	{
		cent->muzzleFlashTime = 0;
	}

	// kick angles
	if (ent->number == cg.snap->ps.clientNum)
	{
		CG_WeaponFireRecoil(ent->weapon);
	}

	if (IS_MORTAR_WEAPON_SET(ent->weapon))
	{
		if (ent->clientNum == cg.snap->ps.clientNum)
		{
			cg.mortarImpactTime        = -1;
			cg.mortarFireAngles[PITCH] = cg.predictedPlayerState.viewangles[PITCH];
			cg.mortarFireAngles[YAW]   = cg.predictedPlayerState.viewangles[YAW];
		}
	}

	// lightning gun only does this this on initial press
	// FIXME: do a switch
	if (ent->weapon == WP_FLAMETHROWER)
	{
		if (cent->pe.lightningFiring)
		{
			return;
		}
	}
	else if (ent->weapon == WP_GRENADE_LAUNCHER ||
	         ent->weapon == WP_GRENADE_PINEAPPLE ||
	         ent->weapon == WP_DYNAMITE ||
	         ent->weapon == WP_SMOKE_MARKER
	         || ent->weapon == WP_LANDMINE
	         || ent->weapon == WP_SATCHEL
	         || ent->weapon == WP_SMOKE_BOMB
	         )
	{
		if (ent->apos.trBase[0] > 0)     // underhand
		{
			return;
		}
	}

	if (ent->weapon == WP_GPG40)
	{
		if (ent->clientNum == cg.snap->ps.clientNum)
		{
			cg.weaponSelect = WP_KAR98;
		}
	}
	else if (ent->weapon == WP_M7)
	{
		if (ent->clientNum == cg.snap->ps.clientNum)
		{
			cg.weaponSelect = WP_CARBINE;
		}
	}

	if ((cent->currentState.event & ~EV_EVENT_BITS) == EV_FIRE_WEAPON_LASTSHOT)
	{
		firesound     = &weap->lastShotSound[0];
		fireEchosound = &weap->flashEchoSound[0];

		// try to use the lastShotSound, but don't assume it's there.
		// if a weapon without the sound calls it, drop back to regular fire sound

		for (c = 0; c < 4; c++)
		{
			if (!firesound[c])
			{
				break;
			}
		}
		if (!c)
		{
			firesound     = &weap->flashSound[0];
			fireEchosound = &weap->flashEchoSound[0];
		}
	}
	else
	{
		firesound     = &weap->flashSound[0];
		fireEchosound = &weap->flashEchoSound[0];
	}

	if (!(cent->currentState.eFlags & EF_ZOOMING))       // don't play sounds or eject brass if zoomed in
	{   // play a sound
		for (c = 0 ; c < 4 ; c++)
		{
			if (!firesound[c])
			{
				break;
			}
		}
		if (c > 0)
		{
			c = rand() % c;
			if (firesound[c])
			{
				trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, firesound[c]);

				if (fireEchosound && fireEchosound[c])     // check for echo
				{
					centity_t *cent = &cg_entities[ent->number];
					vec3_t    porg, gorg, norm; // player/gun origin
					float     gdist;

					VectorCopy(cent->currentState.pos.trBase, gorg);
					VectorCopy(cg.refdef_current->vieworg, porg);
					VectorSubtract(gorg, porg, norm);
					gdist = VectorNormalize(norm);
					if (gdist > 512 && gdist < 4096)       // temp dist.  TODO: use numbers that are weapon specific
					{   // use gorg as the new sound origin
						VectorMA(cg.refdef_current->vieworg, 64, norm, gorg);     // sound-on-a-stick
						trap_S_StartSoundEx(gorg, ent->number, CHAN_WEAPON, fireEchosound[c], SND_NOCUT);
					}
				}
			}
		}

		// do brass ejection
		if (weap->ejectBrassFunc && cg_brassTime.integer > 0)
		{
			weap->ejectBrassFunc(cent);
		}
	}
}

/*
=================
CG_AddSparks
=================
*/
void CG_AddSparks(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale)
{
	localEntity_t *le;
	refEntity_t   *re;
	vec3_t        velocity;
	int           i;

	for (i = 0; i < count; i++)
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet(velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale);
		VectorScale(velocity, (float)speed, velocity);

		le->leType        = LE_SPARK;
		le->startTime     = cg.time;
		le->endTime       = le->startTime + duration - (int)(0.5 * random() * duration);
		le->lastTrailTime = cg.time;

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy(origin, le->pos.trBase);
		VectorMA(le->pos.trBase, 2 + random() * 4, dir, le->pos.trBase);
		VectorCopy(velocity, le->pos.trDelta);
		le->pos.trTime = cg.time;

		le->refEntity.customShader = cgs.media.sparkParticleShader;

		le->bounceFactor = 0.9;

		//le->leBounceSoundType = LEBS_BLOOD;
		//le->leMarkType = LEMT_BLOOD;
	}
}

/*
=================
CG_AddBulletParticles
=================
*/
void CG_AddBulletParticles(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale)
{
	vec3_t velocity, pos;
	int    i;

	// add the falling particles
	for (i = 0; i < count; i++)
	{
		VectorSet(velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale);
		VectorScale(velocity, (float)speed, velocity);

		VectorCopy(origin, pos);
		VectorMA(pos, 2 + random() * 4, dir, pos);

		CG_ParticleBulletDebris(pos, velocity, 300 + rand() % 300);
	}
}

/*
=================
CG_AddDirtBulletParticles
=================
*/
void CG_AddDirtBulletParticles(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale, float width, float height, float alpha, qhandle_t shader)
{
	vec3_t velocity, pos;
	int    i;

	// add the big falling particle
	VectorSet(velocity, 0, 0, (float)speed);
	VectorCopy(origin, pos);

	CG_ParticleDirtBulletDebris_Core(pos, velocity, duration, width, height, alpha, shader);  //600 + rand()%300 ); // keep central one
	for (i = 0; i < count; i++)
	{
		VectorSet(velocity, dir[0] * crandom() * speed * randScale, dir[1] * crandom() * speed * randScale, dir[2] * random() * speed);
		CG_ParticleDirtBulletDebris_Core(pos, velocity, duration + (rand() % (duration >> 1)), width, height, alpha, shader);
	}
}


void CG_RandomDebris(localEntity_t *le)
{
	int i = rand() % 5;

	if (i == 0)
	{
		le->refEntity.hModel  = cgs.media.debFabric[1];
		le->leBounceSoundType = LEBS_BONE;
	}
	else if (i == 1)
	{
		le->refEntity.hModel  = cgs.media.shardMetal1;
		le->leBounceSoundType = LEBS_METAL;
	}
	else if (i == 2)
	{
		le->refEntity.hModel  = cgs.media.shardMetal2;
		le->leBounceSoundType = LEBS_METAL;
	}
	else if (i == 3)
	{
		le->refEntity.hModel  = cgs.media.debRock[1];
		le->leBounceSoundType = LEBS_ROCK;
	}
	else if (i == 4)
	{
		le->refEntity.hModel  = cgs.media.debRock[0];
		le->leBounceSoundType = LEBS_ROCK;
	}
	else
	{
		le->refEntity.hModel  = cgs.media.debRock[2];
		le->leBounceSoundType = LEBS_ROCK;
	}
}

/*
=================
CG_AddDebris
=================
*/
void CG_AddDebris(vec3_t origin, vec3_t dir, int speed, int duration, int count, trace_t *trace)
{
	localEntity_t *le;
	refEntity_t   *re;
	vec3_t        velocity, unitvel;
	float         timeAdd;
	int           i;

	for (i = 0; i < count; i++)
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet(unitvel, dir[0] + crandom() * 0.9, dir[1] + crandom() * 0.9, fabs(dir[2]) > 0.5 ? dir[2] * (0.2 + 0.8 * random()) : random() * 0.6);
		VectorScale(unitvel, (float)speed + (float)speed * 0.5 * crandom(), velocity);

		le->leType    = LE_DEBRIS;
		le->startTime = cg.time;
		// FIXME: this is such a waste - change to (or even drop the multiplicator *2)
		// le->endTime       = le->startTime + 2 * duration;
		// functions calling CG_AddDebris already creating randomnesses - adjust these and do a clean duration param
		le->endTime       = le->startTime + duration + (int)((float)duration * 0.8 * crandom());
		le->lastTrailTime = cg.time;

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy(origin, le->pos.trBase);
		VectorCopy(velocity, le->pos.trDelta);
		le->pos.trTime = cg.time;

		timeAdd = 10.0 + random() * 40.0;
		BG_EvaluateTrajectory(&le->pos, cg.time + (int)timeAdd, le->pos.trBase, qfalse, -1);

		le->bounceFactor = 0.5;
		le->effectWidth  = 5 + random() * 5;
		le->effectFlags |= 1;       // smoke trail

		//le->leMarkType = LEMT_BLOOD;

		// WIP
		// add model & properties of extended debris elements
		// TODO: find better models and/or extend ...
		// TODO: make dependant from surface (snow etc and use related models/sounds)
		// TODO: find a client cvar so purists can disable (see CG_AddLocalEntities)
		if (trace) // && user enabled
		{
			// airborn or solid with no surface set - just throw projectile fragments
			if (trace->fraction == 1.0 || ((trace->contents & CONTENTS_SOLID) && !trace->surfaceFlags))
			{
				if (rand() % 2)
				{
					le->refEntity.hModel = cgs.media.shardMetal1;  // FIXME: find other models or scale them
				}
				else
				{
					le->refEntity.hModel = cgs.media.shardMetal2;
				}

				le->leBounceSoundType = LEBS_METAL;
				continue;
			}

			if (trace->surfaceFlags & SURF_WOOD)
			{
				le->refEntity.hModel  = cgs.media.debWood[rand() % 6];
				le->leBounceSoundType = LEBS_WOOD;
				continue;
			}

			/*
			CG_Printf("--> c:%i sf:%i\n", trace->contents, trace->surfaceFlags);

			if (trace->surfaceFlags & SURF_GRAVEL)
			{
			    CG_Printf("ON GRAVEL\n");
			}

			if (trace->surfaceFlags & SURF_SNOW)
			{
			    CG_Printf("ON SNOW\n");
			}

			if (trace->surfaceFlags & SURF_GRASS)
			{
			    CG_Printf("ON GRASS\n");
			}

			if (trace->surfaceFlags & SURF_CERAMIC)
			{
			    CG_Printf("ON CERAMIC\n");
			}

			// ---

			if (trace->surfaceFlags & SURF_GLASS)
			{
			    CG_Printf("ON GLASS\n");
			}

			if (trace->surfaceFlags & SURF_METAL)
			{
			    CG_Printf("ON METAL\n");
			}

			if (trace->surfaceFlags & SURF_ROOF)
			{
			    CG_Printf("ON ROOF\n");
			}

			if (trace->surfaceFlags & SURF_CARPET)
			{
			    CG_Printf("ON CARPET\n");
			}

			// --

			if (trace->contents & CONTENTS_WATER)
			{
			    CG_Printf("ON WATER\n");
			}

			if (trace->contents & CONTENTS_BODY)
			{
			    CG_Printf("ON BODY\n");
			}
			if (trace->contents & CONTENTS_CORPSE)
			{
			    CG_Printf("ON CORPSE\n");
			}
			*/

			// FIXME: replace with surface related models
			CG_RandomDebris(le);

		}
		else
		{
			// if there is no trace just throw random debris
			CG_RandomDebris(le);
		}
	}
}

/*
==============
CG_WaterRipple
==============
*/
void CG_WaterRipple(qhandle_t shader, vec3_t loc, vec3_t dir, int size, int lifetime)
{
	localEntity_t *le = CG_AllocLocalEntity();
	refEntity_t   *re;

	le->leType  = LE_SCALE_FADE;
	le->leFlags = LEF_PUFF_DONT_SCALE;

	le->startTime = cg.time;
	le->endTime   = cg.time + lifetime;
	le->lifeRate  = 1.0 / (le->endTime - le->startTime);

	re = &le->refEntity;
	VectorCopy(loc, re->origin);
	re->shaderTime    = cg.time / 1000.0f;
	re->reType        = RT_SPLASH;
	re->radius        = size;
	re->customShader  = shader;
	re->shaderRGBA[0] = 0xff;
	re->shaderRGBA[1] = 0xff;
	re->shaderRGBA[2] = 0xff;
	re->shaderRGBA[3] = 0xff;
	le->color[3]      = 1.0f;
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing

ClientNum is a dummy field used to define what sort of effect to spawn
=================
*/
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, int surfFlags)     // modified to send missilehitwall surface parameters
{
	qhandle_t   mod      = 0, mark = 0, shader = 0;
	sfxHandle_t sfx      = 0, sfx2 = 0;
	qboolean    isSprite = qfalse;
	int         duration = 600, lightOverdraw = 0, i, j, markDuration = -1, volume = 127; // keep -1 markDuration for temporary marks
	trace_t     trace;
	vec3_t      lightColor = { 1, 1, 0 }, tmpv, tmpv2, sprOrg, sprVel;
	float       radius     = 32, light = 0, sfx2range = 0;
	vec4_t      projection;

	if (surfFlags & SURF_SKY)
	{
		return;
	}

	switch (weapon)
	{
	case WP_KNIFE:
	case WP_KNIFE_KABAR:
		i = rand() % 4;
		if (!surfFlags)
		{
			sfx    = cgs.media.sfx_knifehit[4];     // different values for different types (stone/metal/wood/etc.)
			mark   = cgs.media.bulletMarkShader;
			radius = 1 + rand() % 2;

			CG_AddBulletParticles(origin, dir, 20, 800, 3 + rand() % 6, 1.0);
		}
		else
		{
			sfx = cgs.media.sfx_knifehit[i];
		}

		// set mark duration
		markDuration = cg_markTime.integer;
		break;
	case WP_LUGER:
	case WP_SILENCER:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
	case WP_COLT:
	case WP_SILENCED_COLT:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
	case WP_MP40:
	case WP_THOMPSON:
	case WP_STEN:
	case WP_GARAND:
	case WP_FG42:
	case WP_FG42SCOPE:
	case WP_KAR98:
	case WP_CARBINE:
	case WP_MOBILE_MG42:
	case WP_MOBILE_MG42_SET:
	case WP_MOBILE_BROWNING:
	case WP_MOBILE_BROWNING_SET:
	case WP_K43:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		volume = 64;

		// clientNum is a dummy field used to define what sort of effect to spawn
		if (!clientNum)
		{
			// RF, why is this here? we need sparks if clientNum = 0, used for warzombie
			CG_AddSparks(origin, dir, 350, 200, 15 + rand() % 7, 0.2);
		}
		else if (clientNum == 1)       // just do a little smoke puff
		{
			vec3_t d, o;
			VectorMA(origin, 12, dir, o);
			VectorScale(dir, 7, d);
			d[2] += 16;

			//  use dirt images
			if (((surfFlags & SURF_GRASS) || (surfFlags & SURF_GRAVEL) || (surfFlags & SURF_SNOW)))       // added SURF_SNOW
			{   // some debris particles
				// added surf_snow
				if (surfFlags & SURF_SNOW)
				{
					CG_AddDirtBulletParticles(origin, dir, 190, 900, 5, 0.25, 80, 32, 0.5, cgs.media.dirtParticle2Shader);
				}
				else
				{
					CG_AddDirtBulletParticles(origin, dir, 190, 900, 5, 0.5, 80, 16, 0.5, cgs.media.dirtParticle1Shader);
				}
			}
			else
			{
				CG_ParticleImpactSmokePuff(cgs.media.smokeParticleShader, o);

				// some debris particles
				CG_AddBulletParticles(origin, dir, 20, 800, 3 + rand() % 6, 1.0);      // rand scale

				// just do a little one
				if (sfx && (rand() % 3 == 0))
				{
					CG_AddSparks(origin, dir, 450, 300, 3 + rand() % 3, 0.5);     // rand scale
				}
			}
		}
		else if (clientNum == 2)
		{
			sfx  = 0;
			mark = 0;

			// needed to do the CG_WaterRipple using a localent since I needed the timer reset on the shader for each shot
			CG_WaterRipple(cgs.media.wakeMarkShaderAnim, origin, tv(0, 0, 1), 32, 1000);
			CG_AddDirtBulletParticles(origin, dir, 190, 900, 5, 0.5, 80, 16, 0.125, cgs.media.dirtParticle2Shader);
			break;

			// play a water splash
			mod      = cgs.media.waterSplashModel;
			shader   = cgs.media.waterSplashShader;
			duration = 250;
		}

		// optimization, only spawn the bullet hole if we are close
		// enough to see it, this way we can leave other marks around a lot
		// longer, since most of the time we can't actually see the bullet holes
		// - small modification.  only do this for non-rifles (so you can see your shots hitting when you're zooming with a rifle scope)
		if (weapon == WP_FG42SCOPE || weapon == WP_GARAND_SCOPE || weapon == WP_K43_SCOPE || (Distance(cg.refdef_current->vieworg, origin) < 384))
		{
			if (clientNum)
			{
				// mark and sound can potentially use the surface for override values
				mark   = cgs.media.bulletMarkShader;    // default
				radius = 1.0f + 0.5f * (rand() % 2);

				if ((surfFlags & SURF_METAL) || (surfFlags & SURF_ROOF))
				{
					sfx  = cgs.media.sfx_bullet_metalhit[rand() % MAX_IMPACT_SOUNDS];
					mark = cgs.media.bulletMarkShaderMetal;
				}
				else if (surfFlags & SURF_WOOD)
				{
					sfx     = cgs.media.sfx_bullet_woodhit[rand() % MAX_IMPACT_SOUNDS];
					mark    = cgs.media.bulletMarkShaderWood;
					radius += 0.4f;   // experimenting with different mark sizes per surface
				}
				else if (surfFlags & SURF_GLASS)
				{
					sfx  = cgs.media.sfx_bullet_glasshit[rand() % MAX_IMPACT_SOUNDS];
					mark = cgs.media.bulletMarkShaderGlass;
				}
				else
				{
					sfx  = cgs.media.sfx_bullet_stonehit[rand() % MAX_IMPACT_SOUNDS];
					mark = cgs.media.bulletMarkShader;
				}

				// set mark duration
				markDuration = cg_markTime.integer;
			}
		}
		break;
	case WP_MAPMORTAR:
		sfx           = cgs.media.sfx_rockexp;
		sfx2          = cgs.media.sfx_rockexpDist;
		sfx2range     = 1200;
		mark          = cgs.media.burnMarkShader;
		markDuration  = cg_markTime.integer * 3;
		radius        = 96; //  bigger mark radius
		light         = 300;
		isSprite      = qtrue;
		duration      = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		VectorScale(dir, 16, sprVel);
		if (CG_PointContents(origin, 0) & CONTENTS_WATER)
		{
			VectorCopy(origin, tmpv);
			tmpv[2] += 10000;

			trap_CM_BoxTrace(&trace, tmpv, origin, NULL, NULL, 0, MASK_WATER);
			CG_WaterRipple(cgs.media.wakeMarkShaderAnim, trace.endpos, dir, 150, 1000);
			CG_AddDirtBulletParticles(trace.endpos, dir, 900, 1800, 15, 0.5, 350, 128, 0.125, cgs.media.dirtParticle2Shader);
		}
		else
		{
			VectorCopy(origin, tmpv);
			tmpv[2] += 20;
			VectorCopy(origin, tmpv2);
			tmpv2[2] -= 20;
			trap_CM_BoxTrace(&trace, tmpv, tmpv2, NULL, NULL, 0, MASK_SHOT);
			if ((trace.surfaceFlags & SURF_GRASS) || (trace.surfaceFlags & SURF_GRAVEL))
			{
				CG_AddDirtBulletParticles(origin, dir, 600, 2000, 10, 0.5, 275, 125, 0.25, cgs.media.dirtParticle1Shader);
			}

			for (i = 0; i < 5; i++)
			{
				for (j = 0; j < 3; j++)
				{
					sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
				}
				sprVel[2] += rand() % 50;
				CG_ParticleExplosion("blacksmokeanim", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60, qfalse);
			}
			VectorMA(origin, 24, dir, sprOrg);
			VectorScale(dir, 64, sprVel);
			CG_ParticleExplosion("explode1", sprOrg, sprVel, 1000, 20, 300, qtrue);
		}
		break;
	case WP_DYNAMITE:
		shader        = cgs.media.rocketExplosionShader;
		sfx           = cgs.media.sfx_dynamiteexp;
		sfx2          = cgs.media.sfx_dynamiteexpDist;
		sfx2range     = 400;
		mark          = cgs.media.burnMarkShader;
		markDuration  = cg_markTime.integer * 3;
		radius        = 128; // bigger mark radius
		light         = 300;
		isSprite      = qtrue;
		duration      = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		// biggie dynamite explosions that mean it -- dynamite is biggest explode, so it gets extra crap thrown on
		// check for water/dirt spurt
		if (CG_PointContents(origin, 0) & CONTENTS_WATER)
		{
			VectorCopy(origin, tmpv);
			tmpv[2] += 10000;

			trap_CM_BoxTrace(&trace, tmpv, origin, NULL, NULL, 0, MASK_WATER);
			CG_WaterRipple(cgs.media.wakeMarkShaderAnim, trace.endpos, dir, 300, 2000);

			CG_AddDirtBulletParticles(trace.endpos, dir, 400 + random() * 200, 900, 15, 0.5, 512, 128, 0.125, cgs.media.dirtParticle2Shader);
			CG_AddDirtBulletParticles(trace.endpos, dir, 400 + random() * 600, 1400, 15, 0.5, 128, 512, 0.125, cgs.media.dirtParticle2Shader);
		}
		else
		{
			VectorSet(tmpv, origin[0], origin[1], origin[2] + 20);
			VectorSet(tmpv2, origin[0], origin[1], origin[2] - 20);
			trap_CM_BoxTrace(&trace, tmpv, tmpv2, NULL, NULL, 0, MASK_SHOT);
			if ((trace.surfaceFlags & SURF_GRASS) || (trace.surfaceFlags & SURF_GRAVEL))
			{
				CG_AddDirtBulletParticles(origin, dir, 400 + random() * 200, 3000, 10, 0.5, 400, 256, 0.25, cgs.media.dirtParticle1Shader);
			}
			for (i = 0; i < 3; i++)
			{
				for (j = 0; j < 3; j++)
				{
					sprOrg[j] = origin[j] + 150 * crandom();
					sprVel[j] = 0.35 * crandom();
				}
				VectorAdd(sprVel, trace.plane.normal, sprVel);
				VectorScale(sprVel, 130, sprVel);
				CG_ParticleExplosion("blacksmokeanim", sprOrg, sprVel, 6000 + random() * 2000, 40, 400 + random() * 200, qfalse);   // JPW NERVE was blacksmokeanimb
			}
			for (i = 0; i < 4; i++)     // random vector based on plane normal so explosions move away from walls/dirt/etc
			{
				for (j = 0; j < 3; j++)
				{
					sprOrg[j] = origin[j] + 100 * crandom();
					sprVel[j] = 0.65 * crandom(); // wider fireball spread
				}
				VectorAdd(sprVel, trace.plane.normal, sprVel);
				VectorScale(sprVel, random() * 100 + 300, sprVel);
				CG_ParticleExplosion("explode1", sprOrg, sprVel, 1000 + rand() % 1450, 40, 400 + random() * 200, (i == 0 ? qtrue : qfalse));
			}
			CG_AddDebris(origin, dir, 400 + random() * 200, rand() % 2000 + 1400, 12 + rand() % 12, &trace);
		}
		break;
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_GPG40:
	case WP_M7:
	case WP_SATCHEL:
	case WP_LANDMINE:
	case WP_MORTAR_SET:
	case WP_MORTAR2_SET:
		if (weapon == WP_SATCHEL)
		{
			sfx  = cgs.media.sfx_satchelexp;
			sfx2 = cgs.media.sfx_satchelexpDist;
		}
		else if (weapon == WP_LANDMINE)
		{
			sfx  = cgs.media.sfx_landmineexp;
			sfx2 = cgs.media.sfx_landmineexpDist;
		}
		else if (weapon == WP_MORTAR_SET || weapon == WP_MORTAR2_SET)
		{
			sfx = sfx2 = 0;
		}
		else if (weapon == WP_GRENADE_LAUNCHER || weapon == WP_GRENADE_PINEAPPLE || weapon == WP_GPG40 || weapon == WP_M7)
		{
			sfx  = cgs.media.sfx_grenexp;
			sfx2 = cgs.media.sfx_grenexpDist;
		}
		else
		{
			sfx  = cgs.media.sfx_rockexp;
			sfx2 = cgs.media.sfx_rockexpDist;
		}
		shader        = cgs.media.rocketExplosionShader;    // copied from RL
		sfx2range     = 400;
		mark          = cgs.media.burnMarkShader;
		markDuration  = cg_markTime.integer * 3;
		radius        = 64;
		light         = 300;
		isSprite      = qtrue;
		duration      = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		// explosion sprite animation
		VectorMA(origin, 16, dir, sprOrg);
		VectorScale(dir, 100, sprVel);

		if (CG_PointContents(origin, 0) & CONTENTS_WATER)
		{
			sfx = cgs.media.sfx_rockexpWater;

			VectorCopy(origin, tmpv);
			tmpv[2] += 10000;

			trap_CM_BoxTrace(&trace, tmpv, origin, NULL, NULL, 0, MASK_WATER);
			CG_WaterRipple(cgs.media.wakeMarkShaderAnim, trace.endpos, dir, 150, 1000);

			CG_AddDirtBulletParticles(trace.endpos, dir, 400, 900, 15, 0.5, 256, 128, 0.125, cgs.media.dirtParticle2Shader);
		}
		else
		{
			VectorCopy(origin, tmpv);
			tmpv[2] += 20;
			VectorCopy(origin, tmpv2);
			tmpv2[2] -= 20;
			trap_CM_BoxTrace(&trace, tmpv, tmpv2, NULL, NULL, 0, MASK_SHOT);
			if ((trace.surfaceFlags & SURF_GRASS) || (trace.surfaceFlags & SURF_GRAVEL))
			{
				CG_AddDirtBulletParticles(origin, dir, 400, 2000, 10, 0.5, 200, 75, 0.25, cgs.media.dirtParticle1Shader);
			}
			CG_ParticleExplosion("explode1", sprOrg, sprVel, 700, 60, 240, qtrue);
			CG_AddDebris(origin, dir, 280, 1400, 7 + rand() % 2, &trace);
		}
		break;
	case WP_PANZERFAUST:
	case WP_BAZOOKA:
	case VERYBIGEXPLOSION:
	case WP_ARTY:
	case WP_SMOKE_MARKER:
		sfx  = cgs.media.sfx_rockexp;
		sfx2 = cgs.media.sfx_rockexpDist;
		if (weapon == VERYBIGEXPLOSION || weapon == WP_ARTY)
		{
			sfx  = cgs.media.sfx_artilleryExp[rand() % 3];
			sfx2 = cgs.media.sfx_artilleryDist;
		}
		else if (weapon == WP_SMOKE_MARKER)
		{
			sfx  = cgs.media.sfx_airstrikeExp[rand() % 3];
			sfx2 = cgs.media.sfx_airstrikeDist;
		}
		sfx2range    = 800;
		mark         = cgs.media.burnMarkShader;
		markDuration = cg_markTime.integer * 3;
		radius       = 128; // bigger mark radius
		light        = 600;
		isSprite     = qtrue;
		duration     = 1000;
		// changed to flamethrower colors
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		// explosion sprite animation
		VectorMA(origin, 24, dir, sprOrg);
		VectorScale(dir, 64, sprVel);

		if (CG_PointContents(origin, 0) & CONTENTS_WATER)
		{
			VectorCopy(origin, tmpv);
			tmpv[2] += 10000;

			trap_CM_BoxTrace(&trace, tmpv, origin, NULL, NULL, 0, MASK_WATER);
			CG_WaterRipple(cgs.media.wakeMarkShaderAnim, trace.endpos, dir, 300, 2000);

			CG_AddDirtBulletParticles(trace.endpos, dir, 400 + random() * 200, 900, 15, 0.5, 512, 128, 0.125, cgs.media.dirtParticle2Shader);
			CG_AddDirtBulletParticles(trace.endpos, dir, 400 + random() * 600, 1400, 15, 0.5, 128, 512, 0.125, cgs.media.dirtParticle2Shader);
		}
		else
		{
			VectorCopy(origin, tmpv);
			tmpv[2] += 20;
			VectorCopy(origin, tmpv2);
			tmpv2[2] -= 20;
			trap_CM_BoxTrace(&trace, tmpv, tmpv2, NULL, NULL, 0, MASK_SHOT);

			if ((trace.surfaceFlags & SURF_GRASS) || (trace.surfaceFlags & SURF_GRAVEL))
			{
				CG_AddDirtBulletParticles(origin, dir, 400 + random() * 200, 3000, 10, 0.5, 400, 256, 0.25, cgs.media.dirtParticle1Shader);
			}
			CG_ParticleExplosion("explode1", sprOrg, sprVel, 1600, 20, 200 + random() * 400, qtrue);

			for (i = 0; i < 4; i++)     // random vector based on plane normal so explosions move away from walls/dirt/etc
			{
				for (j = 0; j < 3; j++)
				{
					sprOrg[j] = origin[j] + 50 * crandom();
					sprVel[j] = 0.35 * crandom();
				}
				VectorAdd(sprVel, trace.plane.normal, sprVel);
				VectorScale(sprVel, 300, sprVel);
				CG_ParticleExplosion("explode1", sprOrg, sprVel, 1600, 40, 260 + rand() % 120, qfalse);
			}
			CG_AddDebris(origin, dir, 400 + random() * 200, rand() % 2000 + 1000, 5 + rand() % 5, &trace);
		}
		break;
	default:
	case WP_FLAMETHROWER:
		return;
		break;
	}

	if (sfx)
	{
		trap_S_StartSoundVControl(origin, -1, CHAN_AUTO, sfx, volume);
	}

	if (sfx2)      // distant sounds for weapons with a broadcast fire sound (so you /always/ hear dynamite explosions)
	{
		vec3_t porg, gorg, norm;    // player/gun origin
		float  gdist;

		VectorCopy(origin, gorg);
		VectorCopy(cg.refdef_current->vieworg, porg);
		VectorSubtract(gorg, porg, norm);
		gdist = VectorNormalize(norm);
		if (gdist > 1200 && gdist < 8000)      // 1200 is max cam shakey dist (2*600) use gorg as the new sound origin
		{
			VectorMA(cg.refdef_current->vieworg, sfx2range, norm, gorg);   // non-distance falloff makes more sense; sfx2range was gdist*0.2
			// sfx2range is variable to give us minimum volume control different explosion sizes (see mortar, panzerfaust, and grenade)
			trap_S_StartSoundEx(gorg, -1, CHAN_WEAPON, sfx2, SND_NOCUT);
		}
	}

	if (mod)
	{
		localEntity_t *le;

		le = CG_MakeExplosion(origin, dir, mod, shader, duration, isSprite);

		le->light         = light;
		le->lightOverdraw = lightOverdraw;
		VectorCopy(lightColor, le->lightColor);
	}

	if (markDuration) // markDuration = cg_markTime.integer * x
	{
		// omnidirectional explosion marks
		if (mark == cgs.media.burnMarkShader)
		{
			VectorSet(projection, 0, 0, -1);
			projection[3] = radius;

			trap_R_ProjectDecal(mark, 1, (vec3_t *) origin, projection, colorWhite, markDuration, (markDuration >> 4));
		}
		else if (mark)
		{
			vec3_t markOrigin;

			VectorSubtract(vec3_origin, dir, projection);
			projection[3] = radius * 32;
			VectorMA(origin, -16.0f, projection, markOrigin);
			// jitter markorigin a bit so they don't end up on an ordered grid
			markOrigin[0] += (random() - 0.5f);
			markOrigin[1] += (random() - 0.5f);
			markOrigin[2] += (random() - 0.5f);
			CG_ImpactMark(mark, markOrigin, projection, radius, random() * 360.0f, 1.0f, 1.0f, 1.0f, 1.0f, markDuration);
		}
	}
}

/*
==============
CG_MissileHitWallSmall
==============
*/
void CG_MissileHitWallSmall(int weapon, int clientNum, vec3_t origin, vec3_t dir)
{
	vec3_t        sprOrg, sprVel;
	static vec4_t projection = { 0, 0, -1.0f, 80.0f };     // {x,y,x,radius}

	// explosion sprite animation
	VectorMA(origin, 16, dir, sprOrg);
	VectorScale(dir, 64, sprVel);

	CG_ParticleExplosion("explode1", sprOrg, sprVel, 600, 6, 50, qtrue);

	// throw some debris
	CG_AddDebris(origin, dir, 280, 1400, 7 + rand() % 2, NULL);

	if (cgs.media.sfx_rockexp)
	{
		trap_S_StartSound(origin, -1, CHAN_AUTO, cgs.media.sfx_rockexp);
	}

	// impact mark
	if (cg_markTime.integer)
	{
		trap_R_ProjectDecal(cgs.media.burnMarkShader, 1, (vec3_t *) origin, projection, colorWhite, cg_markTime.integer, (cg_markTime.integer >> 4));
	}
}

/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer(centity_t *cent, int weapon, vec3_t origin, vec3_t dir, int entityNum)
{
	CG_Bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch (weapon)
	{
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_PANZERFAUST:
	case WP_BAZOOKA:
		CG_MissileHitWall(weapon, 0, origin, dir, 0);   // like the old one
		break;
	case WP_KNIFE:
	case WP_KNIFE_KABAR:
		CG_MissileHitWall(weapon, 0, origin, dir, 1);    // this one makes the hitting fleshy sound. whee
		break;
	default:
		break;
	}
}

/*
============================================================================
BULLETS
============================================================================
*/

/*
===============
CG_SpawnTracer
===============
*/
void CG_SpawnTracer(int sourceEnt, vec3_t pstart, vec3_t pend)
{
	localEntity_t *le;
	float         dist;
	vec3_t        dir;
	vec3_t        start, end;

	VectorCopy(pstart, start);
	VectorCopy(pend, end);

	// make MG42 tracers line up
	if (cg_entities[sourceEnt].currentState.eFlags & EF_MG42_ACTIVE)
	{
		start[2] -= 42;
	}

	VectorSubtract(end, start, dir);
	dist = VectorNormalize(dir);

	if (dist < 2.0 * cg_tracerLength.value)
	{
		return; // segment isnt long enough, dont bother

	}
	if (sourceEnt < cgs.maxclients)
	{
		// for visual purposes, find the actual tag_weapon for this client
		// and offset the start and end accordingly
		if (!((cg_entities[sourceEnt].currentState.eFlags & EF_MG42_ACTIVE) || (cg_entities[sourceEnt].currentState.eFlags & EF_AAGUN_ACTIVE)))          // not MG42
		{
			orientation_t orientation;

			if (CG_GetWeaponTag(sourceEnt, "tag_flash", &orientation))
			{
				vec3_t ofs;

				VectorSubtract(orientation.origin, start, ofs);
				if (VectorLength(ofs) < 64)
				{
					VectorAdd(start, ofs, start);
				}
			}
		}
	}

	// subtract the length of the tracer from the end point, so we dont go through the end point
	VectorMA(end, -cg_tracerLength.value, dir, end);
	dist = VectorDistance(start, end);

	le            = CG_AllocLocalEntity();
	le->leType    = LE_MOVING_TRACER;
	le->startTime = cg.time - (cg.frametime ? (rand() % cg.frametime) / 2 : 0);
	le->endTime   = le->startTime + 1000.0 * dist / cg_tracerSpeed.value;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = le->startTime;
	VectorCopy(start, le->pos.trBase);
	VectorScale(dir, cg_tracerSpeed.value, le->pos.trDelta);
}

/*
===============
CG_DrawTracer
===============
*/
void CG_DrawTracer(vec3_t start, vec3_t finish)
{
	vec3_t     forward, right;
	polyVert_t verts[4];
	vec3_t     line;

	VectorSubtract(finish, start, forward);

	line[0] = DotProduct(forward, cg.refdef_current->viewaxis[1]);
	line[1] = DotProduct(forward, cg.refdef_current->viewaxis[2]);

	VectorScale(cg.refdef_current->viewaxis[1], line[1], right);
	VectorMA(right, -line[0], cg.refdef_current->viewaxis[2], right);
	VectorNormalize(right);

	VectorMA(finish, cg_tracerWidth.value, right, verts[0].xyz);
	verts[0].st[0]       = 1;
	verts[0].st[1]       = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA(finish, -cg_tracerWidth.value, right, verts[1].xyz);
	verts[1].st[0]       = 1;
	verts[1].st[1]       = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA(start, -cg_tracerWidth.value, right, verts[2].xyz);
	verts[2].st[0]       = 0;
	verts[2].st[1]       = 0;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA(start, cg_tracerWidth.value, right, verts[3].xyz);
	verts[3].st[0]       = 0;
	verts[3].st[1]       = 1;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);
}

/*
===============
CG_Tracer
===============
*/
void CG_Tracer(vec3_t source, vec3_t dest, int sparks)
{
	float  len, begin, end;
	vec3_t start, finish;
	vec3_t forward;

	// tracer
	VectorSubtract(dest, source, forward);
	len = VectorNormalize(forward);

	// start at least a little ways from the muzzle
	if (len < 100 && !sparks)
	{
		return;
	}
	begin = 50 + random() * (len - 60);
	end   = begin + cg_tracerLength.value;
	if (end > len)
	{
		end = len;
	}
	VectorMA(source, begin, forward, start);
	VectorMA(source, end, forward, finish);

	CG_DrawTracer(start, finish);
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle)
{
	vec3_t    forward, right, up;
	centity_t *cent;

	if (entityNum == cg.snap->ps.clientNum)
	{
		// see if we're attached to a gun
		if (cg.snap->ps.eFlags & EF_MG42_ACTIVE)
		{
			centity_t *mg42 = &cg_entities[cg.snap->ps.viewlocked_entNum];
			vec3_t    forward;

			AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
			VectorMA(mg42->currentState.pos.trBase, 40, forward, muzzle);       // was -36, made 40 to be in sync with the actual muzzleflash drawing
			muzzle[2] += cg.snap->ps.viewheight;
		}
		else if (cg.snap->ps.eFlags & EF_AAGUN_ACTIVE)
		{
			centity_t *aagun = &cg_entities[cg.snap->ps.viewlocked_entNum];
			vec3_t    forward, right, up;

			AngleVectors(cg.snap->ps.viewangles, forward, right, up);
			VectorCopy(aagun->lerpOrigin, muzzle);                      // modelindex2 will already have been incremented on the server, so work out what it WAS then
			BG_AdjustAAGunMuzzleForBarrel(muzzle, forward, right, up, (aagun->currentState.modelindex2 + 3) % 4);
		}
		else if (cg.snap->ps.eFlags & EF_MOUNTEDTANK)
		{
			if (cg.renderingThirdPerson)
			{
				centity_t *tank = &cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent];

				VectorCopy(tank->mountedMG42Flash.origin, muzzle);
				AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
				VectorMA(muzzle, 14, forward, muzzle);
			}
			else
			{
				// fix firstperson tank muzzle origin if drawgun is off
				if (!cg_drawGun.integer)
				{
					VectorCopy(cg.snap->ps.origin, muzzle);
					AngleVectors(cg.snap->ps.viewangles, forward, right, up);
					VectorMA(muzzle, 48, forward, muzzle);
					muzzle[2] += cg.snap->ps.viewheight;
					VectorMA(muzzle, 8, right, muzzle);
				}
				else
				{
					VectorCopy(cg.tankflashorg, muzzle);
				}
			}
		}
		else
		{
			VectorCopy(cg.snap->ps.origin, muzzle);
			muzzle[2] += cg.snap->ps.viewheight;
			AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
			if (IS_MG_WEAPON_SET(cg.snap->ps.weapon))
			{
				VectorMA(muzzle, 36, forward, muzzle);
			}
			else
			{
				VectorMA(muzzle, 14, forward, muzzle);
			}

			AddLean(cg.snap->ps.viewangles, muzzle, cg.snap->ps.leanf);
		}
		return qtrue;
	}

	cent = &cg_entities[entityNum];

	if (cent->currentState.eFlags & EF_MG42_ACTIVE)
	{
		if (cent->currentState.eType == ET_MG42_BARREL)
		{
			vec3_t forward;

			VectorCopy(cent->currentState.pos.trBase, muzzle);
			AngleVectors(cent->lerpAngles, forward, NULL, NULL);
			VectorMA(muzzle, 40, forward, muzzle);
			muzzle[2] += DEFAULT_VIEWHEIGHT;
		}

	}
	else if (cent->currentState.eFlags & EF_MOUNTEDTANK)
	{
		centity_t *tank = &cg_entities[cent->tagParent];

		VectorCopy(tank->mountedMG42Flash.origin, muzzle);
	}
	else if (cent->currentState.eFlags & EF_AAGUN_ACTIVE)
	{
		centity_t *aagun;
		int       num;

		// find the mg42 we're attached to
		for (num = 0; num < cg.snap->numEntities; num++)
		{
			aagun = &cg_entities[cg.snap->entities[num].number];
			if (aagun->currentState.eType == ET_AAGUN && aagun->currentState.otherEntityNum == cent->currentState.number)
			{
				// found it
				vec3_t forward, right, up;

				AngleVectors(cg.snap->ps.viewangles, forward, right, up);
				VectorCopy(aagun->lerpOrigin, muzzle);                      // modelindex2 will already have been incremented on the server, so work out what it WAS then
				BG_AdjustAAGunMuzzleForBarrel(muzzle, forward, right, up, (aagun->currentState.modelindex2 + 3) % 4);
			}
		}
	}
	else
	{
		VectorCopy(cent->currentState.pos.trBase, muzzle);

		AngleVectors(cent->currentState.apos.trBase, forward, right, up);
		if (cent->currentState.eFlags & EF_PRONE)
		{
			muzzle[2] += PRONE_VIEWHEIGHT;
			if (IS_MG_WEAPON_SET(cent->currentState.weapon))
			{
				VectorMA(muzzle, 36, forward, muzzle);
			}
			else
			{
				VectorMA(muzzle, 14, forward, muzzle);
			}
		}
		else
		{
			muzzle[2] += DEFAULT_VIEWHEIGHT;
			VectorMA(muzzle, 14, forward, muzzle);
			AddLean(cent->lerpAngles, muzzle, cent->pe.leanDirection);
		}
	}

	return qtrue;
}

void SnapVectorTowards(vec3_t v, vec3_t to)
{
	int i;

	for (i = 0 ; i < 3 ; i++)
	{
		if (to[i] <= v[i])
		{
			v[i] = floor(v[i]);
		}
		else
		{
			v[i] = ceil(v[i]);
		}
	}
}

/*
======================
CG_DrawBulletTracer

Renders bullet tracers if  tracer option is valid.
======================
*/
void CG_DrawBulletTracer(vec3_t pstart, vec3_t pend, int sourceEntityNum, int otherEntityNum)
{
	if (cg_tracers.integer == 2 && sourceEntityNum != cg.clientNum)
	{
		return; // Only own tracers
	}
	else if (cg_tracers.integer == 3 && sourceEntityNum == cg.clientNum)
	{
		return; // Only others tracers

	}

	if (otherEntityNum >= 0 && otherEntityNum != ENTITYNUM_NONE)
	{
		if (cg_tracers.integer == 1 || cg_tracers.integer == 3)
		{
			CG_SpawnTracer(otherEntityNum, pstart, pend);
		}
	}
	else
	{
		if (cg_tracers.integer <= 2)
		{
			CG_SpawnTracer(sourceEntityNum, pstart, pend);
		}
	}
}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet(vec3_t end, int sourceEntityNum, qboolean flesh, int fleshEntityNum, int otherEntNum2, float waterfraction, int seed)
{
	trace_t    trace, trace2;
	vec3_t     dir;
	vec3_t     start;
	static int lastBloodSpat;

	// don't ever shoot if we're binoced in
	if (cg_entities[sourceEntityNum].currentState.eFlags & EF_ZOOMING)
	{
		return;
	}

	// snap tracers for MG42 to viewangle of client when antilag is enabled
	if (cgs.antilag && otherEntNum2 == cg.snap->ps.clientNum && (cg_entities[otherEntNum2].currentState.eFlags & EF_MG42_ACTIVE))
	{
		vec3_t  muzzle, forward, right, up;
		float   r, u;
		trace_t tr;

		AngleVectors(cg.predictedPlayerState.viewangles, forward, right, up);
		VectorCopy(cg_entities[cg.snap->ps.viewlocked_entNum].currentState.pos.trBase, muzzle);
		if (cg_entities[cg.snap->ps.viewlocked_entNum].currentState.onFireStart)
		{
			VectorMA(muzzle, 16, up, muzzle);
		}

		r = Q_crandom(&seed) * MG42_SPREAD_MP;
		u = Q_crandom(&seed) * MG42_SPREAD_MP;

		VectorMA(muzzle, 8192, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		cg.bulletTrace = qtrue;
		CG_Trace(&tr, muzzle, NULL, NULL, end, otherEntNum2, MASK_SHOT);
		cg.bulletTrace = qfalse;

		SnapVectorTowards(tr.endpos, muzzle);
		VectorCopy(tr.endpos, end);
	}

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if (sourceEntityNum >= 0 && cg_tracerChance.value > 0)
	{
		if (CG_CalcMuzzlePoint(sourceEntityNum, start))
		{
			int sourceContentType = CG_PointContents(start, 0);
			int destContentType   = CG_PointContents(end, 0);

			// do a complete bubble trail if necessary
			if ((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER))
			{
				CG_BubbleTrail(start, end, .5, 8);
			}
			else if ((sourceContentType & CONTENTS_WATER))         // bubble trail from water into air
			{
				trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(start, trace.endpos, .5, 8);
			}
			else if ((destContentType & CONTENTS_WATER))         // bubble trail from air into water
			{   // only add bubbles if effect is close to viewer
				if (Distance(cg.snap->ps.origin, end) < 1024)
				{
					trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
					CG_BubbleTrail(end, trace.endpos, .5, 8);
				}
			}

			if (cg_tracers.integer)
			{
				// if not flesh, then do a moving tracer
				if (flesh)
				{
					// draw a tracer
					if (random() < cg_tracerChance.value)
					{
						CG_Tracer(start, end, 0);
					}
				}
				else        // (not flesh)
				{
					CG_DrawBulletTracer(start, end, sourceEntityNum, otherEntNum2);
				}
			}
		}
	}

	// impact splash and mark
	if (flesh)
	{
		vec3_t    origin;
		float     rnd, tmpf;
		vec3_t    smokedir, tmpv, tmpv2;
		int       i, headshot;
		centity_t *cent = &cg_entities[fleshEntityNum];

		if (fleshEntityNum < MAX_CLIENTS)
		{
			CG_Bleed(end, fleshEntityNum);
		}

		// smoke puffs (sometimes with some blood)
		VectorSubtract(end, start, smokedir); // get a nice "through the body" vector
		VectorNormalize(smokedir);
		// all this to come up with a decent center-body displacement of bullet impact point
		VectorSubtract(cent->currentState.pos.trBase, end, tmpv);
		tmpv[2] = 0;
		tmpf    = VectorLength(tmpv);
		VectorScale(smokedir, tmpf, tmpv);
		VectorAdd(end, tmpv, origin);
		// whee, got a bullet impact point projected to center body
		CG_GetOriginForTag(cent, &cent->pe.headRefEnt, "tag_mouth", 0, tmpv, NULL);
		tmpv[2] += 5;
		VectorSubtract(tmpv, origin, tmpv2);
		headshot = (VectorLength(tmpv2) < 10);

		if (headshot && cg_blood.integer)
		{
			for (i = 0; i < 5; i++)
			{
				rnd = random();
				VectorScale(smokedir, 25.0 + random() * 25, tmpv);
				tmpv[0] += crandom() * 25.0f;
				tmpv[1] += crandom() * 25.0f;
				tmpv[2] += crandom() * 25.0f;
				CG_GetWindVector(tmpv2);
				VectorScale(tmpv2, 35, tmpv2); // was 75, before that 55
				tmpv2[2] = 0;
				VectorAdd(tmpv, tmpv2, tmpv);
				CG_SmokePuff(origin, tmpv, 5 + rnd * 10, 1, rnd * 0.8, rnd * 0.8, 0.5, 500 + (rand() % 800), cg.time, 0, 0, cgs.media.fleshSmokePuffShader);
			}
		}
		else
		{
			// puff out the front (more dust no blood)
			for (i = 0; i < 10; i++)
			{
				rnd = random();
				VectorScale(smokedir, -35.0 + random() * 25, tmpv);
				tmpv[0] += crandom() * 25.0f;
				tmpv[1] += crandom() * 25.0f;
				tmpv[2] += crandom() * 25.0f;
				CG_GetWindVector(tmpv2);
				VectorScale(tmpv2, 35, tmpv2); // was 75, before that 55
				tmpv2[2] = 0;
				VectorAdd(tmpv, tmpv2, tmpv);
				CG_SmokePuff(origin, tmpv, 5 + rnd * 10, rnd * 0.3f + 0.5f, rnd * 0.3f + 0.5f, rnd * 0.3f + 0.5f, 0.125f, 500 + (rand() % 300), cg.time, 0, 0, cgs.media.smokePuffShader);
			}
		}

		// play the bullet hit flesh sound
		if (fleshEntityNum == cg.snap->ps.clientNum)
		{
			//trap_S_StartSound(NULL, fleshEntityNum, CHAN_BODY, cgs.media.sfx_bullet_fleshhit[rand() % MAX_IMPACT_SOUNDS]);
			trap_S_StartSound(NULL, fleshEntityNum, CHAN_BODY, cgs.media.sfx_bullet_stonehit[rand() % MAX_IMPACT_SOUNDS]);
		}
		else
		{
			trap_S_StartSound(cg_entities[fleshEntityNum].currentState.origin, ENTITYNUM_WORLD, CHAN_BODY, cgs.media.sfx_bullet_stonehit[rand() % MAX_IMPACT_SOUNDS]);
		}

		// if we haven't dropped a blood spat in a while, check if this is a good scenario
		if (cg_blood.integer && cg_bloodTime.integer && (lastBloodSpat > cg.time || lastBloodSpat < cg.time - 500))
		{
			vec3_t trend;

			if (CG_CalcMuzzlePoint(sourceEntityNum, start))
			{
				vec4_t projection;

				VectorSubtract(end, start, dir);
				VectorNormalize(dir);
				VectorMA(end, 128, dir, trend);
				trap_CM_BoxTrace(&trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY);

				if (trace.fraction < 1)
				{
					//CG_ImpactMark( cgs.media.bloodDotShaders[rand()%5], trace.endpos, trace.plane.normal, random()*360,
					//  1,1,1,1, qtrue, 15+random()*20, qfalse, cg_bloodTime.integer * 1000 );
#if 0
					VectorSubtract(vec3_origin, dir, projection);
					projection[3] = 64;
					VectorMA(trace.endpos, -8.0f, projection, markOrigin);
					CG_ImpactMark(cgs.media.bloodDotShaders[rand() % 5], markOrigin, projection, 15.0f + random() * 20.0f, 360.0f * random(),
					              1.0f, 1.0f, 1.0f, 1.0f, cg_bloodTime.integer * 1000);
#else
					VectorSet(projection, 0, 0, -1);
					projection[3] = 15.0f + random() * 20.0f;

					trap_R_ProjectDecal(cgs.media.bloodDotShaders[rand() % 5], 1, (vec3_t *) origin, projection, colorWhite,
					                    cg_bloodTime.integer * 1000, (cg_bloodTime.integer * 1000) >> 4);
#endif
					lastBloodSpat = cg.time;
				}
				else if (lastBloodSpat < cg.time - 1000)
				{
					// drop one on the ground?
					VectorCopy(end, trend);
					trend[2] -= 64;
					trap_CM_BoxTrace(&trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY);

					if (trace.fraction < 1)
					{
						//CG_ImpactMark( cgs.media.bloodDotShaders[rand()%5], trace.endpos, trace.plane.normal, random()*360,
						//  1,1,1,1, qtrue, 15+random()*10, qfalse, cg_bloodTime.integer * 1000 );
#if 0
						VectorSubtract(vec3_origin, dir, projection);
						projection[3] = 64;
						VectorMA(trace.endpos, -8.0f, projection, markOrigin);
						CG_ImpactMark(cgs.media.bloodDotShaders[rand() % 5], markOrigin, projection, 15.0f + random() * 10.0f, 360.0f * random(),
						              1.0f, 1.0f, 1.0f, 1.0f, cg_bloodTime.integer * 1000);
#else
						VectorSet(projection, 0, 0, -1);
						projection[3] = 15.0f + random() * 20.0f;

						trap_R_ProjectDecal(cgs.media.bloodDotShaders[rand() % 5], 1, (vec3_t *) origin, projection, colorWhite,
						                    cg_bloodTime.integer * 1000, (cg_bloodTime.integer * 1000) >> 4);
#endif
						lastBloodSpat = cg.time;
					}
				}
			}
		}

	}
	else        // (not flesh)
	{   // all bullet weapons have the same fx, and this stops pvs issues causing grenade explosions
		int fromweap = WP_MP40; // cg_entities[sourceEntityNum].currentState.weapon;

		if (!fromweap ||
		    (cg_entities[sourceEntityNum].currentState.eFlags & EF_MG42_ACTIVE) ||
		    (cg_entities[sourceEntityNum].currentState.eFlags & EF_MOUNTEDTANK))        // mounted
		{
			fromweap = WP_MP40;
		}

		if (CG_CalcMuzzlePoint(sourceEntityNum, start) || cg.snap->ps.persistant[PERS_HWEAPON_USE])
		{
			if (waterfraction)
			{
				vec3_t dist;
				vec3_t end2;
				vec3_t dir = { 0, 0, 1 };

				VectorSubtract(end, start, dist);
				VectorMA(start, waterfraction, dist, end2);

				trap_S_StartSound(end, -1, CHAN_AUTO, cgs.media.sfx_bullet_waterhit[rand() % 5]);

				CG_MissileHitWall(fromweap, 2, end2, dir, 0);
				CG_MissileHitWall(fromweap, 1, end, trace.plane.normal, 0);
			}
			else
			{
				VectorSubtract(end, start, dir);
				VectorNormalizeFast(dir);
				VectorMA(end, 4, dir, end);

				//CG_RailTrail2( NULL, start, end );

				cg.bulletTrace = qtrue;
				CG_Trace(&trace, start, NULL, NULL, end, 0, MASK_SHOT);
				cg.bulletTrace = qfalse;

				// water check
				cg.bulletTrace = qtrue;
				CG_Trace(&trace2, start, NULL, NULL, end, 0, MASK_WATER | MASK_SHOT);
				cg.bulletTrace = qfalse;

				if (trace.fraction != trace2.fraction)
				{
					//trap_CM_BoxTrace( &trace2, start, end, NULL, NULL, -1, MASK_WATER );

					trap_S_StartSound(end, -1, CHAN_AUTO, cgs.media.sfx_bullet_waterhit[rand() % 5]);

					cg.bulletTrace = qtrue;
					CG_Trace(&trace2, start, NULL, NULL, end, -1, MASK_WATER);
					cg.bulletTrace = qfalse;

					CG_MissileHitWall(fromweap, 2, trace2.endpos, trace2.plane.normal, trace2.surfaceFlags);
					return;
				}

				// better bullet marks
				VectorSubtract(vec3_origin, dir, dir);
				CG_MissileHitWall(fromweap, 1, trace.endpos, dir, trace.surfaceFlags);

				//CG_RailTrail2( NULL, start, trace.endpos );
			}
		}
	}
}
