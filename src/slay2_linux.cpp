//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Linux implementation.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "slay2_linux.h"
#include "slay2_buffer.h"


/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */


Slay2Linux::Slay2Linux()
{
   fileDesc = -1;

   //initialize a recursive mutex for critical section handling
   pthread_mutexattr_t mutexAttr;
   pthread_mutexattr_init(&mutexAttr);
   pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&mutex, &mutexAttr);
}


Slay2Linux::~Slay2Linux()
{
   pthread_mutex_destroy(&mutex);
   shutdown();
}


bool Slay2Linux::init(const char * dev, const unsigned int baudrate)
{
   fileDesc = ::open(dev, O_RDWR | O_NOCTTY); //using "::open" to "say" that the global "open" function is meant, not the "open" method of this class.
   if (fileDesc >= 0)
   {
      setInterfaceAttribs(baudrate); //configure interface
      flush(); //drop all data in input and output buffer
      return true;
   }
   return false;
}


void Slay2Linux::shutdown(void)
{
   if (fileDesc >= 0)
   {
      ::close(fileDesc); //using "::close" to "say" that the global "close" function is meant, not the "close" method of this class.
      fileDesc = -1;
   }
}


unsigned int Slay2Linux::getTime1ms(void)
{
   static unsigned int startSecond;
   struct timespec now;
   unsigned int time1s;
   unsigned int time1ms;

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
   time1ms = 1000u * time1s + time1ms;
   return time1ms;
}


void Slay2Linux::enterCritical(void)
{
   pthread_mutex_lock(&mutex);
}

void Slay2Linux::leaveCritical(void)
{
   pthread_mutex_unlock(&mutex);
}


unsigned int Slay2Linux::getTxCount(void)
{
   unsigned int count = 0;
   if (fileDesc >= 0)
   {
     ioctl(this->fileDesc, TIOCOUTQ, &count);
   }
   return count;
}

int Slay2Linux::transmit(const unsigned char * data, unsigned int len)
{
   if (fileDesc >= 0)
   {
      return ::write(this->fileDesc, data, len);
   }
   return 0;
}


unsigned int Slay2Linux::getRxCount(void)
{
   unsigned int count = 0;
   if (fileDesc >= 0)
   {
      ioctl(this->fileDesc, FIONREAD, &count);
   }
   return count;
}

int Slay2Linux::receive(unsigned char * buffer, unsigned int size)
{
   if (fileDesc >= 0)
   {
      return read(this->fileDesc, buffer, size);
   }
   return 0;
}






