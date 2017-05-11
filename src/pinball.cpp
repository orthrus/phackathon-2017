#include "serial.h"
#include "rs232.h"
#include <iostream>
#include <unistd.h>
#include <stdio.h>

/*int main(void)
{
  printf("**** PINBALL ****\n");
  CSerial ser;
  ser.init(false);
  ser.reset();
  while (true)
  {
    char c = getchar();
    switch(c)
    {
      case 'l':
        printf("left\n");
        ser.flipper(CSerial::BOTTOMLEFT, 100);
        break;
      case 'r':
        printf("right\n");
        ser.flipper(CSerial::BOTTOMRIGHT, 100);
        break;
      case 't':
        printf("top\n");
        ser.flipper(CSerial::TOPRIGHT, 100);
        break;
      case 's':
        printf("shoot\n");
        ser.flipper(CSerial::SHIFTUP, 100);
        break;
      default:
        printf("use l r t s\n");
        break;
    }
  }
  ser.exit();
  return 0;
}*/
