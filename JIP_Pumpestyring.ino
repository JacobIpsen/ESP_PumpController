#include <DateTime.h>
#include <ESPDateTime.h>
#include <TimeElapsed.h>

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


struct LogData{
  String LogDateAndTime;
  String Message;
  bool IsError = false;
};


// Replace with your network credentials
const char* ssid     = "FPHome";
const char* password = "cyberjoe";
WiFiServer server(80);


String header;

 
// constants won't change. They're used here to set pin numbers:
const int Float1Pin = 5;    // D1 the number of the pushbutton pin
const int Float2Pin = 4;    // D2 the number of the pushbutton pin
const int RelayPin = 14;  // D5
const int LEDPin = 12; //D6

// Variables will change:
int LowFloaterState = HIGH;
int HighFloaterState = HIGH;
int RelayState = LOW;

int Float1State = LOW;             // the current reading from the input pin
int Float2State = LOW;             // the current reading from the input pin
int lastFloat1State = LOW;   // the previous reading from the input pin
int lastFloat2State = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long Float1lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long Float2lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned long Float1StateSec = 0 ;
unsigned long Float2StateSec = 0;

String LocalIP = "";

//PumpStop
int StopDelayTime = 10000;    //1000 = 1 sec , This cannot be 0 !!!
int StopPumpTime = 0;

//LowSensor Triggered to LONG time, Start Pump for StopDelayTime milisec
int LowSensorGuardTime =  600;  //600 = 10 Min

//LogArray
LogData LogArray[60];
int LogArrayUpperIndex = 59;


// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;



WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


void setup() {
  Serial.begin(115200);
  setupSensor();
  setupWiFi();
  setupDateTime();

  server.begin();
  
 // timeClient.begin();
}



void setupSensor() {
  pinMode(Float1Pin, INPUT_PULLUP);
  pinMode(Float2Pin, INPUT_PULLUP);

  lastFloat1State = digitalRead(Float1Pin);
  lastFloat2State= digitalRead(Float2Pin);

  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, RelayState);
  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, RelayState);
  Serial.println("Ready to start....");
  
}


void setupDateTime() {
  // setup this after wifi connected
  // you can use custom timeZone,server and timeout
  // DateTime.setTimeZone(-4);
  //   DateTime.setServer("asia.pool.ntp.org");
  //   DateTime.begin(15 * 1000);
  DateTime.setTimeZone(2);

  int retryCount  = 0 ;
  
  while(!DateTime.isTimeValid()){
   retryCount += 1;
   DateTime.begin(15 * 1000);
   if (!DateTime.isTimeValid()) {
     Serial.println("Failed to get time from server. Retry:" + (String)retryCount);
    }
    else
    {
      Serial.println("Time Set Ok");
      break;
    }    
    if (retryCount >5){break;}
  }


  
}

void setupWiFi() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  LocalIP = WiFi.localIP().toString();
  Serial.println(LocalIP);
}




void loop() {
  SensorHandler();
  WebClientListner();
}


