
#include <WiFi.h>
#include <ESP32Time.h>
#include <TM1637Display.h>
#include "DHT.h"


#define DHTTYPE DHT22

const int OutPin =  A10;
const int DHTPIN = A11;
const int CLK = A12; //Set the CLK pin connection to the display
const int DIO = A13; //Set the DIO pin connection to the display


const char* ssid = "RajmondTelWiFi";
const char* password = "ninetor999";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
const char* ntpServer = "pool.ntp.org";

const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 3600;

struct tm timeinfo;
ESP32Time rtc;


unsigned int Hour = 0, Minute = 0, Flag = 0;
int Temps[5] = {18, 18, 18, 18, 18}, TempC = 18;
int TempsR[5] = {18, 18, 18, 18, 18}, TempCR = 18;

String Times[8] = {"00:00", "05:59", "06:00", "11:59", "12:00", "17:59", "18:00", "23:59"}, TimeC = "00:00";
String TimesR[8] = {"00:00", "05:59", "06:00", "11:59", "12:00", "17:59", "18:00", "23:59"}, TimeCR = "00:00";
String TimeF = "00:00", TimeFR = "00:00";

String tx;
int LightUp = 0;

int WaitingForPriod = 0;
int TimeCH = 0, TimeCM = 0, TimeFH = 0, TimeFM = 0, TimeWaitH = 0, TimeWaitM = 0;
long int WaitForEpoch = 0;


double TempDesired = 18;
double TempMeasured = 18, HumidityMeasured = 0;
double  Histeresis = 0.5;

int Out = 0;

DHT dht(DHTPIN, DHTTYPE);

TM1637Display display(CLK, DIO); //set up the 4-Digit Display.
uint8_t dots = 47261417472;

long int DisplayEpoch = 0;
int DisCycle = 0;
int DisVal = 0;


void setup() {
  Serial.begin(115200);


  pinMode(OutPin, OUTPUT);

  
  dht.begin();
  display.setBrightness(0x0a); //set the diplay to maximum

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  } else {
    Serial.println("Failed to obtain time");
    rtc.setTime(0, 0, 0, 1, 1, 2021);
  }

  server.begin();



}

