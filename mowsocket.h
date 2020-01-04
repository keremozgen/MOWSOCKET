//GLOBAL INCLUDES HERE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//GENERAL DEFINES HERE
#define MOWSOCKETOK 1
#define MOWSOCKETERR -1

#define MOW_UDP 1
#define MOW_TCP 2

#define MOW_SEND 1
#define MOW_LISTEN 2

#define MOW_IP4 1
#define MOW_IP6 2

//PREDEFINED SOCKET OPTIONS FOR MOSTLY USED OPTIONS
#define MOW_SO_BROADCAST 1
#define MOW_SO_REUSEADDR 2
#define MOW_SO_DONTLINGER 3
#define MOW_TCP_NODELAY 4

#ifdef _WIN32
char MSOCKET_WIN_ERR_BUFFER[1024] = {0};
int MSOCKET_WIN_ERR_CODE = 0;
#define MOW_SOCKET_STRERROR() MSOCKET_WIN_ERR_CODE = WSAGetLastError();\
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL,MSOCKET_WIN_ERR_CODE, \
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), MSOCKET_WIN_ERR_BUFFER,sizeof(MSOCKET_WIN_ERR_BUFFER),NULL);\
	printf("\nERROR CODE: %d %s %s %d\n", MSOCKET_WIN_ERR_CODE, MSOCKET_WIN_ERR_BUFFER,__FILE__,__LINE__);\
MSOCKET_WIN_ERR_CODE = 0; *MSOCKET_WIN_ERR_BUFFER = 0
#else
#define MOW_SOCKET_STRERROR() printf("\n%s %s %d\n",strerror(errno),__FILE__,__LINE__)
#endif

//GENERAL DEBUG DEFINES HERE
#ifdef _DEBUG
#define MOW_SOCKET_ERROR(x) printf("\nERROR %s %d: %s\n",__FILE__,__LINE__,x);

#else
#define MOW_SOCKET_ERROR(x) ((void)0)
#endif


#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <io.h>
#include <Windows.h>
//#include <stdlib.h>
#pragma comment(lib, "IPHLPAPI.lib")

#ifdef _DEBUG


#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#else

#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h> //TODO:(kerem) check for if windows needs string.h too  --THIS IS HERE FOR STRERROR AND CURRENTLY NO NEED FOR STRERROR IN WINDOWS
#include <netinet/tcp.h>
#endif



//IF ANDROID INCLUDE ANDROID ONLY LIBRARIES
#if defined(__ANDROID__) && !defined(ANDROIDPRINT)
#define ANDROIDPRINT
#include <jni.h>
#include <android/log.h>
#include "android/android_native_app_glue.h"

//ANDROID ONLY DEFINES HERE


#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "MOW", __VA_ARGS__);
#endif


//DEFINE MOWFILE VARIABLES HERE

#ifdef _WIN32
int MOWSOCKET_WIN_WSA_STARTUP = -1;
#endif

struct mowsocket {
	int64_t socketd;
	int *peerd;
	int domain;
	int type;
	int action;
	uint32_t h_address;
	uint32_t h_port;
};

struct mowadapter {    //ALL IN HOST ORDER
	uint32_t h_address;
	uint32_t h_netmask;
	uint32_t h_broadcast;
	struct mowadapter *next;
};


//DEFINE MOWFILE FUNCTIONS HERE
struct mowadapter *m_get_adapters();

void m_clean_adapters(struct mowadapter **adapter);

int m_free_adapters(struct mowadapter **adapters);

int mclose(struct mowsocket **socket_pointer);

struct mowsocket *msocket(int domain, int type, int action, uint32_t h_address, uint16_t h_port);

uint64_t msend(int sockd, char *data, uint64_t data_len);    //SEND WORKS ONLY IF CONNECTED

uint64_t msends(struct mowsocket *mow_socket, char *data, uint64_t data_len);    //SEND WORKS ONLY IF CONNECTED

uint64_t msendto(int sockd, char *data, uint64_t data_len, uint32_t h_p_address, uint16_t h_p_port);

