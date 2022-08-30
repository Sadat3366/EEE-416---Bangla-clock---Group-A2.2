#include <virtuabotixRTC.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h> 
  
virtuabotixRTC myRTC(A0, A1, A2); //
LiquidCrystal lcd(11, 12, 7, 8, 9, 10); //
SoftwareSerial esp8266(2,3);  //rx, tx (Connections flipped while connecting to esp-01 module)   
              
#define serialCommunicationSpeed 115200               
#define time_out 1000
#define DEBUG 0

//time variables 

uint8_t sync_time[6];

uint8_t tim_e[6];
uint8_t ampm; //0 if AM,1 if PM

uint8_t en_date[2];
uint8_t en_year[4];
uint8_t en_m; //month

uint8_t bn_date[2];
uint8_t bn_year[4];
uint8_t bn_m; //month

uint8_t week; 

uint8_t count = 1;
uint8_t scroll = 1;

char sc = '"';
String rec_command;
String rec_data;
//String response = "";
uint8_t prev;

void setup()

{
  Serial.begin(serialCommunicationSpeed);           
  esp8266.begin(serialCommunicationSpeed);
  lcd.begin(16, 2);

  Serial.println("Loading Previous RTC values...");
  load_rtc();
  delay(100);

  initwifi();
  delay(100);

}

void loop()                                                         
{
    Serial.println("Loading and Showing RTC Time...");
    load_rtc();

    Serial.print(tim_e[0]);Serial.print(tim_e[1]);Serial.print(":");Serial.print(tim_e[2]);Serial.print(tim_e[3]);Serial.print(":");Serial.print(tim_e[4]);Serial.print(tim_e[5]);Serial.print("  ");Serial.print(ampm);Serial.print("  ");Serial.print(week);Serial.println();
    Serial.print(en_date[0]);Serial.print(en_date[1]);Serial.print("/");Serial.print(en_m);Serial.print("/");Serial.print(en_year[0]);Serial.print(en_year[1]);Serial.print(en_year[2]);Serial.print(en_year[3]);Serial.print("  EN");Serial.println();
    Serial.print(bn_date[0]);Serial.print(bn_date[1]);Serial.print("/");Serial.print(bn_m);Serial.print("/");Serial.print(bn_year[0]);Serial.print(bn_year[1]);Serial.print(bn_year[2]);Serial.print(bn_year[3]);Serial.print("  BN");Serial.println();

    delay(100);
    show_time();

    if(scroll >= 0 && scroll < 5)
      show_week_day();
    if(scroll >= 5 && scroll < 10)
      show_eng_date();
    if(scroll >= 10 && scroll < 15)
      show_ban_date();
      
    if(count == 1)
    {
      get_time();
      set_time();
    }
    if(count == 2)
    {
      get_eng_date();
      set_eng_date();
    }
    if(count == 3)
    {
      get_ban_date();
      set_ban_date();
    }
    if(count == 4)
    {
      myRTC.updateTime();
      prev = myRTC.hours;
      count = count + 1;
    }  
    if(count == 5)
    {
      Serial.println("Synced");
      //Serial.print(prev);Serial.print("  ");Serial.println(myRTC.hours);
      delay(750); 
    }    
    if((count == 5) && (abs(prev - myRTC.hours) >= 6))
    {
      count = 1;
    }
    
}

String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    //Serial.println("Sent to ESP: ");
    //Serial.print(command);                                            
    esp8266.print(command);                                          
    long int time = millis();                                      
    while( (time+timeout) > millis())                                 
    {      
      while(esp8266.available())                                      
      {
        char c = esp8266.read();                                     
        response+=c;                                                  
      }  
    }    
    if(debug)                                                        
    {
      //Serial.println("Received from ESP: ");
      Serial.print(response);
    }    
    return response;                                                  
}

void initwifi(void)
{
    Serial.println("Closing Previous Connection");
    delay(1);
    rec_command = "AT+CIPCLOSE";
    rec_command += "\r\n";
    rec_data = sendData(rec_command, 2000, 1);
    
    delay(100);
    Serial.println("Initiating WiFi Module, Connecting to Specified WiFi");
    delay(1);
    
    rec_command = "AT+RST";
    rec_command += "\r\n";
    sendData(rec_command, 1000, 0);

    rec_command = "AT+CWJAP=";
    rec_command += sc;
    rec_command += "alu wifi"; //SSID
    rec_command += sc;
    rec_command += ",";
    rec_command += sc;
    rec_command += "12345678"; //PASS
    rec_command += sc;
    rec_command += "\r\n";
    sendData(rec_command, 2000, 1);        
    delay (1000);

    rec_command = "\r\n";
    sendData(rec_command, 1000, 1);
    
}

