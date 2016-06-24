#include <assert.h>
#include <string.h>

#include "BitStream.h"

void BitStream::writeByte(unsigned int byteSize, const void *bytePtr)
{
	if (!byteSize)
		return;

	assert(byteSize + m_size <= m_maxSize && "BitStream writeByte out of m_maxSize!");

	char *stPtr = dataPtr + m_size;
	memcpy(stPtr, bytePtr, byteSize);
	m_size += byteSize;
}

void BitStream::readByte(unsigned int byteSize, void *bytePtr)
{
	if (!byteSize)
		return;

	assert(byteSize + m_size <= m_maxSize && "BitStream writeByte out of m_maxSize!");

	char *stPtr = dataPtr + m_size;
	memcpy(bytePtr, stPtr, byteSize);
	m_size += byteSize;
}

void BitStream::writeString(const char *string, unsigned int maxSize)
{
	if (!string)
		string = "";
	
	unsigned int lenth = strlen(string);
	assert(lenth <= maxSize && "BitStream writeString out of maxSize");

	writeFlag(lenth > 0);
	writeInt(lenth);
	writeByte(lenth, string);
}

void BitStream::readString(char *stringBuf, unsigned int maxSize)
{
	stringBuf[0] = 0;

	if (readFlag())
	{
		unsigned int lenth = readInt();
		assert(lenth <= maxSize && "BitStream writeString out of maxSize");

		readByte(lenth, stringBuf);

		stringBuf[lenth] = 0;
	}
}




