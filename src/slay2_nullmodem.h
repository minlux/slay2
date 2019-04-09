//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Dummy implementation, interconnectiong TX and RX, like a nullmodem cable does.
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_NULLMODEM_H
#define SLAY2_NULLMODEM_H

/* -- Includes ------------------------------------------------------------ */
#include "slay2.h"

/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */
class Slay2Nullmodem : public Slay2
{
public:
   bool init(void);
   void shutdown(void);

   unsigned int getTime1ms(void);

protected: //only for testing purpose, the following functions shall be public
   unsigned int getTxCount(void);
   int transmit(const unsigned char * data, unsigned int len);
   unsigned int getRxCount(void);
   int receive(unsigned char * buffer, unsigned int size);

private:
   static unsigned int time1ms; //time is common for all instances
   Slay2Fifo fifo;
};


/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
