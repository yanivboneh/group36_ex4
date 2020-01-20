#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <time.h>
#include "Server_Comm.h"
#include "Socket_Shared.h"
#include "Socket_Send_Recv_Tools.h"

HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];

static int FindFirstUnusedThreadSlot();
static void CleanupWorkerThreads();
static DWORD ServiceThread(SOCKET *t_socket);


int MainServer(char *port_num_str)
{
	int Ind, bindRes, ListenRes, port_num, error_flag = 0;
	char *end_ptr;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR){
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());                                
		return -1;
	}  
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (MainSocket == INVALID_SOCKET){
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
	}
	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE){
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		goto server_cleanup_2;
	}
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	port_num = (int)strtol(port_num_str, &end_ptr, 10);
	service.sin_port = htons(SERVER_PORT); 
	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR){
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		goto server_cleanup_2;
	}
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR){
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
	}
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;
	printf("Waiting for a client to connect...\n");
	while (TRUE){
		SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET){
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			goto server_cleanup_3;
		}
		printf("Server: Client Connected.\n");
		Ind = FindFirstUnusedThreadSlot();
		if (Ind == NUM_OF_WORKER_THREADS){
			printf("No slots available for client, dropping the connection.\n");
			closesocket(AcceptSocket); //Closing the socket, dropping the connection.
		}
		else{
			ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
											  // AcceptSocket, instead close 
											  // ThreadInputs[Ind] when the
											  // time comes.
			ThreadHandles[Ind] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ServiceThread,
				&(ThreadInputs[Ind]),
				0,
				NULL
			);
		}
	} // for ( Loop = 0; Loop < MAX_LOOPS; Loop++ )

server_cleanup_3:

	CleanupWorkerThreads();

server_cleanup_2:
	if (closesocket(MainSocket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());

server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	return error_flag;
}

static int FindFirstUnusedThreadSlot(){
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}
	return Ind;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

static void CleanupWorkerThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], INFINITE);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(ThreadInputs[Ind]);
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
			else
			{
				printf("Waiting for thread failed. Ending program\n");
				return;
			}
		}
	}
}
char* get_server_move_as_string(int server_move) {
	switch (server_move){
	case 1:
		return ROCK;
	case 2:
		return PAPER;
	case 3:
		return SCISSORS;
	case 4:
		return SPOCK;
	case 5:
		return LIZARD;
	default:
		return "ERROR";
	}
}

int who_is_the_winner(char *player1_move, char *player2_move) {
	int ret_val = -1;
	if (!strcmp(player1_move, player2_move))
		ret_val = 0;
	else if (!strcmp(player1_move, ROCK)){
		if (!strcmp(player2_move, PAPER) || !strcmp(player2_move, SPOCK))
			ret_val = 2;
		else
			ret_val = 1;
	}
	else if (!strcmp(player1_move, PAPER)){
		if (!strcmp(player2_move, SCISSORS) || !strcmp(player2_move, LIZARD))
			ret_val = 2;
		else
			ret_val = 1;
	}
	else if (!strcmp(player1_move, SCISSORS)){
		if (!strcmp(player2_move, ROCK) || !strcmp(player2_move, SPOCK))
			ret_val = 2;
		else
			ret_val = 1;
	}
	else if (!strcmp(player1_move, SPOCK)) {
		if (!strcmp(player2_move, LIZARD) || !strcmp(player2_move, PAPER))
			ret_val = 2;
		else
			ret_val = 1;
	}
	else if (!strcmp(player1_move, LIZARD)){
		if (!strcmp(player2_move, SCISSORS) || !strcmp(player2_move, ROCK))
			ret_val = 2;
		else
			ret_val = 1;
	}
	return ret_val;
}


