#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <cctype>

#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <GameNetworkingSockets/steam/steam_api.h>
#endif

#ifdef _WIN32
#include <windows.h> 
#else
#include <unistd.h>
#include <signal.h>
#endif

#include "client.h"

HSteamNetConnection m_hConnection;
ISteamNetworkingSockets* m_ClientInterface;


void ClientOnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	assert(pInfo->m_hConn == m_hConnection || m_hConnection == k_HSteamNetConnection_Invalid);

	// What's the state of the connection?
	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_None:
		// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
		break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		// Print an appropriate message
		if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
		{
			// Note: we could distinguish between a timeout, a rejected connection,
			// or some other transport problem.
			Printf("We sought the remote host, yet our efforts were met with defeat.  (%s)", pInfo->m_info.m_szEndDebug);
		}
		else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
		{
			Printf("Alas, troubles beset us; we have lost contact with the host.  (%s)", pInfo->m_info.m_szEndDebug);
		}
		else
		{
			// NOTE: We could check the reason code for a normal disconnection
			Printf("The host hath bidden us farewell.  (%s)", pInfo->m_info.m_szEndDebug);
		}

		// Clean up the connection.  This is important!
		// The connection is "closed" in the network sense, but
		// it has not been destroyed.  We must close it on our end, too
		// to finish up.  The reason information do not matter in this case,
		// and we cannot linger because it's already closed on the other end,
		// so we just pass 0's.
		m_ClientInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
		m_hConnection = k_HSteamNetConnection_Invalid;
		break;
	}

	case k_ESteamNetworkingConnectionState_Connecting:
		// We will get this callback when we start connecting.
		// We can ignore this.
		break;

	case k_ESteamNetworkingConnectionState_Connected:
		Printf("Connected to server OK");
		break;

	default:
		// Silences -Wswitch
		break;
	}
}

void ClientSteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	ClientOnSteamNetConnectionStatusChanged(pInfo);
}

void StartClient()
{
	StartMyNetworkedSession();

	// Select instance to use.  For now we'll always use the default.
	m_ClientInterface = SteamNetworkingSockets();

	SteamNetworkingIPAddr serverAddr; 
	serverAddr.Clear();
	serverAddr.ParseString("127.0.0.1:7777");

	// Start connecting
	char szAddr[SteamNetworkingIPAddr::k_cchMaxString];
	serverAddr.ToString(szAddr, sizeof(szAddr), true);
	Printf("Connecting to server at %s", szAddr);
	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)ClientSteamNetConnectionStatusChangedCallback);
	m_hConnection = m_ClientInterface->ConnectByIPAddress(serverAddr, 1, &opt);
	if (m_hConnection == k_HSteamNetConnection_Invalid)
		FatalError("Failed to create connection");

}

void UpdateClient()
{
	ISteamNetworkingMessage* pIncomingMsg = nullptr;
	int numMsgs = m_ClientInterface->ReceiveMessagesOnConnection(m_hConnection, &pIncomingMsg, 1);
	// Nothing? Do nothing.
	if (numMsgs == 0)
	{
		//Printf("no messages");
		return;
	}
		
	else if (numMsgs < 0)
		FatalError("Error checking for messages");
	else
	{
		// Just echo anything we get from the server
		fwrite(pIncomingMsg->m_pData, 1, pIncomingMsg->m_cbSize, stdout);
		fputc('\n', stdout);

		// We don't need this anymore.
		pIncomingMsg->Release();
	}
}