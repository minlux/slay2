//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Different types of buffers.

   Encode DATA or ACK bytes into a respective DATA or ACK stream.
   Decode DATA or ACK stream and extract the respective DATA or ACK bytes.
   In parallel calculate chcksum of encoded or decoded DATA resp. ACK bytes.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include "slay2_buffer.h"


/* -- Defines ------------------------------------------------------------- */
// using namespace std;



/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */
extern "C" unsigned int xcrc32(const unsigned char *buf, int len, unsigned int init);


/* -- Implementation ------------------------------------------------------ */




Slay2Buffer::Slay2Buffer(unsigned char *buffer, unsigned int bufferSize)
{
   this->size = 0;
   this->buffer = buffer;
   if (buffer != NULL)
   {
      this->size = bufferSize;
   }
   flush();
}

void Slay2Buffer::flush()
{
   count = 1; //data begins at position 1!
   step = 0;
   crc = 0xFFFFFFFFuL;
}

const unsigned char * Slay2Buffer::getBuffer()
{
   if (buffer != NULL)
   {
      return &buffer[1]; //data begins at position 1!
   }
   return NULL;
}

unsigned int Slay2Buffer::getCount()
{
   return count - 1; //data begins at position 1!
}

unsigned long Slay2Buffer::getCrc32()
{
   return crc;
}

void Slay2Buffer::addToCrc(unsigned char c)
{
   crc = xcrc32(&c, 1, (unsigned int)crc);
}



bool Slay2AckEncodingBuffer::pushAck(unsigned char c)
{
   if ((buffer != NULL) && (count < (size - 1)))
   {
      //include value into crc
      addToCrc(c);
      //add to buffer
      if (step == 0) buffer[count] = 0;
      buffer[count]    |= (unsigned char)(0x40u | ((c << 2*step) & 0x3F));
      buffer[count + 1] = (unsigned char)(0x40u | (c >> (6 - 2*step)));
      ++count;
      ++step;
      if (step >= 3)
      {
         step = 0;
         ++count;
      }
      return true;
   }
   return false;

}

bool Slay2AckEncodingBuffer::pushAckBig32(unsigned long c)
{
   bool status;
   //32bit, big endian like
   status  = pushAck((unsigned char)(c >> 24));
   status &= pushAck((unsigned char)(c >> 16));
   status &= pushAck((unsigned char)(c >> 8));
   status &= pushAck((unsigned char)c);
   return status;
}


bool Slay2AckEncodingBuffer::pushEndOfAck()
{
   unsigned int count = this->count + (step != 0); //round up to byte boundary;
   if ((buffer != NULL) && (count < size))
   {
      buffer[count++] = SLAY2_END_OF_ACK;
      this->count = count;
      this->step = 0;
      return true;
   }
   return false;
}



bool Slay2DataEncodingBuffer::pushData(unsigned char c)
{
   if ((buffer != NULL) && (count < (size - 1)))
   {
      //include value into crc
      addToCrc(c);
      //add to buffer
      if (step == 0) buffer[count] = 0;
      buffer[count]    |= (unsigned char)(0x80u | ((c << step) & 0x7F));
      buffer[count + 1] = (unsigned char)(0x80u | (c >> (7 - step)));
      ++count;
      ++step;
      if (step >= 7)
      {
         step = 0;
         ++count;
      }
      return true;
   }
   return false;
}

bool Slay2DataEncodingBuffer::pushDataBig32(unsigned long c)
{
   bool status;
   //big endian like
   status  = pushData((unsigned char)(c >> 24));
   status &= pushData((unsigned char)(c >> 16));
   status &= pushData((unsigned char)(c >> 8));
   status &= pushData((unsigned char)c);
   return status;
}

bool Slay2DataEncodingBuffer::pushEndOfData()
{
   unsigned int count = this->count + (step != 0); //round up to byte boundary;
   if ((buffer != NULL) && (count < size))
   {
      buffer[count++] = SLAY2_END_OF_DATA;
      this->count = count;
      this->step = 0;
      return true;
   }
   return false;
}



bool Slay2AckDecodingBuffer::pushAck(unsigned char c)
{
   if ((buffer != NULL) && (count < (size - 1)))
   {
      const unsigned int inc = (step != 0); //0 if step==0, 1 otherwise
      const unsigned int count = this->count + inc;
      const unsigned int left = 8 - (2 * step);
      const unsigned int right = 2 * step;

      //add 6bit data into byte sequnce
      c = c & 0x3F; //c is a 6bit data word
      buffer[count - 1] |= (unsigned char)(c << left);
      buffer[count] = (unsigned char)(c >> right);

      //add "complete" bytes to crc
      if (inc != 0)
      {
         addToCrc(buffer[count -1]);
      }

      //increment number of "complete" bytes
      this->count = count;
      //preset next step
      step = (step + 1) & 3;
      return true;
   }
   return false;
}


unsigned char Slay2AckDecodingBuffer::decodeAck(const unsigned char * buffer, unsigned int byteNumber)
{
   unsigned int row = 4 * (byteNumber / 3); //calculate byte offset
   unsigned int step = byteNumber % 3; //calculate "inter-byte step"
   unsigned int offset = row + step;
   unsigned char data;
   //do decoding
   data  = (buffer[offset] & 0x3F) >> (2 * step);
   data |= buffer[offset + 1] << (6 - (2 * step));
   return data;
}

bool Slay2DataDecodingBuffer::pushData(unsigned char c)
{
   if ((buffer != NULL) && (count < (size - 1)))
   {
      const unsigned int inc = (step != 0); //0 if step==0, 1 otherwise
      const unsigned int count = this->count + inc;
      const unsigned int left = 8 - step;
      const unsigned int right = step;

      //add 7bit data into byte sequnce
      c = c & 0x7F; //c is a 7bit data word
      buffer[count - 1] |= (unsigned char)(c << left);
      buffer[count] = (unsigned char)(c >> right);

      //add "complete" bytes to crc
      if (inc != 0)
      {
         addToCrc(buffer[count -1]);
      }

      //increment number of "complete" bytes
      this->count = count;
      //preset next step
      step = (step + 1) & 7;
      return true;
   }
   return false;
}


unsigned char Slay2DataDecodingBuffer::decodeData(const unsigned char * buffer, unsigned int byteNumber)
{
   unsigned int row = 8 * (byteNumber / 7); //calculate byte offset
   unsigned int step = byteNumber % 7; //calculate "inter-byte step"
   unsigned int offset = row + step;
   unsigned char data;
   //do decoding
   data  = (buffer[offset] & 0x7F) >> step;
   data |= buffer[offset + 1] << (7 - step);
   return data;
}



Slay2Fifo::Slay2Fifo()
{
   flush();
}

unsigned int Slay2Fifo::getCount()
{
   return count;
}

unsigned int Slay2Fifo::getSpace()
{
   return (SLAY2_FIFO_SIZE - count);
}

bool Slay2Fifo::push(unsigned char c)
{
   if (count < SLAY2_FIFO_SIZE)
   {
      buffer[write++] = c;
      if (write >= SLAY2_FIFO_SIZE) //wrap around
      {
         write = 0;
      }
      ++count;
      return true;
   }
   return false;
}

int Slay2Fifo::pop()
{
   if (count > 0)
   {
      unsigned int c = buffer[read++];
      if (read >= SLAY2_FIFO_SIZE) //wrap around
      {
         read = 0;
      }
      --count;
      return c;
   }
   return -1;
}

void Slay2Fifo::flush()
{
   read = 0;
   write = 0;
   count = 0;
}