uint64_t mrecv(int sockd, char *data, uint64_t data_len);    //RECEIVE WORKS ONLY IF CONNECTED

uint64_t mrecvsu(struct mowsocket *mow_udp_socket, char *data, uint64_t data_len);	//RECEIVE WORKS ONLY IF CONNECTED

uint64_t mrecvst(struct mowsocket *mow_tcp_socket, int acceptfd, char *data, uint64_t data_len);	//RECEIVE WORKS ONLY IF CONNECTED

uint64_t mrecvfrom(int sockd, char *data, uint64_t data_len, uint32_t *h_p_address, uint16_t *h_p_port);

int mconnect(int sockd, uint32_t h_p_address, uint16_t h_p_port);

int mconnects(struct mowsocket *mow_socket, uint32_t h_p_address, uint16_t h_p_port);

int64_t maccept(int sockd, uint32_t *h_p_address, uint16_t *h_p_port);

int64_t maccepts(struct mowsocket *mow_socket, uint32_t *h_p_address, uint16_t *h_p_port);

int msetsockopt(int64_t sockfd, int optname, const void *optval, int optlen);

int msetsockpredopt(struct mowsocket *sock, int MOW_OPT_NAME);

//IMPLEMENT MOW FUNCTIONS HERE

int64_t maccept(int sockd, uint32_t *h_p_address, uint16_t *h_p_port){
	struct sockaddr_in sa;
	int sa_len = sizeof(struct sockaddr_in);
	int64_t r = accept(sockd,(struct sockaddr*)&sa,&sa_len);
	if(-1 == r) return MOWSOCKETERR;
	if(NULL != h_p_address) *h_p_address = ntohl(sa.sin_addr.s_addr);
	if(NULL != h_p_port) *h_p_port = ntohs(sa.sin_port);
	return r;
}

int64_t maccepts(struct mowsocket *mow_socket, uint32_t *h_p_address, uint16_t *h_p_port){
	if (NULL == mow_socket) {
		printf("Socket is a null pointer\n");
		return MOWSOCKETERR;
	}
	if(MOW_LISTEN != mow_socket->action){
		printf("Trying to accept on a non listening socket\n");
	}
	if(MOW_TCP != mow_socket->type){
		printf("Unsupported accept operation on UDP socket\n");
	}
	struct sockaddr_in sa;
	int sa_len = sizeof(struct sockaddr_in);
	int64_t r = accept(mow_socket->socketd,(struct sockaddr*)&sa,&sa_len);
	if(-1 == r){
		MOW_SOCKET_STRERROR();
		return MOWSOCKETERR;
	}
	if(NULL != h_p_address) *h_p_address = ntohl(sa.sin_addr.s_addr);
	if(NULL != h_p_port) *h_p_port = ntohs(sa.sin_port);
	return r;
}

