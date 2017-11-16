#ifndef AY38910_H_
#define AY38910_H_
#include <stdint.h>
class AY38910
{
private:
  volatile uint8_t* _port;
  int _BDIR, _BC1;
public:
  AY38910(volatile uint8_t* port, volatile uint8_t* portDDR, int BDIR, int BC1); //BC2 should be pulled high (+5v)
  void Send(uint8_t addr, uint8_t data);
};
#endif