//See: https://www.gnu.org/software/libc/manual/html_node/Terminal-Modes.html#Terminal-Modes
int Slay2Linux::setInterfaceAttribs(unsigned int baudrate)
{
   const int fd = this->fileDesc;
   struct termios tty = { 0 }; //for forwared compatibility: initialize all member to 0 (just to be sure, everything has a defined value)

   //read out current settings
   if (tcgetattr (fd, &tty) != 0)
   {
      return -1;
   }

   //configure "raw" mode (8n1, no parity, no flow-control)
   //set input mode
   tty.c_iflag &= ~( //clearing of all those flags will have the mentiond effect ...
      INPCK    |  //parity off
      IGNPAR   |  //don't care as parity is off
      PARMRK   |  //don't care as parity is off
      ISTRIP   |  //don't stip the input down to 7bit - leaf it 8bit
      IGNCR    |  //don't discard \r
      ICRNL    |  //don't translate \r to \n
      INLCR    |  //don't translate \n to \r
      IXOFF    |  //don't use start/stop control on input
      IXON     |  //don't use start/stop control on output
      IXANY    |  //don't use start/stop control on input/output
      IMAXBEL  |  //BSD extension: don't send BEL character when terminal buffer gets filled
      IGNBRK   |  //do not ignore break character -> forward it as 0 character (in this application, this is an exception in normal communication, an will detected by CRC of higher level protocol)
      BRKINT   |  //don't clear input/output queues on reception of break character
      0
   );
   //set output mode
   tty.c_oflag &= ~( //clearing of all those flags will have the mentiond effect ...
      OPOST    |  //output data is not processed for terminal applications
      ONLCR    |  //don't care as post processing is off (don't translate \n to \r\n)
      // OXTABS   |  //don't care as post processing is off (don\t translate TAB to SPACES)
      // ONOEOT   |  //don't care as post processing is off (don't discard C-d (ASCII 0x04))
      0
   );
   //set control mode
   tty.c_cflag &= ~( //clearing of all those flags will have the mentiond effect ...
      HUPCL    | //don't generate modem disconnect events
      CSTOPB   | //use only one stop bit
      PARENB   | //no parity
      PARODD   | //don't care as parity is off
      CRTSCTS  | //no hw flow control
      // CCTS_OFLOW | //don't use flow control based on CTS wire
      // CRTS_IFLOW | //don't use flow control based on RTS wire
      // MDMBUF   | //don't use flow control based on carrier
      // CIGNORE  | //MUST BE CLEAR, otherwise the driver doesn't take the control mode settings!!!
      CSIZE    | //clear bitfield, that specifies the bits per character (will be set to the desired value some lines below...)
      0
   );
   tty.c_cflag |= ( //setting of all those flags will have the mentiond effect ...
      CLOCAL   | //terminal is connected “locally” and that the modem status lines (such as carrier detect) shall be ignored.
      CREAD    | //input can be read from the terminal (Otherwise, input would be discarded when it arrives)
      CS8      | //eight bits per byte
      0
   );
   //set local modes
   tty.c_lflag &= ~(
      ICANON   |   //input is processed in noncanonical mode (das ist WICHTIG! sonst wird der Input verandert!!!)
      ECHO     | //don't echo input characters back to the terminal
      ECHOE    | //don't handle ERASE character
      ECHOPRT  | //BSD extension: don't handle ERASE character (in display)
      ECHOK    | //don't handle KILL character
      ECHOKE   | //don't handle KILL character (in a special way)
      ECHONL   | //don't echo \n (don't care as canonical is OFF)
      ECHOCTL  | //BSD extension: don't echo control characters with leading '^' (don't care as canonical is off)
      ISIG     | //don't recognize INTR, QUIT and SUSP characters
      IEXTEN   | //ACHTUNG: don't use implementation-defined feature (on linux: don't use the LNEXT and DISCARD characters)
      NOFLSH   | //don't clear the input/output buffer on INTR, QUIT and SUSP
      TOSTOP   | //BSD extension: don't use SIGTTOU signals are generated by background processes that attempt to write to the terminal
      // ALTWERASE | //meaning of WERASE charater -> beginning of a word is a nonwhitespace character following a whitespace character
      FLUSHO   | //While this bit is set, all output is discarded -> deswegen shalte ich es aus!
      PENDIN   | //indicates that there is a line of input that needs to be reprinted -> brauch ich nicht
      0
   );
   tty.c_lflag |= ( //setting of all those flags will have the mentiond effect ...
      // NOKERNINFO | //disables handling of the STATUS character
      0
   );

   //set timeout behaviour (wait up to TIME*100ms while RX buffer is empty (MIN=0))
   tty.c_cc[VMIN]  = 0;
   tty.c_cc[VTIME] = 0;   //wait 0*100ms -> do not wait -> non-blocking read

   //adjust baudrate
   int speed = (int)encodeBaudrate(baudrate);
   cfsetspeed (&tty, speed); //set input and output speed to same baudrate

   //write back the modified  settings
   if (tcsetattr (fd, TCSANOW, &tty) != 0)
   {
      return -1;
   }
   return 0;
}


//currently there is only a subset of baudrates supported!
unsigned int Slay2Linux::encodeBaudrate(unsigned int baudrate)
{
   switch (baudrate)
   {
      case 2400:   return B2400;
      case 4800:   return B4800;
      case 9600:   return B9600;
      case 19200:  return B19200;
      case 38400:  return B38400;
      case 57600:  return B57600;
      case 115200: return B115200;
      default:     break;
   }
   return B0; //hang up
}



void Slay2Linux::flush(void)
{
   tcflush(this->fileDesc, TCIOFLUSH);
}
