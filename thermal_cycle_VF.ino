//libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MemoryFree.h>
//pins
//azefzef zefzef
#define BUTTON_PAUSE 2 // to pause during cycle
#define ENCODER_PIN_CLK 3 // Connected to CLK on KY-040
#define ENCODER_PIN_DT 4 // Connected to DT on KY-040
#define ENCODER_PIN_PRESS 5 // press button
#define ENDING_SWITCH_COLD 6 // Connected to ending switch R1. 0V when detection. 3v3 otherwise
#define ENDING_SWITCH_HOT 7 // Connected to ending switch R2. 0V when detection. 3v3 otherwise
#define RELAY_MOTOR_COLD 8 // To go to cold. Active at 0
#define RELAY_MOTOR_HOT 9  // To go to hot. Active at 0
////--------------------------------------------------------------------------------------------------------------------------------
// functions
void update_encoder_pos();
void change_scale_pos();
void encoder_reset_value();
// VAR FOR ROTARY ENCODER
int encoder_pinALast;  
int encoder_aVal;
long encoder_PosCount = 0; 
long encoder_PosLastCount = 0;
long encoder_PosRealCount = 0;
long encoder_PosLastRealCount = 0;
long encoder_PosNewScale = 0;
// VAR FOR STATE MACHINE
int state_machine = 1;
boolean new_in_state = true;
// VAR for cycles
int cycle_number = 0;
int hot_time = 0;
int cold_time = 0;
int cycle_current_value = 0;
unsigned long t =0; // actual time 
unsigned long t0 =0; //initial time
unsigned long wt =0;  //total waiting time at time t 
boolean hot_wt_ok = false; // flag going to true if hot waiting time has really been spent at hot end 
boolean cold_wt_ok = false; // flag going to true if cold waiting time has really been spent at cold end
//VAR FOR MOTOR
boolean at_cold_ok = false; // flag going to true if hot waiting time has really been spent at hot end 
boolean at_hot_ok = false; // flag going to true if cold waiting 
// VAR FOR SCREEN
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7); // PIN A4 & A5
char string[30];
// Var for UART
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
////------------------------------------------SETUP--------------------------------------------------------------------------------------
void setup() 
{ 
   // Encoder
   pinMode(ENCODER_PIN_CLK,INPUT);
   pinMode(ENCODER_PIN_DT,INPUT);
   pinMode(ENCODER_PIN_PRESS,INPUT_PULLUP);
   encoder_pinALast = digitalRead(ENCODER_PIN_CLK);

   // MOTOR
   pinMode(ENDING_SWITCH_COLD,INPUT_PULLUP); // TODO: remove pullup if 3V3
   pinMode(ENDING_SWITCH_HOT,INPUT_PULLUP); // TODO: remove pullup if 3V3
   pinMode(RELAY_MOTOR_COLD,OUTPUT);
   pinMode(RELAY_MOTOR_HOT,OUTPUT);
   digitalWrite(RELAY_MOTOR_COLD,LOW); // High means relay to NC, motor not running
   digitalWrite(RELAY_MOTOR_HOT,LOW); // High means relay to NC, motor not running

   // Button
   pinMode(BUTTON_PAUSE,INPUT_PULLUP);
   
   // UART console
   Serial.begin (9600);
   inputString.reserve(20);

   // Screen
   lcd.begin(16,2);
   lcd.setBacklightPin(3,POSITIVE);
   lcd.setBacklight(HIGH);
   lcd.setCursor(0,0);
}
////---------------------------------------LOOP-----------------------------------------------------------------------------------------
void loop()
{ 
   //------------------------------------------STATE1--------------------------------------------------------------------------------------
   if (state_machine==1)  // Init state
   {    
    Serial.print("starting state 1"); //UART console debugging
    if (new_in_state)
    {
     lcd.clear();
     lcd.setCursor (0,0);        // go to start of 2nd line
     lcd.print("Cycle program");
     lcd.setCursor (0,1);        // go to start of 2nd line
     lcd.print("Press to start");
     new_in_state = false;
    }
    // Detect button press
    if (encoder_detect_press())
    {    
     state_machine++;
     new_in_state = true;
     encoder_reset_value();
     inputString = "";
    }
   }
   //------------------------------------------STATE2--------------------------------------------------------------------------------------
   else if (state_machine==2)   // cycle number defining state
   {   
    Serial.print("starting state 2"); //UART console debugging
    if (new_in_state)
    {
     lcd.clear();
     lcd.setCursor (0,0); lcd.print("Cycle number");
     lcd.setCursor (0,1); lcd.print("Turn then press");
     new_in_state = false;
    }
    update_encoder_pos();
    if (encoder_PosLastRealCount != encoder_PosRealCount)
    {
     change_scale_pos();
     lcd.clear();
     lcd.setCursor (0,0); lcd.print("Cycle number :");
     lcd.setCursor (0,1); lcd.print(encoder_PosNewScale);
     encoder_PosLastRealCount = encoder_PosRealCount;
    }
    // detect via uart
    if (stringComplete)
    {
      if (inputString.toInt() > 0)
      encoder_PosNewScale = inputString.toInt();
      lcd.clear();
      lcd.setCursor (0,0); lcd.print("Cycle number :");
      lcd.setCursor (0,1); lcd.print(encoder_PosNewScale);
      // clear the string:
      inputString = "";
      stringComplete = false;
    }
    
    // Detect button press
    if (encoder_detect_press())
    {
     cycle_number=encoder_PosNewScale;
     state_machine++;
     new_in_state = true;
     encoder_reset_value();
     inputString = "";
    }
    Serial.print(cycle_number); //UART console debugging
   }
   //--------------------------------------------------STATE3------------------------------------------------------------------------------
   else if (state_machine==3)   // hot time defining state
   {    
    Serial.print("starting state 3"); //UART console debugging
    if (new_in_state)
    {
     lcd.clear();
     lcd.setCursor (0,0); lcd.print("Hot time (s)");
     lcd.setCursor (0,1); lcd.print("Turn then press");
     new_in_state = false;
    }
    update_encoder_pos();
    if (encoder_PosLastRealCount != encoder_PosRealCount)
    {
     change_scale_pos();
     lcd.clear();
     lcd.setCursor (0,0); lcd.print("Hot time (s)");
     lcd.setCursor (0,1); lcd.print(encoder_PosNewScale);
     encoder_PosLastRealCount = encoder_PosRealCount;
    }
    // detect via uart
    if (stringComplete)
    {
      if (inputString.toInt() > 0)
      encoder_PosNewScale = inputString.toInt();
      lcd.clear();
      lcd.setCursor (0,0); lcd.print("Hot time (s)");
      lcd.setCursor (0,1); lcd.print(encoder_PosNewScale);
      // clear the string:
      inputString = "";
      stringComplete = false;
    }
    
    // Detect button press
    if (encoder_detect_press())
    {
     hot_time=encoder_PosNewScale;
     state_machine++;
     new_in_state = true;
     encoder_reset_value();
     inputString = "";
    }
    Serial.print(hot_time); //UART console debugging
   }
   //--------------------------------------------------------STATE4------------------------------------------------------------------------
   else if (state_machine==4)   // cold time minute defining state
   {    
    Serial.print("starting state 4"); //UART console debugging
    if (new_in_state)
    {
     lcd.clear();
     lcd.setCursor (0,0); lcd.print("Cold time (s)");
     lcd.setCursor (0,1); lcd.print("Turn then press");
     new_in_state = false;
    }
    update_encoder_pos();
    if (encoder_PosLastRealCount != encoder_PosRealCount)
    {
     change_scale_pos();
     lcd.clear();
     lcd.setCursor (0,0); lcd.print("Cold time (s)");
     lcd.setCursor (0,1); lcd.print(encoder_PosNewScale);
     encoder_PosLastRealCount = encoder_PosRealCount;
    }
    // detect via uart
    if (stringComplete)
    {
      if (inputString.toInt() > 0)
      encoder_PosNewScale = inputString.toInt();
      lcd.clear();
      lcd.setCursor (0,0); lcd.print("Cold time (s)");
      lcd.setCursor (0,1); lcd.print(encoder_PosNewScale);
      // clear the string:
      inputString = "";
      stringComplete = false;
    }
    
    // Detect button press
    if (encoder_detect_press())
    {
     cold_time=encoder_PosNewScale;
     state_machine++;
     new_in_state = true;
     encoder_reset_value();
     inputString = "";
    }
    Serial.print(cold_time); //UART console debugging
   }
   //--------------------------------------------------------------STATE5------------------------------------------------------------------
   else if (state_machine==5) // show all parameters defined in previous states
   {    
    Serial.print("starting state 5"); //UART console debugging
    while (!encoder_detect_press())  // wait for button press to pass to other state
    {
     lcd.clear();
     lcd.setCursor (0,0); sprintf(string, "Cycle num:%d", cycle_number); lcd.print(string);
     lcd.setCursor (0,1); lcd.print("Press 3s..");
     delay(1000);
     lcd.clear();
     lcd.setCursor (0,0); sprintf(string, "H_t(s):%d", hot_time); lcd.print(string);
     lcd.setCursor (0,1); lcd.print("Press 3s..");
     delay(1000);
     lcd.clear();
     lcd.setCursor (0,0); sprintf(string, "C_t(s):%d", cold_time); lcd.print(string);
     lcd.setCursor (0,1); lcd.print("Press 3s..");
     delay(1000);
    }
    state_machine++;
   }
   //--------------------------------------------------------------------STATE6------------------------------------------------------------
   else if(state_machine==6) // go to cold, initial step
   {
    Serial.print("starting state 6"); //UART console debugging
    lcd.clear();
    lcd.setCursor (0,0); lcd.print("Goto init state");
    motor_go_to_cold();
    if (at_cold_ok==true)
    {
      state_machine++;  
    }
   }
   //--------------------------------------------------------------------------STATE7------------------------------------------------------
   else if(state_machine==7) // cycling
   {
    Serial.print("starting state 7"); //UART console debugging
    cycle_current_value = 0;
    while(cycle_current_value < cycle_number)
    {
      Serial.print("----"); //UART console debugging
      Serial.print(cycle_current_value); //UART console debugging
      Serial.print("----"); //UART console debugging
      Serial.print(freeMemory());//UART console memory tracking
      //initilisation of flags
      hot_wt_ok=false;
      cold_wt_ok=false;
      // handle cycles
      lcd.clear();
      lcd.setCursor (0,0); sprintf(string, "Cycle:%d", cycle_current_value); lcd.print(string);
      lcd.setCursor (10,0); sprintf(string, "/%d", cycle_number); lcd.print(string);
      // going to hot ------------------------------------------------------------------------
      lcd.setCursor (0,1); lcd.print("Going to HOT");
      motor_go_to_hot();
      //stay at hot end------------------------------------------------------------------------------------
      t0 = millis(); // arrival time at the hot end 
      wt=0; // total waiting set to 0 at arrival
      while(wt < hot_time*1000) // TODO: *60
      {
        lcd.setCursor (0,1); lcd.print("                ");
        lcd.setCursor (0,1); sprintf(string, "H_t(s):%lu", wt/1000); lcd.print(string); // to do /60
        lcd.setCursor (10,1); sprintf(string, "/%d", hot_time); lcd.print(string);
        delay(1000);//refreshing every second
        t = millis(); //get time at the end of the loop (ms)
        wt=t-t0; // calculate new total waiting time (ms)
        Serial.print("-"); //UART console debugging
        Serial.print(wt); //UART console debugging
        Serial.print("-"); //UART console debugging
        // check pause button
        if(digitalRead(BUTTON_PAUSE) == false)
        {
          while(digitalRead(BUTTON_PAUSE) == false); // wait for release
          wt = hot_time*1000; //to do *60
        }
      }
      hot_wt_ok = true;// conveyor has really spent hot_time at hot end
      Serial.print("-hot_wt_ok-"); //UART console debugging
      Serial.print(freeMemory());//UART console memory tracking 
      // going to cold--------------------------------------------------------------------------------------
      lcd.setCursor (0,1); lcd.print("                ");
      lcd.setCursor (0,1); lcd.print("Going to COLD");
      motor_go_to_cold();
      //delay(500);
      //stay at cold end---------------------------------------------------------------------------------------------------------
      t0 = millis(); // arrival time at the cold end 
      wt=0; // total waiting set to 0
      while(wt < cold_time*1000) //to do *60
      {
        lcd.setCursor (0,1); lcd.print("                ");
        lcd.setCursor (0,1); sprintf(string, "C_t(s):%lu", wt/1000); lcd.print(string); //to do /60
        lcd.setCursor (10,1); sprintf(string, "/%d", cold_time); lcd.print(string);
        delay(1000);
        t = millis(); //get time at the end of the loop (ms)
        wt=t-t0; // calculate new total waiting time (ms)
        Serial.print("-"); //UART console debugging
        Serial.print(wt); //UART console debugging
        Serial.print("-"); //UART console debugging
        // check pause button
        if(digitalRead(BUTTON_PAUSE) == false)
        {
          while(digitalRead(BUTTON_PAUSE) == false); // wait for release
          wt = cold_time*1000; // to do *60
        }
      }
      cold_wt_ok = true; // conveyor has really spent cold_time at cold end  
      Serial.print("-cold_wt_ok-"); //UART console debugging
      Serial.print(freeMemory());//UART console memory tracking
      if ((hot_wt_ok == true) && (cold_wt_ok == true))
      {
        cycle_current_value++; //cycle_current_value increments only if both hot and cold flags are true 
      }
    }
    //end of cycling---------------------------------------------------------------------------------------------------------------------------------
    if (at_cold_ok == true)
    {
      state_machine++;  
    }
   }
   //--------------------------------------------------------------------------------END-STATE------------------------------------------------
   else if(state_machine>7)
   {
     Serial.print("end of program"); //UART console debugging
     lcd.clear();
     lcd.setCursor (0,0); lcd.print("Process over");
     lcd.setCursor (0,1); lcd.print("Press reset..");
     delay(10000);
   }
  }
