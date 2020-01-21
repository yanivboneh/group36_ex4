/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/* 
 This file was written for instruction purposes for the 
 course "Introduction to Systems Programming" at Tel-Aviv
 University, School of Electrical Engineering, Winter 2011, 
 by Amnon Drory, based on example code by Johnson M. Hart.
*/
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Socket_Send_Recv_Tools.h"
#include "Socket_Shared.h"
#include <stdio.h>
#include <string.h>

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t SendBuffer( const char* Buffer, int BytesToSend, SOCKET sd )
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;
	
	while ( RemainingBytesToSend > 0 )  
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send (sd, CurPlacePtr, RemainingBytesToSend, 0);
		if ( BytesTransferred == SOCKET_ERROR ) 
		{
			printf("send() failed, error %d\n", WSAGetLastError() );
			return TRNS_FAILED;
		}
		
		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t SendString( const char *Str, SOCKET sd )
{
	int TotalStringSizeInBytes;
	TransferResult_t SendRes;
	TotalStringSizeInBytes = (int)( strlen(Str) + 1 ); // terminating zero also sent	
	SendRes = SendBuffer( 
		(const char *)( &TotalStringSizeInBytes ),
		(int)( sizeof(TotalStringSizeInBytes) ), // sizeof(int) 
		sd );
	if ( SendRes != TRNS_SUCCEEDED ) return SendRes ;
	SendRes = SendBuffer( 
		(const char *)( Str ),
		(int)( TotalStringSizeInBytes ), 
		sd );
	return SendRes;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t ReceiveBuffer( char* OutputBuffer, int BytesToReceive, SOCKET sd, char *client_or_server )
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;
	while ( RemainingBytesToReceive > 0 )  {
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if ( BytesJustTransferred == SOCKET_ERROR ) 
		{
			printf("recv() failed, error %d\n", WSAGetLastError() );
			return TRNS_FAILED;
		}		
		else if ( BytesJustTransferred == 0 )
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t ReceiveString( char** OutputStrPtr, SOCKET sd, char *client_or_server)
{
	/* Recv the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t RecvRes;
	char* StrBuffer = NULL;
	if ( ( OutputStrPtr == NULL ) || ( *OutputStrPtr != NULL ) )
	{
		printf("The first input to ReceiveString() must be " 
			   "a pointer to a char pointer that is initialized to NULL. For example:\n"
			   "\tchar* Buffer = NULL;\n"
			   "\tReceiveString( &Buffer, ___ )\n" );
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in 
	   an int variable ), then the string itself. */
		
	RecvRes = ReceiveBuffer( 
		(char *)( &TotalStringSizeInBytes ),
		(int)( sizeof(TotalStringSizeInBytes) ), // 4 bytes
		sd, client_or_server);

	if ( RecvRes != TRNS_SUCCEEDED ) return RecvRes;

	StrBuffer = (char*)malloc( TotalStringSizeInBytes * sizeof(char) );

	if ( StrBuffer == NULL )
		return TRNS_FAILED;

	RecvRes = ReceiveBuffer( 
		(char *)( StrBuffer ),
		(int)( TotalStringSizeInBytes), 
		sd, client_or_server);

	if ( RecvRes == TRNS_SUCCEEDED ) 
		{ *OutputStrPtr = StrBuffer; }
	else
	{
		free( StrBuffer );
	}
		
	return RecvRes;
}

int send_message_with_length(char* message, char* parameters, SOCKET *server_socket) {
	TransferResult_t SendRes;
	int error_flag = 0;
	char *send_message_buffer;// , send_message_len_buffer[MESSAGE_LEN_AS_INT + 1];
	if (parameters != NULL) {
		send_message_buffer = (char*)malloc((strlen(message) + strlen(parameters) + 2) * sizeof(char));
		strcpy(send_message_buffer, message);
		strcat(send_message_buffer, ":");
		strcat(send_message_buffer, parameters);
		//strcat(send_message_buffer, "\n");
	}
	else {
		send_message_buffer = (char*)malloc((strlen(message) + 1) * sizeof(char));
		strcpy(send_message_buffer, message);
		//strcat(send_message_buffer, "\n");
	}
	SendRes = SendString(send_message_buffer, *server_socket);
	if (SendRes == TRNS_FAILED) {
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*server_socket);
		error_flag = -1;
	}
//exit:
	//free(send_message_buffer);
	return error_flag;
}