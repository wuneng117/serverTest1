#include <assert.h>
#include <process.h>

#include "ServerSocket.h"
#include "ServerSocketClient.h"

#pragma comment(lib, "ws2_32.lib")

void printWSAError(const char* stError)
{
	printf("%s(ErrorCode:%d)\n", stError, WSAGetLastError());
}

void printLastError(const char* stError)
{
	printf("%s(ErrorCode:%d)\n", stError, GetLastError());
}

ServerSocket::ServerSocket()
{

	m_iocpThreadNum = 0;
	m_completionPort = NULL;
	for(int i=0; i<MAX_COMPLETION_THREAD_NUM; ++i)
		m_completionThread[i] = NULL;

	m_addClientMonitorThread = NULL;
	m_addClientEvent = NULL;
	m_monitorEndEvent = NULL;

	m_clientIdx = 0;
	m_maxClients = 0;
	m_clientCount = 0;
	m_clientArray = NULL;
}

bool ServerSocket::start(unsigned short port, int max_connect_requests)
{
	m_socket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if(m_socket ==INVALID_SOCKET)
	{
		printWSAError("socket() failed");
		return false;
	}

	//重用地址
	BOOL Reuse = TRUE;
	setsockopt(m_socket,SOL_SOCKET,SO_REUSEADDR,(const char *)&Reuse,sizeof(BOOL));

	memset(&listenSockAddr, 0, sizeof(listenSockAddr));
	listenSockAddr.sin_family = AF_INET;
	listenSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	listenSockAddr.sin_port = htons(port);

	if(bind(m_socket, (sockaddr*)&listenSockAddr, sizeof(listenSockAddr)) == SOCKET_ERROR)
	{
		printWSAError("bind() failed");
		closesocket(m_socket);
		return false;
	}


	//setsockopt(m_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*) &m_socket, sizeof(m_socket));	感觉应该是设置client_socket用的
	
	m_maxClients = max_connect_requests;
	if(listen(m_socket, m_maxClients) == SOCKET_ERROR)
	{
		printWSAError("listen() failed");
		closesocket(m_socket);
		return false;
	}
	
	m_clientArray = new ServerSocketClient*[m_maxClients];

	m_completionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(!m_completionPort)
	{
		closesocket(m_socket);
		return false;
	}

	//bind the IOCP with socket
	if(!CreateIoCompletionPort((HANDLE)m_socket,m_completionPort,0, 0))
	{
		printWSAError("CreateIoCompletionPort() failed");
		CloseHandle(m_completionPort);
		closesocket(m_socket);
		return false;
	}

	SYSTEM_INFO l_si;
	GetSystemInfo(&l_si);
	m_iocpThreadNum	= l_si.dwNumberOfProcessors * 2 + 1;

	//创建IOCP的线程
	unsigned int threadID;
	for (int i=0; i < m_iocpThreadNum; ++i)
	{
		m_completionThread[i] = (HANDLE)_beginthreadex( NULL, 0, IOCPRecvThread, this, 0, &threadID );
	}

	//创建辅助线程
	m_addClientEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_monitorEndEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	m_addClientMonitorThread = (HANDLE)_beginthreadex(NULL,0,addClientMonitorThread,(LPVOID)this,0,&threadID);
	assert(m_addClientMonitorThread && "createThread() failed(addClientMonitorThread)");

	WSAEventSelect(m_socket, m_addClientEvent, FD_ACCEPT);

	return true;
}

void ServerSocket::postEvent(int Msg,void *pData)
{
	/*if(IsShuttingDown())
	{
		m_pMemPool->Free((MemPoolEntry)pData);
		return;
	}*/

	BOOL ret = PostQueuedCompletionStatus(m_completionPort,0,Msg,(LPOVERLAPPED)pData);
	if(!ret)
	{
		delete pData;
	}
}


