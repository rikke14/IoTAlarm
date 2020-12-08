#include "application.h"
#include "LiquidCrystal.h"
#include "string.h"

#define CLEAR "                "

#define DEBOUNCE 350

LiquidCrystal lcd(D0, D1, D2, D3, D4, D5);
SerialLogHandler logHandler;
bool buttonPressFlag = false;   
bool alarmFlag = false;   
bool alarmDoneFlag = false;
bool pressedCountUp = false;   
bool pressedSelect = false;
bool alarmInit = false;
bool checkAlarmFlag = false;

int hh = 0;
int mm = 0;
int currenthh = 0;
int currentmm = 0;
int count = 0;
int selectCount = 0;
int LCDlight = A3;
int PWMpin = A1;
int timeout = 0;
int alarmStart = 0;
unsigned long checkAlarm2 = 0;
int lastChecked = 0;
unsigned long checkAlarm = 0;

char highBuffer[64]; 
char condition[64];

// two constants used to test weather information
const char *temp = "7";
const char *con = "Partly Cloudy";

// enum used to keep track on states
enum State {
	TIME_ALARM,
	WEATHER_STATION,
    SETUP_ALARM,
    ALARM,
    ALARM_DONE
};

State state = TIME_ALARM;

void setup(void)
{  
    //Subscribe to get the actions from IFTTT
    Particle.subscribe("WeatherTodayDK", conditionHandler);
    Particle.subscribe("MaxWeatherDK", tempHandler);
    Serial.begin(9600);
    lcd.begin(16,2);

    // Setting the time for CET
    Time.zone(+1);
	displayTime();

    // pwm pin
    pinMode(PWMpin, OUTPUT);  

    // LCD light pin
    pinMode(LCDlight, OUTPUT);  
    digitalWrite(LCDlight, HIGH); 

    // Interrupt pins
    pinMode(D6, INPUT_PULLDOWN);                                     
    attachInterrupt(D6, buttonPressInterrupt, RISING);  
    pinMode(A4, INPUT_PULLDOWN);                                     
    attachInterrupt(A4, alarmButton, RISING);    

    // function used to test weather data
    tempF();   
}

// interrupt handler (default)
void buttonPressInterrupt(){                                
    buttonPressFlag = true;
}

// interrupt handler (default)
void alarmButton(){                                
    alarmFlag = true;
    //alarmDoneFlag = true;
}

//Handler for when I receive temperature data from IFFFT
void tempHandler(const char *event, const char *data) {
    int high;
    Log.info("event=%s data=%s", event, (data ? data : "NULL"));
    //Retrieve the temperature value from the string using C++'s sscanf() 
    if (sscanf(data, "%d", &high) == 1) {
        //Create a string with the temp value added in
        sprintf(highBuffer, "%d C", high);
        
    }

}

// recieves todays weather condition from IFFFT
void conditionHandler(const char *event, const char *data) {
  // check data (what do I actually receive)
  // make string with condition only (cloudy, rainy, sunny)
  // print it somewhere without overwriting other useful stuff
    Log.info("event=%s data=%s", event, (data ? data : "NULL"));
    if(strlen(data)>64)
        strncpy(condition,"error",64);
    strncpy(condition,data,64);
}

// function used to print time and date in the right format
void displayTime() {
    const char * buff;
    //http://strftime.org for formatting info. This displays the time.
    Log.info(Time.format(Time.now(), "It's %H:%M")); 
    buff = (Time.format(Time.now(), "%H")); 
    currenthh = atoi(buff);
    buff = (Time.format(Time.now(), "%M")); 
    currentmm = atoi(buff);
    char timeBuffer[64];
    sprintf(timeBuffer, Time.format(Time.now(), "%H:%M %a %d %b"));
    lcd.setCursor(0, 0);
    lcd.print(timeBuffer);
}

// changes between time and weather information
void changeDisplay(){
    clearLCD();
    if (state == TIME_ALARM){
        state = WEATHER_STATION;
        weatherAgent ();
        wait();
    }
    else{
        state = TIME_ALARM;
        displayTime();
        wait();
    }
    
}

// prints the time and date on LCD, and updates time every minute
void timeAgent (){
    //Every minute sync the clock with the cloud and update the time on the display.
    if (millis() - lastChecked > 60000) {
        Particle.syncTime();
        displayTime();
        lastChecked = millis();
    }
    if (alarmDoneFlag == true){
        printString(hh,mm,3);
        }
    analogWrite(PWMpin, 0, 0);
}

// prints the weather information on LCD
void weatherAgent (){
    lcd.setCursor(0, 0);
    lcd.print(condition);
    lcd.setCursor(0, 1);
    lcd.print(highBuffer);
}

