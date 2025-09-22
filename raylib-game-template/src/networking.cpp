// Example client/server application using SteamNetworkingSockets based on Valve Corporation chat example

// prevents windows from redefining raylib functionality
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define NOSHOWWINDOW
#define NOUSER

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
#include <iostream>

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

#include "bullets.h"
#include "networking.h"
#include "screens.h"

//the sizes of data packets
#define POSITION_PACKET_SIZE sizeof(PositionDataPacket)
#define ID_PACKET_SIZE sizeof(IDDataPacket)
#define NICKNAME_PACKET_SIZE sizeof(NicknameDataPacket)
#define BULLET_CREATION_PACKET_SIZE sizeof(BulletCreationDataPacket)

/////////////////////////////////////////////////////////////////////////////
//
// serialisers
//
/////////////////////////////////////////////////////////////////////////////

//takes in an int and returns in 4 chars
void SerializeInt(const int inInt, char* outChars)
{
	outChars[0] = (inInt >> 0) & 0xFF;
	outChars[1] = (inInt >> 8) & 0xFF;
	outChars[2] = (inInt >> 16) & 0xFF;
	outChars[3] = (inInt >> 24) & 0xFF;
}

//takes in a series of chars, reading from the start index
const int DeserializeInt(const char* inChars, const int startIndex)
{
	int deserializedInt = 0;
	deserializedInt |= (static_cast<unsigned char>(inChars[startIndex]) << 0);
	deserializedInt |= (static_cast<unsigned char>(inChars[startIndex + 1]) << 8);
	deserializedInt |= (static_cast<unsigned char>(inChars[startIndex + 2]) << 16);
	deserializedInt |= (static_cast<unsigned char>(inChars[startIndex + 3]) << 24);

	return deserializedInt;
}

//takes in 4 chars and returns the int
const int DeserializeInt(const char* inChars)
{
	return DeserializeInt(inChars, 0);
}

//takes in the data packet struct and returns a collection of chars
void SerializePositionDataPacket(const PositionDataPacket& inPacket, char* outPacket)
{
	char tempChars[4];

	//set packet type
	outPacket[0] = (char)inPacket.packetType;

	//set ID
	outPacket[1] = inPacket.id;

	//set posX
	SerializeInt(inPacket.posX, tempChars);
	outPacket[2] = tempChars[0];
	outPacket[3] = tempChars[1];
	outPacket[4] = tempChars[2];
	outPacket[5] = tempChars[3];

	//set posY
	SerializeInt(inPacket.posY, tempChars);
	outPacket[6] = tempChars[0];
	outPacket[7] = tempChars[1];
	outPacket[8] = tempChars[2];
	outPacket[9] = tempChars[3];

}

//takes in a collection of chars and converts it into a positional data packet
PositionDataPacket DeserializePositionDataPacket(const char* inPacket)
{
	PositionDataPacket outPacket;

	//set packet type
	outPacket.packetType = RECEIVED_POSITION_DATA;

	//set ID
	outPacket.id = inPacket[1];

	//set posX
	outPacket.posX = DeserializeInt(inPacket, 2);

	//set posY
	outPacket.posY = DeserializeInt(inPacket, 6);

	return outPacket;
}

//takes in a datapacket for setting id's of clients and returns as 2 chars
void SerializeIDDataPacket(const IDDataPacket& inPacket, char* outPacket)
{
	//set packet type
	outPacket[0] = (char)inPacket.packetType;

	//set ID
	outPacket[1] = inPacket.id;
}

//takes in a series of chars and returns an ID data packet
IDDataPacket DeserializeIDDataPacket(char* inPacket)
{
	IDDataPacket outPacket;

	//set packet type
	outPacket.packetType = (PacketType)inPacket[0];

	//set ID
	outPacket.id = inPacket[1];

	return outPacket;
}

//takes in a nickname data packet and returns a collection of chars for network transmission
void SerializeNicknameDataPacket(const NicknameDataPacket& inPacket, char* outPacket)
{
	//set packet type
	outPacket[0] = (char)inPacket.packetType;

	//set ID
	outPacket[1] = inPacket.id;

	bool endOfName = false;
	//set the nickname
	for (int i = 0; i < 8; i++)
	{
		if (inPacket.nickname[i] == '\0')
		{
			endOfName = true;
		}

		if (endOfName)
		{
			outPacket[i + 2] = ' ';
		}
		else
		{
			outPacket[i + 2] = inPacket.nickname[i];
		}

		
	}
}

