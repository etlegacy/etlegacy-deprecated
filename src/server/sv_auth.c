/**
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
 * @file sv_auth.c
 */

#include "server.h"

cvar_t *auth_enabled;
cvar_t *auth_master;
cvar_t *auth_forceName;
cvar_t *auth_serverKey;
cvar_t *g_gamestate;

static void Auth_Connect_Callback(qboolean success, json_t *payload, client_t *cl)
{
	json_t     *username = NULL;
	char       *username_s;
	const char *reason = "Authentication failed.";

	if (!success || !payload)
	{
		goto error;
	}

	username = json_object_get(payload, "username");

	if (!json_is_string(username))
	{
		goto error;
	}

	Q_strncpyz(cl->username, json_string_value(username), sizeof(cl->username));

	// allow in now, this is faster than retrying
	SV_SendClientGameState(cl);

	return;
error:
	if (payload)
	{
		json_t *detail = json_object_get(payload, "detail");
		if (json_is_string(detail))
		{
			reason = json_string_value(detail);
		}
	}

	// to get a meaningful dialog in the client
	NET_OutOfBandPrint(NS_SERVER, cl->netchan.remoteAddress, "print\n[err_dialog]%s\n", reason);

	// for everyone else on the server
	SV_DropClient(cl, reason);
}

void Auth_Init()
{
	auth_enabled   = Cvar_Get("auth_enabled", "0", CVAR_LATCH);
	auth_master    = Cvar_Get("auth_master", "http://master.etlive.pro", CVAR_LATCH);
	auth_forceName = Cvar_Get("auth_forceName", "0", CVAR_LATCH);
	auth_serverKey = Cvar_Get("auth_serverKey", "", CVAR_LATCH);
	g_gamestate    = Cvar_Get("gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM);
}

static void Auth_Heartbeat_Callback(qboolean success, json_t *payload, client_t *cl)
{
	if (!success)
	{
		if (payload)
		{
			json_t *detail = json_object_get(payload, "detail");
			if (json_is_string(detail))
			{
				Com_Printf("Heartbeat to auth master failed with the message \"%s\"\n", json_string_value(detail));
			}
		}
		else
		{
			Com_Printf("Heartbeat to auth master failed. Maybe it's down?\n");
		}
	}
}

void Auth_Heartbeat()
{
	if (!auth_enabled->value)
	{
		return;
	}

	json_api_request(va("%s/heartbeat/%s/%d/up", auth_master->string, auth_serverKey->string, Cvar_VariableIntegerValue("net_port")), NULL, (json_api_callback)Auth_Heartbeat_Callback, NULL);
}

qboolean Auth_ClientCheck(client_t *cl)
{
	const char *session;

	if (!auth_enabled->value)
	{
		return qtrue;
	}

	session = Info_ValueForKey(cl->userinfo, "etl_session");

	if (strlen(session) == 0)
	{
		NET_OutOfBandPrint(NS_SERVER, cl->netchan.remoteAddress, "print\n[err_dialog]%s\n", "This server runs ETLive. See http://etlive.pro/ for connection info.");
		return qfalse;
	}

	json_api_request(va("%s/connect/%s/%d/%s", auth_master->string, auth_serverKey->string, Cvar_VariableIntegerValue("net_port"), session), NULL, (json_api_callback)Auth_Connect_Callback, cl);

	Q_strncpyz(cl->session, session, sizeof(cl->session));

	return qtrue;
}

qboolean Auth_ClientMessage(client_t *cl)
{
	if (!auth_enabled->value)
	{
		return qtrue;
	}

	return (cl->username[0] != '\0');
}

void Auth_UserinfoChanged(client_t *cl)
{
	if (!auth_enabled->value || !auth_forceName->value)
	{
		return;
	}

	if (cl->username[0] && strcmp(Info_ValueForKey(cl->userinfo, "name"), cl->username))
	{
		Info_SetValueForKey(cl->userinfo, "name", cl->username);
	}
}

void Auth_ClientDisconnect(client_t *cl)
{
	if (!auth_enabled->value || !cl->username[0] || !cl->session[0])
	{
		return;
	}

	json_api_request(va("%s/disconnect/%s/%d/%s", auth_master->string, auth_serverKey->string, Cvar_VariableIntegerValue("net_port"), cl->session), NULL, NULL, NULL);
}
