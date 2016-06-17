#include <assert.h>

#include "ServerSocketClient.h"
#include "BitStream.h"
#include "SendPacket.h"
#include "ServerSocket.h"

#pragma comment(lib, "ws2_32.lib")

void printWSAError(const char* stError)
{
	printf("%s(ErrorCode:%d)\n", stError, WSAGetLastError());
}

void printLastError(const char* stError)
{
	printf("%s(ErrorCode:%d)\n", stError, GetLastError());
}

ServerSocketClient::ServerSocketClient()
{
	memset(&clientSockAddr, 0, sizeof(clientSockAddr));
	m_completionPort = NULL;
	m_listenSocket = InvalidSocket;
	m_clientId = 0;

	m_connectType = CLIENT_CONNECT;
	m_state = SSF_SHUT_DOWN;
	m_pServer = NULL;
	m_nLastTransTickCount = GetTickCount();

	m_PendSendTimes = 0;
	m_PendingSendBytes = 0;

	m_accpetOv = NULL;
}

ServerSocketClient::~ServerSocketClient()
{
}

void ServerSocketClient::init(HANDLE completionPort,SOCKET listenSocket, ServerSocket* pServer)
{
	m_completionPort = completionPort;
	m_listenSocket = listenSocket;
	m_pServer = pServer;
}

void ServerSocketClient::start()
{
	if(m_state != SSF_SHUT_DOWN || m_state != SSF_RESTARTING)
		return;

	if(!m_pServer || m_listenSocket == InvalidSocket)
		return;

	//套接字未完全关闭
	if(m_socket != InvalidSocket)
	{
		CancelIo((HANDLE)m_socket);
		closesocket(m_socket);
	}

	resetVar();

	m_socket = WSASocket(AF_INET, SOCK_STREAM,, 0, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if(m_socket == InvalidSocket)
		return;
	
	OVERLAPPED_PLUS* ov = m_pServer->createBuffer(m_clientId, m_maxPacketSize);
	m_accpetOv = ov;

	DWORD dwBytesRecvd;
	int ret = AcceptEx(m_listenSocket, m_socket, ov->WsaBuf.buf, m_maxPacketSize, sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16, &dwBytesRecvd, ov);
	if(ret != 0)
	{
		DWORD error = GetLastError();
		if(error != ERROR_IO_PENDING)
		{
			postEvent(ServerSocket::OP_RESTART, ov);

			return;
		}
	}

	m_state = SSF_ACCEPTING;
}

void ServerSocketClient::resetVar()
{
	m_state = SSF_SHUT_DOWN;
	m_connectType = CLIENT_CONNECT;
	m_socket = InvalidSocket;

	m_nLastTransTickCount = GetTickCount();

	if(m_pServer)
	{
		m_idleTimeout = m_pServer->getIdleTimeout();
		m_connectTimeout = m_pServer->getConnectTimeout();
	}
	else
	{
		m_idleTimeout = 0;
		m_connectTimeout = 0;
	}

	m_PendSendTimes = 0;
	m_PendingSendBytes = 0;
}
void ServerSocketClient::postEvent(int msg, OVERLAPPED_PLUS* data)
{
	if(msg == SSF_RESTARTING)
		m_state = SSF_RESTARTING;

	if(!data)
		data = m_pServer->createBuffer(m_clientId, 0);

	if(!data)
		return;

	BOOL ret = PostQueuedCompletionStatus(m_completionPort, 0, msg, (LPOVERLAPPED)data);
	if(!ret)
		free(data);
}

void ServerSocketClient::handleConnect(OVERLAPPED_PLUS* ov, int byteReceived)
{
	if(m_state != SSF_ACCEPTING)
	{
		free(ov);
		return;
	}

	m_accpetOv = NULL;

	setsockopt( m_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *) &m_listenSocket, sizeof(m_listenSocket) );

	if(!CreateIoCompletionPort((HANDLE)m_socket, m_completionPort, (ULONG_PTR)ServerSocket::OP_NORMAL, 0))
	{
		printWSAError("handleConnect() failed");
		postEvent(ServerSocket::OP_RESTART, ov);

	}

	m_state = SSF_CONNECTED;
	
	//设置连接属性，比如心跳包
	DWORD ret;
	tcp_keepalive alive;
	alive.keepalivetime = 120000;
	alive.keepaliveinterval = 1000;
	WSAIoctl(m_socket, SIO_KEEPALIVE_VALS, &alive, sizeof(tcp_keepalive), NULL, 0, &ret, NULL,NULL);

	BOOL val = true;
	if(setsockopt(m_socket,SOL_SOCKET,SO_REUSEADDR,(const char *)&val,sizeof(BOOL))==SOCKET_ERROR )
	{
		printWSAError("set SO_REUSEADDR failed");
	}

	val = 1; //m_pServer->GetNagle() ? 0 : 1;  //Ray: 这里很容易写反
	if(setsockopt(m_socket,IPPROTO_TCP,TCP_NODELAY,(const char *)&val,sizeof(BOOL))==SOCKET_ERROR )
	{
		printWSAError("set TCP_NODELAY failed");
	}

	//收数据
	handleReceive(ov->WsaBuf.buf, byteReceived);

}

void ServerSocketClient::handlePacket()
{
	BitStream bitStream(m_packetBuf, m_packetSize);
	bitStream.setPosition(sizeof(SendPacketHead));

	for (int i=0; i<1000; ++i)
	{
		char szBuf[1024];
		bitStream.readString(szBuf, 1024);

		if(i=999)
		{
			printf("%s", szBuf);
		}

	}

	ZeroMemory(m_packetBuf, MAX_BITSTREAM_SIZE);
	m_packetPos = 0;
	m_packetSize = 0;
}

bool ServerSocketClient::sendPacket(const char* buffer, int buffer_size)
{
	int nLeft = buffer_size;
	int sendPos = 0;
	int ret = 0;
	while(nLeft > 0)
	{
		ret = send(m_socket, buffer+sendPos, nLeft, 0);
		if(ret == 0)
		{
			printf("客户端连接断开！\n");
			return false;
		}
		else if(ret < 0)
		{
			printf("send() failed! errCode:%d\n", WSAGetLastError());
			return false;
		}

		nLeft -= ret;
		sendPos += ret;
	}

	return true;
}
