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

int sd;
char id[100];

void* SendThread(void* arg) {
	int ret;

	while (1) {
		char buf[BUF_SIZE];
		char send_buf[BUF_SIZE + ID_LEN + 1];

		fgets(buf, BUF_SIZE, stdin);
		if (!strcmp(buf, "q\n")) {
			return (void*)0;
		}
		sprintf(send_buf, "%s %s", id, buf);
		ret = send(sd, send_buf, strlen(send_buf), 0);
		if (ret < 0) {
			printf("send error\n");
			return (void*)0;
		}
	}
}

void* RecvThread(void* arg) {
	char buf[BUF_SIZE * 2];
	int ret;

	while (1) {
		ret = recv(sd, buf, BUF_SIZE * 2, 0);
		if (ret < 0) {
			printf("recv error\n");
			return (void*)0;
		} else if (ret == 0) {
			printf("server closed\n");
			//return (void*)0;
			exit(0);
		} else {
			buf[ret] = 0;
			fputs(buf, stdout);
		}
	}
}

int main(int argc, char* argv[]) {
	int ret;
	int server_port;
	struct sockaddr_in addr;
	pthread_t tid_send, tid_recv;

	server_port = atoi(argv[2]);
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		printf("socket error\n");
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	ret = inet_pton(AF_INET, argv[1], &addr.sin_addr);
	if (ret <= 0) {
		printf("inet_pton error\n");
		return -1;
	}
	printf("Connecting to server...\n");
	ret = connect(sd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		printf("connect error\n");
		return -1;
	}
	printf("Connect Success\n");
	strcpy(id, argv[3]);
	
	pthread_create(&tid_send, NULL, SendThread, NULL);
	pthread_create(&tid_recv, NULL, RecvThread, NULL);

	pthread_join(tid_send, NULL);

	close(sd);

	return 0;
}
