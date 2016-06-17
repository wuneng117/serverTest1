#include "TChar.h"
#include <stdio.h>
#include <string>

#include "ClientSocket.h"
#include "BitStream.h"

int _tmain(int argc, _TCHAR* argv[])
{
	Net::init();

	ClientSocket* pClientSock = new ClientSocket;
	pClientSock->openConnectTo("192.168.1.41", 5003);

	while (1)
	{
		Sleep(100);
		
		pClientSock->process();

		char sendBuffer[MAX_BITSTREAM_SIZE] = {0};
		BitStream stream(sendBuffer, MAX_BITSTREAM_SIZE);
		SendPacketHead* head  = stream.buildHead();

		char szBuffer[1024];
		sprintf(szBuffer, "没有什么好说的哎呀\n");
		for(int i=0; i<1000; ++i)
			stream.writeString(szBuffer, 1024);

		head->m_packetSize = stream.getSize() - sizeof(SendPacketHead);
		pClientSock->sendPacket(stream.getBuffer(), stream.getSize());

		//sprintf(sendBuffer, "sdjkdfjkgjkdfgbfhdjkgfdjkg");
		//pClientSock->sendPacket(sendBuffer, 2048);


	}
	
	Net::shutdown();
	return 0;
}
