#include <windows.h>
#include <iostream>
#include "slay2_win32.h"

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




int main(int argc, char * argv[])
{
   Slay2Win32 comPort;
   const char * const comPortName = "COM1"; //COM1..9 can use this simple notation
   // const char * const comPortName = "\\\\.\\COM24"; //higher COM ports have to do it this way...
   bool status;
   unsigned int count;
   unsigned int time1ms[2];
   unsigned char c[16] = { 0 };

   cout << "Testing Windows COM port driver." << endl;

   //initialization
   status = comPort.init(comPortName);
   cout << "Initializing " << comPortName << " returned " << status << endl;
   if (status == false)
   {
      cout << "Test failed -> Exit!" << endl;
      return -1;
   }

   //buffer counter
   count = comPort.getTxCount();
   cout << "There are " << count << " bytes in the TX buffer!" << endl;
   count = comPort.getRxCount();
   cout << "There are " << count << " bytes in the RX buffer!" << endl;

   //timing
   time1ms[0] = comPort.getTime1ms();
   cout << "Time 'now', is " << time1ms[0] << endl;
   Sleep(3000);
   time1ms[1] = comPort.getTime1ms();
   cout << "After 3 seconds sleep, time is " << time1ms[1] << endl;
   cout << "Thats a difference of " << time1ms[1] - time1ms[0] << "ms!" << endl;

   //transmitting
   count = comPort.transmit((const unsigned char *)dummy, 200u);
   cout << "Requested transmission fo 200 bytes. Driver buffered " << count << " bytes!" << endl;
   count = comPort.getTxCount();
   cout << "Now, there are " << count << " bytes in the TX buffer!" << endl;
   Sleep(5);
   count = comPort.getTxCount();
   cout << "After 5ms sleep, there are still " << count << " bytes in the TX buffer!" << endl;
   Sleep(50);
   count = comPort.getTxCount();
   cout << "After furhter 50ms sleep, there are still " << count << " bytes in the TX buffer!" << endl;

   //receiving
   cout << "Now going to sleep for 30 seconds, in order to receive data (in the meanwhile)" << endl;
   Sleep(30000);
   count = comPort.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 1 byte
   count = comPort.receive(c, 1);
   cout << "Reading 1 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = comPort.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 2 byte
   count = comPort.receive(c, 2);
   cout << "Reading 2 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = comPort.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 3 byte
   count = comPort.receive(c, 3);
   cout << "Reading 3 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = comPort.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;
   //receive 16 byte
   count = comPort.receive(c, 15);
   cout << "Reading 15 byte from RX buffer returned " << count << endl;
   cout << c << endl;
   count = comPort.getRxCount();
   cout << "Now, there are " << count << " bytes in the RX buffer!" << endl;


   cout << "Test Ende" << endl;
   return 0;
}