void get_time(void)
{
    Serial.println("Getting Online Time...");
    delay(1);

    rec_command = "AT+CIPSTART=";
    rec_command += sc;
    rec_command += "TCP";
    rec_command += sc;
    rec_command += ",";
    rec_command += sc;
    rec_command += "api.thingspeak.com";
    rec_command += sc;
    rec_command += ",80";
    rec_command += "\r\n";
    sendData(rec_command, 1, DEBUG);

    delay(1);

    rec_command = "AT+CIPSEND=90";
    rec_command += "\r\n";
    sendData(rec_command, 1, DEBUG);

    delay(1);

    rec_command = "GET /apps/thinghttp/send_request?api_key=Q1ZBWRBEXXO44K68";
    rec_command += "\r\n";
    rec_command += "Host:api.thingspeak.com";
    rec_command += "\r\n\r\n\r\n\r\n";
    rec_data = sendData(rec_command, 1000, DEBUG);

    delay(1);
}

void set_time(void)
{
  if(rec_data.indexOf("+IPD,53") > 0)
  {
    Serial.println("Error 53");
  }
  else if(rec_data.indexOf("+IPD,132") > 0)
  {
    Serial.println("Error 132");
  }
  else if(rec_data.indexOf("+IPD,") > 0)
    {
      Serial.println("Setting RTC Time");
      Serial.println(rec_data.substring(rec_data.indexOf("+IPD")));
      
      //Serial.println(rec_data.substring(rec_data.indexOf("+IPD")+20, rec_data.indexOf("+IPD")+32));
      
      //parsing the string
      rec_data = rec_data.substring(rec_data.indexOf("+IPD")+20, rec_data.indexOf("+IPD")+32);
      
      //Serial.println(rec_data.indexOf("M")); //10
      //Serial.println(rec_data.substring(rec_data.indexOf("M")-1, rec_data.indexOf("M")));
      
      if(rec_data.substring(rec_data.indexOf("M")-1, rec_data.indexOf("M")) == "P")
      {
        sync_time[0] = ((rec_data.substring(rec_data.indexOf("M")-10, rec_data.indexOf("M")-8).toInt())/10);
        sync_time[1] = ((rec_data.substring(rec_data.indexOf("M")-10, rec_data.indexOf("M")-8).toInt())%10);
        if(sync_time[0] != 1 && sync_time[1] != 2) // 12 PM Special Case
        {
          sync_time[0] = sync_time[0]+1;
          sync_time[1] = sync_time[1]+1;
        }
      }
      if(rec_data.substring(rec_data.indexOf("M")-1, rec_data.indexOf("M")) == "A")
      {
        sync_time[0] = ((rec_data.substring(rec_data.indexOf("M")-10, rec_data.indexOf("M")-8).toInt())/10);
        sync_time[1] = ((rec_data.substring(rec_data.indexOf("M")-10, rec_data.indexOf("M")-8).toInt())%10);
        if(sync_time[0] == 1 && sync_time[1] == 2) // 12 AM Special Case - 0 hrs in 24 hr format
        {
          sync_time[0] = 0;
          sync_time[1] = 0;
        }
      }

      sync_time[2] = (rec_data.substring(rec_data.indexOf("M")-7, rec_data.indexOf("M")-5).toInt())/10;
      sync_time[3] = (rec_data.substring(rec_data.indexOf("M")-7, rec_data.indexOf("M")-5).toInt())%10;
      
      sync_time[4] = (rec_data.substring(rec_data.indexOf("M")-4, rec_data.indexOf("M")-2).toInt())/10;
      sync_time[5] = (rec_data.substring(rec_data.indexOf("M")-4, rec_data.indexOf("M")-2).toInt())%10;

      myRTC.setDS1302Time((sync_time[4]*10) + sync_time[5], (sync_time[2]*10) + sync_time[3], (sync_time[0]*10) + sync_time[1], week, (en_date[0]*10)+en_date[1], en_m, (en_year[0]*1000)+(en_year[1]*100)+(en_year[2]*10)+en_year[3]);

      rec_data = "";
      count = count+1;
    }
}

