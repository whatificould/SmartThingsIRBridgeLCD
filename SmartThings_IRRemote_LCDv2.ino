//*****************************************************************************
/// @file
/// @brief
///   Arduino SmartThings Shield IR Bridge & LCD Status
///   Author: gilbert@whatificould.net
///   Date: 03.03.2014
/// @note
///              ______________
///             |              |
///             |         SW[] |
///             |[]RST         |
///             |         AREF |--
///             |          GND |--
///             |           13 |--X LED
///             |           12 |--X LCD_RS
///             |           11 |--X LCD_E
///           --| 3.3V      10 |--
///           --| 5V         9 |--
///           --| GND        8 |--
///           --| GND          |
///           --| Vin        7 |--X LCD_D4
///             |            6 |--X LCD_D5
///           --| A0         5 |--X LCD_D6
///           --| A1    ( )  4 |--X LCD_D7
///           --| A2         3 |--X THING_RX
///           --| A3  ____   2 |--X THING_TX
///           --| A4 |    |  1 |--
///           --| A5 |    |  0 |--
///             |____|    |____|
///                  |____|
///
/// LCD_VSS  >  Ground
/// LCD_VDD  >  5V
/// LCD_V0   >  10k Pot
/// LCD_RW   >  Ground
///
#include <SoftwareSerial.h> //TODO need to set due to some weird wire language linker, should we absorb this whole library into smartthings
#include <SmartThings.h>
#include <IRremote.h>
#include <LiquidCrystal.h>

//*****************************************************************************
// Pin Definitions    | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                    V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
#define PIN_LED         9
#define PIN_RECV        10
#define PIN_THING_RX    3
#define PIN_THING_TX    2

//*****************************************************************************
// Global Variables   | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                    V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
SmartThingsCallout_t messageCallout;    // call out function forward decalaration
SmartThings smartthing(PIN_THING_RX, PIN_THING_TX, messageCallout);  // constructor

bool isDebugEnabled;    // enable or disable debug in this example
int stateLED;           // state to track last set value of LED
String lcd_defaultline1; // first line in LCD


IRrecv irrecv(PIN_RECV);
IRsend irsend;
decode_results results;
//*****************************************************************************
// LCD                | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                    V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

//*****************************************************************************
// API Functions    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                  V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
void setup()
{
  // setup default state of global variables
  isDebugEnabled = true;
  stateLED = 0;                 // matches state of hardware pin set below

  // setup hardware pins 
  pinMode(PIN_LED, OUTPUT);     // define PIN_LED as an output
  digitalWrite(PIN_LED, LOW);   // set value to LOW (off) to match stateLED=0

  if (isDebugEnabled)
  { // setup debug serial port
    Serial.begin(9600);         // setup serial with a baud rate of 9600
    Serial.println("setup..");  // print out 'setup..' on start
  }

  irrecv.enableIRIn(); // Start the receiver
}

//*****************************************************************************
void loop()
{
  // run smartthing logic
  smartthing.run();  

  if (irrecv.decode(&results)) 
  {
    white();
    irrecv.resume(); // Receive the next value
    Serial.println(results.value, HEX);
    //dump(&results);

    //EXAMPLE: smartthing.send("HEX,XXXCODE");    
    String irCmd;

    if (results.decode_type == NEC) {
      irCmd = String(results.value, HEX) + "," + "NEC" + String(results.bits, DEC) + ":" + String(results.value, HEX);
    } 
    else if (results.decode_type == SONY) {
      irCmd = String(results.value, HEX) + "," + "SNY" + String(results.bits, DEC) + ":" + String(results.value, HEX);
    } 
    else if (results.decode_type == RC5) {
      irCmd = String(results.value, HEX) + "," + "RC5" + String(results.bits, DEC) + ":" + String(results.value, HEX);
    } 
    else if (results.decode_type == RC6) {
      irCmd = String(results.value, HEX) + "," + "RC6" + String(results.bits, DEC) + ":" + String(results.value, HEX);
    }
    else
    {
      irCmd = String(results.value, HEX) + "," + "RAW" + String(results.bits, DEC) + ":";
    }
    Serial.println(irCmd);
    smartthing.send(irCmd);
    irCmd = "";
  }

}

//*****************************************************************************
void on()
{
  stateLED = 1;                 // save state as 1 (on)
  digitalWrite(PIN_LED, HIGH);  // turn LED on
  smartthing.shieldSetLED(1, 1, 1);
  Serial.println("on");
}

void off()
{
  stateLED = 0;                 // set state to 0 (off)
  digitalWrite(PIN_LED, LOW);   // turn LED off
  smartthing.shieldSetLED(0, 0, 1);
  Serial.println("off");
}

