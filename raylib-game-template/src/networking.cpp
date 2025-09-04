// Example client/server application using SteamNetworkingSockets based on Valve Corporation chat example

#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <sstream>
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
#include <windows.h> // Ug, for NukeProcess -- see below
#else
#include <unistd.h>
#include <signal.h>
#endif

#include "networking.h"

/////////////////////////////////////////////////////////////////////////////
//
// Common stuff
//
/////////////////////////////////////////////////////////////////////////////

bool g_bQuit = false;

class NetworkClient;
class NetworkServer;

NetworkClient* myClient = nullptr;
NetworkServer* myServer = nullptr;

SteamNetworkingMicroseconds g_logTimeZero;

ISteamNetworkingSockets* m_pInterface;
HSteamNetPollGroup m_hPollGroup;
std::vector< HSteamNetConnection> m_Clients;
HSteamNetConnection m_hConnection;
HSteamListenSocket m_hListenSock;

// We do this because I won't want to figure out how to cleanly shut
// down the thread that is reading from stdin.
static void NukeProcess(int rc)
{
#ifdef _WIN32
	ExitProcess(rc);
#else
	(void)rc; // Unused formal parameter
	kill(getpid(), SIGKILL);
#endif
}

static void DebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg)
{
	SteamNetworkingMicroseconds time = SteamNetworkingUtils()->GetLocalTimestamp() - g_logTimeZero;
	printf("%10.6f %s\n", time * 1e-6, pszMsg);
	fflush(stdout);
	if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug)
	{
		fflush(stdout);
		fflush(stderr);
		NukeProcess(1);
	}
}