//returns a nickname data packet from a series of chars
NicknameDataPacket DeserializeNicknameDataPacket(char* inPacket)
{
	NicknameDataPacket outPacket;

	//set packet type
	outPacket.packetType = (PacketType)inPacket[0];

	//set ID
	outPacket.id = inPacket[1];

	//set the nickname
	for (int i = 0; i < 8; i++)
	{
		outPacket.nickname[i] = inPacket[i+2];
	}

	return outPacket;
}

//takes in a bullet creation data packet and returns a collection of chars for network transmission
void SerializeBulletCreateDataPacket(const BulletCreationDataPacket& inPacket, char* outPacket)
{
	//set packet type
	outPacket[0] = (char)inPacket.packetType;

	char tempChars[4];

	//set position x
	SerializeInt(inPacket.posX, tempChars);
	outPacket[1] = tempChars[0];
	outPacket[2] = tempChars[1];
	outPacket[3] = tempChars[2];
	outPacket[4] = tempChars[3];

	//set position y
	SerializeInt(inPacket.posY, tempChars);
	outPacket[5] = tempChars[0];
	outPacket[6] = tempChars[1];
	outPacket[7] = tempChars[2];
	outPacket[8] = tempChars[3];

	//set direction
	outPacket[9] = inPacket.shouldTravelRight;
}

//returns a bullet creation data packet given a collection of chars
BulletCreationDataPacket DeserializeBulletCreateDataPacket(char* inPacket)
{
	BulletCreationDataPacket outPacket;

	//set packet type
	outPacket.packetType = (PacketType)inPacket[0];

	//set positiion
	outPacket.posX = DeserializeInt(inPacket, 1);
	outPacket.posY = DeserializeInt(inPacket, 5);

	outPacket.shouldTravelRight = inPacket[9];

	return outPacket;
}


//the id given to this client (set by server)
int myID = 0;

//the nickname of this session
char myNick[9];

//sends data to the server regarding player ID and position
PositionDataPacket myPositionPacket;
void UpdatePacketPosition(int posX, int posY)
{
	myPositionPacket.id = myID; //because of data conversion, this will break at 255
						//if you have over 255 players, you may have other issues

	myPositionPacket.posX = posX;
	myPositionPacket.posY = posY;
}

//client information
std::map<HSteamNetConnection, int> clientIDs;
std::map<int, Vector2Int> clientPositions;
std::map<int, char[8]> clientNicknames;


//forward decl
class NetworkClient;
class NetworkServer;

//instances of the network sessions
NetworkClient* myClient = nullptr;
NetworkServer* myServer = nullptr;
NetworkServer* s_pCallbackInstance;
NetworkClient* s_pClientCallbackInstance;

//network session information
SteamNetworkingMicroseconds g_logTimeZero;
ISteamNetworkingSockets* m_pInterface;
HSteamNetPollGroup m_hPollGroup;
std::vector< HSteamNetConnection> m_Clients;
HSteamNetConnection m_hConnection;
HSteamListenSocket m_hListenSock;
NetworkStatus networkStatus = INACTIVE;

char nextAvailableID = 1;

// kills the session
static void NukeProcess(int rc)
{
#ifdef _WIN32
	ExitProcess(rc);
#else
	(void)rc; // Unused formal parameter
	kill(getpid(), SIGKILL);
#endif
}

//outputs information to the console
//kills the session if the type is a bug
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

//debugs an error and kills the session
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

//prints a string to the console
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
	//starts up the server at the beginning of the application
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

		networkStatus = SERVER_ACTIVE;

	}
private:
	//whenever the connection status changes
	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		s_pCallbackInstance->OnSteamNetConnectionStatusChanged(pInfo);
	}
