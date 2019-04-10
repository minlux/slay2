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
#include "slay2_nullmodem.h"
#include "slay2_buffer.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
unsigned int Slay2Nullmodem::time1ms;


/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */


bool Slay2Nullmodem::init(void)
{
   //nothing todo here
   return true;
}

void Slay2Nullmodem::shutdown(void)
{
   //nothing todo here
}


unsigned int Slay2Nullmodem::getTime1ms(void)
{
   return Slay2Nullmodem::time1ms++;
}


void Slay2Nullmodem::enterCritical(void)
{
   //dummy: do nothing!
}

void Slay2Nullmodem::leaveCritical(void)
{
   //dummy: do nothing!
}


unsigned int Slay2Nullmodem::getTxCount(void)
{
   return fifo.getCount();
}

int Slay2Nullmodem::transmit(const unsigned char * data, unsigned int len)
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
      success = fifo.push(c);
      if (success == false)
      {
         break;
      }
   }
   return count;
}


unsigned int Slay2Nullmodem::getRxCount(void)
{
   return fifo.getCount();
}

int Slay2Nullmodem::receive(unsigned char * buffer, unsigned int size)
{
   int count = 0;
   int c;
   //read data
   while ((size-- > 0) && ((c = fifo.pop()) >= 0))
   {
      *buffer++ = (unsigned char)c;
      ++count;
   }
   return count;
}


