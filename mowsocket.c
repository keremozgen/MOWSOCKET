#include <time.h>
#include <zconf.h>
#include <sys/time.h>
#include "mowsocket.h"

int print_adapters(struct mowadapter* adapters);
void print_adapter(struct mowadapter* adapters);
void print_adapter(struct mowadapter* adapters) {
	if (adapters) {
			char address[16];
			char broadcast[16];
			char netmask[16];
			uint32_t n_address = htonl(adapters->h_address),
					n_broadcast = htonl(adapters->h_broadcast), n_netmask = htonl(adapters->h_netmask);
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
	}
}

int print_adapters(struct mowadapter* adapters) {
	int ret = 0;
	if (adapters) {
		while (NULL != adapters) {
			ret++;
			char address[16];
			char broadcast[16];
			char netmask[16];
			uint32_t n_address = htonl(adapters->h_address),
				n_broadcast = htonl(adapters->h_broadcast), n_netmask = htonl(adapters->h_netmask);
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
			adapters = adapters->next;
		}
	}
	return ret;
}

void shuffle_adapters(struct mowadapter **adapters){
	struct mowadapter **alladapters = NULL;
	int count = 0;
	struct mowadapter *t = *adapters;
	while(t){
		count++;
		alladapters = realloc(alladapters, count * sizeof(struct mowadapter*));
		alladapters[count - 1] = t;
		t = t->next;
	}
	int *index = calloc(count,sizeof(int));
	srand(index);

	for (int i = 0; i < count; ++i) {
		int t;
		int r;
		if(i + 1 == count){
			int rem = 0;
			for (int j = 0; j < count - 1; ++j) {
				rem += index[j];
			}
			index[i] = ((count * (count - 1))/2) - rem;
			continue;
		}
		LBL:
		t = 0;
		r = (int) (((float)rand()/(float)RAND_MAX) * ((float)count));
		for (int j = 0; j < i; ++j) {
			if(r == index[j]) t++;
		}
		if(t == 1) goto LBL;
		else if(t > 1){ printf("!!!Error\n"); return;}
		else index[i] = r;
	}

	*adapters = alladapters[index[0]];
	struct mowadapter *tmp_adapter = *adapters;
	for (int k = 0; k < count; ++k) {
		if(k + 1 == count){
			alladapters[index[k]]->next = NULL;
		}else{
			alladapters[index[k]]->next = alladapters[index[k+1]];
		}
	}
	free(alladapters);
	free(index);
}