//thread for monitor the client number
unsigned int ServerSocket::addClientMonitorThread(LPVOID lpParam)
{
	ServerSocket* pServer = (ServerSocket*)lpParam;

	HANDLE hList[2];
	hList[0] = pServer->m_addClientEvent;
	hList[1] = pServer->m_monitorEndEvent;

	DWORD dwRet;
	while(1)
	{
		dwRet = WaitForMultipleObjects(2, hList, FALSE, 5000);

		if(dwRet == WAIT_OBJECT_0)
		{
			pServer->postEvent(OP_ADDCLIENT);
		}
		else if(dwRet == WAIT_OBJECT_0+1)
		{
			return 0;
		}
		else if(dwRet == WAIT_TIMEOUT)
		{
			pServer->postEvent(OP_MAINTENANCE);
		}
	}


	while(1)
	{
		ClientConnect clientConnect;
		int clientAddrLen = sizeof(clientConnect.clientSockAddr);
		clientConnect.m_socket = accept(pServer->m_socket, (sockaddr*)&clientConnect.clientSockAddr, &clientAddrLen);
		if(clientConnect.m_socket < 0)
			printf("accept() failed\n");
		else
		{
			printf("Client(IP:%s) connected\n", inet_ntoa(clientConnect.clientSockAddr.sin_addr));
			//把客户端连接保存起来
			int idx = g_clientConnectManager->addClient(clientConnect);
			ClientConnect* pClientConnect = g_clientConnectManager->getClient(idx);

			CreateIoCompletionPort((HANDLE)(pClientConnect ->m_socket), pServer->m_completionPort, (ULONG_PTR)pClientConnect, 0);  


			// 开始在接受套接字上处理I/O使用重叠I/O机制  
			// 在新建的套接字上投递一个或多个异步  
			// WSARecv或WSASend请求，这些I/O请求完成后，工作者线程会为I/O请求提供服务      
			// 单I/O操作数据(I/O重叠)  

			OVERLAPPED_PLUS* ov = new OVERLAPPED_PLUS;  
			ZeroMemory(&(ov->overlapped), sizeof(OVERLAPPED));  
			ov->WsaBuf.len = MAX_TCPBUFFER_SIZE;  
			ov->WsaBuf.buf = ov->buffer;  

			DWORD RecvBytes;  
			DWORD Flags = 0;

			WSARecv(pClientConnect->m_socket, &(ov->WsaBuf), 1, &RecvBytes, &Flags, &(ov->overlapped), NULL);  

		}
	}

	return 0;
}

