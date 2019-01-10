//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Linux implementation.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <math.h>
#include "slay2_linux.h"
#include "slay2_buffer.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
static Slay2Fifo m_NullModem;

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */


void Slay2Linux::init(const char * dev)
{
   //todo - init tty-device "dev"
}

void Slay2Linux::shutdown(void)
{
   //todo
}


unsigned long Slay2Linux::getTime1ms(void)
{
   static unsigned long startSecond;
   struct timespec now;
   unsigned long time1s;
   unsigned long time1ms;

   //get time
   clock_gettime(CLOCK_REALTIME, &now);
   if (startSecond == 0)
   {
      startSecond = now.tv_sec;
   }
   //calc time since startup in milliseconds
   time1s = (now.tv_sec - startSecond);
   time1ms = round(now.tv_nsec / 1.0e6);
   if (time1ms > 999)
   {
      ++time1s;
      time1ms = 0;
   }
   time1ms = 1000uL * time1s + time1ms;
   return time1ms;
}


unsigned int Slay2Linux::getTxCount(void)
{
   return m_NullModem.getCount();
}

int Slay2Linux::transmit(const unsigned char * data, unsigned int len)
{
   bool success;
   int count;
   //write data
   for (count = 0; count < len; ++count)
   {
      unsigned char c = *data++;
#if 0
      //randomly inject errors
      if ((rand() % 10000) == 500) //error probability: 1:10000
      {
         const unsigned char old = c;
         const unsigned char toggle = (unsigned char)(rand() & 0xFF);
         c ^= toggle;
         cout << "Injecting error into ";
         if (Slay2AckDecodingBuffer::isAck(old)) cout << "ACK byte" << endl;
         if (Slay2AckDecodingBuffer::isEndOfAck(old)) cout << "END-OF-ACK byte" << endl;
         if (Slay2DataDecodingBuffer::isData(old)) cout << "DATA byte" << endl;
         if (Slay2DataDecodingBuffer::isEndOfData(old)) cout << "END-OF-DATA byte" << endl;
         printf("0x%02X XOR 0x%02X -> 0x%02X\n\n", old, toggle, c);
      }
#endif
      //push into transmitter
      success = m_NullModem.push(c);
      if (success == false)
      {
         break;
      }
   }
   return count;
}


unsigned int Slay2Linux::getRxCount(void)
{
   return m_NullModem.getCount();
}

int Slay2Linux::receive(unsigned char * buffer, unsigned int size)
{
   int count = 0;
   int c;
   //read data
   while ((size-- > 0) && ((c = m_NullModem.pop()) >= 0))
   {
      *buffer++ = (unsigned char)c;
      ++count;
   }
   return count;
}