static void FatalError(const char* fmt, ...)
{
	char text[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	char* nl = strchr(text, '\0') - 1;
	if (nl >= text && *nl == '\n')
		*nl = '\0';
	DebugOutput(k_ESteamNetworkingSocketsDebugOutputType_Bug, text);
}

static void Printf(const char* fmt, ...)
{
	char text[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	char* nl = strchr(text, '\0') - 1;
	if (nl >= text && *nl == '\n')
		*nl = '\0';
	DebugOutput(k_ESteamNetworkingSocketsDebugOutputType_Msg, text);
}

// You really gotta wonder what kind of pedantic garbage was
// going through the minds of people who designed std::string
// that they decided not to include trim.
// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

/////////////////////////////////////////////////////////////////////////////
//
// NetworkServer
//
/////////////////////////////////////////////////////////////////////////////

class NetworkServer
{
public:
	void Run(uint16 nPort)
	{
		// Select instance to use.  For now we'll always use the default.
		// But we could use SteamGameServerNetworkingSockets() on Steam.
		m_pInterface = SteamNetworkingSockets();

		// Start listening
		SteamNetworkingIPAddr serverLocalAddr;
		serverLocalAddr.Clear();
		serverLocalAddr.m_port = nPort;
		SteamNetworkingConfigValue_t opt;
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
		m_hListenSock = m_pInterface->CreateListenSocketIP(serverLocalAddr, 1, &opt);
		if (m_hListenSock == k_HSteamListenSocket_Invalid)
			FatalError("Failed to listen on port %d", nPort);
		m_hPollGroup = m_pInterface->CreatePollGroup();
		if (m_hPollGroup == k_HSteamNetPollGroup_Invalid)
			FatalError("Failed to listen on port %d", nPort);
		Printf("Server listening on port %d\n", nPort);

#define OFF
#ifndef OFF

		// Poll for and handle messages
		while (!g_bQuit)
		{
			char temp[1024];

			while (!g_bQuit)
			{
				UpdateServer();
			}

			//
			// Poll Callbacks
			//
			s_pCallbackInstance = this;
			m_pInterface->RunCallbacks();

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		// Close all the connections
		Printf("Closing connections...\n");
		for (auto it : m_Clients)
		{
			// Send them one more goodbye message.  Note that we also have the
			// connection close reason as a place to send final data.  However,
			// that's usually best left for more diagnostic/debug text not actual
			// protocol strings.
			SendStringToClient(it, "Server is shutting down.  Goodbye.");

			// Close the connection.  We use "linger mode" to ask SteamNetworkingSockets
			// to flush this out and close gracefully.
			m_pInterface->CloseConnection(it, 0, "Server Shutdown", true);
		}
		m_Clients.clear();

		m_pInterface->CloseListenSocket(m_hListenSock);
		m_hListenSock = k_HSteamListenSocket_Invalid;

		m_pInterface->DestroyPollGroup(m_hPollGroup);
		m_hPollGroup = k_HSteamNetPollGroup_Invalid;

#endif
	}
private:

	static NetworkServer* s_pCallbackInstance;


	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		s_pCallbackInstance->OnSteamNetConnectionStatusChanged(pInfo);
	}

	void SendStringToClient(HSteamNetConnection conn, const char* str)
	{
		m_pInterface->SendMessageToConnection(conn, str, (uint32)strlen(str), k_nSteamNetworkingSend_Reliable, nullptr);
	}

	void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		char temp[1024];

		// What's the state of the connection?
		switch (pInfo->m_info.m_eState)
		{
		case k_ESteamNetworkingConnectionState_None:
			// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			// Ignore if they were not previously connected.  (If they disconnected
			// before we accepted the connection.)
			if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
			{

				// Locate the client.  Note that it should have been found, because this
				// is the only codepath where we remove clients (except on shutdown),
				// and connection change callbacks are dispatched in queue order.
				auto itClient = std::find(m_Clients.begin(), m_Clients.end(), pInfo->m_hConn);
				assert(itClient != m_Clients.end());

				// Select appropriate log messages
				const char* pszDebugLogAction;
				if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
				{
					pszDebugLogAction = "problem detected locally";
					sprintf(temp, "Alas, a client hath fallen into shadow.  (%s)", pInfo->m_info.m_szEndDebug);
				}
				else
				{
					// Note that here we could check the reason code to see if
					// it was a "usual" connection or an "unusual" one.
					pszDebugLogAction = "closed by peer";
					sprintf(temp, "A client hath departed");
				}

				// Spew something to our own log.  Note that because we put their nick
				// as the connection description, it will show up, along with their
				// transport-specific data (e.g. their IP address)
				Printf("Connection %s %s, reason %d: %s\n",
					pInfo->m_info.m_szConnectionDescription,
					pszDebugLogAction,
					pInfo->m_info.m_eEndReason,
					pInfo->m_info.m_szEndDebug
				);

				m_Clients.erase(itClient);
			}
			else
			{
				assert(pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
			}

			// Clean up the connection.  This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed.  We must close it on our end, too
			// to finish up.  The reason information do not matter in this case,
			// and we cannot linger because it's already closed on the other end,
			// so we just pass 0's.
			m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting:
		{
			// This must be a new connection
			assert(std::find(m_Clients.begin(), m_Clients.end(), pInfo->m_hConn) == m_Clients.end());

			Printf("Connection request from %s", pInfo->m_info.m_szConnectionDescription);

			// A client is attempting to connect
			// Try to accept the connection.
			if (m_pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
			{
				// This could fail.  If the remote host tried to connect, but then
				// disconnected, the connection may already be half closed.  Just
				// destroy whatever we have on our side.
				m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				Printf("Can't accept connection.  (It was already closed?)");
				break;
			}

			// Assign the poll group
			if (!m_pInterface->SetConnectionPollGroup(pInfo->m_hConn, m_hPollGroup))
			{
				m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				Printf("Failed to set poll group?");
				break;
			}

			// Send them a welcome message
			sprintf(temp, "Welcome to the server");
			SendStringToClient(pInfo->m_hConn, temp);

			// Add them to the client list, using std::map wacky syntax
			m_Clients.push_back(pInfo->m_hConn);
			break;
		}

		case k_ESteamNetworkingConnectionState_Connected:
			// We will get a callback immediately after accepting the connection.
			// Since we are the server, we can ignore this, it's not news to us.
			break;

		default:
			// Silences -Wswitch
			break;
		}
	}
};

NetworkServer* NetworkServer::s_pCallbackInstance = nullptr;

/////////////////////////////////////////////////////////////////////////////
//
// NetworkClient
//
/////////////////////////////////////////////////////////////////////////////

class NetworkClient
{
public:
	void Run(const SteamNetworkingIPAddr& serverAddr)
	{
		// Select instance to use.  For now we'll always use the default.
		m_pInterface = SteamNetworkingSockets();

		// Start connecting
		char szAddr[SteamNetworkingIPAddr::k_cchMaxString];
		serverAddr.ToString(szAddr, sizeof(szAddr), true);
		Printf("Connecting to server at %s", szAddr);
		SteamNetworkingConfigValue_t opt;
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
		m_hConnection = m_pInterface->ConnectByIPAddress(serverAddr, 1, &opt);
		if (m_hConnection == k_HSteamNetConnection_Invalid)
			FatalError("Failed to create connection");
#define OLD
#ifndef OLD
		while (!g_bQuit)
		{
			//
			// RECEIVE
			//
			while (!g_bQuit)
			{
				UpdateClient();
			}

			//
			// Poll Callbacks
			//

			s_pCallbackInstance = this;
			m_pInterface->RunCallbacks();

			//
			// SEND
			//

			std::string DebugMessage = "Client message";

			// Send it to the server and let them parse it
			m_pInterface->SendMessageToConnection(m_hConnection, DebugMessage.c_str(), (uint32)DebugMessage.length(), k_nSteamNetworkingSend_Reliable, nullptr);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		g_bQuit = true;

		// Close the connection gracefully.
		// We use linger mode to ask for any remaining reliable data
		// to be flushed out.  But remember this is an application
		// protocol on UDP.  See ShutdownSteamDatagramConnectionSockets
		m_pInterface->CloseConnection(m_hConnection, 0, "Goodbye", true);
#endif
	}
private:

	static NetworkClient* s_pCallbackInstance;

	//ISteamNetworkingSockets* m_pInterface;

	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		s_pCallbackInstance->OnSteamNetConnectionStatusChanged(pInfo);
	}

	void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
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
			g_bQuit = true;

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
			m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
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
};

NetworkClient* NetworkClient::s_pCallbackInstance = nullptr;

const uint16 DEFAULT_SERVER_PORT = 27020;

void PrintUsageAndExit(int rc = 1)
{
	fflush(stderr);
	printf(
		R"usage(Usage:
    example client SERVER_ADDR
    example server [--port PORT]
)usage"
);
	fflush(stdout);
	exit(rc);
}

int startSessionFromArgument(int argc, const char* argv[])
{
	bool bServer = false;
	bool bClient = false;
	int nPort = DEFAULT_SERVER_PORT;
	SteamNetworkingIPAddr addrServer; addrServer.Clear();

	for (int i = 0; i < argc; ++i)
	{
		if (!bClient && !bServer)
		{
			if (!strcmp(argv[i], "client"))
			{
				bClient = true;
				continue;
			}
			if (!strcmp(argv[i], "server"))
			{
				bServer = true;
				Printf("is server");
				continue;
			}
		}
		if (!strcmp(argv[i], "--port"))
		{
			++i;
			if (i >= argc)
				PrintUsageAndExit();
			nPort = atoi(argv[i]);
			if (nPort <= 0 || nPort > 65535)
				FatalError("Invalid port %d", nPort);
			continue;
		}

		// Anything else, must be server address to connect to
		if (bClient && addrServer.IsIPv6AllZeros())
		{
			if (!addrServer.ParseString(argv[i]))
				FatalError("Invalid server address '%s'", argv[i]);
			if (addrServer.m_port == 0)
				addrServer.m_port = DEFAULT_SERVER_PORT;
			continue;
		}

		PrintUsageAndExit();
	}

	if (bClient == bServer || (bClient && addrServer.IsIPv6AllZeros()))
		PrintUsageAndExit();

	//
	// Initialization
	//

	// Create client and server sockets
#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	SteamDatagramErrMsg errMsg;
	if (!GameNetworkingSockets_Init(nullptr, errMsg))
		FatalError("GameNetworkingSockets_Init failed.  %s", errMsg);
#else
	SteamDatagram_SetAppID(570); // Just set something, doesn't matter what
	SteamDatagram_SetUniverse(false, k_EUniverseDev);

	SteamDatagramErrMsg errMsg;
	if (!SteamDatagramClient_Init(errMsg))
		FatalError("SteamDatagramClient_Init failed.  %s", errMsg);

	// Disable authentication when running with Steam, for this
	// example, since we're not a real app.
	//
	// Authentication is disabled automatically in the open-source
	// version since we don't have a trusted third party to issue
	// certs.
	SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);
#endif

	g_logTimeZero = SteamNetworkingUtils()->GetLocalTimestamp();

	SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg, DebugOutput);

	//
	// Application Loop
	//

	if (bClient)
	{
		myClient = new NetworkClient;
		myClient->Run(addrServer);
	}
	else
	{
		myServer = new NetworkServer;
		myServer->Run((uint16)nPort);
	}

#define OFF
#ifndef OFF

	//
	// Shutdown
	//

	// Give connections time to finish up.  This is an application layer protocol
	// here, it's not TCP.  Note that if you have an application and you need to be
	// more sure about cleanup, you won't be able to do this.  You will need to send
	// a message and then either wait for the peer to close the connection, or
	// you can pool the connection to see if any reliable data is pending.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	GameNetworkingSockets_Kill();
#else
	SteamDatagramClient_Kill();
#endif

	// Ug, why is there no simple solution for portable, non-blocking console user input?
	// Just nuke the process
	//LocalUserInput_Kill();
	NukeProcess(0);
#endif
	return 0;
}