public:
	//sends a character array to a specific client
	void SendStringToClient(HSteamNetConnection conn, const char* str)
	{
		m_pInterface->SendMessageToConnection(conn, str, (uint32)strlen(str), k_nSteamNetworkingSend_Reliable, nullptr);
	}
private:
	//whenever the connection statys changes
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
				}
				else
				{
					pszDebugLogAction = "problem not detected locally";
				}

				// log the issue
				Printf("Connection %s %s, reason %d: %s\n",
					pInfo->m_info.m_szConnectionDescription,
					pszDebugLogAction,
					pInfo->m_info.m_eEndReason,
					pInfo->m_info.m_szEndDebug
				);

				//null his position and name
				int clientID = clientIDs[pInfo->m_hConn];
				clientPositions[clientID] = { 0, 0 };
				for (int i = 0; i < 8; i++)
				{
					clientNicknames[clientID][i] = ' ';
				}
				m_Clients.erase(itClient);

				//let the other clients know about the player who left
				for (auto client : m_Clients)
				{
					IDDataPacket discPacket;
					discPacket.packetType = PLAYER_DISCONNECTED;
					discPacket.id = clientID;
					char serialDiscPacket[NICKNAME_PACKET_SIZE];
					SerializeIDDataPacket(discPacket, serialDiscPacket);

					m_pInterface->SendMessageToConnection(client, serialDiscPacket, NICKNAME_PACKET_SIZE,
						k_nSteamNetworkingSend_Reliable, nullptr);
				}
			}
			else
			{
				assert(pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
			}

			// Clean up the connection.
			m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			break;
		}

		//runs when a player joins
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

			//get an ID for this connected client to use
			char thisClientID = nextAvailableID;
			nextAvailableID++;
			clientIDs[pInfo->m_hConn] = thisClientID;

			//build the data packet with their ID
			IDDataPacket setIdDP;
			setIdDP.packetType = RECEIVED_SET_ID;
			setIdDP.id = thisClientID;
			char charPacket[2];
			SerializeIDDataPacket(setIdDP, charPacket);

			//send the client their ID
			m_pInterface->SendMessageToConnection(pInfo->m_hConn, charPacket, ID_PACKET_SIZE,
				k_nSteamNetworkingSend_Reliable, nullptr);

			// Add them to the client list, using std::map wacky syntax
			m_Clients.push_back(pInfo->m_hConn);

			Printf("Player connected successfully");
			break;
		}

		default:
			break;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// NetworkClient
//
/////////////////////////////////////////////////////////////////////////////

class NetworkClient
{
public:
	//starts up the client
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

		networkStatus = CLIENT_ACTIVE;
	}
private:
	//whenever our connection status changes
	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		s_pClientCallbackInstance->OnSteamNetConnectionStatusChanged(pInfo);
	}

	//whenever our connection status changes
	void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		assert(pInfo->m_hConn == m_hConnection || m_hConnection == k_HSteamNetConnection_Invalid);

		// What's the state of the connection?
		switch (pInfo->m_info.m_eState)
		{
		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			// Print an appropriate message
			if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
			{
				Printf("Stuck trying to connect: (%s)", pInfo->m_info.m_szEndDebug);
			}
			else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				Printf("Lost connection to host: (%s)", pInfo->m_info.m_szEndDebug);
			}
			else
			{
				Printf("Disconnected from host: (%s)", pInfo->m_info.m_szEndDebug);
			}

			// Clean up the connection.
			m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			m_hConnection = k_HSteamNetConnection_Invalid;
			break;
		}

		case k_ESteamNetworkingConnectionState_Connected:
			Printf("Connected to server OK");
			break;

		default:
			break;
		}
	}
};

//default port for connecting to
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

//takes in a char array of arguments and starts the networked session
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

	// Disable authentication when running with Steam
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
	networkStatus = SERVER_STARTING;
	for (int i = 0; i < 8; i++)
	{
		clientNicknames[0][i] = myNick[i];
	}
	
	startSession("server --port 7777");
}

void StartClient()
{
	networkStatus = CLIENT_STARTING;
	startSession("client 127.0.0.1:7777");
}

