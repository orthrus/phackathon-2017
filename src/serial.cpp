/*
 * CSerial.cpp

 */

#include "serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rs232.h"
#include <errno.h>

CSerial::CSerial()
{
	m_iPort=16;
}

void CSerial::init(bool bResult )
{
	printf("--- init ---\n");
	openCom();
	char szWriteBuffer[4];
	szWriteBuffer[0] = SETUP;
	szWriteBuffer[1] = 0;
	szWriteBuffer[2] = 0;
	szWriteBuffer[3] = calcChecksum(szWriteBuffer);

  char szReadBuffer[4096];
	int iReadBufferSize = 4096;
	m_bResult = true;
	writeAndReadSerial(szWriteBuffer, sizeof(szWriteBuffer), szReadBuffer, iReadBufferSize);
	m_cAddress = szReadBuffer[1];
	m_bResult = bResult;

	pthread_t thread1;
	pthread_create( &thread1, NULL, doAsyncRead, pParam);
}
void CSerial::exit()
{
	closeCom();
}
void CSerial::openCom()
{

	int		bdrate=19200;       /* 9600 baud */

	char mode[]={'8','N','1',0};

	if(RS232_OpenComport(m_iPort, bdrate, mode))
	{
		printf("Can not open comport\n");
	}
	return;
}

void CSerial::closeCom()
{
	RS232_CloseComport(m_iPort);
	printf("Comport[%d] closed.\n", m_iPort);
}

void CSerial::flipper( CSerial::EWHAT eWhat, unsigned int uiMilliSeconds)
{
	printf("--- flipper ---\n");
	pthread_t thread1;
	TParam* pParam = new TParam( this, eWhat, uiMilliSeconds );
	pthread_create( &thread1, NULL, doAsyncFlipper, pParam);
}

void *CSerial::doAsyncFlipper( void *ptr )
{
	printf("doAsync\n");
	char cStatus = 0;
	switch (((TParam*)ptr)->eWhat)
	{
		case BOTTOMRIGHT:
		cStatus = 0x01;
		break;
		case TOPRIGHT:
		cStatus = 0x01 | 0x02;
		break;
		case BOTTOMLEFT:
		cStatus = 0x04;
		break;
		case SHIFTUP:
		cStatus = 0x08;
		break;
		case START:
		cStatus = 0x10;
		break;
		case SHIFTDOWN:
		cStatus = 0x20;
		break;
		default:
		cStatus = 0x40;
		break;
}
	((TParam*)ptr)->pCerial->setSingleRelais(true, cStatus);
	usleep( ((TParam*)ptr)->uiMilliSeconds * 1000 );
	((TParam*)ptr)->pCerial->setSingleRelais(false, cStatus);
	delete (TParam*)ptr;
	return 0;
}

void CSerial::toggle( char cMask )
{
	printf("--- toggle ---\n");
	char szWriteBuffer[4];
	szWriteBuffer[0] = TOGGLE;
	szWriteBuffer[1] = m_cAddress;
	szWriteBuffer[2] = cMask;
	szWriteBuffer[3] = calcChecksum(szWriteBuffer);

	char szReadBuffer[4096];
	int iReadBufferSize = 4096;
	writeAndReadSerial(szWriteBuffer, sizeof(szWriteBuffer), szReadBuffer, iReadBufferSize);
}

char CSerial::readRelaisStatus()
{
	char cStatus = 0;
	char szWriteBuffer[4];
	szWriteBuffer[0] = GETPORT;
	szWriteBuffer[1] = m_cAddress;
	szWriteBuffer[1] = 0;
	szWriteBuffer[2] = 0;
	szWriteBuffer[3] = calcChecksum(szWriteBuffer);

	char szReadBuffer[4096];
	int iReadBufferSize = 4096;
	writeAndReadSerial(szWriteBuffer, sizeof(szWriteBuffer), szReadBuffer, iReadBufferSize);
	if ( iReadBufferSize >= 4 )
	{
		//success, set address of card
		cStatus = szReadBuffer[2];
	}
	return cStatus;
}

void CSerial::reset()
{
		writeRelaisStatus(0x00);
}

void CSerial::writeRelaisStatus( const char cStatus )
{
	char szWriteBuffer[4];
	szWriteBuffer[0] = SETPORT;
	szWriteBuffer[1] = m_cAddress;
	szWriteBuffer[2] = cStatus;
	szWriteBuffer[3] = calcChecksum(szWriteBuffer);
	char szReadBuffer[4096];
	int iReadBufferSize = 4096;
	writeAndReadSerial(szWriteBuffer, sizeof(szWriteBuffer), szReadBuffer, iReadBufferSize);
}