void startSession(const char* argument)
{
	//split the string
	std::istringstream iss(argument);
	std::vector<std::string> tokens;
	std::string token;
	while (iss >> token) {
		tokens.push_back(token);
	}

	// build const char* array
	std::vector<const char*> argv;
	for (auto& t : tokens) {
		argv.push_back(t.c_str());
	}

	//get the amount of args
	int argc = static_cast<int>(argv.size());

	startSessionFromArgument(argc, argv.data());
}

void StartServer()
{
	startSession("server --port 7777");
}

void StartClient()
{
	startSession("client 127.0.0.1:7777");
}

void UpdateServer()
{
	ISteamNetworkingMessage* pIncomingMsg = nullptr;
	int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup(m_hPollGroup, &pIncomingMsg, 1);
	if (numMsgs == 0)
		return;
	if (numMsgs < 0)
		FatalError("Error checking for messages");
	assert(numMsgs == 1 && pIncomingMsg);
	auto itClient = std::find(m_Clients.begin(), m_Clients.end(), pIncomingMsg->m_conn);
	assert(itClient != m_Clients.end());

	// '\0'-terminate it to make it easier to parse
	// Assume it's a c-string and print it as-is
	std::string sCmd;

	// Populate a std::string with the data we received, assuming it's a character array
	sCmd.assign((const char*)pIncomingMsg->m_pData, pIncomingMsg->m_cbSize);

	// We don't need this anymore.
	pIncomingMsg->Release();

	const char* cmd = sCmd.c_str();
	Printf(cmd);
}

