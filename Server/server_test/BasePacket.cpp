#include "BasePacket.h"

BasePacket::BasePacket()
{
	m_clientSocket = NULL;
}


BasePacket::~BasePacket()
{
}


SendPacketHead* BasePacket::buildPacketHead(BitStream& packet, int messageType)
{
	SendPacketHead* sendHead = reinterpret_cast<SendPacketHead*>(packet.getBuffer());
	sendHead->m_packetType = messageType;
	sendHead->m_packetSize = 0;

	packet.setPosition(sizeof(SendPacketHead));

	return sendHead;
}