void CSerial::setSingleRelais( bool bEnable, const char cStatus )
{
	char szWriteBuffer[4];
	szWriteBuffer[0] = bEnable ? SETSINGLE : DELSINGLE;
	szWriteBuffer[1] = m_cAddress;
	szWriteBuffer[2] = cStatus;
	szWriteBuffer[3] = calcChecksum(szWriteBuffer);
	char szReadBuffer[4096];
	int iReadBufferSize = 4096;
	writeAndReadSerial(szWriteBuffer, sizeof(szWriteBuffer), szReadBuffer, iReadBufferSize);
}


void *CSerial::doAsyncRead( void *ptr )
{
	((CSerial*)ptr)->doRead();

}

void CSerial::doRead()
{
	const int iReadBufMaxLen = 4096;
	char pReadBuf[iReadBufMaxLen];
	int iReadBufLen = 0;
	while(true)
	{
		iReadBufLen = 0;
		usleep(1000000);

		unsigned char szTmp[1024];
		int iTmpLen = 0;
		int iTimeOut = 10 * 1000 * 1000; //timeout in uS
		int iPollTime = 1 * 1000 * 1000; // polltime in uS
		int iCurrentTime = 0;
		while ( ( iTmpLen = RS232_PollComport(m_iPort, szTmp, sizeof(szTmp) ) ) >= 0 && iCurrentTime < iTimeOut )
		{
			//printf("RS232_PollComport returned[%d].\n", iTmpLen);
			for ( int i = 0; i < iTmpLen; i++ )
			{
				pReadBuf[iReadBufLen] = szTmp[i];
				iReadBufLen++;
				if ( iReadBufLen == iReadBufMaxLen )
				{
					iReadBufLen = 0;
					//printf("RS232 read too many bytes. Exceeding {%u}", iReadBufMaxLen);
				}
			}
			usleep(iPollTime);
			iCurrentTime+=iPollTime;
		}
		//printf( "\nOUTPUT: %s \n",printHexString(pReadBuf,iReadBufLen));
	}

}

void CSerial::writeAndReadSerial( const char* pSendBuf, const int iSendBufLen, char* pReadBuf, int& iReadBufLen )
{
	pthread_mutex_lock(&m_cMutex);
	int iReadBufMaxLen = iReadBufLen;
	iReadBufLen = 0;

	int iResult = RS232_SendBuf(m_iPort, (unsigned char*)pSendBuf, iSendBufLen);
	if (m_bResult)
		printf("RS232_SendBuf returned {%d}]\n", iResult);
	if (iResult != iSendBufLen )
	{
		printf("RS232_SendBuf bytes sent is not equal to the number of bytes that must be send.\n");
		pthread_mutex_unlock(&m_cMutex);
		return;
	}
	usleep(100000);
  if (m_bResult)
	{
		unsigned char szTmp[1024];
		int iTmpLen = 0;
		int iTimeOut = 10 * 1000 * 1000; //timeout in uS
		int iPollTime = 1 * 1000 * 1000; // polltime in uS
		int iCurrentTime = 0;
		while ( ( iTmpLen = RS232_PollComport(m_iPort, szTmp, sizeof(szTmp) ) ) >= 0 && iCurrentTime < iTimeOut )
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
					pthread_mutex_unlock(&m_cMutex);
					return;
				}
			}
			usleep(iPollTime);
			iCurrentTime+=iPollTime;
		}
		printf( "\nOUTPUT: %s \n",printHexString(pReadBuf,iReadBufLen));

	}
	pthread_mutex_unlock(&m_cMutex);
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

char* CSerial::printBinString(const char cIn )
{
	char szTmp[32];
	m_szBinResult[0] = 0x00;
	bool bRelaisSet = false;
	for (char i = 1; i <= 8; i++)
	{
		switch(i)
		{
			case 1:
			bRelaisSet = ( cIn & 0x01 ) == 0x01;
			break;
			case 2:
			bRelaisSet = ( cIn & 0x02 ) == 0x02;
			break;
			case 3:
			bRelaisSet = ( cIn & 0x04 ) == 0x04;
			break;
			case 4:
			bRelaisSet = ( cIn & 0x08 ) == 0x08;
			break;
			case 5:
			bRelaisSet = ( cIn & 0x10 ) == 0x10;
			break;
			case 6:
			bRelaisSet = ( cIn & 0x20 ) == 0x20;
			break;
			case 7:
			bRelaisSet = ( cIn & 0x40 ) == 0x40;
			break;
			case 8:
			bRelaisSet = ( cIn & 0x80 ) == 0x80;
			break;

		}
		sprintf( szTmp, "%d ", bRelaisSet ? 1 : 0);
		strcat(m_szBinResult, szTmp );
	}
	return m_szBinResult;
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