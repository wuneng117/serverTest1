#pragma once
#include "singleton.h"
#include "TcpSocket.h"
#include <stdio.h>

class ClientSocket:public TcpSocket
{
public:
	enum SocketState
	{
		InvalidState,
		Connected,
		ConnectionPending,
	};

private:
	sockaddr_in m_clientSockAddr;

public:
	int m_socketState;
	ClientSocket();
	~ClientSocket();

	int openConnectTo(const char* ip, unsigned short port);

	void onReceive();
	bool sendPacket(const char* buffer, int buffer_size);

	virtual void handlePacket();

	void process();
};
