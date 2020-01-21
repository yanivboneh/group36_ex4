#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Server_Game.h"
#include "Socket_Shared.h"
#include "Socket_Send_Recv_Tools.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

char* get_server_move_as_string(int server_move) {
	switch (server_move) {
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
	else if (!strcmp(player1_move, ROCK)) {
		if (!strcmp(player2_move, PAPER) || !strcmp(player2_move, SPOCK))
			ret_val = 2;
		else
			ret_val = 1;
	}
	else if (!strcmp(player1_move, PAPER)) {
		if (!strcmp(player2_move, SCISSORS) || !strcmp(player2_move, LIZARD))
			ret_val = 2;
		else
			ret_val = 1;
	}
	else if (!strcmp(player1_move, SCISSORS)) {
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
	else if (!strcmp(player1_move, LIZARD)) {
		if (!strcmp(player2_move, SCISSORS) || !strcmp(player2_move, ROCK))
			ret_val = 2;
		else
			ret_val = 1;
	}
	return ret_val;
}

int play_against_another_client(SOCKET *server_socket, HANDLE *mutexhandle) {
	int error_flag = 0;
	FILE *file;
	DWORD WaitingTime = 30000;
	TCHAR RecvRes = NULL;
	DWORD WaitRes = WaitForSingleObject(*mutexhandle, INFINITE);
	//-----------------------Critical Section-------------------------//
	if (WaitRes != WAIT_OBJECT_0)
	{
		if (WaitRes == WAIT_ABANDONED)
		{
			printf("Some thread has previously exited without releasing a mutex."
				" This is not good programming. Please fix the code.\n");
			return (ISP_MUTEX_ABANDONED);
		}
		else
			return(ISP_MUTEX_WAIT_FAILED);
	}

	if ((file = fopen("GameSession.txt", "r")) != NULL)
	{
		// file exists, another client is waiting
		fclose(file);
		ReleaseMutex(*mutexhandle);
		//-----------------------out of Critical Section-------------------------//
		error_flag = send_message_with_length("SERVER_INVITE", NULL, server_socket);
		error_flag = send_message_with_length("SERVER_PLAYER_MOVE_REQUEST", NULL, server_socket);
		//RecvRes = ReceiveString(&rcv_buffer, *server_socket, "server");

	}
	else
	{
		// file does not exist, create file and wait for another player

		file = fopen("GameSession.txt", "w");


	}
}

int play_against_server(SOCKET *server_socket, char *username) {
	time_t t;
	int random_server_move, error_flag = 0, game_result = 0;
	char *server_move = NULL, *token = NULL;
	TransferResult_t RecvRes;
	srand((unsigned)time(&t));
	random_server_move = rand() % 6 + 1;
	server_move = get_server_move_as_string(random_server_move);
	while (TRUE) {
		char player_move[MAX_MOVE_LEN], message_type[MAX_MESSAGE_LEN + 1], *parameters = NULL;
		int parameters_len = 0;
		TCHAR *rcv_buffer = NULL;
		printf("Server: SERVER_PLAYER_MOVE_REQUEST\n");
		error_flag = send_message_with_length("SERVER_PLAYER_MOVE_REQUEST", NULL, server_socket);
		if (error_flag == -1) {
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*server_socket);
			return 1;
		}
		RecvRes = ReceiveString(&rcv_buffer, *server_socket, "server");
		token = strtok(rcv_buffer, ":");
		if (token != NULL) {
			strcpy(message_type, token);
			token = strtok(NULL, ":");
			strcpy(player_move, token);
		}
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
		game_result = who_is_the_winner(get_server_move_as_string(random_server_move), player_move);
		if (game_result == 1) {
			printf("Server won: %s > %s", get_server_move_as_string(random_server_move), parameters);
			parameters_len = 12 + (int)strlen(server_move) + (int)strlen(player_move) + 4;//12 = len of "Server"; 4 = len of 3 ';' + endofstring char
			parameters = (char*)malloc(parameters_len * sizeof(char));
			strcpy(parameters, "Server;");
			strcat(parameters, server_move);
			strcat(parameters, ";");
			strcat(parameters, player_move);
			strcat(parameters, ";");
			strcat(parameters, "Server");
			error_flag = send_message_with_length("SERVER_GAME_RESULTS", parameters, server_socket);
		}                                                           //SERVER_GAME_RESULTS:Server;server_move;your_move;who_won
		else {
			printf("Player won: %s > %s", get_server_move_as_string(random_server_move), parameters);
			parameters_len = 6+ (int)strlen(username) + (int)strlen(server_move) + (int)strlen(player_move) + 4;//6 = len of "Server"; 4 = len of 3 ';' + endofstring char
			parameters = (char*)malloc(parameters_len * sizeof(char));
			strcpy(parameters, "Server;");
			strcat(parameters, server_move);
			strcat(parameters, ";");
			strcat(parameters, player_move);
			strcat(parameters, ";");
			strcat(parameters, username);
			error_flag = send_message_with_length("SERVER_GAME_RESULTS", parameters, server_socket);
		}
		free(parameters);
	}
	return error_flag;
}

int server_game_handler(char *username, SOCKET *server_socket, HANDLE *mutexhandle) {
	int error_flag = 0;
	TransferResult_t RecvRes;
	TCHAR *rcv_buffer = NULL;
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
			play_against_another_client(server_socket, mutexhandle);
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_CPU")) {
			error_flag = play_against_server(server_socket, username);
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_DISCONNECT")) {
			closesocket(*server_socket);
		}
	}
	free(AcceptedStr);
	return error_flag;
}
