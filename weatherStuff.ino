#include "application.h"
#include "LiquidCrystal.h"

LiquidCrystal lcd(D0, D1, D2, D3, D4, D5);

void setup(void)
{  
    //Subscribe to get the actions from IFTTT
    Particle.subscribe("WeatherTodayDK", conditionHandler);
    Particle.subscribe("MaxAndMinWeather", tempHandler);
    Particle.subscribe("testerOwO", timeSyncHandler, ALL_DEVICES);
    Serial.begin(9600);
    lcd.begin(16,2);
    lcd.print("---");

    // Setting the time for CEST
    Time.zone(+2);
	displayTime();
}

//Handler for when I receive temperature data
void tempHandler(const char *event, const char *data) {
    int high;
    Log.info("event=%s data=%s", event, (data ? data : "NULL"));
    //Retrieve the two temperature values from the string using C++'s sscanf() 
    if (sscanf(data, "%d", &high) == 1) {
        
        //Create a string with the temp value added in
        char highBuffer[64];
        sprintf(highBuffer, "High Temp:  %d", high);
        
        lcd.setCursor(0, 1);
        lcd.print(highBuffer);
    }
    
    else {
        lcd.setCursor(0, 1);
        lcd.print("High Temp: Error");
    }
}


void conditionHandler(const char *event, const char *data) {
  // check data (what do I actually receive)
  // make string with condition only (cloudy, rainy, sunny)
  // print it somewhere without overwriting other useful stuff
}

void displayTime() {
    
    //http://strftime.org for formatting info. This displays the time.
    Log.info(Time.format(Time.now(), "It's %H:%M")); 
    char timeBuffer[64];
    sprintf(timeBuffer, Time.format(Time.now(), "%H:%M"));
    lcd.setCursor(0, 0);
    lcd.print(timeBuffer);
}

SerialLogHandler logHandler;
int i = 0;
void timeSyncHandler(const char *event, const char *data) {
  i++;
  Log.info("%d: event=%s data=%s", i, event, (data ? data : "NULL"));
}

int lastChecked = 0;

void loop() {
    
    //Every minute sync the clock with the cloud and update the time on the display.
    if (millis() - lastChecked > 60000) {
        Particle.syncTime();
        displayTime();
        lastChecked = millis();
    }
}