void UpdateClient()
{
	ISteamNetworkingMessage* pIncomingMsg = nullptr;
	int numMsgs = m_pInterface->ReceiveMessagesOnConnection(m_hConnection, &pIncomingMsg, 1);
	// Nothing? Do nothing.
	if (numMsgs == 0)
		return;
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

void CloseServer()
{
	// Close all the connections
	Printf("Closing connections...\n");
	for (auto it : m_Clients)
	{
		// Send them one more goodbye message.  Note that we also have the
		// connection close reason as a place to send final data.  However,
		// that's usually best left for more diagnostic/debug text not actual
		// protocol strings.
		//SendStringToClient(it, "Server is shutting down.  Goodbye.");

		// Close the connection.  We use "linger mode" to ask SteamNetworkingSockets
		// to flush this out and close gracefully.
		m_pInterface->CloseConnection(it, 0, "Server Shutdown", true);
	}
	m_Clients.clear();

	m_pInterface->CloseListenSocket(m_hListenSock);
	m_hListenSock = k_HSteamListenSocket_Invalid;

	m_pInterface->DestroyPollGroup(m_hPollGroup);
	m_hPollGroup = k_HSteamNetPollGroup_Invalid;

	//
	// Shutdown
	//

	// Give connections time to finish up.  This is an application layer protocol
	// here, it's not TCP.  Note that if you have an application and you need to be
	// more sure about cleanup, you won't be able to do this.  You will need to send
	// a message and then either wait for the peer to close the connection, or
	// you can pool the connection to see if any reliable data is pending.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	GameNetworkingSockets_Kill();
#else
	SteamDatagramClient_Kill();
#endif

	// Ug, why is there no simple solution for portable, non-blocking console user input?
	// Just nuke the process
	//LocalUserInput_Kill();
	NukeProcess(0);
}

void CloseClient()
{
	// Close the connection gracefully.
	// We use linger mode to ask for any remaining reliable data
	// to be flushed out.  But remember this is an application
	// protocol on UDP.  See ShutdownSteamDatagramConnectionSockets
	m_pInterface->CloseConnection(m_hConnection, 0, "Goodbye", true);

	//
// Shutdown
//

// Give connections time to finish up.  This is an application layer protocol
// here, it's not TCP.  Note that if you have an application and you need to be
// more sure about cleanup, you won't be able to do this.  You will need to send
// a message and then either wait for the peer to close the connection, or
// you can pool the connection to see if any reliable data is pending.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	GameNetworkingSockets_Kill();
#else
	SteamDatagramClient_Kill();
#endif

	// Ug, why is there no simple solution for portable, non-blocking console user input?
	// Just nuke the process
	//LocalUserInput_Kill();
	NukeProcess(0);
}