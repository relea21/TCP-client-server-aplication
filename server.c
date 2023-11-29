#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 100

void process_messages(messages messages_received, 
	client_tcp_info clients_logged[MAX_CONNECTIONS],
	int *num_clients_logged, struct pollfd poll_fds[MAX_CONNECTIONS],
	int num_clients, int curr,
	struct sockaddr_in cli_addresses[MAX_CONNECTIONS])
{
	u_int8_t sw;
	int rec_user;
	switch (messages_received.type_of_messages) {
		case 0:
			sw = 1;
			rec_user = -1;
			for(int i = 0; i < (*num_clients_logged); i++) {
				if(strcmp(clients_logged[i].id, messages_received.info_client.id) == 0) {
					if(clients_logged[i].is_conected == 1) {
						sw = 0;
					} else {
						rec_user = i;
					}
				}		
			}
			if(sw == 1) {
				if(rec_user == -1) {
					memcpy(clients_logged[*num_clients_logged].id, 
									messages_received.info_client.id,	
					 		strlen(messages_received.info_client.id) + 1);
					clients_logged[*num_clients_logged].is_conected = 1;
					clients_logged[*num_clients_logged].clientFD = poll_fds[curr].fd;
					clients_logged[*num_clients_logged].nr_topics = 0;

					memcpy(clients_logged[*num_clients_logged].ip,
						 inet_ntoa(cli_addresses[curr].sin_addr),
						  strlen(inet_ntoa(cli_addresses[curr].sin_addr)) + 1);
					clients_logged[*num_clients_logged].port = ntohs(cli_addresses[curr].sin_port);

					printf("New client %s connected from %s:%d\n",
				 			clients_logged[*num_clients_logged].id,
							clients_logged[*num_clients_logged].ip,
							clients_logged[*num_clients_logged].port);
					(*num_clients_logged)++;	
				} else {
					clients_logged[rec_user].is_conected = 1;
					clients_logged[rec_user].clientFD = poll_fds[curr].fd;
					memcpy(clients_logged[rec_user].ip,
						 inet_ntoa(cli_addresses[curr].sin_addr),
						  strlen(inet_ntoa(cli_addresses[curr].sin_addr)) + 1);
					clients_logged[rec_user].port = ntohs(cli_addresses[curr].sin_port);
					printf("New client %s connected from %s:%d\n",
				 					clients_logged[rec_user].id,
						clients_logged[rec_user].ip, clients_logged[rec_user].port);
				}

			} else {
				printf("Client %s already connected.\n", messages_received.info_client.id);
				messages send_message;
				send_message.type_of_messages = 2;
				memset(&send_message.client_action, 0, sizeof(client_action));
				memset(&send_message.info_client, 0, sizeof(client_tcp_info));
				send_all(poll_fds[curr].fd, &send_message, sizeof(send_message));
			}
			break;

		case 1:
			for(int i = 0; i < *num_clients_logged; i++) {
				if(clients_logged[i].clientFD == poll_fds[curr].fd) {
					if(strcmp(messages_received.client_action.action,
									 					"subscribe") == 0) {
									
						int nr_top = clients_logged[i].nr_topics;
						char topic[50];
						memcpy(topic, messages_received.client_action.topic.topic,
								 strlen(messages_received.client_action.topic.topic) + 1);								
						memcpy(clients_logged[i].subscribed[nr_top].topic, topic, strlen(topic));
						clients_logged[i].subscribed[nr_top].sf =
													 messages_received.client_action.topic.sf;							
						clients_logged[i].nr_topics++;
					} else {
						int nr_topic;
						for(int j = 0; j < clients_logged[i].nr_topics; j++) {
							if(strcmp(clients_logged[i].subscribed[j].topic,
										 messages_received.client_action.topic.topic) == 0) {
								nr_topic = j;
								break;
							}
						}

						for(int j = nr_topic; j < clients_logged[i].nr_topics - 1; j++) {
							clients_logged[i].subscribed[j] = clients_logged[i].subscribed[j + 1];
						}

						clients_logged[i].nr_topics--;
					}
					
				}
			}
			break;	
		default:
			printf("Unrecognize command\n");		
	}
}

