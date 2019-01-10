//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Linux implementation.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <time.h>
#include <math.h>
#include "slay2_linux.h"
#include "slay2_buffer.h"


/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

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
   return 0;
}

int Slay2Linux::transmit(const unsigned char * data, unsigned int len)
{
   return -1; //todo
}


unsigned int Slay2Linux::getRxCount(void)
{
   return 0;
}

int Slay2Linux::receive(unsigned char * buffer, unsigned int size)
{
   return 0;
}


