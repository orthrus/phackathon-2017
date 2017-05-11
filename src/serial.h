/*
 * CSerial.h
 */

#ifndef CSERIAL_H_
#define CSERIAL_H_

#include "rs232.h"
#include <pthread.h>

class CSerial
{
public:
	CSerial();

	enum EWHAT
	{
		START,
		BOTTOMLEFT,
		BOTTOMRIGHT,
		TOPRIGHT,
		SHIFTUP,
		SHIFTDOWN
	};

	void init(bool bResult = false);
	void exit();
  void flipper( EWHAT eWhat, unsigned int uiMilliSeconds );
	void toggle(char cMask);
	void reset();
private:
  pthread_mutex_t m_cMutex;
  struct TParam
	{
		TParam( CSerial* pCerial,	EWHAT eWhat, unsigned int uiMilliSeconds )
		{
			this->pCerial = pCerial;
			this->uiMilliSeconds = uiMilliSeconds;
			this->eWhat = eWhat;
		}
		CSerial* pCerial;
		unsigned int uiMilliSeconds;
		EWHAT eWhat;
	};

	static void *doAsyncFlipper( void *ptr );
	void setSingleRelais( bool bEnable, const char cStatus );

  int m_iPort;
	bool m_bResult;
	char readRelaisStatus();
	void writeRelaisStatus( const char cStatus );
	void writeAndReadSerial( const char* pSendBuf, const int iSendBufLen, char* pReadBuf, int& iReadBufLen );
  char*  printHexString(const char* pInBuf, const int iInLen );
	char* printBinString(const char cIn );
	char m_szHexResult[4096];
	char m_szBinResult[4096];
	char m_cAddress;
	char createByte( int b0, int b1, int b2, int b3, int b4, int b5, int b6, int b7 );
	char calcChecksum( char* pBuffer );

	void openCom();
	void closeCom();
	enum ECMD
	{
		NOP = 0,
		SETUP = 1,
		GETPORT = 2,
		SETPORT = 3,
		GETOPTION = 4,
		SETOPTION = 5,
		SETSINGLE = 6,
		DELSINGLE = 7,
		TOGGLE = 8
	};
};

#endif /* CSERIAL_H_ */
