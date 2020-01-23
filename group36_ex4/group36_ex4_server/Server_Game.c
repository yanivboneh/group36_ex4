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

bool playing_against_server = FALSE;
/*
Description: The function coverts a randomize nuber to one of the possible moves
Parameters: int server_move - randomized num between 1-5
Returns:	char* representing the move that goes to the randomized number
*/
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
/*
Description: The function recieves both players moves as string and determines the winner 
Parameters: char* player1_move - self explanatory
			char* player2_move - self explanatory
Returns:	int ret_val - equals 1 if player wins, 2 if player 2 wins, -1 if it's a tie
*/
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
/*
Description: This function concatenates the parameters with ';' between them
Parameters: char* parameters_cat - the string to contain all the parameters
			char* oponent_name - self explanatory
			char* oponent_move - self explanatory
			char* user_move - self explanatory
			char* winner - self explanatory
Returns:	None
*/
void concatenate_parameters(char* parameters_cat, char* oponent_name, char* oponent_move, char* user_move, char* winner) {
	strcpy(parameters_cat, oponent_name);
	strcat(parameters_cat, ";");
	strcat(parameters_cat, oponent_move);
	strcat(parameters_cat, ";");
	strcat(parameters_cat, user_move);
	strcat(parameters_cat, ";");
	strcat(parameters_cat, winner);
	//return parameters_cat;
}

int play_against_another_client(SOCKET *server_socket, HANDLE *mutexhandle) {
	int error_flag = 0;
	FILE *file;
	DWORD WaitingTime = 30000;
	TCHAR RecvRes;
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
		error_flag = send_message("SERVER_INVITE", NULL, server_socket);
		error_flag = send_message("SERVER_PLAYER_MOVE_REQUEST", NULL, server_socket);
		//RecvRes = ReceiveString(&rcv_buffer, *server_socket, "server");

	}
	else
	{
		// file does not exist, create file and wait for another player

		file = fopen("GameSession.txt", "w");
	}
}
/*
Description: This function handles the client vs. server game option, including input and output messages from the client
Parameters: char* username - client username
			SOCKET *server_socket - main communication socket
Returns:	-1 in case of an error, 0 otherwise.
*/
int play_against_server(SOCKET *server_socket, char *username) {
	time_t t;
	int random_server_move, error_flag = 0, game_result = 0;
	char *server_move = NULL, *token = NULL;
	TransferResult_t RecvRes;
	srand((unsigned)time(&t));
	random_server_move = rand() % 5 + 1;
	server_move = get_server_move_as_string(random_server_move);

	char player_move[MAX_MOVE_LEN], message_type[MAX_MESSAGE_LEN + 1], parameters[MAX_LEN_OF_PARAMETERS];
	TCHAR *rcv_buffer = NULL;

	error_flag = send_message("SERVER_PLAYER_MOVE_REQUEST", NULL, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		goto exit;
	}
	RecvRes = ReceiveString(&rcv_buffer, *server_socket, TIME_OUT_IN_MSEC);
	token = strtok(rcv_buffer, ":");
	if (token != NULL) {
		strcpy(message_type, token);
		token = strtok(NULL, ":");
		strcpy(player_move, token);
	}
	if (RecvRes == TRNS_FAILED) {
		printf("Service socket error occured while reading, closing thread.\n");
		closesocket(*server_socket);
		error_flag = -1;
		goto exit;
	}
	else if (RecvRes == TRNS_DISCONNECTED) {
		printf("Connection error occured while reading, closing thread.\n");
		error_flag = -1;
		goto exit;
	}
	game_result = who_is_the_winner(get_server_move_as_string(random_server_move), player_move);
	if (game_result == 1) {
		concatenate_parameters(parameters, "Server", server_move, player_move, "Server");
	}                                                           //SERVER_GAME_RESULTS:Server;server_move;your_move;who_won
	else if (game_result == 2) {
		concatenate_parameters(parameters, "Server", server_move, player_move, username);
	}
	else {
		concatenate_parameters(parameters, "Server", server_move, player_move, "Tie");
	}
	error_flag = send_message("SERVER_GAME_RESULTS", parameters, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		goto exit;
	}
	error_flag = send_message("SERVER_GAME_OVER_MENU", NULL, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
	}
exit:
	return error_flag;
}

/*
Description: This function handles the main menu of the game, including input and output messages from the client
Parameters: char* username - client username
			SOCKET *server_socket - main communication socket
			HANDLE *mutexhandle - handle for client vs. client case
Returns:	-1 in case of an error, 0 otherwise.
*/
int server_game_handler(char *username, SOCKET *server_socket, HANDLE *mutexhandle) {
	int error_flag = 0;
	TransferResult_t RecvRes;
	char *AcceptedStr = NULL;
	error_flag = send_message("SERVER_APPROVED", NULL, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		return 1;
	}
main_menu:
	error_flag = send_message("SERVER_MAIN_MENU", NULL, server_socket);
	if (error_flag == -1) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		return 1;
	}
	while (TRUE) {
		TCHAR *rcv_buffer = NULL;
		RecvRes = ReceiveString(&rcv_buffer, *server_socket, TIME_OUT_IN_MSEC);
		if (RecvRes == TRNS_FAILED) {
			printf("Service socket error occured while reading, closing thread.\n");
			closesocket(*server_socket);
			error_flag = -1;
			goto exit;
		}
		else if (RecvRes == TRNS_DISCONNECTED) {
			printf("Connection error occured while reading, closing thread.\n");
			error_flag = -1;
			goto exit;
		}
		if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_VERSUS")) {
			play_against_another_client(server_socket, mutexhandle);
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_CPU")) {
			playing_against_server = TRUE;
			error_flag = play_against_server(server_socket, username);
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_REPLAY")) {
			if (playing_against_server) {
				error_flag = play_against_server(server_socket, username);
			}
			else {
				play_against_another_client(server_socket, mutexhandle);
			}
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_MAIN_MENU")) {
			playing_against_server = FALSE;
			goto main_menu;
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_DISCONNECT")) {
			closesocket(*server_socket);
			goto exit;
			
		}
		if (error_flag == -1) {
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*server_socket);
			return 1;
		}
	}
exit:
	return error_flag;
}
