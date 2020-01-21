#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <winsock2.h>

int server_game_handler(char *username, SOCKET *server_socket, HANDLE *mutexhandle);

#endif