int mconnect(int sockd, uint32_t h_p_address, uint16_t h_p_port) {
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(h_p_address);
	sa.sin_port = htons(h_p_port);
	int r = connect(sockd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
	if (0 == r) return MOWSOCKETOK;
	MOW_SOCKET_STRERROR();
	return MOWSOCKETERR;
}

int mconnects(struct mowsocket *mow_socket, uint32_t h_p_address, uint16_t h_p_port) {
	if (NULL == mow_socket) {
		printf("Socket is a null pointer\n");
		return MOWSOCKETERR;
	}
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(h_p_address);
	sa.sin_port = htons(h_p_port);
	int r = connect(mow_socket->socketd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
	if (0 == r) return MOWSOCKETOK;
	MOW_SOCKET_STRERROR();
	return MOWSOCKETERR;
}

uint64_t mrecv(int sockd, char* data, uint64_t data_len) {
	if (NULL == data || 0 == data_len) {
		MOW_SOCKET_ERROR("Data or length is 0\n");
		return 0;
	}
#ifdef _WIN32
	int64_t ret = recv(sockd, data, data_len, 0);
		
#else
	int64_t ret = read(sockd, data, data_len);
#endif
	if (ret < 0) {
		MOW_SOCKET_STRERROR();
		return 0;
	}
	return (uint64_t) ret;
}

uint64_t mrecvst(struct mowsocket *mow_tcp_socket, int acceptfd, char *data, uint64_t data_len){
	if (NULL == data || 0 == data_len) {
		MOW_SOCKET_ERROR("Data or length is 0\n");
		return 0;
	}
	if (NULL == mow_tcp_socket) {
		printf("Socket is a null pointer\n");
		return 0;
	}
	if(acceptfd == mow_tcp_socket->socketd){
		MOW_SOCKET_ERROR("Accepted descriptor is same with socket descriptor\n");
		return 0;
	}
#ifdef _WIN32
	int64_t ret = recv(acceptfd, data, data_len,0);
#else
	int64_t ret = read(acceptfd, data, data_len);
#endif
	if (ret < 0) {
		MOW_SOCKET_STRERROR();
		return 0;
	}
	return (uint64_t) ret;

}

uint64_t mrecvsu(struct mowsocket *mow_udp_socket, char *data, uint64_t data_len) {
	if (NULL == data || 0 == data_len) {
		MOW_SOCKET_ERROR("Data or length is 0\n");
		return 0;
	}
	if (NULL == mow_udp_socket) {
		printf("Socket is a null pointer\n");
		return 0;
	}
	if(MOW_TCP == mow_udp_socket->type){
		MOW_SOCKET_ERROR("Can't receive from TCP socket without accept fd\n");
		return 0;
	}
#ifdef _WIN32
	int64_t ret = recv(mow_udp_socket->socketd, data, data_len,0);
#else
	int64_t ret = read(mow_udp_socket->socketd, data, data_len);
#endif
	if (ret < 0) {
		MOW_SOCKET_STRERROR();
		return 0;
	}
	return (uint64_t) ret;
}

uint64_t mrecvfrom(int sockd, char *data, uint64_t data_len, uint32_t *h_p_address, uint16_t *h_p_port) {
	if (NULL == data || 0 == data_len) {
		MOW_SOCKET_ERROR("Data or length is 0\n");
		return 0;
	}
	struct sockaddr_in sa;
	uint32_t len = sizeof(struct sockaddr_in);
	int64_t ret = recvfrom(sockd, data, data_len, 0, (struct sockaddr*)&sa, &len);
	if (ret < 0) {
		MOW_SOCKET_STRERROR();
		return 0;
	}
	if (NULL != h_p_address) *h_p_address = ntohl(sa.sin_addr.s_addr);
	if (NULL != h_p_port) *h_p_port = ntohs(sa.sin_port);
	return (uint64_t) ret;
}

int msetsockpredopt(struct mowsocket *sock, int MOW_OPT_NAME) {
	if (NULL == sock) {
		printf("msetsockpredopt null socket\n");
		return MOWSOCKETERR;
	}
	int optval = 1;
	if (MOW_SO_BROADCAST == MOW_OPT_NAME && MOW_UDP == sock->type) {
		if (MOWSOCKETOK != msetsockopt(sock->socketd, SO_BROADCAST, &optval, sizeof(int))) {
			MOW_SOCKET_ERROR("MSETSOCKOPT");
			return MOWSOCKETERR;
		} else return MOWSOCKETOK;
	} else if (MOW_SO_REUSEADDR == MOW_OPT_NAME) {
		if (MOWSOCKETOK != msetsockopt(sock->socketd, SO_REUSEADDR, &optval, sizeof(int))) {
			MOW_SOCKET_ERROR("MSETSOCKOPT SO_REUSEADDR");
			return MOWSOCKETERR;
		}
		#ifdef _WIN32
		else return MOWSOCKETOK;
		#else
		if (MOWSOCKETOK != msetsockopt(sock->socketd, SO_REUSEPORT, &optval, sizeof(int))) {
			MOW_SOCKET_ERROR("MSETSOCKOPT SO_REUSEPORT")
			return MOWSOCKETERR;
		}
		#endif
	} else if (MOW_TCP == sock->type && MOW_SO_DONTLINGER == MOW_OPT_NAME) {
		struct linger slinger;
		slinger.l_onoff = 1;
		slinger.l_linger = 0;
		if (MOWSOCKETOK != msetsockopt(sock->socketd, SO_LINGER, &slinger, sizeof(struct linger))) {
			MOW_SOCKET_ERROR("MSETSOCKOPT SO_LINGER");
			return MOWSOCKETERR;
		} else return MOWSOCKETOK;
	} else if(MOW_TCP == sock->type && MOW_TCP_NODELAY == MOW_OPT_NAME){
		if (0 != setsockopt(sock->socketd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int))) {
			MOW_SOCKET_ERROR("MSETSOCKOPT TCP_NODELAY");
			return MOWSOCKETERR;
		} else return MOWSOCKETOK;
	} else {
		printf("Unsupported option name on %s socket msetsockpredopt\n",
			   (sock->type == MOW_UDP) ? "MOW_UDP" : "MOW_TCP");
		return MOWSOCKETERR;
	}
}

int msetsockopt(int64_t sockfd, int optname, const void *optval, int optlen) {
	if (0 != setsockopt(sockfd, SOL_SOCKET, optname, optval, optlen)) {
		MOW_SOCKET_ERROR("MSETSOCKOPT");
		return MOWSOCKETERR;
	} else return MOWSOCKETOK;
}


uint64_t msendto(int sockd, char *data, uint64_t data_len, uint32_t h_p_address, uint16_t h_p_port) {
	if (NULL == data || 0 == data_len) {
		MOW_SOCKET_ERROR("Data or length is 0\n");
		return 0;
	}
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(h_p_address);
	sa.sin_port = htons(h_p_port);
	int send_max = 65507;	//MAX 
	uint64_t total = 0;
#ifdef WIN32
	int ret = 0;	
#else
	ssize_t ret = 0;
#endif
	int optlen = sizeof(int);
	if (data_len > send_max && MOWSOCKETOK != msetsockopt(sockd, SO_SNDBUF, &send_max, optlen)) {
		MOW_SOCKET_ERROR("setsockopt SO_SNDBUF");
		send_max = 1024;
	}
	while (data_len > 0) {
		ret = sendto(sockd, data + total, (data_len > send_max) ? send_max : (data_len), 0, (struct sockaddr*) & sa, sizeof(struct sockaddr_in));
		if (ret < 0) {
			MOW_SOCKET_STRERROR();
			return 0;
		}
		total += ret;
		data_len -= ret;
		printf("\r%llu left\t\t\t ", data_len);
	}
	
	return total;
}

uint64_t msends(struct mowsocket *mow_socket, char *data, uint64_t data_len) {
	if (NULL == data || 0 == data_len) {
		MOW_SOCKET_ERROR("Data or length is 0\n");
		return 0;
	}
	if (NULL == mow_socket) {
		printf("Socket is a null pointer\n");
		return 0;
	}

	int send_max = 65507;	//MAX 
	uint64_t total = 0;
#ifdef WIN32
	int ret = 0;
#else
	ssize_t ret = 0;
#endif
	int optlen = sizeof(int);
	if (data_len > send_max&& MOWSOCKETOK != msetsockopt(mow_socket->socketd, SO_SNDBUF, &send_max, optlen)) {
		MOW_SOCKET_ERROR("setsockopt SO_SNDBUF");
		send_max = 1024;
	}
	while (data_len > 0) {
#ifdef _WIN32
		ret = send(mow_socket->socketd, data + total, (data_len > send_max) ? send_max : (data_len), 0);
#else
		ret = write(mow_socket->socketd, data + total, (data_len > send_max) ? send_max : (data_len));
#endif
		if (ret < 0) {
			MOW_SOCKET_STRERROR();
			return 0;
		}
		total += ret;
		data_len -= ret;
		printf("%d bytes sent\n", ret);
	}

	return total;
}

uint64_t msend(int sockd, char *data, uint64_t data_len) {
	if (NULL == data || 0 == data_len) {
		MOW_SOCKET_ERROR("Data or length is 0\n");
		return 0;
	}
	int send_max = 65507;	//MAX 
	uint64_t total = 0;
#ifdef WIN32
	int ret = 0;
#else
	ssize_t ret = 0;
#endif
	int optlen = sizeof(int);
	if (data_len > send_max&& MOWSOCKETOK != msetsockopt(sockd, SO_SNDBUF, &send_max, optlen)) {
		MOW_SOCKET_ERROR("setsockopt SO_SNDBUF");
		send_max = 1024;
	}
	while (data_len > 0) {
#ifdef _WIN32
		ret = send(sockd, data + total, (data_len > send_max) ? send_max : (data_len), 0);
#else
		ret = write(sockd, data + total, (data_len > send_max) ? send_max : (data_len));
#endif
		if (ret < 0) {
			MOW_SOCKET_STRERROR();
			return 0;
		}
		total += ret;
		data_len -= ret;
	}

	return total;
}

struct mowsocket *msocket(int domain, int type, int action, uint32_t h_address, uint16_t h_port) {
#ifdef _WIN32
	if (0 != MOWSOCKET_WIN_WSA_STARTUP) {
		WSADATA wsaData;
		MOWSOCKET_WIN_WSA_STARTUP = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (MOWSOCKET_WIN_WSA_STARTUP != 0) {
			printf("WSAStartup failed: %d\n", MOWSOCKET_WIN_WSA_STARTUP);
			MOW_SOCKET_STRERROR();
			return NULL;
		}
	}
#endif
	struct mowsocket *ret = NULL;
	int s_domain = 0, s_type = 0, s_protocol = 0;
	//CONTROL PARAMETERS
	if (MOW_IP4 != domain && MOW_IP6 != domain) {
		MOW_SOCKET_ERROR("Unsupported socket domain\n");
		goto MSOCKET_RETURN;
	}
	s_domain = (domain == MOW_IP4) ? AF_INET : AF_INET6;
	if (MOW_TCP != type && MOW_UDP != type) {
		MOW_SOCKET_ERROR("Unsupported socket type\n");
		goto MSOCKET_RETURN;
	}
	s_type = (type == MOW_UDP) ? SOCK_DGRAM : SOCK_STREAM;
	s_protocol = (type == MOW_UDP) ? IPPROTO_UDP : IPPROTO_TCP;
	if (MOW_SEND != action && MOW_LISTEN != action) {
		MOW_SOCKET_ERROR("Unsupported socket action\n");
		goto MSOCKET_RETURN;
	}
	int64_t sd = socket(s_domain, s_type, s_protocol);
	if (sd < 0) {
		MOW_SOCKET_STRERROR();
	}
	ret = calloc(sizeof(struct mowsocket), 1);
	if (NULL == ret) {
		MOW_SOCKET_ERROR("Can't allocate memory for socket structure\n");
		goto MSOCKET_RETURN;
	}
	ret->domain = domain;
	ret->type = type;
	ret->action = action;
	ret->h_address = h_address;
	ret->h_port = h_port;
	ret->socketd = sd;
	//BIND THE SOCKET TO ADDRESS
	if (MOW_IP4 == domain) {
		struct sockaddr_in sa;
		sa.sin_family = s_domain;
		sa.sin_addr.s_addr = htonl(h_address);
		sa.sin_port = htons(h_port);

		if (0 != bind(ret->socketd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in))) {
			MOW_SOCKET_STRERROR();
			goto MSOCKET_FREE_STRUCT;
		}
	} else if (MOW_IP6 == type) {    //NO NEED TO ENDIAN CONVERT BUT CHANGE THE FUNCTION SIGNATURE FOR THIS
		printf("IPv6 addresses are not supported\n");
		goto MSOCKET_FREE_STRUCT;
	}
	if (MOW_LISTEN == action && MOW_TCP == type) {
		if (0 != listen(ret->socketd, 1)) {    //TODO:(kerem) CHANGE THE BACKLOG OR ADD A FUNCTION TO CHANGE IT AFTERWARDS
			MOW_SOCKET_STRERROR();
		}
	}

	goto MSOCKET_RETURN;

	MSOCKET_FREE_STRUCT:
	free(ret);
	ret = NULL;
	MSOCKET_RETURN:
	return ret;
}