void run_server(int socketTCP, int socketUDP)
{

	struct pollfd poll_fds[MAX_CONNECTIONS];
	struct sockaddr_in cli_addresses[MAX_CONNECTIONS];
	int num_clients = 0;
	int rc;

	messages received_message;
	messages send_message;

	client_tcp_info clients_logged[MAX_CONNECTIONS];
	int num_clients_looged = 0;

	rc = listen(socketTCP, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");


	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	num_clients++;

	poll_fds[1].fd = socketUDP;
	poll_fds[1].events = POLLIN;
	num_clients++;

	poll_fds[2].fd = socketTCP;
	poll_fds[2].events = POLLIN;
	num_clients++;

	while (1)
	{

		rc = poll(poll_fds, num_clients, -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < num_clients; i++)
		{
			if (poll_fds[i].revents & POLLIN)
			{
				if(poll_fds[i].fd == STDIN_FILENO) {
					char buf_input[MSG_MAXSIZE];
					memset(buf_input, 0, MSG_MAXSIZE + 1);
					rc = read(STDIN_FILENO, buf_input, sizeof(buf_input));
					DIE(rc < 0, "read failed!\n");
					buf_input[strlen(buf_input) - 1] = '\0';
					if(strcmp(buf_input, "exit") == 0) {
						send_message.type_of_messages = 2;
						memset(&send_message.client_action, 0, sizeof(client_action));
						memset(&send_message.info_client, 0, sizeof(client_tcp_info));
						for(int i = 3; i < num_clients; i++) {
							send_all(poll_fds[i].fd, &send_message, sizeof(send_message));
						}
						return;
					}
				} else if (poll_fds[i].fd == socketUDP) {
					udp_message udp_message;
					struct sockaddr_in client_adr;
					socklen_t client_len = sizeof(client_adr);
					int rc = recvfrom(socketUDP, &udp_message,
								 sizeof(udp_message), MSG_WAITALL,
								(struct sockaddr *)&client_adr, &client_len);
					DIE(rc < 0, "recv");			
					memset(&send_message, 0, sizeof(messages));			
					send_message.type_of_messages = 3;
					memcpy(send_message.packet.ip,
						 inet_ntoa(client_adr.sin_addr),
						  strlen(inet_ntoa(client_adr.sin_addr)) + 1);
					send_message.packet.port = ntohs(client_adr.sin_port);
					send_message.packet.mesj = udp_message;
					
					for (int j = 0; j < num_clients_looged; j++) {
						if(clients_logged[j].is_conected == 1) {
							for (int k = 0; k < clients_logged[j].nr_topics; k++) {
								if (strcmp(clients_logged[j].subscribed[k].topic,
												udp_message.topic) == 0) {
									send_all(clients_logged[j].clientFD, &send_message, sizeof(send_message));
									break;
								}
							}
						}
					}
				}
				else if (poll_fds[i].fd == socketTCP)
				{

					socklen_t cli_len = sizeof(cli_addresses[i]);
					int newsockfd =
						accept(socketTCP, (struct sockaddr *)&cli_addresses[num_clients], &cli_len);
					DIE(newsockfd < 0, "accept");

					int const c = 1;
    				int err = setsockopt(newsockfd, IPPROTO_TCP, c, (char *)&(c), sizeof(int));
    				DIE(err != 0, "setsockopt() failed");

					poll_fds[num_clients].fd = newsockfd;
					poll_fds[num_clients].events = POLLIN;
					
					num_clients++;
				}
				else
				{
					
					int rc = recv_all(poll_fds[i].fd, &received_message,
									  sizeof(received_message));
					DIE(rc < 0, "recv");

					if (rc == 0)
					{
						for(int j = 0; j < num_clients_looged; j++) {
							if(clients_logged[j].clientFD == poll_fds[i].fd) {
								clients_logged[j].clientFD = -1;
								clients_logged[j].is_conected = 0;
								printf("Client %s disconnected.\n", clients_logged[j].id);
							}
						}
						close(poll_fds[i].fd);

						for (int j = i; j < num_clients - 1; j++)
						{
							poll_fds[j] = poll_fds[j + 1];
						}

						num_clients--;
					}
					else {
						process_messages(received_message,
							 clients_logged, &num_clients_looged,
							 poll_fds, num_clients, i, cli_addresses);
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	if (argc != 2)
	{
		printf("\n Usage: %s <port>\n", argv[0]);
		return 1;
	}

	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");


	int socketTCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socketTCP < 0, "socketTCP");

	int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(socketUDP < 0, "socketUDP");


	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);


	int enable = 1;
	if (setsockopt(socketTCP, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	if (setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;


	rc = bind(socketTCP, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bindTCP");

	rc = bind(socketUDP, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bindUDP");


	run_server(socketTCP, socketUDP);

	close(socketTCP);
	close(socketUDP);
	return 0;
}