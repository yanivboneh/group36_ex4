/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/* 
 This file was written for instruction purposes for the 
 course "Introduction to Systems Programming" at Tel-Aviv
 University, School of Electrical Engineering, Winter 2011, 
 by Amnon Drory.
*/
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#ifndef SOCKET_EXAMPLE_SHARED_H
#define SOCKET_EXAMPLE_SHARED_H

#define SERVER_ADDRESS_STR "127.0.0.1"
#define SERVER_PORT 2345
#define MAX_USERNAME_LENGTH 20
#define MAX_MESSAGE_LEN 26//The longest message length is 26 charecters
#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )



#endif // SOCKET_EXAMPLE_SHARED_H