//thread for receiving data from client
unsigned int ServerSocket::IOCPRecvThread(LPVOID lpParam)
{
	ServerSocket* pServer = (ServerSocket*)lpParam;

	BOOL bRet;
	DWORD dwByteCountBak=0; 
	OVERLAPPED_PLUS* ov = NULL;  
	OP_CODE opCode = OP_NORMAL;
	ServerSocketClient* pClient = NULL;
	stOpParam* pParam = NULL;
	bool bWriteFlag = false, bConnectSuccess = false;

	while(1)
	{
		bRet = GetQueuedCompletionStatus(pServer->m_completionPort, &dwByteCountBak, (ULONG_PTR*)(&opCode), (LPOVERLAPPED*)&ov, INFINITE);

		
		//关闭线程
		if(opCode == OP_QUIT)
			break;

		//有些操作需要先获取客户端
		if(opCode != OP_ADDCLIENT && opCode != OP_MAINTENANCE && opCode != OP_TIMETRACE)
		{
			if(ov != NULL)
			{
				pClient = pServer->getClient(ov->ClientId);
				if(!pClient)
				{
					free(ov);
					continue;
				}
			}
			else
				continue;
		}
		
		//出错处理
		if(bRet == FALSE)
		{
			printWSAError("GetQueuedCompletionStatus() failed");
			//pClient->onNetFail(  出错处理
			continue;
		}

		//这个之后再写吧……
		if(opCode == OP_TIMETRACE)
		{
			
		}
		//设置客户端连接类型
		else if(opCode == OP_SETCONNECTTYPE)
		{
			pParam = (stOpParam*)ov->WsaBuf.buf;
			pClient->setConnectType(pParam->val);
		}

		else if(opCode == OP_ADDCLIENT)
			//pServer->addClient(ADD_CLIENT_NUM);这个函数用途不一样，要改
		else if(opCode == OP_MAINTENANCE)
			//pServer->OP_MAINTENANCE(); //维护所有客户端
		else if(opCode == OP_SEND)
			//pClient->sendPacket((OVERLAPPED_PLUS*)ov)	//发送数据处理
		else if(opCode == OP_RESTART)
		{
			free(ov);
			//pClient->restart();	//客户端重新连接
		}
		else if(opCode == OP_NORMAL)
		{
			if(dwByteCountBak == 0)
			{
				printWSAError("dwByteCountBak == 0, is error");
				//pClient->onNetFail(  出错处理
				continue;
			}

			bWriteFlag = ov->bCtxWrite;
			if(bWriteFlag)
			{
				//pClient->handleSend() 发送处理
			}
			else
			{
				bConnectSuccess = true;
				if(pClient->getState() == ServerSocketClient::SSF_ACCEPTING)
					bConnectSuccess = pClient->handleConnect(ov, dwByteCountBak);
				if(bConnectSuccess)
					pClient->handleReceive();	
			}
		}

		else
		{
			printf("无效opCode=%d", opCode);
		}

		// 为下一个重叠调用建立单I/O操作数据  
		ZeroMemory(&(ov->overlapped), sizeof(OVERLAPPED)); // 清空内存  
		ov->WsaBuf.len = m_maxPacketSize;  
		ov->WsaBuf.buf = ov->buffer;  
		WSARecv(ServerSocketClient->m_socket, &(ov->WsaBuf), 1, &dwByteCountBak, &Flags, &(ov->overlapped), NULL);  
	}

	return 0;
}

OVERLAPPED_PLUS* ServerSocket::createBuffer(int clientId, int size)
{
	OVERLAPPED_PLUS* ov = (OVERLAPPED_PLUS*)malloc(size+sizeof(OVERLAPPED_PLUS));
	
	if(ov == NULL)
		return NULL;

	memset(ov, 0, sizeof(OVERLAPPED_PLUS));

	if(size)
		ov->WsaBuf.buf = (char*)ov + sizeof(OVERLAPPED_PLUS);
	else
		ov->WsaBuf.buf = NULL;

	ov->WsaBuf.len = size;
	ov->ClientId   = clientId;
	ov->bCtxWrite  = 0;

	return ov;
}

void ServerSocket::releaseBuffer(OVERLAPPED_PLUS* ov)
{
	
}

int ServerSocket::addClient(unsigned int num)
{
	//首先判断是否超过最大连接
	if(m_clientCount + num > m_maxClients)
		num = m_maxClients - m_clientCount;

	for (int i=0; i<num; ++i)
	{
		ServerSocketClient* pClient = new ServerSocketClient;
		pClient->init(m_completionPort, m_socket, this);
		pClient->setIdleTimeout(m_idleTimeout);
		pClient->setConnectTimeout(m_connectTimeout);
		int clientId = assingClientId();
		pClient->setClientId(clientId);
		m_clientMap[clientId] = pClient;
		m_clientArray[m_clientCount] = pClient;
		pClient->start();
		++m_clientCount;
	}

}

ServerSocketClient* ServerSocket::getClient(int idx)
{
	ServerSocketClient* client = NULL; 

	std::map<int, ServerSocketClient>::iterator it = m_clientMap.find(idx);
	if(it != m_clientMap.end())
		client = &(it->second);

	return client;
}

int ServerSocket::assingClientId()
{
	InterlockedIncrement( (LONG volatile*)&m_clientIdx ); //原子操作变量+1
	return m_clientIdx;
}



