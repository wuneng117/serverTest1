#include <process.h>

#include "ClientSocket.h"

#pragma comment(lib, "ws2_32.lib")

//thread for receiving data from client


ClientSocket::ClientSocket()
{
	m_socketState = InvalidState;
	setMaxPacketSize(MAX_PACKET_SIZE);
	m_recvBuf = new char[m_maxPacketSize];
}

ClientSocket::~ClientSocket()
{
	closesocket(m_socket);
}

int ClientSocket::openConnectTo(const char* ip, unsigned short port)
{
	m_socket = Net::openSocket();
	Net::setBlocking(m_socket, false);

	memset(&m_clientSockAddr, 0, sizeof(m_clientSockAddr));
	m_clientSockAddr.sin_family = AF_INET;
	m_clientSockAddr.sin_port = htons(port);
	m_clientSockAddr.sin_addr.s_addr = inet_addr(ip);


	int ret = connect(m_socket, (struct sockaddr*)&(m_clientSockAddr), sizeof(m_clientSockAddr));
	if(ret == -1)
	{
		int error = WSAGetLastError();
		if(error == WSAEWOULDBLOCK)
		{
			printf("still trying connect...");
			m_socketState = ConnectionPending;
		}
		else
		{
			printf("connect() failed:%d\n", WSAGetLastError());
			closesocket(m_socket);
			m_socket = InvalidSocket;
		}
	}

	return m_socket;
}

void ClientSocket::handleReceive()
{
	memset(m_packetBuf, 0, m_maxPacketSize);
	int ret = 0;

	ret = recv(m_socket, m_packetBuf, m_maxPacketSize, 0);
	if(ret< 0)
	{
		printf("recv() failed! error:%d\n", WSAGetLastError());
		m_socketState = InvalidState;
		closesocket(m_socket);
		return;
	}

	if(ret == 0)
		return;

	//把数据写入缓存里
	onReceive(m_packetBuf, ret);
}

bool ClientSocket::sendPacket(const char* buffer, int buffer_size)
{
	if(m_socketState != Connected)
	{
		printf("connection is not prepared");
		return false;
	}

	int nLeft = buffer_size;
	int sendPos = 0;
	int ret = 0;
	while(nLeft > 0)
	{
		ret = send(m_socket, buffer+sendPos, nLeft, 0);
		if(ret == 0)
		{
			printf("sendPacket Failed！\n");
			return false;
		}
		else if(ret < 0)
		{
			printf("send() failed! errCode:%d\n", WSAGetLastError());
			m_socketState = InvalidState;
			closesocket(m_socket);
			return false;
		}

		nLeft -= ret;
		sendPos += ret;
	}

	return true;
}

void ClientSocket::handlePacket()
{

	//用完清除
	memset(m_packetBuf, 0, m_maxPacketSize);
	m_packetSize = 0;
	m_packetPos = 0;
}

void ClientSocket::process()
{
	if(m_socketState == InvalidState)
	{
		if(m_socket != InvalidSocket)
		{
			printf("socketState is invalid, but socket in not invalid, colse socket");
			closesocket(m_socket);
			m_socket = InvalidSocket;
		}
	}
	
	else if(m_socketState == ConnectionPending)
	{
		int optval = 0;
		socklen_t optlen = sizeof(optval);
		int ret = getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen);
		if(ret == -1)
		{
			printf("getsockopt() failed, state is ConnectionPending(%d)", WSAGetLastError());
			closesocket(m_socket);
			m_socket = InvalidSocket;
		}
		else
		{
			if(optval == WSAEINPROGRESS)
			{
				printf("still trying connect...");
			}
			else if(optval == 0)
			{
				printf("connected!");
				m_socketState = Connected;

				//连接上的操作

			}
			else
			{
				printf("optval failed, state is ConnectionPending(%d)", WSAGetLastError());
				closesocket(m_socket);
				m_socket = InvalidSocket;
			}
		}
	}

	else if(m_socketState == Connected)
	{
		handleReceive();
	}
}