//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Linux implementation.
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_LINUX_H
#define SLAY2_LINUX_H

/* -- Includes ------------------------------------------------------------ */
#include "slay2.h"

/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */
class Slay2Linux : public Slay2
{
public:
   Slay2Linux();
   ~Slay2Linux();
   bool init(const char * dev);
   void shutdown(void);

   unsigned int getTime1ms(void);

protected: //only for testing purpose, the following functions shall be public
   unsigned int getTxCount(void);
   int transmit(const unsigned char * data, unsigned int len);
   unsigned int getRxCount(void);
   int receive(unsigned char * buffer, unsigned int size);

private:
   int setInterfaceAttribs(int speed);
   void flush(void);

   int fileDesc;
};


/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
