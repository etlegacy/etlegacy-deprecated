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
 * @file sv_ettv_parse.c
 * @brief Parse a message received from the server
 */
#include "server.h"
#include "sv_ettv.h"

void ETTV_ParseCommandString(msg_t *msg)
{
	//TODO: implement
}

void ETTV_ParseGamestate(msg_t *msg)
{
	//TODO: implement
}

void ETTV_ParseSnapshot(msg_t *msg)
{
	//TODO: implement
}

void ETTV_ParsePlayerStates(msg_t *msg)
{
	//TODO: implement
}

void ETTV_ParseCurrentState(msg_t *msg)
{
	//TODO: implement
}

void ETTV_ParseBinaryMessage(msg_t *msg)
{
	//TODO: implement
}

void ETTV_ParseServerMessage(msg_t *msg)
{
	int cmd;

	MSG_Bitstream(msg);

	// get the reliable sequence acknowledge number
	clc.reliableAcknowledge = MSG_ReadLong(msg);

	if (clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS)
	{
		clc.reliableAcknowledge = clc.reliableSequence;
	}

	// parse the message
	while (1)
	{
		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP, "ETTV_ParseServerMessage: read past end of server message");
			break;
		}

		cmd = MSG_ReadByte(msg);

		if (cmd == svc_EOF)
		{
			break;
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP, "ETTV_ParseServerMessage: Illegible server message %d", cmd);
			break;
		case svc_nop:
			break;
		case svc_serverCommand:
			ETTV_ParseCommandString(msg);
			break;
		case svc_gamestate:
			ETTV_ParseGamestate(msg);
			break;
		case svc_snapshot:
			ETTV_ParseSnapshot(msg);
			break;
		case svc_download:
			//FIXME: we should not be having these
			Com_Error(ERR_DROP, "ETTV_ParseServerMessage: Illegible server message %d (download)", cmd);
			break;
		case svc_tv_playerstate:
			ETTV_ParsePlayerStates(msg);
			break;
		case svc_tv_currentstate:
			ETTV_ParseCurrentState(msg);
			break;
		}
	}

	ETTV_ParseBinaryMessage(msg);
}