int play_against_server(SOCKET *server_socket) {
	time_t t;
	int random_server_move, error_flag = 0;
	char *server_move = NULL;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	TCHAR *rcv_buffer = NULL;
	char send_buffer[MAX_MESSAGE_LEN], *AcceptedStr = NULL;;
	srand((unsigned)time(&t));
	random_server_move = rand() % 6+1;
	server_move = get_server_move_as_string(random_server_move);
	printf("Server: SERVER_PLAYER_MOVE_REQUEST\n");
	error_flag = send_message_with_length("SERVER_PLAYER_MOVE_REQUEST", NULL, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		return 1;
	}
	RecvRes = ReceiveString(&rcv_buffer, *server_socket, "server");
	if (RecvRes == TRNS_FAILED) {
		printf("Service socket error occured while reading, closing thread.\n");
		closesocket(*server_socket);
		//return 1;
	}
	else if (RecvRes == TRNS_DISCONNECTED) {
		printf("Connection error occured while reading, closing thread.\n");
		//goto some_error;
	}
	printf("Server: rcv_buffer = :%s\n", rcv_buffer);
	if (who_is_the_winner(get_server_move_as_string(random_server_move), rcv_buffer) == 1) {
		printf("Server won: %s > %s", get_server_move_as_string(random_server_move), rcv_buffer);
	}
	else {
		printf("Server won: %s > %s", get_server_move_as_string(random_server_move), rcv_buffer);
	}
	return error_flag;
}

int server_game_handler(char *username, SOCKET *server_socket){
	int error_flag = 0;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	TCHAR *rcv_buffer = NULL;
	char send_message_buffer[MAX_MESSAGE_LEN], send_message_len_buffer[MESSAGE_LEN_AS_INT];
	char *AcceptedStr = NULL;
	//printf("Server: SERVER_APPROVED\n");
	error_flag = send_message_with_length("SERVER_APPROVED", NULL, server_socket);
	error_flag = send_message_with_length("SERVER_MAIN_MENU", NULL, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		return 1;
	}
	while (TRUE) {
		//*AcceptedStr = NULL;
		RecvRes = ReceiveString(&rcv_buffer, *server_socket, "server");
		if (RecvRes == TRNS_FAILED) {
			printf("Service socket error occured while reading, closing thread.\n");
			closesocket(*server_socket);
			//return 1;
		}
		else if (RecvRes == TRNS_DISCONNECTED) {
			printf("Connection error occured while reading, closing thread.\n");
			//goto some_error;
		}
		printf("Server: rcv_buffer = :%s\n", rcv_buffer);
		printf("Server: Going out of ReceiveString\n");
		if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_VERSUS")) {
			//todo
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_CPU")) {
			error_flag = play_against_server(server_socket);
		}		
	}
	free(AcceptedStr);
	return error_flag;
}



//Service thread is the thread that opens for each successful client connection and "talks" to the client.
static DWORD ServiceThread(SOCKET *t_socket)
{
	int error_flag = 0;
	char username[MAX_USERNAME_LENGTH];
	char *token = NULL, message_type[MAX_MESSAGE_LEN+1];
	TCHAR *rcv_buffer = NULL;
	TransferResult_t RecvRes;
	RecvRes = ReceiveString(&rcv_buffer, *t_socket, "server");
	printf("Server: rcv_buffer = :%s\n", rcv_buffer);
	if (RecvRes == TRNS_FAILED){
		printf("Service socket error occured while reading, closing thread.\n");
		closesocket(*t_socket);
		//return 1;
	}
	else if (RecvRes == TRNS_DISCONNECTED){
		printf("Connection error occured while reading, closing thread.\n");
		//goto some_error;
	}
	token = strtok(rcv_buffer, ":"); //extract message type
	strcpy(message_type, token);
	token = strtok(NULL, ":"); //extract username
	strcpy(username, token);
	while (TRUE){
		if (RecvRes == TRNS_FAILED){
			printf("Service socket error while reading, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
		else if (RecvRes == TRNS_DISCONNECTED){
			printf("Connection closed while reading, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
		if (STRINGS_ARE_EQUAL(message_type, "CLIENT_REQUEST")) {
			//strcpy(send_buffer, "SERVER_APPROVED");
			error_flag = send_message_with_length("SERVER_APPROVED", NULL, t_socket);
			error_flag = server_game_handler(username, t_socket);
		}
		//if (SendRes == TRNS_FAILED){
		//	printf("Service socket error while writing, closing thread.\n");
		//	closesocket(*t_socket);
		//	return 1;
		//}	
	}
	closesocket(*t_socket);
	return 0;
}
