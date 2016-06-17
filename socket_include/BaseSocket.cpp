#include "BaseSocket.h"

BaseSocket::BaseSocket()
{
	m_socket = InvalidSocket;
	m_maxpacketSize = 1024;
	m_idleTimeout = 0;
	m_connectTimeout = 0;
}


int Net::openSocket()
{
	int nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(nSocket < 0)
	{
		printf("socket() failed:%d\n", WSAGetLastError());
		return InvalidSocket;
	}

	return nSocket;
}


int Net::setBlocking(int socket, bool blockingIO)
{
	unsigned long notblock = !blockingIO;
	int ret = ioctlsocket(socket, FIONBIO, &notblock);
	if(ret != 0)
	{
		int error = WSAGetLastError();
		printf("ioctlsocket() failed:%d\n", error);
		return error;
	}

	return 0;
}

void Net::init()
{
	WSADATA wsaData;
	const WORD wVersionRequested = MAKEWORD(2, 2);
	int wsa_startup_err = WSAStartup(wVersionRequested, &wsaData);
	assert(wsa_startup_err == 0);
}

void Net::shutdown()
{
	WSACleanup();
}

