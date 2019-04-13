//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Different types of buffers.
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_BUFFER_H
#define SLAY2_BUFFER_H

/* -- Includes ------------------------------------------------------------ */
#include <string.h>

/* -- Defines ------------------------------------------------------------- */

#define SLAY2_FRAME_PAYLOAD   (256)    //max. payload of an SLAY2 frame is 256 bytes
#define SLAY2_RX_BUFFER       (SLAY2_FRAME_PAYLOAD + 8)           //in addition to the payloay bytes, a receive buffer must keep the sequence number, channel number and CRC, plus 2 reserved bytes
#define SLAY2_TX_BUFFER       ((8 * SLAY2_RX_BUFFER) / 7 + 3)     //transmitter must buffer the encoded date, wich is 8/7 of the tx data. 1 additional byte each for "round up", end of frame character and 1 reserved byte
#define SLAY2_ACK_BUFFER      (16)     //16bytes is enough for encoded and decoded ACK frames...

#define SLAY2_FIFO_SIZE       (1024)


// #define SLAY2_ACK_ID    (0x40)      //bit[7:6] = 0b01, bit[5:0] = xxx
// #define SLAY2_DATA_ID   (0x80)      //bit[7] = 0b1, bit[6:0] = xxx
#define SLAY2_END_OF_ACK      (1)
#define SLAY2_END_OF_DATA     (2)

/* -- Types --------------------------------------------------------------- */
class Slay2Buffer
{
public:
   Slay2Buffer(unsigned char *buffer, unsigned int bufferSize);
   void flush();
   const unsigned char * getBuffer();
   unsigned int getCount();
   unsigned long getCrc32();

protected:
   void addToCrc(unsigned char c);
   unsigned char *buffer;
   unsigned int size;
   unsigned int count; //number of "complete" bytes in buffer
   unsigned int step;
   unsigned long crc;
};



//push acknowlede bytes into SLAY2 bit-stream
class Slay2AckEncodingBuffer : public Slay2Buffer
{
public:
   Slay2AckEncodingBuffer() : Slay2Buffer(_buffer, sizeof(_buffer)) { };
   bool pushAck(unsigned char c);
   bool pushAckBig32(unsigned long c);
   bool pushEndOfAck();

private:
   unsigned char _buffer[SLAY2_ACK_BUFFER];
};

//push data bytes into SLAY2 bit-stream
class Slay2DataEncodingBuffer : public Slay2Buffer
{
public:
   Slay2DataEncodingBuffer() : Slay2Buffer(_buffer, sizeof(_buffer)) { };
   bool pushData(unsigned char c);
   bool pushDataBig32(unsigned long c);
   bool pushEndOfData();

private:
   unsigned char _buffer[SLAY2_TX_BUFFER];
};



//extract acknowlede bits from SLAY2 bit-stream
//and put them into a byte-stream
class Slay2AckDecodingBuffer : public Slay2Buffer
{
public:
   Slay2AckDecodingBuffer() : Slay2Buffer(_buffer, sizeof(_buffer)) { };
   bool pushAck(unsigned char c);

   static bool isAck(unsigned char c) { return ((c & 0xC0) == 0x40); }
   static bool isEndOfAck(unsigned char c) { return (c == SLAY2_END_OF_ACK); }
   static unsigned char decodeAck(const unsigned char * buffer, unsigned int byteNumber);

private:
   unsigned char _buffer[SLAY2_ACK_BUFFER];
};

//extract data bits from SLAY2 bit-stream
//and put them into a byte-stream
class Slay2DataDecodingBuffer : public Slay2Buffer
{
public:
   Slay2DataDecodingBuffer() : Slay2Buffer(_buffer, sizeof(_buffer)) { };
   bool pushData(unsigned char c);

   static bool isData(unsigned char c) { return ((c & 0x80) == 0x80); }
   static bool isEndOfData(unsigned char c) { return (c == SLAY2_END_OF_DATA); }
   static unsigned char decodeData(const unsigned char * buffer, unsigned int byteNumber);

private:
   unsigned char _buffer[SLAY2_RX_BUFFER];
};




//normal fifo
class Slay2Fifo
{
public:
   Slay2Fifo();
   unsigned int getCount();
   unsigned int getSpace();
   bool push(unsigned char c);
   int pop();
   void flush();


private:
   unsigned char buffer[SLAY2_FIFO_SIZE];
   unsigned int read;
   unsigned int write;
   unsigned int count;
};


//this is a special kind of fifo.
//it does not implement a circular buffer. it is just a linear buffer!
//at the beginning, the buffer is empty.
//when data is pushed to the fifo, the data is copied into the buffer.
//push can be called successively. in this case the data is appended to the end
//of the buffered data (if buffer space is left).
//user can get a reference to start of data, in buffer (and data length).
//this reference point to a linear data sequence (of the given length).
//when data is poped from the buffer, the given number of data bytes are
//removed from the beginning of the data (in the buffer).
//Dadurch wird der Buffer aber nicht freigegeben. Erst wenn alle Daten
//aus dem Buffer ausgelesen wurden, wird der Buffer zurueckgesetzt.
//
//Dieser Buffer ist also geeignet um "Bursts" abzupuffern. Er muss aber
//stets schnelle ausgelesen werden, als geschrieben werden, sodass er
//immer wieder komplett leer werden kann (da ja erst durch das leer werden
//der belegte Speicher freigegeben wird).
template<int N>
class Slay2LinearFifo
{
public:
   Slay2LinearFifo()
   {
      flush();
   }

   //get number of data bytes, buffered in fifo
   unsigned int getCount()
   {
      return count;
   }

   //get number of bytes left, to be buffered in fifo
   unsigned int getSpace()
   {
      return (N - write);
   }

   //push new data to the end of the fifo. data will be concatenated. return success or fail
   bool push(const unsigned char * data, unsigned int len)
   {
      const unsigned int freeSpace = (N - write);
      if (len <= freeSpace)
      {
         memcpy(&buffer[write], data, len);
         count += len;
         write += len;
         buffer[write] = 0; //ensure zero termination of data (don't worry: i have reserved one more space in buffer for that!)
         return true;
      }
      return false;
   }

   //get reference (and count) to the "oldest" buffered data (in the fifo)
   unsigned int top(unsigned char ** data)
   {
      *data = &buffer[read];
      return count;
   }

   //drop away the first N bytes within the buffer. return success or fail
   bool pop(unsigned int count)
   {
      //pop all buffered data?
      if (count == this->count)
      {
         flush();
         return true;
      }
      //pop partial
      if (count < this->count)
      {
         read += count;
         this->count -= count;
         return true;
      }
      //can't pop more data than available
      return false;
   }

   //clear all data in buffer
   void flush()
   {
      read = 0;
      write = 0;
      count = 0;
      buffer[0] = 0; //zero termination of data
   }


private:
   unsigned char buffer[N + 1]; //one extra byte (that is always used for zero termination of data)
   unsigned int read;
   unsigned int write;
   unsigned int count;
};


/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */


#endif
