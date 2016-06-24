#pragma once
#include "BitStream.h"

struct SendPacketHead
{
	int m_packetType;
	int m_packetSize;
};

class ServerSocketClient;
class BasePacket
{
public:
	BasePacket();
	~BasePacket();

public:
	SendPacketHead* buildPacketHead(BitStream& packet, int messageType = 0);

protected:
	ServerSocketClient* m_clientSocket;

};

