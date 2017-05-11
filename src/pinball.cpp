#include "serial.h"
#include "rs232.h"
#include "cam.h"

#include <iostream>

int main(void)
{
    CCam cam;

    cam.Start();

    std::cout << "Press any key to quit.." << std::endl;
    std::cin.get();
    return 0;
}