void UpdateServer()
{
	//temp flag
	bool shouldUpdateNicknames = false;

	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup(m_hPollGroup, &pIncomingMsg, 1);
		if (numMsgs == 0)
			break;
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

		

		const char* cmd = sCmd.c_str();
		//Printf(cmd);

		

		char* message = (char*)pIncomingMsg->m_pData;
		if (message != nullptr)
		{
			Vector2 thisBulletPos;

			//get the type
			PacketType packetType = (PacketType)message[0];

			switch (packetType)
			{
			case RECEIVED_POSITION_DATA:
				PositionDataPacket incomingPos = DeserializePositionDataPacket(message);
				clientPositions[incomingPos.id] = { incomingPos.posX, incomingPos.posY };
				break;
			case RECEIVED_SET_ID:
				IDDataPacket incomingID = DeserializeIDDataPacket(message);
				myID = incomingID.id;
				break;
			case SEND_NICKNAME_TO_SERVER:
				NicknameDataPacket incomingNick = DeserializeNicknameDataPacket(message);
				for (int i = 0; i < 8; i++)
				{
					clientNicknames[incomingNick.id][i] = incomingNick.nickname[i];
				}
				shouldUpdateNicknames = true;
				break;
			case SEND_NICKNAMES_TO_CLIENT:
				break;
			case REQUEST_BULLET_SPAWN:
				BulletCreationDataPacket incomingRequestBulletPacket = DeserializeBulletCreateDataPacket(message);
				thisBulletPos = { static_cast<float>(incomingRequestBulletPacket.posX), static_cast<float>(incomingRequestBulletPacket.posY) };
				CreateBullet(thisBulletPos, incomingRequestBulletPacket.shouldTravelRight);
				break;
			default:
				break;
			}
		}

		

		// We don't need this anymore.
		pIncomingMsg->Release();
	}

	clientPositions[0] = { myPositionPacket.posX, myPositionPacket.posY };

	//
	// Poll Callbacks
	//

	//send data to clients
	for (auto client : m_Clients)
	{
		//in clientPos, first means its ID and second means its position
		for (auto& clientPos : clientPositions)
		{
			PositionDataPacket curClientPacket;
			curClientPacket.packetType = RECEIVED_POSITION_DATA;
			curClientPacket.id = clientPos.first;
			curClientPacket.posX = clientPos.second.x;
			curClientPacket.posY = clientPos.second.y;

			char serializedPacket[POSITION_PACKET_SIZE];
			SerializePositionDataPacket(curClientPacket, serializedPacket);

			m_pInterface->SendMessageToConnection(client, serializedPacket, POSITION_PACKET_SIZE,
				k_nSteamNetworkingSend_Unreliable, nullptr);
		}

		

		if (shouldUpdateNicknames)
		{
			//send nickname data
			for (auto& clientNick : clientNicknames)
			{
				NicknameDataPacket nickPacket;
				nickPacket.packetType = SEND_NICKNAMES_TO_CLIENT;
				nickPacket.id = clientNick.first;
				//copy nickname data
				for (int i = 0; i < 8; i++)
				{
					nickPacket.nickname[i] = clientNick.second[i];
				}

				char serialNickPacket[NICKNAME_PACKET_SIZE];
				SerializeNicknameDataPacket(nickPacket, serialNickPacket);

				m_pInterface->SendMessageToConnection(client, serialNickPacket, NICKNAME_PACKET_SIZE,
					k_nSteamNetworkingSend_Reliable, nullptr);
			}
		}
		
		
	}

	//send bullet data
	for (int i = 0; i < BULLET_POOL_SIZE; i++)
	{
		if (BulletIsValid(i))
		{
			PositionDataPacket curBulletPacket;
			curBulletPacket.packetType = BULLET_LOCATION;
			curBulletPacket.id = i;
			Vector2 bulPos = GetBulletPosition(i);
			curBulletPacket.posX = static_cast<int>(bulPos.x);
			curBulletPacket.posY = static_cast<int>(bulPos.y);

			char serialBulletPacket[POSITION_PACKET_SIZE];
			SerializePositionDataPacket(curBulletPacket, serialBulletPacket);

			for (auto client : m_Clients)
			{
				m_pInterface->SendMessageToConnection(client, serialBulletPacket, POSITION_PACKET_SIZE,
					k_nSteamNetworkingSend_Unreliable, nullptr);
			}

			
		}

		else if (BulletJustDied(i))
		{
			ResetBulletDiedStatus(i);
			IDDataPacket curBulletDestPacket;
			curBulletDestPacket.packetType = BULLET_DESTROYED;
			curBulletDestPacket.id = i;

			char serialBulletDestPacket[ID_PACKET_SIZE];
			SerializeIDDataPacket(curBulletDestPacket, serialBulletDestPacket);

			for (auto client : m_Clients)
			{
				m_pInterface->SendMessageToConnection(client, serialBulletDestPacket, ID_PACKET_SIZE,
					k_nSteamNetworkingSend_Reliable, nullptr);
			}
		}


	}
	

	m_pInterface->RunCallbacks();

}