////-------------------------------------FUNCTIONS-------------------------------------------------------------------------------------------

////--------------------------------------------------------------------------------------------------------------------------------
void change_scale_pos()
{
   if (state_machine==2)
   {
    // x10 scale
    encoder_PosNewScale = 10*encoder_PosRealCount;
   }
   else if (state_machine==3)
   {
    // Logarithmic scale
    if (encoder_PosRealCount <= 10) encoder_PosNewScale = encoder_PosRealCount;
    else if (encoder_PosRealCount <= 18) encoder_PosNewScale = 10 + (encoder_PosRealCount - 10)*5;
    else if (encoder_PosRealCount <= 23) encoder_PosNewScale = 50 + (encoder_PosRealCount - 18)*10;
    else if (encoder_PosRealCount <= 31) encoder_PosNewScale = 100 + (encoder_PosRealCount - 23)*50;
    else if (encoder_PosRealCount <= 36) encoder_PosNewScale = 500 + (encoder_PosRealCount - 31)*100;
    else if (encoder_PosRealCount <= 1000) encoder_PosNewScale = 1000 + (encoder_PosRealCount - 36)*1000;
   }
   else if (state_machine==4)
   {
    // Logarithmic scale
    if (encoder_PosRealCount <= 10) encoder_PosNewScale = encoder_PosRealCount;
    else if (encoder_PosRealCount <= 18) encoder_PosNewScale = 10 + (encoder_PosRealCount - 10)*5;
    else if (encoder_PosRealCount <= 23) encoder_PosNewScale = 50 + (encoder_PosRealCount - 18)*10;
    else if (encoder_PosRealCount <= 31) encoder_PosNewScale = 100 + (encoder_PosRealCount - 23)*50;
    else if (encoder_PosRealCount <= 36) encoder_PosNewScale = 500 + (encoder_PosRealCount - 31)*100;
    else if (encoder_PosRealCount <= 1000) encoder_PosNewScale = 1000 + (encoder_PosRealCount - 36)*1000;
   }
   else
   {
    encoder_PosNewScale = encoder_PosRealCount;
   }
}
////--------------------------------------------------------------------------------------------------------------------------------
void encoder_reset_value() // reset all encoder variables
{
   encoder_PosCount = 0; 
   encoder_PosLastCount = 0;
   encoder_PosRealCount = 0;
   encoder_PosLastRealCount = 0;
   encoder_PosNewScale = 0;
}
////--------------------------------------------------------------------------------------------------------------------------------
bool encoder_detect_press()  // detect encoder press
{
   if (digitalRead(ENCODER_PIN_PRESS)==false)
   {
    while (digitalRead(ENCODER_PIN_PRESS)!=true); // wait button to be released
    delay(50); // 50ms short delay
    return true;
   }
   return false;
}
////--------------------------------------------------------------------------------------------------------------------------------
void update_encoder_pos() {
   // detect movement
   encoder_aVal = digitalRead(ENCODER_PIN_CLK);
   if (encoder_aVal != encoder_pinALast)  // Means the knob is rotating
   {
    if (digitalRead(ENCODER_PIN_DT) != encoder_aVal)encoder_PosCount --; // Dir left
    else encoder_PosCount++; // Dir right
   } 
   encoder_pinALast = encoder_aVal;
   if (encoder_PosLastCount != encoder_PosCount) // if changement, update real count
   {
    if (encoder_PosCount < 0)encoder_PosCount = 0; // cannot be negative
    encoder_PosLastCount = encoder_PosCount;    
    if (encoder_PosCount % 2 == 0) encoder_PosRealCount = encoder_PosCount/2; // divide by 2 to avoid jumps    
   }   
}
////--------------------------------------------------------------------------------------------------------------------------------
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
} 
////--------------------------------------------------------------------------------------------------------------------------------
void motor_go_to_cold()
{   
  // go to cold
  Serial.print("-going to cold-"); //UART console debugging
  while (at_cold_ok !=true)
  {
    while (digitalRead(ENDING_SWITCH_COLD)!=false)//!at_cold())
    {
        digitalWrite(RELAY_MOTOR_COLD,HIGH); // HIGH means relay to N0, motor running
        delay(100);
    }
    digitalWrite(RELAY_MOTOR_COLD,LOW); // Stop motor
    delay(1000);
    Serial.print("--at cold?--"); //UART console debugging
    if (digitalRead(ENDING_SWITCH_COLD)==false)
    {
      at_cold_ok=true;
      Serial.print("--cold--"); //UART console debugging
    }
  }
  at_hot_ok=false; 
}
////--------------------------------------------------------------------------------------------------------------------------------
void motor_go_to_hot()
{   
  // go to hot
  Serial.print("-going to hot-"); //UART console debugging
  while (at_hot_ok!=true)
  {
    while (digitalRead(ENDING_SWITCH_HOT)!=false)//at_hot())
    {
      //if (digitalRead(ENDING_SWITCH_HOT)!=false)
      //{
        digitalWrite(RELAY_MOTOR_HOT,HIGH); // HIGH means relay to N0, motor running
        delay(100);
      //}
    }
    digitalWrite(RELAY_MOTOR_HOT,LOW); // Stop motor
    delay(1000);
    Serial.print("--at hot?--"); //UART console debugging
    if (digitalRead(ENDING_SWITCH_HOT)==false)
    {
      at_hot_ok=true;
      Serial.print("--hot--"); //UART console debugging
    }    
  }
  at_cold_ok=false;   
}
////--------------------------------------------------------------------------------------------------------------------------------