#ifdef __ANDROID__
void android_main(struct android_app* app) {
#else
int main() {
#endif
#if defined(_WIN32) && defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	struct mowadapter* alladapters = m_get_adapters();
	printf("================START================\n");
	int total_adapter_count = print_adapters(alladapters);
	printf("================END OF START================\n");
	for (int i = 0; i < 10; ++i) {
		if (alladapters) {
			shuffle_adapters(&alladapters);
			if(total_adapter_count != print_adapters(alladapters)){
				printf("Adapter count changed\n");
			}
		}
	}
	if(NULL != alladapters) m_free_adapters(&alladapters);
	alladapters = m_get_adapters();
	if(NULL == alladapters){
	printf("Can't get adapters\n");
	return MOWSOCKETERR;
	}
	m_clean_adapters(&alladapters);

	if(0){
		struct mowsocket *udp_sock = msocket(MOW_IP4,MOW_UDP,MOW_LISTEN,0,3398);
		struct mowsocket *tcp_sock = msocket(MOW_IP4,MOW_TCP,MOW_LISTEN,0,3340);
		//if(MOWSOCKETERR == msetsockpredopt(tcp_sock,MOW_TCP_NODELAY)){printf("Can't set option\n");}
		if(MOWSOCKETERR == msetsockpredopt(tcp_sock,MOW_SO_DONTLINGER)){printf("Can't set option\n");}

		if(NULL != udp_sock && NULL != tcp_sock) {
			char *SERVERMSG = calloc(100,1);
			int SERVERMSG_LEN = 100;
			uint64_t retl = mrecvsu(udp_sock,SERVERMSG,SERVERMSG_LEN);
			if(retl > 0){
				printf("Successfully received udp message %s\n",SERVERMSG);
			}else{
				printf("Can't receive udp message %llu returned\n",retl);
			}
			free(SERVERMSG);
			while(1){
				int r = maccepts(tcp_sock,0,0);
				printf("Accepted peer\n");
				if(MOWSOCKETERR == r){
					printf("Can't accept tcp socket\n");
				}
				SERVERMSG = calloc(100,1);
				retl = mrecvst(tcp_sock,r ,SERVERMSG ,100);
				if(retl > 0){
					printf("%s recevied from tcp socket\n",SERVERMSG);
				}else{
					printf("Cannot receive tcp message\n");
				}

				free(SERVERMSG);
			}
		}else{
			printf("Can't create sockets\n");
			return 0;
		}
		int r = mclose(&udp_sock);
		if(MOWSOCKETERR == r) printf("Can't close\n");
		else printf("Successfully udp closed\n");
		r = mclose(&tcp_sock);
		if(MOWSOCKETERR == r) printf("Can't close\n");
		else printf("Successfully tcp closed\n");
	}else{
		struct mowsocket *udp_sock = msocket(MOW_IP4,MOW_UDP,MOW_SEND,0,3399);
		struct mowsocket *tcp_sock = msocket(MOW_IP4,MOW_TCP,MOW_SEND,0,3341);
		//if(MOWSOCKETERR == msetsockpredopt(tcp_sock,MOW_TCP_NODELAY)){printf("Can't set option\n");}
		if(MOWSOCKETERR == msetsockpredopt(tcp_sock,MOW_SO_DONTLINGER)){printf("Can't set option\n");}

		if(NULL != udp_sock && NULL != tcp_sock) {
			if(MOWSOCKETERR == mconnects(udp_sock,alladapters[0].h_address,3398)){
				printf("Can't connect to peer\n");
			}
			char *SERVERMSG = "SERVERMSG3219900\0";
			int SERVERMSG_LEN = strlen(SERVERMSG);
			uint64_t retl = msends(udp_sock,SERVERMSG,SERVERMSG_LEN);
			if(retl == SERVERMSG_LEN){
				printf("Successfully sent udp message %s to the peer that has connected\n",SERVERMSG);
			}else{
				printf("Can't send udp message to the peer successfully %llu returned\n",retl);
			}

			char *TCPMSG = "SERVERTCPMESSAGETCP\0";
			int tcp_len = strlen(TCPMSG);
			if(MOWSOCKETERR == mconnects(tcp_sock,alladapters[0].h_address,3340)){
				printf("Can't connect to tcp server\n");
			}
			retl = msends(tcp_sock,TCPMSG,tcp_len);
			if(retl != tcp_len){
				printf("Can't send successfully %llu rather than %llu\n",retl,tcp_len);
			}else{
				printf("Successfully sent %s tcp message\n",TCPMSG);
			}

		}else{
			printf("Can't create sockets\n");
			return 0;
		}
		int r = mclose(&udp_sock);
		if(MOWSOCKETERR == r) printf("Can't close\n");
		else printf("Successfully udp closed\n");
		r = mclose(&tcp_sock);
		if(MOWSOCKETERR == r) printf("Can't close\n");
		else printf("Successfully tcp closed\n");
	}


	return 0;

	struct mowsocket *sock = msocket(MOW_IP4, MOW_UDP,MOW_SEND,0,3399);
	if(NULL == sock){
	printf("Can't create socket\n");
	return MOWSOCKETERR;
	}
	#if 0
	if(MOWSOCKETERR == msetsockpredopt(sock,MOW_SO_BROADCAST)){
		printf("Can't set sock opt for broadcast\n");
		return MOWSOCKETERR;
	}
	#endif
	char *tmpMessage = "DENEME123\0";
	char *tbuf = calloc(100,1);
	uint32_t p_a;
	uint16_t p_p;
	mrecvfrom(sock->socketd,tbuf,100,&p_a,&p_p);
	printf("%s recvd\n",tbuf);
	printf("Sent\n");
	if(MOWSOCKETERR == mconnects(sock,p_a,p_p)){
		printf("Can't connect to peer that has sent a message\n");
	}else{
		printf("Connected to peer that has sent a message\n");
		printf("%llu bytes sent\n",msends(sock,tbuf,100));

	}
	free(tbuf);
	if(MOWSOCKETERR == maccepts(sock,&p_a,&p_p)){
		printf("Can't accept\n");
	}else {
		printf("Accepted\n");
	}
	for (int j = 0; j < 0; ++j) {
		char *tbuf = calloc(100,1);
		//msendto(sock->socketd,tmpMessage,strlen(tmpMessage),alladapters->next->h_address,8888);
		uint32_t p_a;
		uint16_t p_p;
		mrecvfrom(sock->socketd,tbuf,100,&p_a,&p_p);
		printf("%s recvd\n",tbuf);
		printf("Sent\n");
		if(MOWSOCKETERR == mconnects(sock,p_a,p_p)){
			printf("Can't connect to peer that has sent a message\n");
		}else{
			printf("Connected to peer that has sent a message\n");
			int r = mclose(&sock);
			if(MOWSOCKETERR == r) printf("Can't close\n");
			else printf("Successfully closed\n");
		}
		free(tbuf);
		tbuf = calloc(100,1);
		mrecvs(sock,tbuf,100);
		printf("%s recvs\n",tbuf);
		free(tbuf);

	}

	int r = mclose(&sock);
	if(MOWSOCKETERR == r) printf("Can't close\n");
	else printf("Successfully closed\n");


	if(NULL != alladapters) m_free_adapters(&alladapters);
	printf("Exiting!\n");
#if defined(_WIN32) && defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif
return 0;
}