void UpdateClient()
{
	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = m_pInterface->ReceiveMessagesOnConnection(m_hConnection, &pIncomingMsg, 1);
		// Nothing? Do nothing.
		if (numMsgs == 0)
			break;
		else if (numMsgs < 0)
			FatalError("Error checking for messages");
		else
		{
			//is this an id packet?
			char* message = (char*)pIncomingMsg->m_pData;
			if (message == nullptr)
			{
				break;
			}

			//get the type
			PacketType packetType = (PacketType)message[0];

			switch (packetType)
			{
			case RECEIVED_POSITION_DATA:
				PositionDataPacket incomingPos = DeserializePositionDataPacket(message);
				clientPositions[incomingPos.id] = { incomingPos.posX, incomingPos.posY };
				break;
			case RECEIVED_SET_ID:
				IDDataPacket incomingID = DeserializeIDDataPacket(message);
				myID = incomingID.id;

				//send the server our nickname
				NicknameDataPacket nickPacket;
				nickPacket.packetType = SEND_NICKNAME_TO_SERVER;
				nickPacket.id = myID;
				for (int i = 0; i < 8; i++)
				{
					nickPacket.nickname[i] = myNick[i];
				}

				char serialNickname[NICKNAME_PACKET_SIZE];
				SerializeNicknameDataPacket(nickPacket, serialNickname);

				m_pInterface->SendMessageToConnection(m_hConnection, serialNickname,
					NICKNAME_PACKET_SIZE, k_nSteamNetworkingSend_Reliable, nullptr);
				break;
			case SEND_NICKNAME_TO_SERVER:
				break;
			case SEND_NICKNAMES_TO_CLIENT:
				NicknameDataPacket incomingNick = DeserializeNicknameDataPacket(message);
				for (int i = 0; i < 8; i++)
				{
					clientNicknames[incomingNick.id][i] = incomingNick.nickname[i];
				}
				break;
			case BULLET_LOCATION:
				PositionDataPacket incomingBullet = DeserializePositionDataPacket(message);
				AddBulletToArray(incomingBullet.id, incomingBullet.posX, incomingBullet.posY);
				break;
			case BULLET_DESTROYED:
				IDDataPacket incomingDestBullet = DeserializeIDDataPacket(message);
				RemoveBulletFromArray(incomingDestBullet.id);
				break;
			case SET_POSITION:
				IDDataPacket incomingSetPosPacket = DeserializeIDDataPacket(message);
				if (GetMyID() == incomingSetPosPacket.id)
				{
					ResetMyPlayerPosition();
				}
				break;
			case PLAYER_DISCONNECTED:
				IDDataPacket incomingPacket = DeserializeIDDataPacket(message);
				clientPositions[incomingPacket.id] = { 0, 0 };
				break;
			default:
				break;
			}

			pIncomingMsg->Release();
		}
	}

	m_pInterface->RunCallbacks();

	//send the server my position
	char serialPacket[POSITION_PACKET_SIZE];
	SerializePositionDataPacket(myPositionPacket, serialPacket);

	m_pInterface->SendMessageToConnection(m_hConnection, serialPacket,
		POSITION_PACKET_SIZE, k_nSteamNetworkingSend_Unreliable, nullptr);

}

