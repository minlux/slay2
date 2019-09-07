//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol, Scheduler.

   Handle transmission of ACK or DATA frames.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
// #include <iostream>
#include "slay2_scheduler.h"
#include "slay2.h"


/* -- Defines ------------------------------------------------------------- */
// using namespace std;

#ifndef SLAY2_TRANSMISSION_TIMEOUT
 #define SLAY2_TRANSMISSION_TIMEOUT     (60)   //set default transmission timeout:
                                                //transmission of 300 bytes (max length of a data frame) takes ~27ms at 115k, 8N1
                                                //timeout is 27ms for transmission
                                                //         + 27ms for to complete a ongoing transmission on the "reply channel"
                                                //         +  6ms generous timeout for the reply of the ACK frame
#endif

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */

Slay2TxScheduler::Slay2TxScheduler()
{
   //init data and ack frame fifo
   for (int i = 0; i < SLAY2_SCHEDULER_FIFO_DEPTH; ++i)
   {
      dataFifo[i] = &_dataBuffer[i];
      ackFifo[i]  = &_ackBuffer[i];
   }
   reset();
}


void Slay2TxScheduler::reset(void)
{
   dataFifoCount = 0;
   ackFifoCount = 0;
   nackCount = 0;
   txSeqNr = 0; //start with sequence number 0
}


//determine next frame according to their priority:
// 1. ACK frames
// 2. Retransmission of out-timed frames
// 3. New data frames
Slay2Buffer * Slay2TxScheduler::getNextXfer(const unsigned int time1ms,
                                            Slay2Channel * channels[], const unsigned int channelCount)
{
   //any ack frame to be transmitted?
   if (ackFifoCount > 0)
   {
      //get the oldest ack frame
      Slay2Buffer * next = ackFifo[0];
      // cout << "->: ACK "
      //       << (unsigned int)Slay2AckDecodingBuffer::decodeAck(next->getBuffer(), 0)
      //       << endl;
      //"shuffle/pop" fifo
      for (int i = 0; i < SLAY2_SCHEDULER_FIFO_DEPTH - 1; ++i)
      {
         ackFifo[i] = ackFifo[i+1];
      }
      --ackFifoCount;
      return next;
   }

   //any pending data frames in fifo to be retransmitted because of timeout
   if (dataFifoCount > 0)
   {
      //the oldest one, is expect to be acknowledged first
      if ((time1ms - dataFifoTimeout[0][0]) > dataFifoTimeout[0][1]) //does (currentTime - transmissionTime) exceed the transmission timeout?
      {
         Slay2Buffer * next = dataFifo[0];
         // cout << "--> RTX: DATA "
         //      << (unsigned int)Slay2DataDecodingBuffer::decodeData(next->getBuffer(), 0)
         //      << endl;
         dataFifoTimeout[0][0] = time1ms; //store timestamp of new transmission
         dataFifoTimeout[0][1] = SLAY2_TRANSMISSION_TIMEOUT; //set transmission timeout: transmission of 300 bytes (max length of a data frame) takes ~27ms at 115k, 8N1
                                            //timeout is 27ms for transmission
                                            //         + 27ms for to complete a ongoing transmission on the "reply channel"
                                            //         +  6ms generous timeout for the reply of the ACK frame
         ++nackCount; //increment NACK counter
         return next;
      }
   }

   //check if any channel has pending data to be transmitted
   //the lower the channel number, the higher the transmission priority
   for (unsigned int ch = 0; ch < channelCount; ++ch)
   {
      Slay2Channel * channel = channels[ch];
      if (channel != NULL)
      {
         unsigned int count = channel->txFifo.getCount();
         //check for transmit condition
         if ((count >= SLAY2_FRAME_PAYLOAD) || //enough data to make one complete frame
             ((count > 0) && (channel->txMore == false))) //at leaste one pending byte and no more data will follow
         {
            //try to allocate a fifo entry
            if (dataFifoCount >= SLAY2_SCHEDULER_FIFO_DEPTH)
            {
               // cout << "Slay2TxScheduler::getNextXfer overflow" << endl;
               return NULL;
            }

            // cout << "->: DATA "
            //      << (unsigned int)txSeqNr
            //      << endl;

            Slay2DataEncodingBuffer * data = dataFifo[dataFifoCount];
            //setup new data frame (no error expected here)
            data->flush();
            data->pushData(txSeqNr++); //set sequence number
            data->pushData(ch); //set channel number
            //set payload data
            if (count > SLAY2_FRAME_PAYLOAD)
            {
               count = SLAY2_FRAME_PAYLOAD; //do limitation
            }
            for (unsigned int pay = 0; pay < count; ++pay)
            {
               unsigned char c = (unsigned char)channel->txFifo.pop();
               data->pushData(c);
            }
            data->pushDataBig32(data->getCrc32());
            data->pushEndOfData();

            //caluculate timeout.
            //1. transmission speed, 115k2 bps, 8N1
            //2. transmission time: 1ms per 10 bytes
            //3. max length of a data frame is about 300bytes
            //4. there may be up to 24 bytes in the tx fifo, at the time this transmission is scheduled -> ofset of ~3ms
            //5. at the time the receiver receives this data frame, its tx fifo may contain ~325 output bytes (worst cast) -> offset of ~300ms
            //6. transmission of an ack frame takes about 1ms
            //7. receiver may need some time to preocess the input and answer with an ack -> offset 2ms
            dataFifoTimeout[dataFifoCount][0] = time1ms;
            dataFifoTimeout[dataFifoCount][1] = 3 + (count / 10) + 30 + 1 + 2; //rule4 + rule2 + rule5 + rule6 + rule7
            ++dataFifoCount;
            return data;
         }
      }
   }
   // cout << "Slay2TxScheduler::getNextXfer NULL" << endl;
   return NULL;
}