void SensorHandler(){
  // read the state of the switch into a local variable:
  int Float1Reading = digitalRead(Float1Pin);
  int Float2Reading = digitalRead(Float2Pin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (Float1Reading != lastFloat1State) {
    // reset the debouncing timer
    Float1lastDebounceTime = millis();
  }
  if (Float2Reading != lastFloat2State) {
    // reset the debouncing timer
    Float2lastDebounceTime = millis();
  }

  if ((millis() - Float1lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (Float1Reading != Float1State) {
      Float1State = Float1Reading;
      LowFloaterState = !LowFloaterState;
      LogEvent("Low floater state changed to: " + (String)LowFloaterState,true);  

    }
  }

  if ((millis() - Float2lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    if (Float2Reading != Float2State) {
      Float2State = Float2Reading;
      HighFloaterState = !HighFloaterState;
      LogEvent("High floater state changed to:" + (String)HighFloaterState,true);  

    }
  }

  //Sensor 1 Triggered in long time(to safe guard Sensor 2 not working)
  if(Float1StateSec >= LowSensorGuardTime && LowFloaterState==HIGH && RelayState==LOW)
  {
    LogEventWithError("Low sensor High to long...",true,true);  
    LogEvent("Start pump...",true);  
    RelayState = HIGH;
    digitalWrite(RelayPin, RelayState);   
  }

  //Sensor 2 Triggered (to safe guard Sensor 2 not working)
  if(Float1StateSec >= LowSensorGuardTime && LowFloaterState==LOW && HighFloaterState==HIGH && RelayState==LOW)
  {
    LogEventWithError("Low sensor NOT triggered And High IS...",true,true);  
    LogEvent("Start pump...",true);  
    RelayState = HIGH;
    digitalWrite(RelayPin, RelayState);   
  }



  // Now start state machine
  if (LowFloaterState==HIGH&&HighFloaterState==HIGH&&RelayState==LOW)  {
    LogEvent("Start pump...",true);  
    RelayState = HIGH;
    digitalWrite(RelayPin, RelayState);
  }

  if ((LowFloaterState==LOW||HighFloaterState==LOW)&&RelayState==HIGH&&StopPumpTime==0)  {
    LogEvent("Stop pump... "+ (String)(StopDelayTime/1000)  +" sek delay...",true);  

    StopPumpTime = millis();
    StopPumpTime = StopPumpTime + StopDelayTime;

//    delay(10000);
//    LogEvent("Pump stopped...",true);  
//    RelayState = LOW;
//    digitalWrite(RelayPin, RelayState);
  }

if (StopPumpTime != 0 &&  millis() >=  StopPumpTime){
    StopPumpTime = 0;
    LogEvent("Pump stopped...",true);  
    RelayState = LOW;
    digitalWrite(RelayPin, RelayState);
}



  // SET LED state = RelayState... on=on / off=off
  digitalWrite(LEDPin, RelayState);


 // Updates StateTimers
 
 Float1StateSec = (millis() - Float1lastDebounceTime) / 1000;
 Float2StateSec = (millis() - Float2lastDebounceTime) / 1000;



  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastFloat1State = Float1Reading;
  lastFloat2State = Float2Reading;
  
}



void WebClientListner(){
    WiFiClient client = server.available();   // Listen for incoming clients

if (client) {                             // If a new client connects,
    LogEvent("New Client.",false);  
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");

            bool RenderUIHTML = true;


             if (header.indexOf("GET /api/log") >= 0) {
                RenderUIHTML = false;
                client.println("Content-type:text/json");
                client.println("Connection: close");
                client.println();

                String ResponceData;
                bool FirstLine = true;
                ResponceData = "[";
                for (int i = 0;i < LogArrayUpperIndex;i++){
                  if (LogArray[i].Message != NULL){
                    if (LogArray[i].Message != ""){
                      if (!FirstLine) ResponceData += ",";
                                           
                      ResponceData += "{\"LogDateTime\":\""+LogArray[i].LogDateAndTime+"\"";
                      ResponceData += ",\"LogMessage\":\""+LogArray[i].Message+"\"";
                      ResponceData += ",\"IsError\":\""+(String)LogArray[i].IsError+"\"}";
                      
                      FirstLine = false;                      
                    }   
                  }
                }

                ResponceData += "]";
                client.println(ResponceData);
             }


             if (header.indexOf("GET /api/sensor") >= 0) {
                RenderUIHTML = false;
                client.println("Content-type:text/json");
                client.println("Connection: close");
                client.println();

                String ResponceData;
                ResponceData = "{";
                ResponceData += "\"HighFloaterState\":\"" + (String)HighFloaterState+"\"";
                ResponceData += ",\"HighFloaterStateSec\":\"" + (String)Float2StateSec +"\"";
                
                ResponceData += ",\"LowFloaterState\":\"" + (String)LowFloaterState+"\"";
                ResponceData += ",\"LowFloaterStateSec\":\"" + (String)Float1StateSec+"\"";
                
                ResponceData += "}";
                client.println(ResponceData);
              
            } 




            if (header.indexOf("GET /Pump/on") >= 0) {
                  RenderUIHTML = false;
              
                  LogEvent("Manuel Start pump...",true);  
                  RelayState = HIGH;
                  digitalWrite(RelayPin, RelayState);
              
                  StopPumpTime = millis();
                  StopPumpTime = StopPumpTime + StopDelayTime; 

                  client.println("Content-type:text/html");
                  client.println("Connection: close");
                  client.println();
                  
                  //client.println("<Script>window.history.replaceState({}, document.title, '/');</Script>");
                  client.println("<Script>window.location.replace(\"http://"+  LocalIP+"\"); </Script>");
            
            }

            
            
            if (RenderUIHTML)
            {



            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
                        
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"5\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; text-align: center;}");
            client.println(" img{display:block; margin:0px; padding: 0px;border:0px; border-collapse:collapse;}");

            client.println("#parent_div_1 {");
            client.println("  width: 160px;");
            client.println("  height: 420px;");
            client.println("  border: 1px solid black;");
            client.println("  margin-right: 10px; text-align: left;"); 
            client.println("  float: left;   background-color: #efc776;  }");
            
            client.println("#parent_div_2 {");
            client.println("  width: 500px;");
            client.println("  height: 420px;");
            client.println("  border: 0px solid black;");
            client.println("  margin-right: 10px;");
            client.println("  float: left;  text-align: left; }");

            client.println("img {  display: block; margin-left: auto; margin-right: auto;width: 65%;  background-color: white; }");

            
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Pumpe status</h1>");
            client.println();

            //Live Image
            // Large 2kb and over base64 images doesent work from constant each time
            client.println("<div id='parent_div_1'>");
            client.println("<Strong>Live</Strong></br>");
            client.print("<img src=\"data:image/png;base64,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\" >");                      
            if (HighFloaterState==HIGH) {client.print("<img src=\"data:image/png;base64,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\" >");} 
                                   else {client.print("<img src=\"data:image/png;base64,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\" >");}
            if (LowFloaterState==HIGH)  {client.print("<img src=\"data:image/png;base64,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\" >");} 
                                   else {client.print("<img src=\"data:image/png;base64,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\" >");}

            client.print("<img src=\"data:image/png;base64,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\">");
            client.print("<img src=\"data:image/png;base64,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\">");
            client.println("</div>");


            //Log 
            client.println("<div id='parent_div_2'>");
            client.println("<Strong>Log</Strong></br>");

            for (int i = 0;i < LogArrayUpperIndex;i++){
              if (LogArray[i].Message  != NULL){
              if (LogArray[i].Message != ""){
               if (!LogArray[i].IsError) {
                client.println(LogArray[i].LogDateAndTime + " " + LogArray[i].Message +"</br>");
               }else
               {
                client.println("<font color=\"#FF0000\"><Strong>" + LogArray[i].LogDateAndTime + " " + LogArray[i].Message +"</Strong></font></br>");
               }

              
               }   
              }
            }
            client.println("</div>");

            client.println("<p><a href=\"/Pump/on\"><button class=\"button\">Pump ON for "+ (String)(StopDelayTime/1000)  +" sek</button></a></p>");
            
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop

            break;
            }
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    LogEvent("Client disconnected.",false);  
    }
  
}






void LogEvent(String LogThisText,bool SaveToLog)
{
  LogEventWithError(LogThisText,SaveToLog,false);
}

void LogEventWithError(String LogThisText,bool SaveToLog, bool IsError)
{
  Serial.println(DateTime.toString() + ' ' + LogThisText);


if (SaveToLog == true){
  ShiftLogArray();
  //LogArray[0] = DateTime.toString() + ' ' + LogThisText;

  LogData tmp;
  tmp.LogDateAndTime = DateTime.toString();
  tmp.Message = LogThisText;
  tmp.IsError =IsError;

  LogArray[0] = tmp;
  }
}




void ShiftLogArray(){

for (int index = LogArrayUpperIndex; index >= 1;index--){
      if (LogArray[index-1].Message != NULL){
      if (LogArray[index-1].Message != ""){
        LogArray[index] = LogArray[index-1] ;
      }   
      }
     
  }
}
  


    