// main loop
//______________________________________________________________________________
void loop() {
    if (alarmFlag == true){
        Log.info("alarm pressed");
        state = SETUP_ALARM;
        detachInterrupt(D6);
        detachInterrupt(A4);
        attachInterrupt(D6, countUp, RISING);                                    
        attachInterrupt(A4, select, RISING);  
        alarmFlag = false;
        clearLCD();
        wait();
    }

    if (millis() - timeout > 65000) {
        digitalWrite(LCDlight, LOW);
        timeout = millis();
        detachInterrupt(D6);
        detachInterrupt(A4);
        attachInterrupt(D6, lightLCD, RISING);                                    
        attachInterrupt(A4, lightLCD, RISING); 
    }

    if (alarmDoneFlag == true){
        if (checkAlarmFlag == true){
            if (millis() - checkAlarm2 > checkAlarm) {
                digitalWrite(LCDlight, HIGH);
                clearLCD();
                timeout = millis();
                analogWrite(PWMpin, 127, 200);
                detachInterrupt(D6);
                detachInterrupt(A4);
                attachInterrupt(D6, stopPWM, RISING, 1);                                    
                attachInterrupt(A4, stopPWM, RISING, 1); 
                lcd.setCursor(0, 0);
                lcd.print("PRESS ANY BUTTON");
                lcd.setCursor(0, 1);
                lcd.print("TO CONTINUE");
                checkAlarmFlag = false;
                state = ALARM;

            }   
        }
    }


    if (buttonPressFlag == true){
        changeDisplay();
        buttonPressFlag = false;
        }

    switch(state){
    case TIME_ALARM: 
        timeAgent(); 
        break;
    case WEATHER_STATION: 
        //weatherAgent(); 
        break;
    case SETUP_ALARM:
        alarmSetup();
        break;
    case ALARM:
        //delay(200);
        break;
    case ALARM_DONE:
        clearLCD();
        state = TIME_ALARM;
        break;
    default: 
        timeAgent(); 
        break;
}
}
//______________________________________________________________________________

// used to test weather recieved 
void tempF(){
    char conBuffer[64]; 
    if(strlen(con)>64)
        strncpy(condition,"error",64);
        strncpy(condition,con,64);
        int high = 0;
    if (sscanf(temp, "%d", &high) == 1) {
        //Create a string with the temp value added in
        sprintf(highBuffer, "%d C", high);
        
    }
}

// clears the LCD
void clearLCD(){
    lcd.setCursor(0, 0);
    lcd.print(CLEAR);
    lcd.setCursor(0, 1);
    lcd.print(CLEAR);
}

// prints the alarm time in 3 different ways
void printString (int hh, int mm, int slot){
    char buffer [64];
    char arrow [12];
    sprintf(buffer, "%d:%d",hh,mm);
    sprintf(arrow, "^^");

    if (slot == 1){
        lcd.setCursor(0, 0);
        lcd.print(buffer);
        lcd.setCursor(0, 1);
        lcd.print(arrow);
        Log.info(buffer);
    }
    else if (slot == 2){
        lcd.setCursor(0, 0);
        lcd.print(buffer);
        lcd.setCursor(0, 1);
        lcd.print(CLEAR);
        lcd.setCursor(3, 1);
        lcd.print(arrow);
        Log.info(buffer);
    }
    else if (slot == 3){
        lcd.setCursor(0, 1);
        lcd.print("Alarm:");
        lcd.setCursor(11, 1);
        lcd.print(buffer);
    }
    
    
}

// setup for alarm time
void alarmSetup(){
    if (alarmInit == false){
        hh = 0;
        mm = 0;
        count = 0;
        selectCount = 0;
        alarmInit = true;
        printString(hh,mm,1);
        Log.info("%d",alarmInit);
        pressedCountUp = false;
        pressedSelect = false;
        wait();
    }

    if (pressedCountUp == true){
        count++;
        if (selectCount == 0){
            if (count == 24){
                count = 0; 
            }
            hh = count;
            printString(hh,mm,1);
        }
        else if (selectCount == 1){
            if (count == 60){
                count = 0; 
            }
            mm = count;
            printString(hh,mm,2);
        }
    pressedCountUp = false;
    wait();
    }
    
    if (pressedSelect == true){
        selectCount++;
        if (selectCount == 1){
        count = 0;
        printString(hh,mm,2);
        wait();
    }
        else if (selectCount == 2){
        wait();
        wait();
        state = TIME_ALARM;
        alarmInit = false;
        clearLCD();
        displayTime();
        setAlarmTime();
        setupDone();
    }
        pressedSelect = false;
        wait();
    }
    
}

// interrupt handler (set alarm mode)
void countUp(){                                
    pressedCountUp = true;
}

// interrupt handler (set alarm mode)
void select(){                                
    pressedSelect = true;
}

// sets interrupts back to default
void setupDone(){
    detachInterrupt(D6);
    detachInterrupt(A4);
    attachInterrupt(D6, buttonPressInterrupt, RISING);                                    
    attachInterrupt(A4, alarmButton, RISING);   
    alarmDoneFlag = true;
    wait();
}

// for deboune on buttons
void wait(){
    noInterrupts();
    timeout = millis();
    delay(DEBOUNCE);
    interrupts();
}

// turns on the light on LCD
void lightLCD(){
    digitalWrite(LCDlight, HIGH);
    setupDone();
    wait();
}

// sets the timer for the alarm
void setAlarmTime(){
    unsigned long seconds;

    if (currentmm < mm){
        seconds = (mm-currentmm)*60;
        if (currenthh < hh){
            seconds += (hh-currenthh)*60*60;
        }
        else if (currenthh > hh){
            seconds += (24-currenthh+hh)*60*60;
        }

    }
    else if (currentmm > mm){
        seconds = (60-currentmm+mm)*60;
        if (currenthh < hh){
            seconds += ((hh-currenthh)*60*60)-(60*60);
        }
        else if (currenthh > hh){
            seconds += ((24-currenthh+hh)*60*60)-(60*60);
        }
    }    
    else if (currentmm == mm){
        seconds = 0;
        if (currenthh < hh){
            seconds += (hh-currenthh)*60*60;
        }
        else if (currenthh > hh){
            seconds += (24-currenthh+hh)*60*60;
        }
    }
    checkAlarm = seconds*1000;
    checkAlarm2 = millis();
    checkAlarmFlag = true;
}

// interrupt handler (stop the alarm after its triggered)
void stopPWM(){
    analogWrite(PWMpin, 0, 0);
    state = ALARM_DONE;
    setupDone();
}