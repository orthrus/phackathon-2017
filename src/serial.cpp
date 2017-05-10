/*
 * CSerial.cpp

 */

#include "Serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rs232.h"
#include <errno.h>


CSerial::CSerial()
{
}

CSerial::~CSerial()
{
}

void CSerial::init()
{
	char szWriteBuffer[4];
	szWriteBuffer[0] = SETUP;
	szWriteBuffer[1] = 0;
	szWriteBuffer[2] = 0;
	szWriteBuffer[3] = calcChecksum(szWriteBuffer);

  char szReadBuffer[4];
	int iReadBufferSize = 4;
	writeAndReadSerial(szWriteBuffer, sizeof(szWriteBuffer), szReadBuffer, iReadBufferSize);
	printf( "\nOUTPUT: %s \n",printHexString(szReadBuffer,iReadBufferSize));
}

void CSerial::flipperLinksOnder()
{
	//writeAndReadSerial();
}

void CSerial::writeAndReadSerial( const char* pSendBuf, const int iSendBufLen, char* pReadBuf, int& iReadBufLen )
{
	int iReadBufMaxLen = iReadBufLen;
	iReadBufLen = 0;
	int 	cport_nr=16,        /* /dev/ttyUSB0 */
			bdrate=19200;       /* 9600 baud */

	char mode[]={'8','N','1',0};

	if(RS232_OpenComport(cport_nr, bdrate, mode))
	{
		printf("Can not open comport\n");
		return;
	}

	int iResult = RS232_SendBuf(cport_nr, (unsigned char*)pSendBuf, iSendBufLen);
	printf("RS232_SendBuf returned {%d}]\n", iResult);
	if (iResult != iSendBufLen )
	{
		printf("RS232_SendBuf bytes sent is not equal to the number of bytes that must be send.\n");
		RS232_CloseComport(cport_nr);
		printf("Comport[%d] closed.\n", cport_nr);
		return;
	}

	unsigned char szTmp[1024];
	int iTmpLen = 0;
	int iTimeOut = 10 * 1000 * 1000; //timeout in uS
	int iPollTime = 1 * 1000 * 1000; // polltime in uS
	int iCurrentTime = 0;
	while ( ( iTmpLen = RS232_PollComport(cport_nr, szTmp, sizeof(szTmp) ) ) >= 0 && iCurrentTime < iTimeOut )
	{
		printf("RS232_PollComport returned[%d].\n", iTmpLen);
		for ( int i = 0; i < iTmpLen; i++ )
		{
			pReadBuf[iReadBufLen] = szTmp[i];
			iReadBufLen++;
			if ( iReadBufLen == iReadBufMaxLen )
			{
				iReadBufLen = 0;
				printf("RS232 read too many bytes. Exceeding {%u}", iReadBufMaxLen);
				return;
			}
		}
		usleep(iPollTime);
		iCurrentTime+=iPollTime;
	}
	RS232_CloseComport(cport_nr);
	printf("Comport[%d] closed.\n", cport_nr);
}



char* CSerial::printHexString(const char* pInBuf, const int iInLen )
{
	char szTmp[32];
	m_szHexResult[0] = 0x00;
	for (int i = 0; i < iInLen; i++)
	{
		sprintf( szTmp, "%02x ", pInBuf[i] );
		strcat(m_szHexResult, szTmp );
	}
	return m_szHexResult;
}

char CSerial::createByte( int b0, int b1, int b2, int b3, int b4, int b5, int b6, int b7 )
{
	return (char)
		( b0 == 1  ? 1 : 0 ) |
		( b1 == 1 ? 2 : 0 ) |
		( b2 == 1 ? 4 : 0 ) |
		( b3 == 1 ? 8 : 0 ) |
		( b4 == 1 ? 16 : 0 ) |
		( b5 == 1 ? 32 : 0 ) |
		( b6 == 1 ? 64 : 0 ) |
		( b7 == 1 ? 128 : 0 );
}

char CSerial::calcChecksum( char* pBuffer )
{
	return pBuffer[0] ^ pBuffer[1] ^ pBuffer[2];
}
