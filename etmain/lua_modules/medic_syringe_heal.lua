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

local CH_REVIVE_DIST = 64 -- import as et constant?
local SKILL_POINTS_ADD = 2 -- xp added to a healer

-- vector math

local PITCH = 1 -- up / down
local YAW = 2 -- left / right
local ROLL = 3 -- fall over

function toAngleVectors(angles)
	local TAU = math.pi * 2
	local angle = angles[YAW] * (TAU / 360)
	local sy = math.sin(angle)
	local cy = math.cos(angle)

	angle = angles[PITCH] * (TAU / 360)
	local sp = math.sin(angle)
	local cp = math.cos(angle)

	angle = angles[ROLL] * (TAU / 360)
	local sr = math.sin(angle)
	local cr = math.cos(angle)

	local forward = { 
		cp * cy, 
		cp * sy, 
		-sp 
	}
	local right = {
		(-1 * sr * sp * cy + -1 * cr * -sy),
		(-1 * sr * sp * sy + -1 * cr * cy),
		-1 * sr * cp
	}
	local up = {
		(cr * sp * cy + -sr * -sy),
		(cr * sp * sy + -sr * cy),
		cr * cp
	}

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

function revokeSyringe(clientNum)
	-- todo: implement et.GetPlayerWeapon(clientNum, WP_MEDIC_SYRINGE) ?
	local weapon, ammo, ammoclip = et.GetCurrentWeapon(clientNum)
	et.AddWeaponToPlayer(clientNum, weapon, ammo, ammoclip + 1, 0)
end
 
function checkMedicSyringeHeal(healer)
	local angles = et.gentity_get(healer, "ps.viewangles")
	local viewHeight = et.gentity_get(healer, "ps.viewheight")
	local leanValue = et.gentity_get(healer, "ps.leanf")
	local trPos = et.gentity_get(healer, "s.pos")
	local forward, right, up = toAngleVectors(angles)
	local muzzlept = calcMuzzlePoint(trPos.trBase, viewHeight, leanValue, right)
	local endpt = multAddVector(muzzlept, CH_REVIVE_DIST, forward)
	local result = et.G_HistoricalTrace(healer, muzzlept, nil, nil, endpt, healer, et.MASK_SHOT)
	-- stuck in solid, offset the origin forward
	if result.startsolid then
		endpt = multAddVector(muzzlept, 8, forward)
		result = et.trap_Trace(muzzlept, nil, nil, healer, et.MASK_SHOT)
	end
	-- no hit give back syringe
	if result.fraction == 1.0 or result.entityNum >= et.MAX_CLIENTS then
		revokeSyringe(healer)
		return 1
	end
	local healee = result.entityNum
	local pm_type = et.gentity_get(healee, "ps.pm_type")
	-- limbo players are handled by the game
	if pm_type ~= et.PM_NORMAL then
		return 0 -- pass control to game
	end
	local healeeTeam = et.gentity_get(healee, "sess.sessionTeam")
	local healerTeam = et.gentity_get(healer, "sess.sessionTeam")
	if healeeTeam ~= healerTeam then
		revokeSyringe(healer)
		return 1
	end
	local healeeHealth = et.gentity_get(healee, "health")
	local healeeMaxHealth = et.gentity_get(healee, "ps.stats", et.STAT_MAX_HEALTH)
	if healeeHealth > healeeMaxHealth * 0.25 then
		revokeSyringe(healer)
		return 1
	end
	local healerMedSkill = et.gentity_get(healer, "sess.skill", et.SK_FIRST_AID)
	local finalHealth = healeeMaxHealth * (healerMedSkill >= 3 and 1 or 0.5)
	et.gentity_set(healee, "health", finalHealth)
	et.G_Sound(healee, 8) -- GAMESOUND_MISC_REVIVE, import?
	et.gentity_set(healee, "pers.lasthealth_client", healer) 
	et.G_AddSkillPoints(healer, et.SK_FIRST_AID, SKILL_POINTS_ADD)
	return 1
end

function et_WeaponFire(clientNum, weapNum)
	if weapNum == et.WP_MEDIC_SYRINGE then
		return checkMedicSyringeHeal(clientNum)
	end
	return 0 -- pass control to game
end
