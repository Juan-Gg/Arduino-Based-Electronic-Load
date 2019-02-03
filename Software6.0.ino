/**********************************************************
	DINAMIC DC LOAD SOFTWARE VERSION 6.0 - MAIN
Written by JuanGg on October 2018
Feel free to copy and make use of this code as long as you
give credit to the original author.
***********************************************************/


/**********************************************************
                      PIN DEFINITION
 **********************************************************/
////////////Display & keyboard//////////////////
const int data = 2;
const int clk = 3;
const int dig1 = 4;
const int dig2 = 5;
const int dig3 = 6;
const int dig4 = 7;
const int btnRead = 8;
const int onLed = 9;


//////////Analog & control/////////////////////
const int pwmDAC = 10;
const int fan = 11;
const int relay = 12;

const int measCurrent = A0;
const int measVoltage = A1;
const int measTemp = A2;
const int knob = A3;

/**********************************************************
                    GLOBAL VARIABLES
 **********************************************************/
////////////////////Display varibles///////////////////////
const int displayTime = 5;
int activeDigit = 0;

char characters[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                     'a', 'b', 'c', 'd', 'e', 'f', 'i', 'l', 'n', 'r', 't',
                     'p', 'v', 'º', '-', '*'
                    };
int numChars = 26;

int commonPins[] = {dig1, dig2, dig3, dig4};
byte segments[] = {B11111100, B01100000, B11011010, B11110010, B01100110,
                   B10110110, B10111110, B11100000, B11111110, B11110110,
                   B11101110, B00111110, B10011100, B01111010, B10011110,
                   B10001110, B01100000, B00011100, B00101010, B00001010,
                   B00011110, B11001110, B01111100, B11000110, B00000010,
                   B00000000
                  };


char dataOut[5] = {'8', '8', '8', '8'}; //Characters to be displayed
boolean ledsOut[] = {true, true, true, true};//Leds on {decPoint1, decPoint2, V, A}

//Keyboard variables
boolean btnsPressed[4];

///////////////////////Mode variables & timing//////////////////////////////
//Timing variables
unsigned long prevDisplayMillis = 0;
unsigned long prevRollingDisplayMillis = 0;
unsigned long prevSetMillis = 0;
unsigned long prevKnobMillis = 0;
unsigned long prevInfoMillis = 0;
unsigned long prevDebounceMillis = 0;
unsigned long prevAdqMillis = 0;

// Display timing
int rollingTime = 2000;
int infoTime = 500; //Time info is displayed
int setTime = 2000; //Idle time to set parameters. Resets when the knob is moved.
int knobTime1 = 10; //Time between knob value increments.Value adjustment.
int knobTime2 = 200; //Same as before, for mode adjustment.

//Parameter to be set 1-4, according to buttons on front pannel
byte thingToSet = 0;

//Information that is shown on the display.
byte displayMode = 1;
byte displayModeLen = 5;
byte thingToDisplay = 1;
byte thingToDisplayLen = 4;
char displayModeInfo[5][4] = {"*v**", "*a**", "*p**", "*t**", "*all"};

//Modes of operation, corresponding values and labels.
byte operationMode = 1;
byte operationModeLen = 4;
char operationModeInfo[4][4] = {"*cc*", "*cp*", "*cr*", "*ein"};

char operationModeSetInfo[4][4] = {"*c**", "*p**", "*r**", "*err"};
char errorDisplay[] = "*err";
boolean infoShown = false;
boolean showingInfo = false;

float setCurrent = 0;
float setPower = 0;
float setResistance = 0;

const float maxVoltage =  24;
const float maxTemp =  40;
const float fanOnTemp = 25;

const float maxCurrent = 4.000;
const float maxPower =  30;
const float minResistance = 1;
const float maxResistance =  99.99;

boolean loadOn = false; //Load on or off
boolean overload = false;

///////////////////////Calibration variables//////////////////////////
const float measCurrentCal = 0.00430;
const float measVoltageCal = 0.02384;

const float setCurrentCal = 825;

byte measIndex = 0;
const byte numAverages = 8;
int updateRate = 1000;

float measVoltages[numAverages];
float measCurrents[numAverages];

int knobCenter;
float knobVal = 0;

/**********************************************************
                          SETUP
 **********************************************************/
