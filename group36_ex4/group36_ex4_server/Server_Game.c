
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

int play_against_another_client(SOCKET *server_socket, HANDLE *mutexhandle, char *username) {
	int winner = 0;
	int error_flag = 0;
	FILE *file;
	DWORD WaitingTime = 15000;
	TCHAR RecvRes = NULL;
	TCHAR *rcv_buffer = NULL;
	char *player_move = NULL, message_type[MAX_MESSAGE_LEN + 1], invite_message[100], result_message[100],  *position_ptr;
	char parameters[MAX_PARAMETERS_LENGTH], *token = NULL, *opponent_move[MAX_MESSAGE_LEN + 1], opponent_username[MAX_MESSAGE_LEN + 1];
	BOOL ReleaseRes = NULL;;
	DWORD WaitRes = NULL;


		//-----------------------Critical Section Start-------------------------//
		WaitRes = WaitForSingleObject(*mutexhandle, INFINITE);// wait for mutex to be released

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
			/*                    file exists, another client is waiting, start session                */
			strcpy(pSecond_player->player_user_name, username);
			pSecond_player->player_event = CreateEvent(NULL, false, false, "second_player_event");
			ReleaseRes = ReleaseMutex(*mutexhandle);
			if (ReleaseRes == FALSE)
				return (ISP_MUTEX_RELEASE_FAILED);
			fclose(file);
			//------------------------------------Critical Section end---------------------------------//

			/*----------------------------------------------------------------------------------------------
										Send event: self is ready, mutex released
			-----------------------------------------------------------------------------------------------*/
			SetEvent(pSecond_player->player_event);

			strcpy(invite_message, "SERVER_INVITE:");
			strcat(invite_message, pFirst_player->player_user_name);
			error_flag = send_message_with_length(invite_message, NULL, server_socket);
			if (error_flag == -1) {
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*server_socket);
				return 1;
			}
			free(rcv_buffer);
			rcv_buffer = NULL;
			error_flag = send_message_with_length("SERVER_PLAYER_MOVE_REQUEST", NULL, server_socket);
			if (error_flag == -1) {
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*server_socket);
				return 1;
			}
			//wait for reply
			RecvRes = ReceiveString(&rcv_buffer, *server_socket, "server");
			token = strtok(rcv_buffer, ":");
			if (token != NULL) {
				strcpy(message_type, token);
				token = strtok(NULL, ":"); //extract move
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
			if (STRINGS_ARE_EQUAL(message_type, "CLIENT_PLAYER_MOVE")) {
				/*------------------------------------------------------------------------------------
										Wait for event: opponent played, mutex released
				-------------------------------------------------------------------------------------*/
				WaitRes = WaitForSingleObject(pFirst_player->player_event, INFINITE);// wait for event

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
				//-----------------------Critical Section Start-------------------------//

				WaitRes = WaitForSingleObject(*mutexhandle, INFINITE);// wait for mutex to be released

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
				//Read opponents move
				fgets(opponent_move, MAX_PARAMETERS_LENGTH, file);

				// Write self move
				strcpy(player_move, parameters);
				strcat(player_move, '\n');
				rewind(file);
				fputs(parameters, file);
				rewind(file);
				fclose(file);
				/*----------------------------------------------------------------------------------------------
											   Send event: self played
			   -----------------------------------------------------------------------------------------------*/
				SetEvent(pSecond_player->player_event);
				ReleaseRes = ReleaseMutex(*mutexhandle);
				if (ReleaseRes == FALSE)
					return (ISP_MUTEX_RELEASE_FAILED);
				//------------------------------------Critical Section end---------------------------------//

				//Check for winner
				winner = who_is_the_winner(player_move, opponent_move);
				//Send game result to client
				sctrcpy(result_message, "SERVER_GAME_RESULT:");
				strcat(result_message, pFirst_player->player_user_name); strcat(result_message, ";");
				strcat(result_message, opponent_move); strcat(result_message, ";");
				strcat(result_message, player_move); strcat(result_message, ";");


				if (winner == 1) {//send your username as won 

					strcat(result_message, username);
					error_flag = send_message_with_length(result_message, NULL, server_socket);
					if (error_flag == -1) {
						printf("Service socket error while writing, closing thread.\n");
						closesocket(*server_socket);
						return 1;
					}


				}

				else {//send opponents username as won 
					strcat(result_message, pFirst_player->player_user_name);
					error_flag = send_message_with_length(result_message, NULL, server_socket);
					if (error_flag == -1) {
						printf("Service socket error while writing, closing thread.\n");
						closesocket(*server_socket);
						return 1;
					}
				}



				//Send request for next steps SERVER_GAME_OVER_ MENU


				//
			}


		}
		else
			/*              file does not exist, create file and wait for another player         */
		{
			fclose(file);
			pFirst_player->player_event = CreateEvent(NULL, false, false, "first_player_event");
			strcpy(pFirst_player->player_user_name, username);
			file = fopen("GameSession.txt", "w");
			fclose(file);
			ReleaseRes = ReleaseMutex(*mutexhandle);
			if (ReleaseRes == FALSE)
				return (ISP_MUTEX_RELEASE_FAILED);
			//------------------------------------Critical Section end---------------------------------//

			/*------------------------------------------------------------------------------------
									Wait for event: another opponent in
			-------------------------------------------------------------------------------------*/
			WaitRes = WaitForSingleObject(pSecond_player->player_event, INFINITE);// wait for event

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

				//Send invite and move request to client
				strcpy(invite_message, "SERVER_INVITE:");
				strcat(invite_message, pSecond_player->player_user_name);
				rcv_buffer = NULL;
				error_flag = send_message_with_length(invite_message, NULL, server_socket);
				if (error_flag == -1) {
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*server_socket);
					return 1;
				}
				free(rcv_buffer);
				rcv_buffer = NULL;
				error_flag = send_message_with_length("SERVER_PLAYER_MOVE_REQUEST", NULL, server_socket);
				if (error_flag == -1) {
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*server_socket);
					return 1;
				}


				//Get move from client
				RecvRes = ReceiveString(&rcv_buffer, *server_socket, "server");
				token = strtok(rcv_buffer, ":");
				if (token != NULL) {
					strcpy(message_type, token);
					token = strtok(NULL, ":"); //extract move
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
				if (STRINGS_ARE_EQUAL(message_type, "CLIENT_PLAYER_MOVE")) {
					//-----------------------Critical Section Start-------------------------//

					WaitRes = WaitForSingleObject(*mutexhandle, INFINITE);// wait for mutex to be released
					//Write move to file
					strcpy(player_move, parameters);
					strcat(player_move, '\n');
					rewind(file);
					fputs(parameters, file);
					rewind(file);
					fclose(file);

					/*----------------------------------------------------------------------------------------------
											Send event: self played
					-----------------------------------------------------------------------------------------------*/
					SetEvent(pFirst_player->player_event);
					ReleaseRes = ReleaseMutex(*mutexhandle);
					if (ReleaseRes == FALSE)
						return (ISP_MUTEX_RELEASE_FAILED);
					//------------------------------------Critical Section end---------------------------------//
				/*------------------------------------------------------------------------------------
										Wait for event: Opponent played
				-------------------------------------------------------------------------------------*/
					WaitRes = WaitForSingleObject(pSecond_player->player_event, INFINITE);// wait for event

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
					//Read opponents move
					//-----------------------Critical Section Start-------------------------//

					WaitRes = WaitForSingleObject(*mutexhandle, INFINITE);// wait for mutex to be released

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
					//Read opponents move
					fgets(opponent_move, MAX_PARAMETERS_LENGTH, file);

					//Check for winner
					winner = who_is_the_winner(player_move, opponent_move);
					//Send game result to client
					sctrcpy(result_message, "SERVER_GAME_RESULT:");
					strcat(result_message, pFirst_player->player_user_name); strcat(result_message, ";");
					strcat(result_message, opponent_move); strcat(result_message, ";");
					strcat(result_message, player_move); strcat(result_message, ";");


					if (winner == 1) {//send your username as won 

						strcat(result_message, username);
						error_flag = send_message_with_length(result_message, NULL, server_socket);
						if (error_flag == -1) {
							printf("Service socket error while writing, closing thread.\n");
							closesocket(*server_socket);
							return 1;
						}


					}

					else {//send opponents username as won 
						strcat(result_message, pSecond_player->player_user_name);
						error_flag = send_message_with_length(result_message, NULL, server_socket);
						if (error_flag == -1) {
							printf("Service socket error while writing, closing thread.\n");
							closesocket(*server_socket);
							return 1;
						}
					}

				}
			}

		}
		//Send client game over menu
		free(rcv_buffer);
		rcv_buffer = NULL;
		error_flag = send_message_with_length("SERVER_GAME_OVER_MENU", NULL, server_socket);
		if (error_flag == -1) {
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*server_socket);
			return 1;
		}
		error_flag = server_game_handler(username, server_socket, &mutexhandle);
		
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
		if ((get_server_move_as_string(random_server_move), rcv_buffer) == 1) {
			printf("Server won: %s > %s", get_server_move_as_string(random_server_move), rcv_buffer);
		}
		else {
			printf("Server won: %s > %s", get_server_move_as_string(random_server_move), rcv_buffer);
		}
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
			play_against_another_client(server_socket, mutexhandle, username);
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_CPU")) {
			error_flag = play_against_server(server_socket);
		}
		else if (STRINGS_ARE_EQUAL(rcv_buffer, "CLIENT_DISCONNECT")) {
			closesocket(*server_socket);
		}
	}
	free(AcceptedStr);
	return error_flag;
}
//#define _CRT_SECURE_NO_WARNINGS
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//
//#include "Server_Game.h"
//#include "Socket_Shared.h"
//#include "Socket_Send_Recv_Tools.h"
//
//#include <stdio.h>
//#include <stdbool.h>
//#include <string.h>
//#include <winsock2.h>
//#include <time.h>
//
//char* get_server_move_as_string(int server_move) {
//	switch (server_move) {
//	case 1:
//		return ROCK;
//	case 2:
//		return PAPER;
//	case 3:
//		return SCISSORS;
//	case 4:
//		return SPOCK;
//	case 5:
//		return LIZARD;
//	default:
//		return "ERROR";
//	}
//}
//
//int who_is_the_winner(char *player1_move, char *player2_move) {
//	int ret_val = -1;
//	if (!strcmp(player1_move, player2_move))
//		ret_val = 0;
//	else if (!strcmp(player1_move, ROCK)) {
//		if (!strcmp(player2_move, PAPER) || !strcmp(player2_move, SPOCK))
//			ret_val = 2;
//		else
//			ret_val = 1;
//	}
//	else if (!strcmp(player1_move, PAPER)) {
//		if (!strcmp(player2_move, SCISSORS) || !strcmp(player2_move, LIZARD))
//			ret_val = 2;
//		else
//			ret_val = 1;
//	}
//	else if (!strcmp(player1_move, SCISSORS)) {
//		if (!strcmp(player2_move, ROCK) || !strcmp(player2_move, SPOCK))
//			ret_val = 2;
//		else
//			ret_val = 1;
//	}
//	else if (!strcmp(player1_move, SPOCK)) {
//		if (!strcmp(player2_move, LIZARD) || !strcmp(player2_move, PAPER))
//			ret_val = 2;
//		else
//			ret_val = 1;
//	}
//	else if (!strcmp(player1_move, LIZARD)) {
//		if (!strcmp(player2_move, SCISSORS) || !strcmp(player2_move, ROCK))
//			ret_val = 2;
//		else
//			ret_val = 1;
//	}
//	return ret_val;
//}
//
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
			play_against_another_client(server_socket, mutexhandle, username);
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
