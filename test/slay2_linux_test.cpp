#include <unistd.h>
#include <iostream>
#include "slay2_linux.h"

using namespace std;

static const char * const dummy = "Lorem ipsum dolor sit amet, consectetur adipisici elit, \
sed eiusmod tempor incidunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, \
quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. \
sed eiusmod tempor incidunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, \
quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. \
sed eiusmod tempor incidunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, \
quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. \
sed eiusmod tempor incidunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, \
quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. \
Quis aute iure reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. \
Excepteur sint obcaecat cupiditat non proident, \
sunt in culpa qui officia deserunt mollit anim id est laborum.";



/*
   For test, you can create two virtual serial interfaces which are "interconnected":
   socat -d -d pty,raw,echo=0 pty,raw,echo=0
   This creates devices "/dev/pts/x"
*/
int main(int argc, char * argv[])
{
   Slay2Linux tty;
   const char * const devName = "/dev/ttyUSB0";
   // const char * const devName = "/dev/pts/2";
   bool status;
   unsigned int count;
   unsigned int time1ms[2];
   unsigned char c[16] = { 0 };

   cout << "Testing Linux TTY device driver." << endl;

   //initialization
   status = tty.init(devName);
   cout << "Initializing " << devName << " returned " << status << endl;
   if (status == false)
   {
      cout << "Test failed -> Exit!" << endl;
      return -1;
   }

   //buffer counter
   count = tty.getTxCount();
   cout << "There are " << count << " bytes in the TX buffer!" << endl;
   count = tty.getRxCount();
   cout << "There are " << count << " bytes in the RX buffer!" << endl;

   //timing
   time1ms[0] = tty.getTime1ms();
   cout << "Time 'now', is " << time1ms[0] << endl;
   sleep(3);
   time1ms[1] = tty.getTime1ms();
   cout << "After 3 seconds sleep, time is " << time1ms[1] << endl;
   cout << "Thats a difference of " << time1ms[1] - time1ms[0] << "ms!" << endl;

   //transmitting
   count = tty.transmit((const unsigned char *)dummy, 200u);
   cout << "Requested transmission fo 200 bytes. Driver buffered " << count << " bytes!" << endl;
   count = tty.getTxCount();
   cout << "Now, there are " << count << " bytes in the TX buffer!" << endl;
   usleep(5000);
   count = tty.getTxCount();
   cout << "After 5ms sleep, there are still " << count << " bytes in the TX buffer!" << endl;
   usleep(50000);
   count = tty.getTxCount();
   cout << "After furhter 50ms sleep, there are still " << count << " bytes in the TX buffer!" << endl;

   //receiving
   cout << "Now going to sleep for 30 seconds, in order to receive data (in the meanwhile)" << endl;
   sleep(30);
   count = tty.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 1 byte
   count = tty.receive(c, 1);
   cout << "Reading 1 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = tty.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 2 byte
   count = tty.receive(c, 2);
   cout << "Reading 2 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = tty.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 3 byte
   count = tty.receive(c, 3);
   cout << "Reading 3 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = tty.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 16 byte
   count = tty.receive(c, 15);
   cout << "Reading 15 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = tty.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;


   cout << "Test Ende" << endl;
   return 0;
}
