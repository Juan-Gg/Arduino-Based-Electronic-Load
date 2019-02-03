/**********************************************************
	DINAMIC DC LOAD SOFTWARE VERSION 6.0 - UI
Written by JuanGg on October 2018
Feel free to copy and make use of this code as long as you
give credit to the original author.
***********************************************************/



//Display

  //Takes 0-99.99 float and puts it on the dataOut
  //array to be displayed. Also handles decimal points.
  void displayNum(float num)
  {
    if (num >= 0 && num < 100)
    {
      char data[6];
      dtostrf(num, 4, 3, data);
      for (byte i = 0, j = 0; i < 6; i++, j++)
      {
        if (data[i] != '.')
          dataOut[j] = data[i];
        else if (i == 1)
        {
          ledsOut[0] = true;
          ledsOut[1] = false;
          j--;
        }
        else if (i == 2)
        {
          ledsOut[0] = false;
          ledsOut[1] = true;
          j--;
        }
      }
    }
    else
    {
      for (byte i = 0; i < 4; i++)
        dataOut[i] = 'e';
    }
  }

  //Refreshes display and reads keyboard.
  void updateDisplay()
  {
    if (millis() - prevDisplayMillis >= displayTime)
    {
      prevDisplayMillis = millis();
      if (activeDigit == 3)
        activeDigit = 0;
      else
        activeDigit ++;
      displayOut(activeDigit, dataOut[activeDigit], ledsOut[activeDigit]);
      btnsPressed[activeDigit] = !digitalRead(btnRead);
    }
  }
  //Sends didgits from the dataOut array to the actual display.
  void displayOut(int digit, char dispOut, boolean dispLeds)
  {
    for (int i = 0; i < 4; i++)
      digitalWrite(commonPins[i], LOW);
    for (int pos = 0; pos < numChars; pos++)
    {
      if (dispOut == characters[pos])
      {
        if (dispLeds)
          shiftOut(data, clk, LSBFIRST, segments[pos] + B1);
        else
          shiftOut(data, clk, LSBFIRST, segments[pos]);
        digitalWrite(commonPins[digit], HIGH);
        break;
      }
    }
  }

  //Reads the knob as an analog input and outputs a +- value.
  int readKnob()
  {
    int knobVal = analogRead(knob);

    if (knobVal < knobCenter - 400)
      return -1000;
    else if (knobVal > knobCenter +400)
      return 1000;

    if (knobVal > knobCenter + 10)
      return map(knobVal, knobCenter + 10, 1023, 0, 100);
    else if (knobVal < knobCenter - 10)
      return map(knobVal, knobCenter - 10, 0, 0, -100);
    return 0;
  }
