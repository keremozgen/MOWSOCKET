# MOWSOCKET
MOW SOCKET is a cross platform, easy to use and a basic socket library. Currently only supports IPv4. It is a basic header file you can
include it and start using. You don't need to use CMakeFile.txt or fips build tool. Only on android you may need to show ifaddrs-android.c ifaddrs-android.c as source files too.
Here some functions are explained:
	
	struct mowadapter* m_get_adapters() You can get all network interfaces
	
	void m_clean_adapters(struct mowadapter** adapter) with this function you can eliminate interfaces that are not connected.
	
	struct mowsocket* msocket(int domain, int type, int action, uint32_t h_address, uint16_t h_port)
	Domain can be MOW_IP4 or MOW_IP6 but MOW_IP6 not supported yet.
	Type can be MOW_UDP or MOW_TCP
	Action can be MOW_SEND or MOW_LISTEN. MOW_LISTEN for creating a listening socket. But a listening socket can send packets too.
	Address and port are in the host order, can be used from adapter for a specific interface or INADDR_ANY, INADDR_BROADCAST.
   
One thing to note is int* peerd; variable in mowsocket struct is not used currently. After accept use the returned socket descriptor and
close the accepted socket with the returned descriptor.
