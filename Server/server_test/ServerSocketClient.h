#pragma once

#include "TcpSocket.h"

class ServerSocket;
struct OVERLAPPED_PLUS;

//each client has own connect
class ServerSocketClient:public TcpSocket
{
public:

	enum SERVER_STATE_FLAGS
	{
		SSF_RESTARTING,
		SSF_DEAD,
		SSF_ACCEPTING,
		SSF_CONNECTING,
		SSF_CONNECTED,
		SSF_SHUTTING_DOWN,
		SSF_SHUT_DOWN,     //已经关闭
	};

	enum SERVER_TYPE
	{
		CLIENT_CONNECT,
		SERVER_CONNECT,
	};

private:
	sockaddr_in clientSockAddr;
	HANDLE m_completionPort;	//IOCP完成端口
	int m_listenSocket;			//服务器监听端口
	int m_clientId;				//连接ID

	int m_connectType;			//连接类型
	int m_state;				//客户端状态
	ServerSocket* m_pServer;	
	int	m_nLastTransTickCount;      //the time of the system run

	int	m_PendSendTimes;
	int	m_PendingSendBytes;

	OVERLAPPED_PLUS* m_accpetOv;	//accept时接收的ov
public:
	ServerSocketClient();
	~ServerSocketClient();
	
	void init(HANDLE completionPort,SOCKET listenSocket, ServerSocket* pServer);
	void start();
	void resetVar();

	void setClientId(int clientId){m_clientId = clientId;}
	void setConnectType(int type) {m_connectType = type;}
	int  getConnectType() {return m_connectType;}
	int  getState() {return m_state;}

	void postEvent(int msg, OVERLAPPED_PLUS* data);

	bool handleConnect(OVERLAPPED_PLUS* ov, int byteReceived);
	virtual void handlePacket();
	bool sendPacket(const char* buffer, int buffer_size);
};
