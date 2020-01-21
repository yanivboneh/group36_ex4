#ifndef SERVER_COMM_H
#define SERVER_COMM_H
#include <stdio.h>
#include <stdbool.h>
#include <winsock2.h>


#define NUM_OF_WORKER_THREADS 2
#define MAX_LOOPS 3
#define SEND_STR_SIZE 35

int MainServer(char *port_num);

typedef struct _ThreadInputs_t {
	HANDLE mutexhandle;
	SOCKET socket;

}ThreadInputs_t;

typedef struct _ClientsDetails_t {
	char first_player_user_name[100];
	char second_player_user_name[100];
	
}ClientsDetails_t;

ClientsDetails_t  ClientsDetails;

#endif // 