int mclose(struct mowsocket **socket_pointer) {
	int ret = MOWSOCKETERR;
	if (NULL == socket_pointer || NULL == *socket_pointer) {
		printf("Trying to close a null socket pointer!\n");
		goto MCLOSE_RET;
	}
	//OTHER FUNCTIONS
	if (MOW_TCP == (*socket_pointer)->type &&
		NULL != (*socket_pointer)->peerd) {    //SHUTDOWN CONNECTION IF THERE ARE PEERS
#ifdef _WIN32
		int shutdown_ret = shutdown((*socket_pointer)->socketd, SD_BOTH);
#else
		int shutdown_ret = shutdown((*socket_pointer)->socketd, SHUT_RDWR);
#endif
		if (0 != shutdown_ret) {
			MOW_SOCKET_STRERROR();
		}
	}
	int close_ret = 0;
	#ifdef _WIN32
	close_ret = closesocket((*socket_pointer)->socketd);
	#else
	close_ret = close((*socket_pointer)->socketd);
	#endif
	if (0 != close_ret) {
		MOW_SOCKET_STRERROR();
		goto MCLOSE_RET;
	}
	ret = MOWSOCKETOK;
	free(*socket_pointer);
	*socket_pointer = NULL;
	MCLOSE_RET:
	return ret;
}

