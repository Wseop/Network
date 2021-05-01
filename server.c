#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

const int BUF_SIZE = 4096;
const int ID_LEN = 100;
const int MAX_CLIENT = 10;

int client_num = 0;
int arr_client_sd[10];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* Broadcast(void* arg) {
	int client_sd = *(int*)arg;
	int ret;

	while (1) {
		char buf[BUF_SIZE + ID_LEN + 1];
		char buf2[BUF_SIZE * 2];
		char* id;
		char* msg;

		ret = recv(client_sd, buf, BUF_SIZE + ID_LEN + 1, 0);
		if (ret < 0) {
			printf("recv error - client %d\n", client_sd);
			break;
		} else if (ret == 0) {
			printf("client %d disconnected\n", client_sd);
			break;
		} else {
			id = strtok(buf, " ");
			msg = strtok(NULL, "");
			pthread_mutex_lock(&mutex);
			sprintf(buf2, "[%s] > %s", id, msg);
			for (int i = 0; i < client_num; i++) {
				if (arr_client_sd[i] == client_sd) continue;
				ret = send(arr_client_sd[i], buf2, strlen(buf2), 0);
				if (ret < 0) {
					printf("send error - client %d -> %d\n", client_sd, arr_client_sd[i]);
				}
			}
			pthread_mutex_unlock(&mutex);
		}
	}

	pthread_mutex_lock(&mutex);
	for (int i = 0; i < client_num; i++) {
		if (arr_client_sd[i] == client_sd) {
			for (int j = i; j < client_num - 1; j++) {
				arr_client_sd[j] = arr_client_sd[j + 1];
			}
		}
	}
	client_num--;
	pthread_mutex_unlock(&mutex);
	close(client_sd);
}

int main(int argc, char* argv[]) {
	int server_sd, client_sd;
	int ret, on = 1;
	int server_port;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len;

	for (int i = 0; i < MAX_CLIENT; i++) {
		arr_client_sd[i] = -1;
	}

	server_port = atoi(argv[1]);
	server_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sd < 0) {
		printf("socket error");
		return -1;
	}
	ret = setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	if (ret < 0) {
		printf("setsockopt error");
		return -1;
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);
	ret = bind(server_sd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		printf("bind error");
		return -1;
	}
	ret = listen(server_sd, SOMAXCONN);
	if (ret < 0) {
		printf("listen error");
		return -1;
	}
	printf("Wait connection...\n");

	while (1) {
		pthread_t tid;

		addr_len = sizeof(client_addr);
		client_sd = accept(server_sd, (struct sockaddr*)&client_addr, &addr_len);
		
		if (client_sd < 0) {
			printf("accept error\n");
			continue;
		}

		if (client_num >= MAX_CLIENT) {
			printf("Accept Denied (MAX_CLIENT Over)\n");
			close(client_sd);
		} else {
			pthread_mutex_lock(&mutex);
			printf("Client %d connected\n", client_sd);
			printf("IP = %s, PORT = %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

			arr_client_sd[client_num++] = client_sd;
			pthread_mutex_unlock(&mutex);

			pthread_create(&tid, NULL, Broadcast, (void*)(&client_sd));
		}
	}

	return 0;
}
