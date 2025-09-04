#ifdef __cplusplus
extern "C" {
#endif

	int StartMyNetworkedSession();

	void Printf(const char* fmt, ...);
	void FatalError(const char* fmt, ...);

#ifdef __cplusplus
}
#endif