void CloseServer()
{
	networkStatus = INACTIVE;
	// Close all the connections
	Printf("Closing connections...\n");
	for (auto it : m_Clients)
	{
		//shut down each client
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

	// wait for half a second
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	delete myServer;

#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	GameNetworkingSockets_Kill();
#else
	SteamDatagramClient_Kill();
#endif

	// kill the process
	NukeProcess(0);
}

void CloseClient()
{
	networkStatus = INACTIVE;
	// Close the connection gracefully.
	m_pInterface->CloseConnection(m_hConnection, 0, "Goodbye", true);

	//
	// Shutdown
	//

	// wait for half a second
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	delete myClient;

#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	GameNetworkingSockets_Kill();
#else
	SteamDatagramClient_Kill();
#endif

	// kill the session
	NukeProcess(0);
}

//runs every frame
void UpdateNetwork()
{
	switch (networkStatus)
	{
	case SERVER_ACTIVE:
		UpdateServer();
		break;
	case CLIENT_ACTIVE:
		UpdateClient();
		break;
	default:
		break;
	}
}

//cleanly shuts down this network session
void CloseNetwork()
{
	switch (networkStatus)
	{
	case SERVER_ACTIVE:
		CloseServer();
		break;
	case CLIENT_ACTIVE:
		CloseServer();
		break;
	default:
		break;
	}
}

int GetClientCount()
{
	//return m_Clients.size();
	return clientPositions.size();
}

Vector2Int GetClientPosition(int clientID)
{
	//make sure client id is valid
	if (clientID > GetClientCount())
	{
		return NULL_VECTOR;
	}

	return clientPositions[clientID];
}

void ResetClientPosition(int clientID)
{
	//make sure client id is valid
	if (clientID > GetClientCount())
	{
		return;
	}

	//make sure we are the server
	if (!IsServer())
	{
		return;
	}

	//are we resting our posiiton?
	if (clientID == GetMyID())
	{
		ResetMyPlayerPosition();
		return;
	}

	//build the packet
	IDDataPacket myResetPacket;
	myResetPacket.packetType = SET_POSITION;
	myResetPacket.id = clientID;

	//serialise it
	char serializedResetPacket[ID_PACKET_SIZE];
	SerializeIDDataPacket(myResetPacket, serializedResetPacket);

	//send it off to the clients
	for (auto client : m_Clients)
	{
		m_pInterface->SendMessageToConnection(client, serializedResetPacket, ID_PACKET_SIZE,
			k_nSteamNetworkingSend_Reliable, nullptr);
	}
	
}

enum NetworkStatus GetNetworkStatus()
{
	return networkStatus;
}

int GetMyID()
{
	return myID;
}

char* GetNickname()
{
	return myNick;
}

char* GetClientNickname(int clientID)
{
	return clientNicknames[clientID];
}

void SetNickname(char* inNick)
{
	for (int i = 0; i < 8; i++)
	{
		myNick[i] = inNick[i];
	}
}

int Vector2Int_IsValid(Vector2Int a)
{
	Vector2Int nullVec = NULL_VECTOR;
	if (a.x == nullVec.x && a.y == nullVec.y)
	{
		return false;
	}
	//i dont like this, but oh well
	if (a.x == 0 && a.y == 0)
	{
		return false;
	}

	return true;
	
}

char GetNextAvailableID()
{
	return nextAvailableID;
}

int IsServer()
{
	if (networkStatus == NetworkStatus::SERVER_ACTIVE
		|| networkStatus == NetworkStatus::SERVER_STARTING)
	{
		return true;
	}

	return false;
}

void CreateBulletOnServer(int posX, int posY, char inShouldTravelRight)
{
	BulletCreationDataPacket myBulletPacket;
	myBulletPacket.packetType = REQUEST_BULLET_SPAWN;
	myBulletPacket.posX = posX;
	myBulletPacket.posY = posY;
	myBulletPacket.shouldTravelRight = inShouldTravelRight;

	char serialPacket[BULLET_CREATION_PACKET_SIZE];
	SerializeBulletCreateDataPacket(myBulletPacket, serialPacket);

	m_pInterface->SendMessageToConnection(m_hConnection, serialPacket,
		BULLET_CREATION_PACKET_SIZE, k_nSteamNetworkingSend_Reliable, nullptr);
}