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
	SEND_NICKNAME,
	RECEIVED_NICKNAME

};

typedef struct PositionDataPacket
{
	enum PacketType packetType;
	char id;
	int posX;
	int posY;
} PositionDataPacket;

typedef struct SetIDDataPacket
{
	enum PacketType packetType;
	char id;
} SetIDDataPacket;

typedef struct NicknameDataPacket
{
	enum PacketType packetType;
	char id;
	char nickname[8];
} NicknameDataPacket;

	//called when game scene is started
	void StartServer();
	void StartClient();

	//called in main update loop
	void UpdateNetwork();
	void CloseNetwork();

	//called in screen_gameplay
	void UpdatePacketPosition(int posX, int posY);
	int GetClientCount();
	Vector2Int GetClientPosition(int clientID);
	enum NetworkStatus GetNetworkStatus();
	int GetMyID();

	void SetNickname(char* inNick);

	int Vector2Int_IsValid(Vector2Int a);

#ifdef __cplusplus
}
#endif

