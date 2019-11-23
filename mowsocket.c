#include "mowsocket.h"

void print_adapters(struct mowadapter** adapters);


void print_adapters(struct mowadapter** adapters) {
	if (adapters) {
		uint32_t pos = 0;
		struct mowadapter* adapter = adapters[pos];
		while (adapter) {
			char address[16];
			char broadcast[16];
			char netmask[16];
			uint32_t n_address = htonl(adapter->Address),
				n_broadcast = htonl(adapter->Broadcast), n_netmask = htonl(adapter->Netmask);
			int s = inet_ntop(AF_INET, &n_address, address, 16) && inet_ntop(AF_INET, &n_broadcast, broadcast, 16)
				&& inet_ntop(AF_INET, &n_netmask, netmask, 16);
			switch (s)
			{
			case 0:
				printf("Can't print address\n");
				break;
			default:
				printf("--------------------------\n");
				printf("Address: %s\nBroadcast: %s\nNetmask: %s\n", address, broadcast, netmask);
				break;
			}
			adapter = adapters[++pos];
		}
	}
}

#ifdef __ANDROID__
void android_main(struct android_app* app) {
#else
void main() {
#endif
#if defined(_WIN32) && defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	struct mowadapter** alladapters = m_get_adapters();
	if (alladapters) {
		print_adapters(alladapters);
		m_free_adapters(alladapters);
	}

#if defined(_WIN32) && defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif
}