void m_clean_adapters(struct mowadapter **adapter) {
	if (NULL != adapter) {
		struct mowadapter *c_adapter = *adapter, *p_adapter = c_adapter;
		int relocated = 0;
		while (c_adapter) {
			if (c_adapter->h_address == INADDR_LOOPBACK || c_adapter->h_address == INADDR_ANY) {    //IF LOOPBACK
				relocated++;
				if (p_adapter == c_adapter) {
					c_adapter = c_adapter->next;
					free(p_adapter);
					p_adapter = c_adapter;
					*adapter = p_adapter;
				} else {
					p_adapter->next = c_adapter->next;
					free(c_adapter);
					c_adapter = p_adapter->next;
				}
			} else {
				p_adapter = c_adapter;
				c_adapter = c_adapter->next;
			}

		}

	}
}

#ifdef _WIN32
struct mowadapter* m_get_adapters() {
	//MSDN STYLE OF GETTING ADAPTERS
	struct mowadapter* head_adapter = NULL, * c_adapter = NULL, * p_adapter = NULL;
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO*)calloc(sizeof(IP_ADAPTER_INFO), 1);
	assert(pAdapterInfo);
	if (NULL == pAdapterInfo) {
		MOW_SOCKET_ERROR("Can't allocate memory");
		goto MOW_SOCKET_FREE_STRUCT;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)calloc(ulOutBufLen, 1);
		assert(pAdapterInfo);
		if (NULL == pAdapterInfo) {
			MOW_SOCKET_ERROR("Can't allocate memory");
			goto MOW_SOCKET_FREE_STRUCT;
		}
	}

	uint64_t adapter_count = 0;
	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			if (NULL == c_adapter) {
				c_adapter = (struct mowadapter*) calloc(sizeof(struct mowadapter), 1);
				if (NULL == c_adapter) {
					MOW_SOCKET_ERROR("Can't allocate memory for adapter\n");
					assert(c_adapter);
					goto MOW_SOCKET_FREE_STRUCT;
				}
				if (0 == adapter_count) head_adapter = c_adapter;
				else p_adapter->next = c_adapter;
			}
			adapter_count++;
			uint32_t n_addr = 0;
			uint32_t n_mask = 0;
			char tt[16] = "192.168.1.28";
			uint32_t tu;
			inet_pton(AF_INET, tt, &tu);
			tu = htonl(tu);
			tu = htonl(tu);
			char kk[16];
			inet_ntop(AF_INET, &tu, kk, 16);
			switch (InetPtonA(AF_INET, pAdapter->IpAddressList.IpAddress.String, &n_addr)
				| InetPtonA(AF_INET, pAdapter->IpAddressList.IpMask.String, &n_mask))
			{
			case 1:	//NO ERROR
				c_adapter->h_address = ntohl(n_addr);
				c_adapter->h_netmask = ntohl(n_mask);
				c_adapter->h_broadcast = ntohl(n_addr | ~(n_mask));
				p_adapter = c_adapter;
				c_adapter = c_adapter->next;
				break;
			case 0:
				printf("%s %s\n", pAdapter->IpAddressList.IpAddress.String, pAdapter->IpAddressList.IpMask.String);
				MOW_SOCKET_ERROR("Not valid IPV4 address");
				break;
			case -1:
				printf("%d returned\n", WSAGetLastError());
				MOW_SOCKET_ERROR("Error");
				break;
			default:
				break;
			}
			pAdapter = pAdapter->Next;
		}
	}
	else {
		printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
	}

	if (0 == adapter_count) goto MOW_SOCKET_FREE_STRUCT;
	goto MOW_SOCKET_ADAPTERS_RETURN;
