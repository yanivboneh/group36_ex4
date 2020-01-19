/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*
 This file was written for instruction purposes for the
 course "Introduction to Systems Programming" at Tel-Aviv
 University, School of Electrical Engineering.
Last updated by Amnon Drory, Winter 2011.
 */
 /*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "client_comm.h"
#include "Socket_Shared.h"
#include "Socket_Send_Recv_Tools.h"

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

SOCKET m_socket;

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

//Reading data coming from the server
static DWORD RecvDataThread(void)
{
	TransferResult_t RecvRes;

	while (1)
	{
		char *AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (RecvRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			printf("Server closed connection. Bye!\n");
			return 0x555;
		}
		else
		{
			printf("%s\n", AcceptedStr);
		}

		free(AcceptedStr);
	}

	return 0;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

//Sending data to the server
static DWORD SendDataThread(LPVOID lpParam)
{
	//char *end_ptr;
	char SendStr[256];
	TransferResult_t SendRes;

	while (TRUE)
	{
		gets_s(SendStr, sizeof(SendStr)); //Reading a string from the keyboard

		if (STRINGS_ARE_EQUAL(SendStr, "quit"))
			return 0x555; //"quit" signals an exit from the client side

		SendRes = SendString(SendStr, m_socket);

		if (SendRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
	}
}


//int client_game_handler(char *username, SOCKET *server_socket){}

int MainClient(char *server_ip, char *port_num_str, char *username)
{
	int client_input_for_error = 0, port_num = 0;
	char *end_ptr;
	char send_buffer[MAX_MESSAGE_LEN];
	struct sockaddr_in socket_address;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	SOCKADDR_IN clientService;
	HANDLE hThread[2];

	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	// Create a socket.
reconnecting:
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	/*
	 The parameters passed to the socket function can be changed for different implementations.
	 Error detection is a key part of successful networking code.
	 If the socket call fails, it returns INVALID_SOCKET.
	 The if statement in the previous code is used to catch any errors that may have occurred while creating
	 the socket. WSAGetLastError returns an error number associated with the last error that occurred.
	 */

	 //Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(server_ip); //Setting the IP address to connect to
	port_num = (int)strtol(port_num_str, &end_ptr, 10);
	clientService.sin_port = htons(SERVER_PORT); //Setting the port to connect to.
	while (TRUE){
		if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
			printf("Failed connecting to server on %s:%s\n"
				"Choose what to do next:\n"
				"1. Try to reconnect\n"
				"2. Exit\n",
				server_ip, port_num_str);
			scanf_s("%d", &client_input_for_error);
			if (client_input_for_error == 1) {
				goto reconnecting;
			}
			else {
				//goto some exit
			}
			WSACleanup();
			return 0;
		}
		else {
			printf("Connected to server on %s:%s\n", server_ip, port_num_str);
			printf("Client: CLIENT_REQUEST:%s\n", username);
			strcpy(send_buffer, "CLIENT_REQUEST:");
			strcat(send_buffer, username);
			SendRes = SendString(send_buffer, m_socket);
			char* AcceptedStr = NULL;
			char* message_type = NULL;

			RecvRes = ReceiveString(&AcceptedStr, m_socket);
			if (RecvRes == TRNS_FAILED){
				printf("Socket error while trying to write data to socket\n");
				return 0x555;
			}
			if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_APPROVED")){
				break;
				printf("Client: SERVER_APPROVED\n");
				//TODO: goto two options menu
			}
			else if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_DENIED")){
				printf("Server on %s:%s denied the connection request.\n", server_ip, port_num_str);
				//goto two options menu
			}
		}
	}
	
	// Send and receive data.
	/*
		In this code, two integers are used to keep track of the number of bytes that are sent and received.
		The send and recv functions both return an integer value of the number of bytes sent or received,
		respectively, or an error. Each function also takes the same parameters:
		the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.

	*/

	hThread[0] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)SendDataThread,
		username,
		0,
		NULL
	);
	hThread[1] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)RecvDataThread,
		username,
		0,
		NULL
	);

	WaitForMultipleObjects(2, hThread, FALSE, INFINITE);

	TerminateThread(hThread[0], 0x555);
	TerminateThread(hThread[1], 0x555);

	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

	closesocket(m_socket);

	WSACleanup();

	return-1;
}