//*****************************************************************************
void messageCallout(String message)
{

  smartthing.shieldSetLED(0, 0, 0);
  // if debug is enabled print out the received message
  if (isDebugEnabled)
  {
    Serial.print("Rx: '");
    Serial.print(message);
    Serial.println("' ");
  }

  String type = message.substring(0,3);
  int startCode = message.indexOf(':');
  String lenStr = message.substring(3,startCode);
  String codeStr = message.substring(startCode + 1);
  unsigned long code;

  //turn the hex string to a long unsigned
  if(type != "RAW")
    code = stringToNum(codeStr,16); //will not work for RAW

  int len =  stringToNum(lenStr,10);

  //For each type - NEC,SON,PAN,JVC,RC5,RC6,etc...the first 3  
  if(type == "NEC")
  {
    Serial.println("NEC-SEND");
    Serial.println(len);
    Serial.println(code,HEX);

    irsend.sendNEC(code,len);
    irrecv.enableIRIn();
  }
  else if(type == "SNY")
  {
    irsend.sendSony(code,len);
    irrecv.enableIRIn();
  }
  else if(type == "RC5")
  {
    irsend.sendRC5(code,len);
    irrecv.enableIRIn();
  }
  else if(type == "RC6")
  {
    irsend.sendRC6(code,len);
    irrecv.enableIRIn();
  } 

//*****************************************************************************
  if (message.equals("Aon"))
    {
      lcd.clear();
      lcd.print(lcd_defaultline1);
      lcd.setCursor(0,1);
      lcd.print("Light A ON  ");

  }
  else if (message.equals("Aoff"))
  { 
      lcd.clear();
      lcd.print(lcd_defaultline1);
      lcd.setCursor(0,1);
      lcd.print("Light A Off ");
     
  }
  else if (message.equals("Bon"))
  {
      lcd.clear();
      lcd.print(lcd_defaultline1);
      lcd.setCursor(0,1);
      lcd.print("Light B On  ");
       
  }
  else if (message.equals("Boff"))
  {
    lcd.clear();
    lcd.print(lcd_defaultline1);
    lcd.setCursor(0,1);
    lcd.print("Light B Off ");
   
  }
  else if (message.equals("on"))
  {
    lcd.clear();
    lcd.print(lcd_defaultline1);
    lcd.setCursor(0,1);
    lcd.print("Lights On");
   
  }
  else if (message.equals("off"))
  {
    lcd.clear();
    lcd.print(lcd_defaultline1);
    lcd.setCursor(0,1);
    lcd.print("Lights Off ");
   
  }  
}

// Dumps out the decode_results structure.
// Call this after IRrecv::decode()
// void * to work around compiler issue
//void dump(void *v) {
//  decode_results *results = (decode_results *)v
void dump(decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } 
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } 
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } 
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } 
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  }
  else if (results->decode_type == PANASONIC) {	
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->panasonicAddress,HEX);
    Serial.print(" Value: ");
  }
  else if (results->decode_type == JVC) {
     Serial.print("Decoded JVC: ");
  }
  Serial.print(results->value, HEX);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 0; i < count; i++) {     if ((i % 2) == 1) {       Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    } 
    else {
      Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println("");
}

//*****************************************************************************
// Local Functions  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                  V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************

unsigned long stringToNum(String s, int base) //10 for decimal, 16 for hex
{
  unsigned long i = 0;
  unsigned long value = 0;
  unsigned long place = s.length();
  char c;
  unsigned long sign = 1;

  for(i; i < s.length(); i++) 
  {     
    place--;     
    c = s[i];     
    if(c == '-') 
    {       
      sign = -1;     
    } else if (c >= '0' && c <= '9')  //0 to 9     
    {       
      value += ( c - '0') *  exponent(base,place);     
    } else if (c >= 'A' && ((c - 'A' + 10) < base))  //65     
    {       
      value += (( c - 'A') + 10) *  exponent(base,place);     
    }     
      else if (c >= 'a' && (c - 'a' +  10) < base)  //97
    {
      value += (( c - 'a') + 10) *  exponent(base,place);
    }     
  }
  value *= sign;
  return value;  
}

unsigned long exponent(int num, int power)
{
  unsigned long total = num;
  unsigned long i = 1;
  for(power;  i < power; i++)
  {
    total *=  num;
  }
  return (power == 0) ? 1 : total;

}

void green()
{
  smartthing.shieldSetLED(0, 1, 0);
}

void blue()
{
  smartthing.shieldSetLED(0, 0, 1);
}
void red()
{
  smartthing.shieldSetLED(1, 0, 0);
}
void white()
{
  smartthing.shieldSetLED(1, 1, 1);
}


void lightsoff()
{
  delay(100);
  smartthing.shieldSetLED(0, 0, 0);
}
