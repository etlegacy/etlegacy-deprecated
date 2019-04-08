--[[
    ET: Legacy
    Copyright (C) 2012-2019 ET:Legacy team <mail@etlegacy.com>

    This file is part of ET: Legacy - http://www.etlegacy.com

    ET: Legacy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ET: Legacy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
]]--

-- local constants

local G_MISC_MEDIC_SYRINGE_HEAL = 2
local CH_REVIVE_DIST = 64
local M_TAU_F = 6.28318530717958647693
local PITCH = 1 -- up / down
local YAW = 2 -- left / right
local ROLL = 3 -- fall over
local CONTENTS_SOLID = 0x00000001
local CONTENTS_BODY = 0x02000000
local CONTENTS_CORPSE = 0x04000000
local MASK_SHOT = (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE)
local STAT_MAX_HEALTH = 4
local GAMESOUND_MISC_REVIVE = 8

local medicSyringeHealEnabled = false

-- vector math

function toAngleVectors(angles)
	local forward = {} 
	local right = {}
	local up = {}	
	local angle = angles[YAW] * (M_TAU_F / 360)
	local sy = math.sin(angle)
	local cy = math.cos(angle)

	angle = angles[PITCH] * (M_TAU_F / 360)
	local sp = math.sin(angle)
	local cp = math.cos(angle)

	angle = angles[ROLL] * (M_TAU_F / 360)
	local sr = math.sin(angle)
	local cr = math.cos(angle)

	forward[1] = cp * cy
	forward[2] = cp * sy
	forward[3] = -sp

	right[1] = (-1 * sr * sp * cy + -1 * cr * -sy)
	right[2] = (-1 * sr * sp * sy + -1 * cr * cy)
	right[3] = -1 * sr * cp

	up[1] = (cr * sp * cy + -sr * -sy)
	up[2] = (cr * sp * sy + -sr * cy)
	up[3] = cr * cp

	return forward, right, up
end

function snapVector(v)
	return { math.tointeger(v[1]), math.tointeger(v[2]), math.tointeger(v[3]) }
end

function multAddVector(v, s, b)
	return { v[1] + b[1] * s, v[2] + b[2] * s, v[3] + b[3] * s }
end


function copyVector(v)
	return { v[1], v[2], v[3] }
end

-- helpers

function addLean(right, leanValue, point)
	if leanValue ~= 0.0 then
		return multAddVector(point, leanValue, right)
	end
	return copyVector(point)
end

function calcMuzzlePoint(base, viewHeight, leanValue, right)
	local muzzlePoint = copyVector(base)
	muzzlePoint[3] = muzzlePoint[3] + viewHeight
	muzzlePoint = snapVector(addLean(right, leanValue, muzzlePoint))
	return muzzlePoint
end

-- syringe handling

function revokeSyringe(entNum)
	-- todo: implement et.GetPlayerWeapon(entNum, WP_MEDIC_SYRINGE) ?
	local weapon, ammo, ammoclip = et.GetCurrentWeapon(entNum)
	et.AddWeaponToPlayer(entNum, weapon, ammo, ammoclip + 1, 0)
end
 
function checkMedicSyringeHeal(entNum)
	local angles = et.gentity_get(entNum, "ps.viewangles")
	local viewHeight = et.gentity_get(entNum, "ps.viewheight")
	local leanValue = et.gentity_get(entNum, "ps.leanf")
	local trPos = et.gentity_get(entNum, "s.pos")
	local forward, right, up = toAngleVectors(angles)
	local muzzleOrigin = calcMuzzlePoint(trPos.trBase, viewHeight, leanValue, right)
	local endPos = multAddVector(muzzleOrigin, CH_REVIVE_DIST, forward)
	local result = et.G_HistoricalTrace(entNum, muzzleOrigin, nil, nil, endPos, entNum, MASK_SHOT)
	-- stuck in solid, offset the origin forward
	if result.startsolid then
		endPos = multAddVector(muzzleOrigin, 8, forward)
		result = et.trap_Trace(muzzleOrigin, nil, nil, entNum, MASK_SHOT)
	end
	-- no hit give back syringe
	if result.fraction == 1.0 then
		revokeSyringe(entNum)
		return 1
	end
	-- miss (refactor)
	if result.entityNum > et.MAX_CLIENTS then
		revokeSyringe(entNum)
		return 1
	end
	local reviveeNum = result.entityNum
	local pm_type = et.gentity_get(reviveeNum, "ps.pm_type")
	-- don't handle limbo players
	if pm_type ~= et.PM_NORMAL then
		return 0
	end
	local reviveeTeam = et.gentity_get(reviveeNum, "sess.sessionTeam")
	local reviverTeam = et.gentity_get(entNum, "sess.sessionTeam")
	if reviveeTeam ~= reviverTeam then
		revokeSyringe(entNum)
		return 1
	end
	local reviveeHealth = et.gentity_get(reviveeNum, "health")
	local reviveeMaxHealth = et.gentity_get(reviveeNum, "ps.stats", 3)
	if reviveeHealth > reviveeMaxHealth * 0.25 then
		revokeSyringe(entNum)
		return 1
	end
	local finalHealth = 0
	local reviverMedSkill = et.gentity_get(entNum, "sess.skill", et.SK_FIRST_AID)
	if reviverMedSkill >= 3 then
		finalHealth = reviveeMaxHealth
	else
		finalHealth = reviveeMaxHealth * 0.5
	end
	et.gentity_set(reviveeNum, "health", finalHealth)
	et.G_Sound(reviveeNum, GAMESOUND_MISC_REVIVE) -- todo: export as et constants?
	et.gentity_set(reviveeNum, "pers.lasthealth_client", entNum)
	if not et.gentity_get(reviveeNum, "isProp") then
		et.G_AddSkillPoints(entNum, et.SK_FIRST_AID, 2)
	end
	return 1
end

function et_WeaponFire(clientNum, weapNum)
	if weapNum == et.WP_MEDIC_SYRINGE and medicSyringeHealEnabled then
		return checkMedicSyringeHeal(clientNum)
	end
	return 0
end

function et_InitGame()
	local g_misc = tonumber(et.trap_Cvar_Get("g_misc"))
	medicSyringeHealEnabled = (g_misc & G_MISC_MEDIC_SYRINGE_HEAL) ~= 0
end