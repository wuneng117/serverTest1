#include "TChar.h"
#include <stdio.h>
#include "ServerSocket.h"
#include <string>

int _tmain(int argc, _TCHAR* argv[])
{
	new ClientConnectManager;	//全局单例初始化

	initWinSock();
	ServerSocket listenSocket;
	listenSocket.startListen(5003,100);
	
	while(1)
	{
		Sleep(500);
	}

	return 0;
}
