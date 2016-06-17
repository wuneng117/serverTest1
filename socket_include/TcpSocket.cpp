#include "TcpSocket.h"
#include "SendPacket.h"

TcpSocket::TcpSocket()
{

	m_packetBuf = NULL;
	m_packetSize = 0;
	m_packetPos = 0;
}

TcpSocket::~TcpSocket()
{

}

void TcpSocket::setMaxPacketSize(int maxSize)
{
	m_maxPacketSize = maxSize;
}

void TcpSocket::handleReceive(char* pRecvBuf, int nRecvSize)
{
	int nRecvPos = 0;

	//������һ��BitSteam
	if(m_packetSize > 0)
	{
		int nLastSize = m_packetSize - m_packetPos;	//ʣ����Ҫ���յ��ֽ���

		//���Ҳû�н������
		if(nRecvSize < nLastSize)
		{
			addPacketBuf(pRecvBuf, nRecvSize);
			nRecvSize = 0;
		}
		else
		{
			addPacketBuf(pRecvBuf,nLastSize);
			//���ݰ����������ӡ����
			handlePacket();

			nRecvPos += nLastSize;
			nRecvSize -= nLastSize;
		}
	}

	//��BitStream�Ĵ���
	while(1)
	{
		if(nRecvSize <= 0)
			break;

		//����ͷδ������ȫ
		if(nRecvSize < sizeof(SendPacketHead) - m_packetPos)
		{
			addPacketBuf(pRecvBuf+nRecvPos, nRecvSize);
			m_packetSize = 0;
			break;
		}
		else
		{
			SendPacketHead* pHead = reinterpret_cast<SendPacketHead*>(pRecvBuf+nRecvPos);

			m_packetSize = pHead->m_packetSize+sizeof(SendPacketHead);

			//�Ѿ��յ�������BitStream
			if(nRecvSize >= pHead->m_packetSize)
			{
				addPacketBuf(pRecvBuf+nRecvPos, m_packetSize);

				//BitStream���������ӡ����
				handlePacket();

				nRecvPos += m_packetSize;
				nRecvSize -= m_packetSize;
			}
			//BitStreamδ������ȫ
			else
			{
				addPacketBuf(pRecvBuf+nRecvPos, nRecvSize);
				break;
			}
		}
	}
}

void  TcpSocket::addPacketBuf(const char* addBuf, int addSize)
{
	if(m_packetSize != -1)
		assert(addSize <= m_packetSize-m_packetPos);

	memcpy(m_packetBuf+m_packetPos, addBuf, addSize);
	m_packetPos += addSize;
}