MOW_SOCKET_FREE_STRUCT:
	if (NULL != head_adapter) m_free_adapters(head_adapter);
MOW_SOCKET_ADAPTERS_RETURN:
	if (pAdapterInfo)
		free(pAdapterInfo);
	return head_adapter;
}
#else

struct mowadapter *m_get_adapters() {
	struct ifaddrs *ifa = NULL;
	struct ifaddrs *ifad = NULL;

	if (-1 == getifaddrs(&ifad) || NULL == ifad) {  //ON ERROR
		MOW_SOCKET_STRERROR();
		goto MOW_SOCKET_ADAPTERS_RETURN;
	}
	uint32_t n_addr = 0;
	uint32_t n_mask = 0;
	struct sockaddr_in *sadr;
	uint64_t adapter_count = 0;
	ifa = ifad;
	struct mowadapter *head_adapter = NULL, *c_adapter = NULL, *p_adapter = NULL;

	while (ifa) {
		sadr = (struct sockaddr_in *) ifa->ifa_netmask;
		if (sadr && sadr->sin_addr.s_addr != INADDR_ANY) {    //DISCARD NETMASK 0.0.0.0 MAYBE DISCARD LOOPBACK TOO
			if (NULL == c_adapter) {
				c_adapter = (struct mowadapter *) calloc(sizeof(struct mowadapter), 1);
				if (NULL == c_adapter) {
					MOW_SOCKET_ERROR("Can't allocate memory for adapter\n");
					assert(c_adapter);
					goto MOW_SOCKET_FREE_STRUCT;
				}
				if (0 == adapter_count) head_adapter = c_adapter;
				else p_adapter->next = c_adapter;
			}
			adapter_count++;
			n_mask = sadr->sin_addr.s_addr;
			sadr = (struct sockaddr_in *) ifa->ifa_addr;
			n_addr = sadr->sin_addr.s_addr;

			c_adapter->h_address = ntohl(n_addr);
			c_adapter->h_netmask = ntohl(n_mask);
			c_adapter->h_broadcast = ntohl(n_addr | ~(n_mask));
			p_adapter = c_adapter;
			c_adapter = c_adapter->next;
		}
		ifa = ifa->ifa_next;
	}
	if (0 == adapter_count) goto MOW_SOCKET_FREE_STRUCT;
	goto MOW_SOCKET_ADAPTERS_RETURN;
	MOW_SOCKET_FREE_STRUCT:
	if (NULL != head_adapter) m_free_adapters(head_adapter);
	MOW_SOCKET_ADAPTERS_RETURN:
	if (ifad)
		freeifaddrs(ifad);
	return head_adapter;
}

#endif


int m_free_adapters(struct mowadapter **adapters) {
	assert(adapters);
	assert(*adapters);
	if (NULL != adapters && NULL != *adapters) {
		struct mowadapter *c = *adapters;
		while (NULL != c) {
			struct mowadapter *next = c->next;
			free(c);
			c = next;
		}
	}
	*adapters = NULL;
	return 0;
}










