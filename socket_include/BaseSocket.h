#pragma once

#include <WinSock2.h>
#include <MSWSock.h>
#include <MSTcpIP.h>
#include <WS2tcpip.h>
#include <assert.h>
#include <stdio.h>

#define  InvalidSocket  -1

//base
class BaseSocket
{
protected:
	int m_socket;
	int m_maxPacketSize;		//最大数据包大小
	unsigned int m_idleTimeout;
	unsigned int m_connectTimeout;

public:
	BaseSocket();
	~BaseSocket(){};

	void setMaxPacketSize(int maxSize)				{m_maxPacketSize = maxSize;}
	void setIdleTimeout(unsigned int idleTimeout)		{m_idleTimeout = idleTimeout;}
	void setConnectTimeout(unsigned int connectTimeout)	{m_connectTimeout = connectTimeout;}
	unsigned int getIdleTimeout()					{return m_idleTimeout;}
	unsigned int getConnectTimeout()				{return m_connectTimeout;}

};

struct Net
{
	static int openSocket();

	static int setBlocking(int socket, bool blockingIO);

	
	
	static void init();
	static void shutdown();

};