void get_eng_date(void)
{
    Serial.println("Getting Online English Date...");
    delay(1);

    rec_command = "AT+CIPSTART=";
    rec_command += sc;
    rec_command += "TCP";
    rec_command += sc;
    rec_command += ",";
    rec_command += sc;
    rec_command += "api.thingspeak.com";
    rec_command += sc;
    rec_command += ",80";
    rec_command += "\r\n";
    sendData(rec_command, 1, DEBUG);

    delay(1);

    rec_command = "AT+CIPSEND=90";
    rec_command += "\r\n";
    sendData(rec_command, 1, DEBUG);

    delay(1);

    rec_command = "GET /apps/thinghttp/send_request?api_key=6EX7E9O58AOAB9DG";
    rec_command += "\r\n";
    rec_command += "Host:api.thingspeak.com";
    rec_command += "\r\n\r\n\r\n\r\n";
    rec_data = sendData(rec_command, 1000, DEBUG);

    delay(1);
}

void set_eng_date(void)
{
  if(rec_data.indexOf("+IPD,53") > 0)
  {
    Serial.println("Error 53");
  }
  else if(rec_data.indexOf("+IPD,132") > 0)
  {
    Serial.println("Error 132");
  }
  else if(rec_data.indexOf("+IPD,") > 0)
    {
      Serial.println("Setting RTC English Date");
      Serial.println(rec_data.substring(rec_data.indexOf("+IPD,")));

      //Serial.println(rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1)));

      //parsing the string

      int dat_e = (rec_data.substring(rec_data.lastIndexOf(",")-2, rec_data.lastIndexOf(","))).toInt();
      en_date[0] = dat_e/10;
      en_date[1] = dat_e%10;

      Serial.println(rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2));
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "January")
        en_m = 1;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "February")
        en_m = 2;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "March")
        en_m = 3;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "April")
        en_m = 4;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "May")
        en_m = 5;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "June")
        en_m = 6;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "July ")
        en_m = 7;
      if(((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "August") || ((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "August "))
        en_m = 8;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "September")
        en_m = 9;        
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "October")
        en_m = 10;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "November")
        en_m = 11;
      if((rec_data.substring(rec_data.indexOf(",",rec_data.indexOf(",")+1)+2, rec_data.lastIndexOf(",")-2)) == "December")
        en_m = 12;
                
      int yea_r = (rec_data.substring(rec_data.lastIndexOf(",")+2, rec_data.lastIndexOf(",")+6)).toInt();
      en_year[0] = yea_r/1000;
      en_year[1] = (yea_r%1000)/100;
      en_year[2] = ((yea_r%1000)%100)/10;
      en_year[3] = ((yea_r%1000)%100)%10;

      //Serial.println(rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1)));
      if((rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1))) == "Saturday")
        week = 1;
      if((rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1))) == "Sunday")
        week = 2;
      if((rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1))) == "Monday")
        week = 3;
      if((rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1))) == "Tuesday")
        week = 4;
      if((rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1))) == "Wednesday")
        week = 5;
      if((rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1))) == "Thursday")
        week = 6;
      if((rec_data.substring(rec_data.indexOf(",")+9, rec_data.indexOf(",",rec_data.indexOf(",")+1))) == "Friday")
        week = 7;

      myRTC.setDS1302Time((sync_time[4]*10) + sync_time[5], (sync_time[2]*10) + sync_time[3], (sync_time[0]*10) + sync_time[1], week, (en_date[0]*10)+en_date[1], en_m, (en_year[0]*1000)+(en_year[1]*100)+(en_year[2]*10)+en_year[3]);
      
      rec_data = "";
      count = count+1;
    }
}

void get_ban_date(void)
{
    Serial.println("Getting Online Bangla Date...");
    delay(1);

    rec_command = "AT+CIPSTART=";
    rec_command += sc;
    rec_command += "TCP";
    rec_command += sc;
    rec_command += ",";
    rec_command += sc;
    rec_command += "api.thingspeak.com";
    rec_command += sc;
    rec_command += ",80";
    rec_command += "\r\n";
    sendData(rec_command, 1, DEBUG);

    delay(1);

    rec_command = "AT+CIPSEND=90";
    rec_command += "\r\n";
    sendData(rec_command, 1, DEBUG);

    delay(1);

    rec_command = "GET /apps/thinghttp/send_request?api_key=5MKIPC0USLFNP3UT";
    rec_command += "\r\n";
    rec_command += "Host:api.thingspeak.com";
    rec_command += "\r\n\r\n\r\n\r\n";
    rec_data = sendData(rec_command, 1000, DEBUG);

    delay(1);
}

