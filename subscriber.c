#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>

#include "common.h"
#include "helpers.h"

void run_tcp_client(int sockfd)
{
	char buf[MSG_MAXSIZE + 1] = {};

	messages received_message;
	messages send_message;
	struct pollfd poll_fds[2];
	int num_clients = 0;

	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	num_clients++;

	poll_fds[1].fd = sockfd;
	poll_fds[1].events = POLLIN;
	num_clients++;
	while (1) {
		int rc = poll(poll_fds, num_clients, -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < num_clients; i++) {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == STDIN_FILENO) {
					memset(buf, 0, MSG_MAXSIZE + 1);
					rc = read(STDIN_FILENO, buf, sizeof(buf));
					DIE(rc < 0, "read failed!\n");

					buf[strlen(buf) - 1] = '\0';

					if(strcmp(buf, "exit") == 0) {
						return;
					} else {
						char action[20];
						char topic[50];
						int sf;
						sscanf(buf, "%s%s%d", action, topic, &sf);
						if(strcmp(action, "subscribe") == 0) {
							memset(&send_message, 0, sizeof(messages));
							send_message.type_of_messages = 1;
							memcpy(send_message.client_action.action, action, strlen(action) + 1);
							memcpy(send_message.client_action.topic.topic, topic, strlen(topic) + 1);
							send_message.client_action.topic.sf = sf;
							send_all(sockfd, &send_message, sizeof(messages));
							printf("Subscribed to topic.\n");
						} else {
							memset(&send_message, 0, sizeof(messages));
							send_message.type_of_messages = 1;
							memcpy(send_message.client_action.action, action, strlen(action) + 1);
							memcpy(send_message.client_action.topic.topic, topic, strlen(topic) + 1);
							send_all(sockfd, &send_message, sizeof(messages));
							printf("Unsubscribed from topic.\n");
						}
						
					}
				} else if (poll_fds[i].fd == sockfd) {
					rc = recv_all(sockfd, &received_message, sizeof(received_message));
					DIE(rc < 0, "recv_all\n");

					switch (received_message.type_of_messages)
					{
					case 2:
						return;
					case 3:
						if(received_message.packet.mesj.type == 0) {
							
							uint8_t semn = (u_int8_t)received_message.packet.mesj.content[0];
							int val = ntohl(*(int *)(received_message.packet.mesj.content + 1));
							if(semn == 1) {
								val = (-1) * val;
							}
							printf("%s:%d - %s - INT - %d\n", received_message.packet.ip, 
											received_message.packet.port, 
											received_message.packet.mesj.topic,
											val);
						}
						if(received_message.packet.mesj.type == 1) {
							float val = ntohs(*(u_int16_t *)received_message.packet.mesj.content);
							printf("%s:%d - %s - SHORT_REAL - %.2f\n", received_message.packet.ip, 
											received_message.packet.port, 
											received_message.packet.mesj.topic,
											val / 100);
							
						}
						if(received_message.packet.mesj.type == 2) {

							uint8_t semn = (u_int8_t)received_message.packet.mesj.content[0];
							float val = ntohl(*(u_int32_t *)(received_message.packet.mesj.content + 1));
							u_int8_t putere = *(u_int8_t *)(received_message.packet.mesj.content + 5);

							double imp = pow(10, putere);
							if(semn == 1) {
								val = (-1) * val;
							}
							printf("%s:%d - %s - FLOAT - %.4f\n", received_message.packet.ip, 
											received_message.packet.port, 
											received_message.packet.mesj.topic,
											val / imp);



						}
						if(received_message.packet.mesj.type == 3) {
							printf("%s:%d - %s - STRING - %s\n", received_message.packet.ip, 
											received_message.packet.port, 
											received_message.packet.mesj.topic,
											received_message.packet.mesj.content);
						}

						break;
					default:
						printf("Comanda nepermisa\n");
						break;
					}
					
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int sockfd = -1;
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	if (argc != 4) {
		printf("\n Usage: %s <id> <ip> <port>\n", argv[0]);
		return 1;
	}

	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");


	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");


	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	messages initial_message;

	memset(&initial_message, 0, sizeof(messages));
	initial_message.type_of_messages = 0;

	client_tcp_info client;
	client.clientFD = -1;
	memcpy(client.id, argv[1], strlen(argv[1]) + 1);
	

	initial_message.info_client = client;
	send_all(sockfd, &initial_message, sizeof(initial_message));

	run_tcp_client(sockfd);

	close(sockfd);

	return 0;
}