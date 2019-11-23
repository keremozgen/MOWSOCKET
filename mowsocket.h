//GLOBAL INCLUDES HERE
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

//GENERAL DEFINES HERE
#define MOWSOCKETOK 1
#define MOWSOCKETERR 0

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

struct mowadapter {	//ALL IN HOST ORDER
	uint32_t Address;
	uint32_t Netmask;
	uint32_t Broadcast;
};


//DEFINE MOWFILE FUNCTIONS HERE
struct mowadapter** m_get_adapters();

int m_free_adapters();


//IMPLEMENT MOW FUNCTIONS HERE
#ifdef _WIN32
struct mowadapter** m_get_adapters() {
	//MSDN STYLE OF GETTING ADAPTERS
	struct mowadapter** adapters = NULL;
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
			adapter_count++;
			struct mowadapter** t = realloc(adapters, (adapter_count + 1) * sizeof(struct mowadapter**));
			assert(t);
			if (NULL == t) {
				MOW_SOCKET_ERROR("Can't reallocate memory");
				goto MOW_SOCKET_FREE_STRUCT;
			}
			struct mowadapter* adapter = NULL;
			uint32_t n_addr = 0;
			uint32_t n_mask = 0;
			char tt[16] = "192.168.1.28";
			uint32_t tu;
			inet_pton(AF_INET, tt, &tu);
			tu = htonl(tu);
			tu = htonl(tu);
			char kk[16];
			inet_ntop(AF_INET, &tu, kk, 16);
			//printf("%s\n", kk);


			switch (InetPtonA(AF_INET, pAdapter->IpAddressList.IpAddress.String, &n_addr)
				| InetPtonA(AF_INET, pAdapter->IpAddressList.IpMask.String, &n_mask))
			{
			case 1:	//NO ERROR
				adapter = calloc(sizeof(struct mowadapter), 1);
				//assert(adapter);
				if (NULL == adapter) {
					MOW_SOCKET_ERROR("Can't reallocate memory");
					goto MOW_SOCKET_FREE_STRUCT;
				}
				adapters = t;
				adapters[adapter_count - 1] = adapter;
				adapters[adapter_count] = NULL;
				adapter->Address = ntohl(n_addr);
				adapter->Netmask = ntohl(n_mask);
				adapter->Broadcast = ntohl(n_addr | ~(n_mask));
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
	goto MOW_SOCKET_ADAPTERS_RETURN;
MOW_SOCKET_FREE_STRUCT:
	if (NULL != adapters) m_free_adapters(adapters);
MOW_SOCKET_ADAPTERS_RETURN:
	if (pAdapterInfo)
		free(pAdapterInfo);
	return adapters;


}
#else
struct mowadapter** m_get_adapters() {

}
#endif



int m_free_adapters(struct mowadapter** adapters) {
	assert(adapters);
	if (NULL != adapters) {
		int index = 0;
		struct mowadapter* t = adapters[index];
		while (NULL != t) {
			free(t);
			t = adapters[++index];
		}
		free(adapters);
	}
	return 0;
}










