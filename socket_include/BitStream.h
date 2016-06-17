#pragma once
#include "SendPacket.h"

class BitStream
{
public:
	BitStream(void *buffer, unsigned int maxSize)
	{
		dataPtr = (char*)buffer;
		m_maxSize = maxSize;
		m_size = 0;
	}
	~BitStream()
	{

	}

	SendPacketHead* buildHead(int messageType = 0);
	void setPosition(unsigned int pos) {m_size = pos;}

	void writeByte(unsigned int byteSize, const void *bytePtr);
	void readByte(unsigned int byteSize, void *bytePtr);
	
	void writeInt(int i){ writeByte(4, &i); }
	int readInt(){ int ret = 0; readByte(4, &ret); return ret; }

	void writeFloat(float f){ writeByte(4, &f); }
	float readFloat(){ float ret = 0; readByte(4, &ret); return ret; }

	void writeFlag(bool flag){ writeByte(1, &flag); }
	bool readFlag(){ bool ret = 0;  readByte(1, &ret); return ret; }

	void writeString(const char *string, unsigned int maxSize);
	void readString(char *stringBuf, unsigned int maxSize);

	char* getBuffer(){ return dataPtr; }
	unsigned int getSize(){ return m_size; }
	unsigned int getMaxSize(){ return m_maxSize; }

protected:
	char *dataPtr;
	unsigned int m_maxSize;
	unsigned int m_size;

};
