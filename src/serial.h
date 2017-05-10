/*
 * CSerial.h
 */

#ifndef CSERIAL_H_
#define CSERIAL_H_

#include "rs232.h"

class CSerial
{
public:
	CSerial();
	virtual ~CSerial();

	void init();
  void flipperLinksOnder();

private:
	void writeAndReadSerial( const char* pSendBuf, const int iSendBufLen, char* pReadBuf, int& iReadBufLen );
  char*  printHexString(const char* pInBuf, const int iInLen );
	char m_szHexResult[4096];

	char createByte( int b0, int b1, int b2, int b3, int b4, int b5, int b6, int b7 );
	char calcChecksum( char* pBuffer );
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
