#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/timerfd.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1024
#define MAX_L_CONTENT 1500

struct chat_packet {
	uint16_t len;
	char message[MSG_MAXSIZE + 1];
};

typedef struct {
	char topic[50];
	int sf;
}topics;

typedef struct {
	char id[50];
	char ip[50];
	uint16_t port;
	topics subscribed[50];
	int nr_topics;
	uint8_t is_conected;
	int clientFD;
}client_tcp_info;

typedef struct {
	char action[20];
	topics topic;
}client_action;

typedef struct {
	char topic[50];
	u_int8_t type;
	char content[MAX_L_CONTENT];
}udp_message;

typedef struct {
	udp_message mesj;
	char ip[50];
	uint16_t port;
}send_to_client;

typedef struct {
	int type_of_messages;
	client_tcp_info info_client;
	client_action client_action;
	send_to_client packet;
}messages;

#endif