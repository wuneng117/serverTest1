#pragma once
#include <hash_map>
#include "singleton.h"
#include "TcpSocket.h"

using namespace stdext;
class ServerSocketClient;

struct OVERLAPPED_PLUS
{
	OVERLAPPED overlapped;  
	INT64		seq;
	int			time;
	int			queueSize;          //save the sendlist size
	int			CtrlFlag;			//the flag of the client(like PT_NEG)
	char		bCtxWrite;
	int			ClientId;           //the number of the client(dtServerSocketClient)
	//ClientConnect*	pClient;
	WSABUF		WsaBuf;
};

//传递参数
struct stOpParam
{
	int msg;
	int val;
};

//lisnten the client connect
class ServerSocket :public BaseSocket
{
public:
	enum
	{
		MAX_COMPLETION_THREAD_NUM	= 1,		//单线程
		ADD_CLIENT_NUM				= 20,
		MAX_PENDING_SEND			= 50,
	};

	enum OP_CODE
	{
		OP_TIMETRACE,			 //跟踪查看IOCP线程的处理速度
		OP_NORMAL,
		OP_QUIT,				//关闭IOCP线程
		OP_RESTART,				
		OP_DISCONNECT,
		OP_SEND,
		OP_ADDCLIENT,			//增加acceptex等待的客户端		
		OP_MAINTENANCE,			//维护所有客户端
		OP_SETCONNECTTYPE,		//设置客户端连接类型(客户端，其他服务器）

		OP_ON_CONNECT,
		OP_ON_DISCONNECT,
		OP_ON_PACKET,
	};

private:
	sockaddr_in listenSockAddr;

	//IOCP完成端口和线程
	HANDLE m_completionPort;		//IOCP完成端口
	int m_iocpThreadNum;			//iocp线程数
	HANDLE m_completionThread[MAX_COMPLETION_THREAD_NUM];	//iocp线程池

	//辅助监视acceptex端口数量的线程和事件
	HANDLE m_addClientMonitorThread;
	HANDLE m_addClientEvent;
	HANDLE m_monitorEndEvent;

public:


	ServerSocket();
	~ServerSocket()	{closesocket(m_socket);};

	bool start(unsigned short port, int max_connect_requests);

	void postEvent(int Msg,void *pData);

	static unsigned int WINAPI addClientMonitorThread(LPVOID lpParam);
	static unsigned int WINAPI IOCPRecvThread(LPVOID lpParam);

	OVERLAPPED_PLUS* createBuffer(int clientId, int size);
	void releaseBuffer(OVERLAPPED_PLUS* ov);

//管理客户端连接
protected:
	hash_map<int, ServerSocketClient*> m_clientMap;
	ServerSocketClient** m_clientArray;

	volatile int m_clientIdx;
	int m_clientCount;
	int m_maxClients;

public:
	int addClient(unsigned int num);
	ServerSocketClient* getClient(int clientId);
	int assingClientId();
};


//manager the clientConnect
class ClientConnectManager:public Singleton<ClientConnectManager>
{

};

//global var
#define g_clientConnectManager ClientConnectManager::getInstance()