bool Slay2TxScheduler::acknowledgeXfer(const unsigned char seqNr)
{
   if (dataFifoCount != 0) //are there pending data frames in the fifo, waiting for acknowlede?
   {
      Slay2DataEncodingBuffer * data = dataFifo[0]; //the oldest one, is expect to be acknowledged first
      unsigned char actSeqNr = Slay2DataDecodingBuffer::decodeData(data->getBuffer(), 0); //first data byte is expected to be the sequence number
      //does actual and expected sequence number match?
      if (actSeqNr == seqNr)
      {
         // cout << "<-: ACK "
         //      << (unsigned int)seqNr
         //      << endl << endl;

         //yes -> flush buffer and "rotate/pop" fifo
         data->flush();
         for (int i = 0; i < SLAY2_SCHEDULER_FIFO_DEPTH - 1; ++i)
         {
            dataFifo[i] = dataFifo[i+1];
            dataFifoTimeout[i][0] = dataFifoTimeout[i+1][0];
            dataFifoTimeout[i][1] = dataFifoTimeout[i+1][1];
         }
         dataFifo[SLAY2_SCHEDULER_FIFO_DEPTH - 1] = data;
         dataFifoTimeout[SLAY2_SCHEDULER_FIFO_DEPTH - 1][0] = 0;
         dataFifoTimeout[SLAY2_SCHEDULER_FIFO_DEPTH - 1][1] = 0;
         --dataFifoCount;
         nackCount = 0;
         return true;
      }
      // else
      // {
      //    cout << "Slay2TxScheduler::acknowledgeXfer false, "
      //         << (unsigned int)actSeqNr << " != " << (unsigned int)seqNr << endl;
      //    return false;
      // }
   }
   // cout << "Slay2TxScheduler::acknowledgeXfer false" << endl;
   return false;
}


bool Slay2TxScheduler::scheduleAck(const unsigned char seqNr)
{
   if (ackFifoCount < SLAY2_SCHEDULER_FIFO_DEPTH)
   {
      Slay2AckEncodingBuffer * ack = ackFifo[ackFifoCount];
      //setup new acknowledge
      ack->flush();
      ack->pushAck(seqNr);
      ack->pushAckBig32(ack->getCrc32());
      ack->pushEndOfAck();
      ++ackFifoCount;
      return true;
   }
   // cout << "Slay2TxScheduler::scheduleAck false" << endl;
   return false;
}


unsigned int Slay2TxScheduler::getNackCount(void)
{
   return nackCount;
}
