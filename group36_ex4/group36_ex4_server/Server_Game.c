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


int play_against_server(SOCKET *server_socket) {
	time_t t;
	int random_server_move, error_flag = 0;
	char *server_move = NULL, *token = NULL;
	//TransferResult_t SendRes;
	TransferResult_t RecvRes;
	//TCHAR *rcv_buffer = NULL;
	//char *player_move = NULL;
	srand((unsigned)time(&t));
	random_server_move = rand() % 6 + 1;
	server_move = get_server_move_as_string(random_server_move);
	while (TRUE) {
		char *player_move = NULL, message_type[MAX_MESSAGE_LEN + 1], parameters[MAX_PARAMETERS_LENGTH];
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
			token = strtok(NULL, ":"); //extract username
			strcpy(parameters, token);
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
		if (who_is_the_winner(get_server_move_as_string(random_server_move), rcv_buffer) == 1) {
			printf("Server won: %s > %s", get_server_move_as_string(random_server_move), rcv_buffer);
		}
		else {
			printf("Server won: %s > %s", get_server_move_as_string(random_server_move), rcv_buffer);
		}
	}
	return error_flag;
}

int server_game_handler(char *username, SOCKET *server_socket) {
	int error_flag = 0;
	//TransferResult_t SendRes;
	TransferResult_t RecvRes;
	//TCHAR *rcv_buffer = NULL;
	//char send_message_buffer[MAX_MESSAGE_LEN], send_message_len_buffer[MESSAGE_LEN_AS_INT];
	char *AcceptedStr = NULL;
	error_flag = send_message_with_length("SERVER_APPROVED", NULL, server_socket);
	error_flag = send_message_with_length("SERVER_MAIN_MENU", NULL, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		return 1;
	}
	while (TRUE) {
		TCHAR *rcv_buffer = NULL;
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
		printf("Server: rcv_buffer = %s\n", rcv_buffer);
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