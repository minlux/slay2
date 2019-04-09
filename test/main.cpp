#include <iostream>
#include <cstdlib>
#include "slay2.h"
#include "slay2_nullmodem.h"

using namespace std;

#define APP_CH_CNT   (8)
#define APP_TEST_CNT (1)



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
static unsigned int dummyCrc;


static Slay2Nullmodem slay2;    //serial layer 2 protocol driver
static Slay2Channel * ser[APP_CH_CNT]; //serial communication channels
static unsigned int rxCrc[APP_CH_CNT];



extern "C" unsigned int xcrc32(const unsigned char *buf, int len, unsigned int init);


static void on_serial_receive(void * const obj, const unsigned char * const data, const unsigned int len)
{
   unsigned int channel = (unsigned int)((unsigned long)(obj));
   rxCrc[channel] = xcrc32(data, len, rxCrc[channel]);
   //forward to next channel
   if (channel < APP_CH_CNT-1)
   {
      ser[channel+1]->send(data, len);
   }
}


int main(int argc, char * argv[])
{
   //init random number generator
   const unsigned int seed = slay2.getTime1ms();
   cout << "Init with seed=" << seed << endl;
   srand(seed);

   //calc crc of test-data
   const unsigned int dummyLen = strlen(dummy) + 1;
   dummyCrc = xcrc32((const unsigned char *)dummy, dummyLen, 0xFFFFFFFFuL);


   //init communiction driver
   slay2.init();

   //open communication channels
   for (int i = 0; i < APP_CH_CNT; ++i)
   {
      ser[i] = slay2.open(i);
      ser[i]->setReceiver(&on_serial_receive, (void *)((unsigned long)i));
   }

   //start test loop
   unsigned int start = slay2.getTime1ms();
   for (int test = 0; test < APP_TEST_CNT; ++test)
   {
      //init crc
      for (int i = 0; i < APP_CH_CNT; ++i) rxCrc[i] = 0xFFFFFFFFuL;
      //start transfer
      ser[0]->send((const unsigned char *)dummy, dummyLen);
      while (rxCrc[APP_CH_CNT-1] != dummyCrc)
      {
         slay2.task();
      }
   }
   unsigned int stop = slay2.getTime1ms();
   const unsigned int dauer = stop - start;
   cout << "Uebertragungsdauer [ms]: " << dauer << endl;
   cout << "Uebertragungsgeschw [kbps]: " << (1.0*APP_TEST_CNT*dummyLen*(APP_CH_CNT-1))/dauer << endl;

   //close channels
   for (int i = 0; i < APP_CH_CNT; ++i)
   {
      slay2.close(ser[i]);
   }
   slay2.shutdown();

   cout << "done" << endl;
   return 0;
}
