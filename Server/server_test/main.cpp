#include "TChar.h"
#include <stdio.h>
#include "ServerSocket.h"
#include <string>

int _tmain(int argc, _TCHAR* argv[])
{
	new ClientConnectManager;	//ȫ�ֵ�����ʼ��

	initWinSock();
	ServerSocket listenSocket;
	listenSocket.startListen(5003,100);
	
	while(1)
	{
		Sleep(500);
	}

	return 0;
}
