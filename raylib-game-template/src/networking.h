#pragma once

typedef struct Vector2Int
{
	int x;
	int y;
} Vector2Int;

#define NULL_VECTOR {-999, -999}

#ifdef __cplusplus
extern "C" {
#endif

enum NetworkStatus
{
	INACTIVE,
	SERVER_STARTING,
	SERVER_ACTIVE,
	CLIENT_STARTING,
	CLIENT_ACTIVE
};

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

typedef struct PositionDataPacket
{
	enum PacketType packetType;
	char id;
	int posX;
	int posY;
} PositionDataPacket;

typedef struct IDDataPacket
{
	enum PacketType packetType;
	char id;
} IDDataPacket;

typedef struct NicknameDataPacket
{
	enum PacketType packetType;
	char id;
	char nickname[8];
} NicknameDataPacket;

typedef struct BulletCreationDataPack
{
	enum PacketType packetType;
	int posX;
	int posY; 
	char shouldTravelRight;
} BulletCreationDataPack;

typedef struct BulletDataPack
{
	enum PacketType packetType;
	char id;
	int posX;
	int posY; 
} BulletDataPack;

	//called when game scene is started
	void StartServer();
	void StartClient();

	//called in main update loop
	void UpdateNetwork();
	void CloseNetwork();

	//called in screen_gameplay
	void UpdatePacketPosition(int posX, int posY);
	int GetClientCount();
	char GetNextAvailableID();
	Vector2Int GetClientPosition(int clientID);
	enum NetworkStatus GetNetworkStatus();
	int GetMyID();
	int IsServer();

	char* GetNickname();
	void SetNickname(char* inNick);
	char* GetClientNickname(int clientID);

	void CreateBulletOnServer(int posX, int posY, char inShouldTravelRight);

	int Vector2Int_IsValid(Vector2Int a);

#ifdef __cplusplus
}
#endif

