//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Win32 implementation.
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_WIN32_H
#define SLAY2_WIN32_H

/* -- Includes ------------------------------------------------------------ */
#include "slay2.h"

/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */
class Slay2Win32 : public Slay2
{
public:
   Slay2Win32();
   ~Slay2Win32();
   bool init(const char * serPortName);
   void shutdown(void);

   unsigned int getTime1ms(void);

// protected: //only for testing purpose, the following functions shall be public
   unsigned int getTxCount(void);
   int transmit(const unsigned char * data, unsigned int len);
   unsigned int getRxCount(void);
   int receive(unsigned char * buffer, unsigned int size);

private:
   void flush(void);

   void * fileHandle;
};


/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
