#pragma once
#include "BaseSocket.h"

class TcpSocket :public BaseSocket
{
public:
	TcpSocket();
	~TcpSocket();


	char* getPacketBuf()			{return m_packetBuf;}
	void  setPacketSize(int size)	{m_packetSize = size;}
	void  addPacketPos(int pos)		{m_packetPos += pos;}
	void  addPacketBuf(const char* addBuf, int addSize);
	int getPacketSize()				{return m_packetSize;}
	int getPacketPos()				{return m_packetPos;}

	virtual void handlePacket() = 0;
	void handleReceive(char* pRecvBuf, int nRecvSize);

protected:

	char* m_packetBuf;	//接受数据包缓存区
	int  m_packetSize;	//本次接收数据包总大小
	int m_packetPos;	//已接受数据包大小
};
