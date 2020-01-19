#include "client_comm.h"
#include <stdio.h>


int main(int argc, char *argv[])
{
	int error_flag = 0;
	error_flag = MainClient(argv[1], argv[2], argv[3]);
	return error_flag;
}