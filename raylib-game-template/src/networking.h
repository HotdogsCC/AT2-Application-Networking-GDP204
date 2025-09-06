typedef struct Vector2Int
{
	int x;
	int y;
} Vector2Int;
typedef struct DataPacket
{
	char id;
	int posX;
	int posY;
} DataPacket;


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

#ifdef __cplusplus
}
#endif