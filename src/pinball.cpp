#include "serial.h"
#include <unistd.h>


int main(void)
{
  printf("**** PINBALL ****\n");
  CSerial ser;
  ser.init(false);
  ser.reset();
  ser.flipper(CSerial::START, 100);
  ser.flipper(CSerial::BOTTOMLEFT, 100);
  ser.flipper(CSerial::BOTTOMRIGHT, 100);
  ser.flipper(CSerial::TOPRIGHT, 100);
  ser.flipper(CSerial::SHIFTUP, 100);
  ser.flipper(CSerial::SHIFTDOWN, 100);
  usleep(600000000);
  ser.exit();
  return 0;
}
