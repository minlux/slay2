//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol, Scheduler.
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_SCHEDULER_H
#define SLAY2_SCHEDULER_H

/* -- Includes ------------------------------------------------------------ */
#include <string.h>
#include "slay2_buffer.h"

/* -- Defines ------------------------------------------------------------- */
#define SLAY2_SCHEDULER_FIFO_DEPTH     (3) //shall not be less than 3

/* -- Types --------------------------------------------------------------- */
class Slay2Channel; //forward declaration


class Slay2TxScheduler
{
public:
   Slay2TxScheduler();
   void reset(void);
   Slay2Buffer * getNextXfer(const unsigned long time1ms,
                             Slay2Channel * channels[], const unsigned int channelCount);
   bool acknowledgeXfer(const unsigned char seqNr);
   bool scheduleAck(const unsigned char seqNr);
   unsigned int getNackCount(void);

private:
   Slay2DataEncodingBuffer _dataBuffer[SLAY2_SCHEDULER_FIFO_DEPTH];
   Slay2DataEncodingBuffer * dataFifo[SLAY2_SCHEDULER_FIFO_DEPTH];
   unsigned long dataFifoTimeout[SLAY2_SCHEDULER_FIFO_DEPTH];
   unsigned int dataFifoCount; //number of valid entries in the fifo
   Slay2AckEncodingBuffer _ackBuffer[SLAY2_SCHEDULER_FIFO_DEPTH];
   Slay2AckEncodingBuffer * ackFifo[SLAY2_SCHEDULER_FIFO_DEPTH];
   unsigned int ackFifoCount; //number of valid entries in the fifo
   unsigned int nackCount; //no/negative acknowledge counter
   unsigned char txSeqNr;
};



/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