void set_ban_date(void)
{
  if(rec_data.indexOf("+IPD,53") > 0)
  {
    Serial.println("Error 53");
  }
  else if(rec_data.indexOf("+IPD,132") > 0)
  {
    Serial.println("Error 132");
  }
  else if(rec_data.indexOf("+IPD,") > 0)
    {
      Serial.println("Setting Bangla Date");
      Serial.println(rec_data.substring(rec_data.indexOf("+IPD")));
      //parsing the string
      
      int dat_e = (rec_data.substring(rec_data.lastIndexOf("-")+1, rec_data.lastIndexOf("-")+4)).toInt();
      bn_date[0] = dat_e/10;
      bn_date[1] = dat_e%10;

      Serial.println(rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(",")));
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Baisakh")
        bn_m = 1;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Joishtho")
        bn_m = 2;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Asadha")
        bn_m = 3;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Srabon")
        bn_m = 4;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Bhadra")
        bn_m = 5;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Ashshin")
        bn_m = 6;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Kartik")
        bn_m = 7;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Ogrohayon")
        bn_m = 8;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Push")
        bn_m = 9;        
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Magh")
        bn_m = 10;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Falgun")
        bn_m = 11;
      if((rec_data.substring(rec_data.indexOf(" ", rec_data.indexOf("-")+2)+1, rec_data.lastIndexOf(","))) == "Choitro")
        bn_m = 12;
  
      int yea_r = (rec_data.substring(rec_data.lastIndexOf(",")+2, rec_data.lastIndexOf(",")+6)).toInt();
      bn_year[0] = yea_r/1000;
      bn_year[1] = (yea_r%1000)/100;
      bn_year[2] = ((yea_r%1000)%100)/10;
      bn_year[3] = ((yea_r%1000)%100)%10;

      rec_data = "";
      count = count+1;
    }
}

void load_rtc(void)
{
  myRTC.updateTime();
  //Serial.println(myRTC.hours);
  if(myRTC.hours >= 13)
  {
    tim_e[0] = (myRTC.hours-11)/10;
    tim_e[1] = (myRTC.hours-11)%10;
  }
  else if(myRTC.hours == 0)
  {
    tim_e[0] = 1;
    tim_e[1] = 2;
  }
  else
  {
    tim_e[0] = (myRTC.hours)/10;
    tim_e[1] = (myRTC.hours)%10;
  }

  tim_e[2] = myRTC.minutes/10;
  tim_e[3] = myRTC.minutes%10;

  tim_e[4] = myRTC.seconds/10;
  tim_e[5] = myRTC.seconds%10;

  if(myRTC.hours >= 12)
    ampm = 1; //0 if AM,1 if PM
  else
    ampm = 0; //0 if AM,1 if PM

  week = myRTC.dayofweek;

  en_date[0] = myRTC.dayofmonth/10;
  en_date[1] = myRTC.dayofmonth%10;

  en_m = myRTC.month;
  
  en_year[0] = myRTC.year/1000;
  en_year[1] = (myRTC.year%1000)/100;
  en_year[2] = ((myRTC.year%1000)%100)/10;
  en_year[3] = ((myRTC.year%1000)%100)%10; 
}

void show_time(void)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(tim_e[0]);
  lcd.print(tim_e[1]);
  lcd.print(":");
  lcd.print(tim_e[2]);
  lcd.print(tim_e[3]);
  lcd.print(":");
  lcd.print(tim_e[4]);
  lcd.print(tim_e[5]);
  lcd.print("  ");
  if(ampm == 1)
    lcd.print("PM");
  else
    lcd.print("AM");

  lcd.print(" ");
  
  delay(1);

  scroll = scroll+1;

  if(scroll >= 15)
  {
    scroll = 0;
  }
}
void show_week_day(void)
{
  lcd.setCursor(0, 1);
  lcd.print(week);
}
void show_eng_date(void)
{
  lcd.setCursor(0, 1);
  lcd.print(en_date[0]);
  lcd.print(en_date[1]);
  lcd.print("/");
  lcd.print(en_m);
  lcd.print("/");
  lcd.print(en_year[0]);
  lcd.print(en_year[1]);
  lcd.print(en_year[2]);
  lcd.print(en_year[3]);
  lcd.print(" ");
  lcd.print("EN");
}
void show_ban_date(void)
{
  lcd.setCursor(0, 1);
  lcd.print(bn_date[0]);
  lcd.print(bn_date[1]);
  lcd.print("/");
  lcd.print(bn_m);
  lcd.print("/");
  lcd.print(bn_year[0]);
  lcd.print(bn_year[1]);
  lcd.print(bn_year[2]);
  lcd.print(bn_year[3]);
  lcd.print(" ");
  lcd.print("BN");
}