void setup() {
  //Initialize serial
  Serial.begin(9600);

  //Input-outputs
  pinMode(data, OUTPUT);
  pinMode(clk, OUTPUT);
  pinMode(dig1, OUTPUT);
  pinMode(dig2, OUTPUT);
  pinMode(dig3, OUTPUT);
  pinMode(dig4, OUTPUT);
  pinMode(onLed, OUTPUT);
  pinMode(btnRead, INPUT_PULLUP);


  pinMode(pwmDAC, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(measCurrent, INPUT);
  pinMode(measVoltage, INPUT);
  pinMode(measTemp, INPUT);
  pinMode(knob, INPUT);
  pinMode(measCurrent, INPUT);

  //DAC-ADC stuff
  setupPWM12();
  analogReference(INTERNAL);

  //KnobCentering. Varies with USB 5V or regulated 5V
  delay(100);
  for (byte i = 0; i < 5; i++)
  {
    knobCenter += analogRead(knob); //Assume no one is moving it at power up
    delay(100);
  }
  knobCenter /= 5;
  knobCenter += 10;
}

/**********************************************************
                       MAIN LOOP
 **********************************************************/
void loop()
{
  //////////////////Refresh display/////////////////////
  updateDisplay();
  if (loadOn)
    digitalWrite(onLed, HIGH);
  else
    digitalWrite(onLed, LOW);

  ///////////Measure voltage, current, temperature/////
  takeMeasurements();

  //////////Over load control & protection////////////
  //Cooling fan
  if(getTemp() > fanOnTemp)
    analogWrite(fan, constrain(map(getTemp(), fanOnTemp, maxTemp-5, 160, 255), 0, 255));
  else if(getTemp() < fanOnTemp -2)
    analogWrite(fan, 0);

  if(getVoltage() > maxVoltage || getCurrent() > maxCurrent ||
              getPower() > maxPower || getTemp() > maxTemp)
    overload = true;
  if(getVoltage() < maxVoltage -1 && getCurrent() < maxCurrent - 0.5&&
              getPower() < maxPower - 5 && getTemp() < maxTemp -3)
    overload = false;

  if (overload) //When overload conditions, show OL message and stop the load
  { //until operating conditions are restored.
    halt();
    dataOut[0] = '*';
    dataOut[1] = '0';
    dataOut[2] = 'l';
    dataOut[3] = '*';
    for (byte i = 0; i < 4; i++)
      ledsOut[i] = false;
    return;
  }

  //////////////Things to do in each operation mode////////////////
  if (loadOn)
  {
    switch (operationMode)
    {
      case 1:
        CC();
        break;
      case 2:
        CP();
        break;
      case 3:
        CR();
        break;
      case 4:
        EIn();
        break;
    }
  }
  else
    halt();

  //////////////Things to do in each display mode//////////////////
  if (thingToSet == 0)
  {
    if (displayMode == 5)
    {
      if (millis() - prevRollingDisplayMillis > rollingTime)
      {
        prevRollingDisplayMillis = millis();
        thingToDisplay ++;
        if (thingToDisplay > thingToDisplayLen)
          thingToDisplay = 1;
      }
    }
    else
      thingToDisplay = displayMode;

    switch (thingToDisplay)
    {
      case 1:
        displayNum(getVoltage());
        ledsOut[2] = true;
        ledsOut[3] = false;
        break;
      case 2:
        displayNum(getCurrent());
        ledsOut[2] = false;
        ledsOut[3] = true;
        break;
      case 3:
        displayNum(getPower());
        ledsOut[2] = true;
        ledsOut[3] = true;
        break;
      case 4:
        displayNum(getTemp());
        dataOut[2] = 'º';
        dataOut[3] = 'c';
        ledsOut[1] = false;
        ledsOut[2] = false;
        ledsOut[3] = false;
        break;
    }
  }


  /////////////////Keyboard & debounce////////////////////
  if (millis() - prevDebounceMillis > 300) //Keyboard debounce
  {
    for (byte i = 0; i < 4; i++)
    {
      if (btnsPressed[i])
      {
        thingToSet = i + 1; //1 display, 2 set, 3 mode, 4 on-off
        infoShown = false;
        showingInfo =  false;
        prevInfoMillis = millis();
        prevSetMillis = millis();
        prevDebounceMillis = millis();
        break;
      }
    }
  }

  //Reset setTime counter when the knob is turned.
  if (readKnob() != 0)
    prevSetMillis = millis();

  //Back to stand-by when inactivity time expires.
  if (thingToSet != 0 && (millis() - prevSetMillis >= setTime))
    thingToSet = 0;

  ///////////////Display, value, mode, on off adjustment/////////////
  switch (thingToSet) {
    case 1: //Set display mode
      for (byte i = 0; i < 4; i++)
      {
        dataOut[i] = displayModeInfo[displayMode - 1][i];
        ledsOut[i] = false;
      }
      if (millis() - prevKnobMillis >= knobTime2)
      {
        if (readKnob() > 1)
        {
          displayMode ++;
          if (displayMode > displayModeLen)
            displayMode = 1;
        }
        else if (readKnob() < -1)
        {
          displayMode --;
          if (displayMode < 1)
            displayMode = displayModeLen;
        }
        prevKnobMillis = millis();
      }
      break;
    case 2: //Set value of the present mode
      if ( !infoShown && !showingInfo)
      {
        for (byte i = 0; i < 4; i++)
        {
          dataOut[i] = operationModeSetInfo[operationMode - 1][i];
          ledsOut[i] = false;
        }
        showingInfo = true;
      }
      else if (showingInfo && millis() - prevInfoMillis >= infoTime)
      {
        infoShown = true;
        showingInfo = false;
      }
      if (infoShown && !showingInfo)
      {
        if (operationMode == 4) //Can't set anything on External Input Mode.
        {
          thingToSet = 0;
          break;
        }
        else
        {
          if (millis() - prevKnobMillis >= knobTime1)
          {
            knobVal = readKnob();
            prevKnobMillis = millis();
          }
          else
            knobVal = 0;

          switch (operationMode)
          {
            case 1: // Constant current adjustment.
              setCurrent += knobVal * 0.0001;
              setCurrent = constrain(setCurrent, 0, maxCurrent);
              displayNum(setCurrent);
              ledsOut[2] = false;
              ledsOut[3] = true;
              break;
            case 2: // Constant power adjustment.
              setPower += knobVal * 0.001;
              setPower = constrain(setPower, 0, maxPower);
              displayNum(setPower);
              ledsOut[2] = true;
              ledsOut[3] = true;
              break;
            case 3: // Constant resistance adjustment.
              setResistance += knobVal * 0.001;
              setResistance = constrain(setResistance, minResistance, maxResistance);
              displayNum(setResistance);
              ledsOut[2] = true;
              ledsOut[3] = true;
              break;
            default:
              for (byte i = 0; i < 4; i++)
              {
                dataOut[i] = errorDisplay[i];
                ledsOut[i] = false;
              }
          }
        }
      }
      break;
    case 3: //Set operation mode
      loadOn = false; //Turns load off for safety
      for (byte i = 0; i < 4; i++)
      {
        dataOut[i] = operationModeInfo[operationMode - 1][i];
        ledsOut[i] = false;
      }
      if (millis() - prevKnobMillis >= knobTime2)
      {
        if (readKnob() > 1)
        {
          operationMode ++;
          if (operationMode > operationModeLen)
            operationMode = 1;
        }
        else if (readKnob() < -1)
        {
          operationMode --;
          if (operationMode < 1)
            operationMode = operationModeLen;
        }
        prevKnobMillis = millis();
      }
      break;
    case 4: //Turn load on or off
      loadOn = !loadOn;
      thingToSet = 0;
      break;
  }
}

/**********************************************************
                         METHODS
 **********************************************************/

//Modes
void CC()
{
  analogWrite12(pwmDAC, setCurrent * setCurrentCal);
  digitalWrite(relay, LOW);
}

void CP() // P = V*I, I= P/V
{
  analogWrite12(pwmDAC, (setPower / getVoltage()) * setCurrentCal);
  digitalWrite(relay, LOW);
}

void CR() // I = V/R
{
  analogWrite12(pwmDAC, (getVoltage() / setResistance) * setCurrentCal);
  digitalWrite(relay, LOW);
}

void EIn()
{
  digitalWrite(relay, HIGH);
}

void halt()
{
  analogWrite12(pwmDAC, 0);
  digitalWrite(relay, LOW);
}



