#pragma once

//vector2 type for integers
typedef struct Vector2Int
{
	int x;
	int y;
} Vector2Int;

//used to make vectors nullable
#define NULL_VECTOR {-999, -999}

#ifdef __cplusplus
extern "C" {
#endif

//the current status of the network
enum NetworkStatus
{
	INACTIVE,
	SERVER_STARTING,
	SERVER_ACTIVE,
	CLIENT_STARTING,
	CLIENT_ACTIVE
};

//the type of data packet being sent/receieved
enum PacketType {
	RECEIVED_POSITION_DATA,
	RECEIVED_SET_ID,
	SEND_NICKNAME_TO_SERVER,
	SEND_NICKNAMES_TO_CLIENT,
	REQUEST_BULLET_SPAWN,
	BULLET_LOCATION,
	BULLET_DESTROYED,
	PLAYER_DISCONNECTED

};

//contains positional and ID data
typedef struct PositionDataPacket
{
	enum PacketType packetType;
	char id;
	int posX;
	int posY;
} PositionDataPacket;

//contains ID data
typedef struct IDDataPacket
{
	enum PacketType packetType;
	char id;
} IDDataPacket;

//contains nickname and ID data
typedef struct NicknameDataPacket
{
	enum PacketType packetType;
	char id;
	char nickname[8];
} NicknameDataPacket;

//contains positional and travel directional data
typedef struct BulletCreationDataPacket
{
	enum PacketType packetType;
	int posX;
	int posY; 
	char shouldTravelRight;
} BulletCreationDataPacket;

	//initalises the server
	void StartServer();
	//initalises the client
	void StartClient();

	//called every frame to refresh network
	void UpdateNetwork();

	//shuts down network cleanly
	void CloseNetwork();

	//called every frame to tell the network where this player is
	void UpdatePacketPosition(int posX, int posY);

	//returns the amount of clients connected to the session
	int GetClientCount();

	//returns the next available ID for clients
	char GetNextAvailableID();

	//returns the client position given an ID
	Vector2Int GetClientPosition(int clientID);

	//returns the network status of this session
	enum NetworkStatus GetNetworkStatus();

	//returns the network ID of this client
	int GetMyID();

	//returns if this session is a server
	int IsServer();

	//returns the nickname of this session
	char* GetNickname();

	//sets the nickname of this session
	void SetNickname(char* inNick);

	//returns the nickname of a particular client given their ID
	char* GetClientNickname(int clientID);

	//sends a request to the server to spawn a bullet
	void CreateBulletOnServer(int posX, int posY, char inShouldTravelRight);

	//returns if a given vector is valid; is it not 'null'?
	int Vector2Int_IsValid(Vector2Int a);

#ifdef __cplusplus
}
#endif

