#include "AY38910.h"
#include <Arduino.h>
AY38910::AY38910(volatile uint8_t* port, volatile uint8_t* portDDR, int BDIR, int BC1)
{
  _port = port;
  _BDIR = BDIR;
  _BC1 = BC1;

  *portDDR = 0xFF; //Set port to all outputs
  pinMode(_BDIR, OUTPUT);
  pinMode(_BC1, OUTPUT);
}

void AY38910::Send(uint8_t addr, uint8_t data)
{
  for(int i = 0; i<2; i++)
  {
    digitalWrite(_BC1, LOW); //Set chip to "inactive"
    digitalWrite(_BDIR, LOW);
    *_port = i==0 ? addr : data; //Send address first, then data payload
    digitalWrite(_BC1, HIGH); //Set chip to "latch"
    digitalWrite(_BDIR, HIGH);
    digitalWrite(_BC1, LOW); //Set chip to "write"
    digitalWrite(_BDIR, HIGH);
    digitalWrite(_BC1, LOW); //Set chip to "inactive"
    digitalWrite(_BDIR, LOW);
  }
}