void loop() {

  TempMeasured = dht.readTemperature();
  HumidityMeasured = dht.readHumidity();


  Hour = rtc.getHour(true);
  Minute = rtc.getMinute();



  if (Hour < 10) {
    TimeC = "0" + String(Hour) + ":";
  } else {
    TimeC = String(Hour) + ":";
  }
  if (Minute < 10) {
    TimeC = TimeC + "0" + String(Minute);
  } else {
    TimeC = TimeC + String(Minute);
  }

  if (WaitingForPriod == 0) {
    TimeF = TimeC; // ha nem varok akkor kijelzem a jelenlegi idot
  }

  
  if (DisplayEpoch < rtc.getEpoch()) {
    Serial.println(DisCycle);
    
    DisplayEpoch = rtc.getEpoch() + 4;
      switch (DisCycle) {
    case 0:
      DisVal = 100 * Hour + Minute;
      display.showNumberDecEx(DisVal, 0b01000000, false, 4, 0);
      break;

    case 1:
      DisVal = (int)(100*TempMeasured);
      Serial.println(DisVal);
      display.showNumberDecEx(DisVal, 0b01000000, false, 4, 0);
      break;
    case 2:
      DisVal =(int)(100*HumidityMeasured);
      display.showNumberDecEx(DisVal, 0b01000000, false, 4, 0);
      break;
    case 3:
      DisVal = TempCR;
      display.showNumberDec(DisVal);
      break;
    default:
      //DisCycle = 0;
      break;
  }
    if (DisCycle < 3) {
      DisCycle = DisCycle + 1;
    }
    else
    {
      DisCycle = 0;
    }
  }
  

  WiFiClient client = server.available();   // Listen for incoming clients


  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            //Serial.println(header);


            int in = header.indexOf("t1=");



            TimeFR = TimeF;
            TimeCR = TimeC;

            if ((header.indexOf("TempC=") >= 0) && (header.indexOf("TempC=") < header.indexOf("Host"))) {
              Flag = 1;
              TempCR = header.substring(header.indexOf("TempC=") + 6).toInt();
              TimeFR = header.substring(header.indexOf("holdTime=") + 9, header.indexOf("holdTime=") + 11);
              TimeFR = TimeFR + ":" + header.substring(header.indexOf("holdTime=") + 14, header.indexOf("holdTime=") + 16);
            } else {
              if ((header.indexOf("cTime=") >= 0) && (header.indexOf("cTime=") < header.indexOf("Host"))) {
                Flag = 2;
                TimeCR = header.substring(header.indexOf("cTime=") + 6, header.indexOf("cTime=") + 8);
                TimeCR = TimeCR + ":" + header.substring(header.indexOf("cTime=") + 11, header.indexOf("cTime=") + 13);
              } else {
                if ((header.indexOf("t1=") >= 0) && (header.indexOf("t1=") < header.indexOf("Host"))) {
                  Flag = 3;

                  TempsR[0] = header.substring(header.indexOf("Temp=") + 5).toInt();
                  TempsR[1] = header.substring(header.indexOf("Temp1=") + 6).toInt();
                  TempsR[2] = header.substring(header.indexOf("Temp2=") + 6).toInt();
                  TempsR[3] = header.substring(header.indexOf("Temp3=") + 6).toInt();
                  TempsR[4] = header.substring(header.indexOf("Temp4=") + 6).toInt();


                  for (int i = 0; i < 8; ++i) {
                    tx = "t" + String(i + 1) + "=";
                    TimesR[i] = header.substring(header.indexOf(tx) + 3, header.indexOf(tx) + 5);
                    TimesR[i] = TimesR[i] + ":" + header.substring(header.indexOf(tx) + 8, header.indexOf(tx) + 10);
                  }

                } else {
                  Flag = 0;
                }

              }

            }




            //Serial.println((header.substring(in+3,in+5)).toInt());
            Serial.print("Time:");
            Serial.println(rtc.getTime());



            // Display the HTML web page

            client.println("<!DOCTYPE html>");
            client.println("<html>");
            client.println("<head>");
            client.println("<title>ESP32 Web Server</title>");

            client.println("<meta name='viewport' content='width=device-width, initial-");
            client.println("scale=1'>");

            client.println("<link rel='icon' href='data:,'>");
            client.println("<style>");

            client.println("html {");
            client.println("font-family: Helvetica;");
            client.println("display: inline-block;");
            client.println("margin: 0px auto;");
            client.println("background-color: lightseagreen;");
            client.println("/*text-align: center*/;");
            client.println("}");
            client.println("form {");
            client.println("    font-size: 0.5cm;");

            client.println("}");
            client.println("input{");
            client.println("    font-size: 0.7cm;");

            client.println("   height: 1cm;");
            client.println("}");

            client.println(".TempBlock{");
            client.println("   max-width: 1.8cm;");
            client.println("}");
            if (WaitingForPriod == 1) {
              client.println("#holdTime{background-color: lawngreen;}");
            }
            if (LightUp > 0 && LightUp < 5) {
              client.print("#t");
              client.print(String(LightUp*2-1));
              client.println("{background-color: lawngreen;}");
              client.print("#t");
              client.print(String(LightUp*2));
              client.println("{background-color: lawngreen;}");
            }

            if (LightUp == 5) {
              client.print("#cTime{background-color: lawngreen;}");
            }


            client.println("#ref{height: 2cm;}");

            client.println("#Measuremets{");
            client.println("display: inline-block;");
            client.println("padding: 0.5cm;");
            client.println("border-style:groove;");
            client.println("background-color: darkseagreen;");
            client.println("border-color: black;}");

            if (Out == 0) {
              client.println("#Out{color:blue;}");
            };
            if (Out == 1) {
              client.println("#Out{color:red;}");
            };
            client.println("</style>");


            client.println("</head>");
            client.println("<body>");
            client.println("<h1>Temperature control</h1>");

            client.println("<div id='Measuremets'>");
            client.print("<h2>Temperature =  ");
            client.print(String(TempMeasured));
            client.println(" &#x2103;</h2>");
            client.print("<h2>Humidity = ");
            client.print(String(HumidityMeasured));
            client.println(" %</h2>");
            client.println("</div>");


            client.println("<form action='/action_page.php'>");
            client.println("<br><input type='submit' id='ref' value='Refresh'><br><br><br>");
            client.println("</form>");

            if (Out == 0) {
              client.println("<h2 id='Out'>Heat is OFF</h2>");
            };
            if (Out == 1) {
              client.println("<h2 id='Out'>Heat is ON</h2>");
            };

            client.println("   <h2>Currently set temperature</h2>");
            client.println("    <form action='/action_page.php'>");
            client.println("       <label for='V=CurrentTemperature'>Temperature: <br></label><br>");
            client.print("      <input type='number' class='TempBlock' id='TempC' name='TempC' value='");
            client.print(String(TempCR));
            client.println("'min='1' max='25'><br>");
            client.print("<label for='holdTime'>for:</label><br>");
            client.print("<input type='time' id='holdTime' name='holdTime' value='");
            client.print(TimeFR);
            client.println("'><br>");
            client.println("      <br><br><input type='submit' value='Send'><br><br><br>");

            client.println("    </form>");



            client.println("<h2>Current time:</h2>");
            client.println("<form action='/action_page.php'>");
            client.println(" <label for='cTime'>Time:</label><br>");
            client.print(" <input type='time' id='cTime' name='cTime' value='");
            client.print(TimeCR);
            client.println("'><br>");
            client.println("  <br><br><input type='submit' value='Send'><br><br><br>");

            client.println(" </form>");



            client.println("   <h2>Program</h2>");




            client.println("   <form action='/action_page.php'>");
            client.println("   <label for='Temp'>Default temperature: <br></label><br>");
            client.print("   <input type='number' class='TempBlock' id='Temp' name='Temp' value='");
            client.print(String(TempsR[0]));
            client.println("'min='1'  max='25'><br><br><br>");


            client.println("     <label for='t1'>Interval 1: <br>from:</label><br>");
            client.print("    <input type='time' id='t1' name='t1' value='");
            client.print(TimesR[0]);
            client.println("'><br>");
            client.println("     <label for='t2'>to:</label><br>");
            client.print("     <input type='time' id='t2' name='t2' value='");
            client.print(TimesR[1]);
            client.println("'><br>");
            client.println("    <label for='Temp1'>Temperature:</label><br>");
            client.print("   <input type='number' class='TempBlock' id='Temp1' name='Temp1' value='");
            client.print(String(TempsR[1]));
            client.println("'min='1'  max='25'>");

            client.println("   <br><br><br>");
            client.println("   <label for='t3'>Interval 2: <br>from:</label><br>");
            client.print("   <input type='time' id='t3' name='t3' value='");
            client.print(TimesR[2]);
            client.println("'><br>");
            client.println("  <label for='t4'>to:</label><br>");
            client.print("  <input type='time' id='t4' name='t4' value='");
            client.print(TimesR[3]);
            client.println("'><br>");
            client.println("  <label for='Temp2'>Temperature:</label><br>");
            client.print("   <input type='number' class='TempBlock' id='Temp2' name='Temp2' value='");
            client.print(String(TempsR[2]));
            client.println("'min='1'  max='25'>");

            client.println("  <br><br><br>");
            client.println(" <label for='t5'>Interval 3: <br>from:</label><br>");
            client.print("   <input type='time' id='t5' name='t5' value='");
            client.print(TimesR[4]);
            client.println("'><br>");
            client.println("   <label for='t6'>to:</label><br>");
            client.print("   <input type='time' id='t6' name='t6' value='");
            client.print(TimesR[5]);
            client.println("'><br>");
            client.println("  <label for='Temp3'>Temperature:</label><br>");
            client.print("   <input type='number' class='TempBlock' id='Temp3' name='Temp3' value='");
            client.print(String(TempsR[3]));
            client.println("'min='1'  max='25'>");

            client.println(" <br><br><br>");
            client.println(" <label for='t7'>Interval 4: <br>from:</label><br>");
            client.print(" <input type='time' id='t7' name='t7' value='");
            client.print(TimesR[6]);
            client.println("'><br>");
            client.println(" <label for='t8'>to:</label><br>");
            client.print("  <input type='time' id='t8' name='t8' value='");
            client.print(TimesR[7]);
            client.println("'><br>");;
            client.println(" <label for='Temp4'>Temperature:</label><br>");
            client.print("   <input type='number' class='TempBlock' id='Temp4' name='Temp4' value='");
            client.print(String(TempsR[4]));
            client.println("'min='1'  max='25'>");

            client.println("  <br><br><input type='submit' value='Send'><br><br>");
            client.println(" </form>");

            client.println(" <p>Click the 'Send' button and the form-data will be sent to a page on the ");
            client.println("server called 'action_page.php'.</p>");
            client.println(" </body>");
            client.println("</html>");



            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
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
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  switch (Flag) {
    case 1:
      // statements

      TimeF = TimeFR;

      TimeCH = (TimeC.substring(0, 2)).toInt();
      TimeCM = (TimeC.substring(3, 5)).toInt();
      TimeFH = (TimeF.substring(0, 2)).toInt();
      TimeFM = (TimeF.substring(3, 5)).toInt();
      TimeWaitH = 0;
      TimeWaitM = 0;

      if (TimeCM < TimeFM) {
        TimeWaitM = TimeFM - TimeCM;
        if (TimeCH <= TimeFH) {
          TimeWaitH = TimeFH - TimeCH;
        } else {
          TimeWaitH = 24 - TimeCH + TimeFH;
        }
      } else {

        if (TimeCM > TimeFM) {
          TimeWaitM = 60 - TimeCM + TimeFM;
          if (TimeCH < TimeFH) {
            TimeWaitH = TimeFH - TimeCH - 1;
          } else {
            TimeWaitH = 24 - TimeCH + TimeFH - 1;
          }
        } else {
          TimeWaitM = 0;
          if (TimeCH <= TimeFH) {
            TimeWaitH = TimeFH - TimeCH;
          } else {
            TimeWaitH = 24 - TimeCH + TimeFH;
          }
        }
      }

      WaitForEpoch = rtc.getEpoch() + 3600 * TimeWaitH + 60 * TimeWaitM;

      if (WaitForEpoch <= rtc.getEpoch()) {
        WaitingForPriod = 0;
      } else {
        WaitingForPriod = 1;
      }
      break;


    case 2:
      rtc.setTime(0, (TimeCR.substring(3, 5)).toInt(), (TimeCR.substring(0, 2)).toInt() - 1, rtc.getDayofYear(), rtc.getMonth(), rtc.getYear());
      TimeC = TimeCR;
      break;
    default:
      // statements
      break;
  }

  if (WaitingForPriod == 1) {
    TempDesired = TempCR;
    if (WaitForEpoch <= rtc.getEpoch()) {
      WaitingForPriod = 0;
    }
    LightUp = 0;
  } else {

    if ((TimeC >= TimesR[0]) && (TimeC <= TimesR[1])) {
      TempDesired = TempsR[1];
      TempCR=TempsR[1];
      LightUp = 1;
    } else {
      if ((TimeC >= TimesR[2]) && (TimeC <= TimesR[3])) {
        TempDesired = TempsR[2];
        TempCR=TempsR[2];
        LightUp = 2;
      } else {
        if ((TimeC >= TimesR[4]) && (TimeC <= TimesR[5])) {
          TempDesired = TempsR[3];
          TempCR=TempsR[3];
          LightUp = 3;
        } else {
          if ((TimeC >= TimesR[6]) && (TimeC <= TimesR[7])) {
            TempDesired = TempsR[4];
            TempCR=TempsR[4];
            LightUp = 4;
          } else {
            TempDesired = TempsR[0];
            TempCR=TempsR[0];
            LightUp = 5;
          }
        }
      }
    }
    
  }




  if (TempMeasured > TempDesired + Histeresis) {
    Out = 0;
  }
  if (TempMeasured < TempDesired - Histeresis) {
    Out = 1;
  }
 

  
  if(Out==1){
    digitalWrite(OutPin, HIGH);
    }else{
      digitalWrite(OutPin, LOW);
      }

  Flag = 0;
}
