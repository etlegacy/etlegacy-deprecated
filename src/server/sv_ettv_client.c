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
 * @file sv_ettv_client.c
 * @brief ettv client hoops and loops
 */
#include "server.h"
#include "sv_ettv.h"

clientConnection_t ettv;

/**
 * @brief Responses to broadcasts, etc
 *
 * Compare first n chars so it doesn’t bail if token is parsed incorrectly.
 */
void ETTV_ConnectionlessPacket(netadr_t from, msg_t *msg)
{
	char *s;
	char *c;

	MSG_BeginReadingOOB(msg);
	MSG_ReadLong(msg);      // skip the -1

	s = MSG_ReadStringLine(msg);

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);

	Com_DPrintf("ETTV packet %s: %s\n", NET_AdrToString(from), c);

	// challenge from the server we are connecting to
	if (!Q_stricmp(c, "challengeResponse"))
	{
		if (ettv.state != CA_CONNECTING)
		{
			Com_Printf("Unwanted challenge response received.  Ignored.\n");
		}
		else
		{
			// start sending challenge response instead of challenge request packets
			ettv.challenge = atoi(Cmd_Argv(1));
			if (Cmd_Argc() > 2)
			{
				ettv.onlyVisibleClients = atoi(Cmd_Argv(2));
			}
			else
			{
				ettv.onlyVisibleClients = 0;
			}
			ettv.state              = CA_CHALLENGING;
			ettv.connectPacketCount = 0;
			ettv.connectTime        = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			ettv.serverAddress = from;
			Com_DPrintf("challenge: %d\n", ettv.challenge);
		}
		return;
	}

	// server connection
	if (!Q_stricmp(c, "connectResponse"))
	{
		if (ettv.state >= CA_CONNECTED)
		{
			Com_Printf("Dup connect received.  Ignored.\n");
			return;
		}
		if (ettv.state != CA_CHALLENGING)
		{
			Com_Printf("connectResponse packet while not connecting.  Ignored.\n");
			return;
		}
		if (!NET_CompareAdr(from, ettv.serverAddress))
		{
			Com_Printf("connectResponse from a different address.  Ignored.\n");
			Com_Printf("%s should have been %s\n", NET_AdrToString(from),
					   NET_AdrToString(ettv.serverAddress));
			return;
		}

		Com_CheckUpdateStarted();

		Netchan_Setup(NS_CLIENT, &ettv.netchan, from, Cvar_VariableValue("net_qport"));
		ettv.state              = CA_CONNECTED;
		ettv.lastPacketSentTime = -9999;     // send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if (!Q_stricmp(c, "infoResponse"))
	{
		CL_ServerInfoPacket(from, msg);
		return;
	}

	// server responding to a get playerlist
	if (!Q_stricmp(c, "statusResponse"))
	{
		CL_ServerStatusResponse(from, msg);
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!Q_stricmp(c, "disconnect"))
	{
		CL_DisconnectPacket(from);
		return;
	}

	// echo request from server
	if (!Q_stricmp(c, "echo"))
	{
		NET_OutOfBandPrint(NS_CLIENT, from, "%s", Cmd_Argv(1));
		return;
	}

	// cd check
	if (!Q_stricmp(c, "keyAuthorize"))
	{
		// we don't use these now, so dump them on the floor
		return;
	}

	// echo request from server
	if (!Q_stricmp(c, "print"))
	{
		// NOTE: we may have to add exceptions for auth and update servers
		if (NET_CompareAdr(from, ettv.serverAddress) || NET_CompareAdr(from, rcon_address))
		{
			CL_PrintPacket(msg);
		}
		return;
	}

	Com_DPrintf("ETTV Unknown connectionless packet command %s.\n", c);
}

void ETTV_PacketEvent(netadr_t from, msg_t *msg)
{
	int headerBytes;

	if (Com_UpdatePacketEvent(from))
	{
		return;
	}

	if (msg->cursize >= 4 && *( int * ) msg->data == -1)
	{
		ETTV_ConnectionlessPacket(from, msg);
		return;
	}

	ettv.lastPacketTime = ettv.realtime;

	if (ettv.state < CA_CONNECTED)
	{
		return;     // can't be a valid sequenced packet
	}

	if (msg->cursize < 4)
	{
		Com_Printf("%s: Runt packet\n", NET_AdrToString(from));
		return;
	}

	// packet from server
	if (!NET_CompareAdr(from, ettv.netchan.remoteAddress))
	{
		Com_DPrintf("%s:sequenced packet without connection\n", NET_AdrToString(from));
		// client isn't connected - don't send disconnect
		return;
	}

	if (!CL_Netchan_Process(&ettv.netchan, msg))
	{
		return;     // out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	ettv.serverMessageSequence = LittleLong(*( int * ) msg->data);

	ettv.lastPacketTime = ettv.realtime;
	ETTV_ParseServerMessage(msg);

	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	if (ettv.demorecording && !ettv.demowaiting)
	{
		CL_WriteDemoMessage(msg, headerBytes);
	}
}
