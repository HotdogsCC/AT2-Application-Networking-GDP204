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
	void StartServer();
	void StartClient();

	void UpdateNetwork();
	void CloseNetwork();

#ifdef __cplusplus
}
#endif