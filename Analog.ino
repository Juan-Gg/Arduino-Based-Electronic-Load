/**********************************************************
	DINAMIC DC LOAD SOFTWARE VERSION 6.0 - ANALOG
Written by JuanGg on October 2018
Feel free to copy and make use of this code as long as you
give credit to the original author.
***********************************************************/


//PWM DAC
//https://arduino.stackexchange.com/questions/12718/increase-pwm-bit-resolution
/* Configure digital pins 9 and 10 as 16-bit PWM outputs. */
void setupPWM12()
{
  DDRB |= _BV(PB1) | _BV(PB2);        /* set pins as outputs */
  TCCR1A = _BV(COM1A1) | _BV(COM1B1)  /* non-inverting PWM */
           | _BV(WGM11);                   /* mode 14: fast PWM, TOP=ICR1 */
  TCCR1B = _BV(WGM13) | _BV(WGM12)
           | _BV(CS10);                    /* no prescaling */
  ICR1 = 0xfff;                      /* TOP counter value */
}

/* 12-bit version of analogWrite(). Works only on pins 9 and 10. */
void analogWrite12(uint8_t pin, uint16_t val)
{
  val = constrain(val,0,4096);
  switch (pin) {
    case  9: OCR1A = val; break;
    case 10: OCR1B = val; break;
  }
}

//Voltage-Current-Temp Measurement
void takeMeasurements()
{
  if(millis() - prevAdqMillis >= updateRate/numAverages)
  {
    prevAdqMillis = millis();
    if(measIndex == numAverages -1)
      measIndex = 0;
    else
      measIndex ++;
    measVoltages[measIndex] = analogRead(measVoltage) * measVoltageCal;
    measCurrents[measIndex] = analogRead(measCurrent) * measCurrentCal;
  }
}

float getVoltage()
{
  float sum = 0;
  for(byte i=0; i<numAverages; i++)
    sum += measVoltages[i];
  return sum/=numAverages;
}

float getCurrent()
{
  float sum = 0;
  for(byte i=0; i<numAverages; i++)
    sum += measCurrents[i];
  return sum/=numAverages;
}

float getPower()
{
  return getVoltage() * getCurrent();
}

float getTemp()
{
  return int(-27 * log(analogRead(measTemp)) + 190);
}
