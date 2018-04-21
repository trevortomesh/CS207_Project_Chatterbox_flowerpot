#include <Time.h>
#include <WaveHC.h>
#include <WaveUtil.h>
#include <TimeLib.h>
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 



//#include <Time.h>
//initializing the intput and output pins
const int lightPin = A3;
const int moisturePin = A2; 
const int tmpPin=A1;
const int redpin = 8;
const int greenpin = 9;

//initializing valves
int light;
int soild;
int temperature;
int tks;
int tks1;

//reference part by adafruit:https://github.com/adafruit/WaveHC
//**********************************************
SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader file;   // This object represent the WAV file for a pi digit or period
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

void error_P(const char *str) {
  PgmPrint("Error: ");
  SerialPrint_P(str);
  sdErrorCheck();
  while(1);
}

void playcomplete(char *name) {
  playfile(name);
  while (wave.isplaying);
  
  // see if an error occurred while playing
  sdErrorCheck();
}
/*
 * Open and start playing a WAV file
 */
void playfile(char *name) {
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  if (!file.open(root, name)) {
    PgmPrint("Couldn't open file ");
    Serial.print(name); 
    return;
  }
  if (!wave.create(file)) {
    PgmPrintln("Not a valid WAV");
    return;
  }
  // ok time to play!
  wave.play();
}



void sdErrorCheck(void) {
  if (!card.errorCode()) return;
  PgmPrint("\r\nSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  PgmPrint(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}
//**********************************************

#define error(msg) error_P(PSTR(msg))

void setup() {
  //set pins to output
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  Serial.begin(9600);// Initialize serial communication  // set up Serial library at 9600 bps
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");


  //reference part:
  //**********************************************
  PgmPrintln("Pi speaker");
  
  if (!card.init()) {
    error("Card init. failed!");
  }
  if (!vol.init(card)) {
    error("No partition!");
  }
  if (!root.openRoot(vol)) {
    error("Couldn't open dir");
  }

  PgmPrintln("Files found:");
  root.ls();
//**********************************************

  tks1=0;//this variable is used to detect if speak thank you is needed
  
}
void loop() {  
  light = Ldetect();//detect the light intensity
  
  soild = Sdetect();//detect the Soil moisture
  
  temperature=Tdetect();//detect the temperature
  
  int con=0;//this variable counts condition problem,when a new problem appear con++

  if(tks1==0)//if it is just opened go in
  {
    tks1=1;
    playcomplete("cb1.wav");//introduce itself
    delay(500);
    
    conditionOut(light,soild,temperature);//output all three conditions
  }

    if (Serial.available()) {
    processSyncMessage();
  }
  if (timeStatus()!= timeNotSet) {
    digitalClockDisplay();  
  }
  else{
    Serial.println("Time Not Set!");
  }
  delay(1000);
  
  
  //if light problem appear go in
  if(light==3)
  {
    Serial.println("Too bright!");
    playcomplete("Lhigh.wav");
    con++;
    tks=1;
  }
  //if soil problem appear go in
  if(soild==0)
  {
    Serial.println("Water Me!");
    playcomplete("Sdry.wav");
    con++;
    tks=1;
  }
  else if(soild==3)
  {
    Serial.println("Overwatered!");
    playcomplete("Swet.wav");
    con++;
    tks=1;
  }


  if(temperature<15||temperature>30)//if temperature problem appear go in
  {
    Serial.println("Abnormal Temperature!");
    playcomplete("Tbad.wav");
    con++;
    tks=1;
  }

  
  if(con==0)//good condition light turn green
  {
    digitalWrite(greenpin,HIGH);//green
    digitalWrite(redpin,LOW);
    if(tks==1) //if the plant's condition just getting good,go in.
    {
      playcomplete("tks.wav");
      tks=0;
    }
  }
  else if(con==1)//normal condition light turns yellow
  {
    digitalWrite(greenpin,HIGH);
    digitalWrite(redpin,HIGH);
    //yellow
  }
  else if(con==2)//bad condition light turns red
  {
    digitalWrite(redpin,HIGH);//red
    digitalWrite(greenpin,LOW);
  }
  else if(con==3)//terrible condition light blink in red
  {
    for(int i=0;i<10;i++)
    {
      digitalWrite(redpin,HIGH);
      digitalWrite(greenpin,LOW);
      delay(500);
      digitalWrite(redpin,LOW);
      digitalWrite(greenpin,LOW);
      delay(500);
    }
    //blink red

  }
 
   delay(500);
}


//light detect part
int Ldetect(){
    int light;
    light = analogRead(lightPin);//read the data from potpin which
    light=map(light,0,800,0,3);
 
}


//temperature detect part
int Tdetect(){
  int reading; //raw reading from A1 
  float volts;//Voltage output number 
  float celsius; //celsius output number
   reading = analogRead(tmpPin); //read A1
   volts = reading/1023.0 * 5.0;
   celsius=volts*100.0-50; //made it into celsius

   
   return celsius;
}



//soil moisture detect part
//******this part is oringinally from https://create.arduino.cc/projecthub/arduino/plant-communicator-7ea06f?ref=search&ref_id=plant&offset=8 by Arduino_Genuino
int Sdetect(){
   int tempValue = 0; // variable to temporarly store moisture value 
   for (int a = 0; a < 10; a++) { 
     tempValue += analogRead(moisturePin); 
     delay(100); 
   } 
   tempValue=tempValue/10;
   tempValue=map(tempValue,1023,0,0,3);
   return tempValue; 
}

//Out put the conditions through sound
 void conditionOut(int light,int tempValue,int temperature){

  //light condition output part
      playcomplete("lig.wav");
    if(light==0)
    {
      playcomplete("low.wav");
    }
    else if (light == 1)
    {
      playcomplete("mid.wav");
    }
    else if (light ==2 )
    {
      playcomplete("high.wav");
    }
    else if ( light ==3)
    {
      playcomplete("vhigh.wav");
    }
    delay(500);
    
    //soild condition output part
   playcomplete("soi.wav");
    if(tempValue==0)
    {
      playcomplete("low.wav");
    }
    else if (tempValue == 1)
    {
      playcomplete("mid.wav");
    }
    else if (tempValue ==2 )
    {
      playcomplete("high.wav");
    }
    else if ( tempValue ==3)
    {
      playcomplete("vhigh.wav");
    }
    delay(500);
    
   //temperature output part
      playcomplete("tmp.wav");
   numberSp(temperature);
   playcomplete("cel.wav");
 }



 //this part is for playing numbers;
void numberSp(int i)
{
  if (i<0)//if the number is below 0 go in
  {
    playcomplete("minus.wav");
    i=-i;
  }
  //initializing two string
  String owav = "0.wav";
  String wav =".wav";
  //get the number in tens and ones
  int tens = i/10;
  int ones = i - tens*10;
  //connect the number and string ,get a new string
  String ten = tens + owav;
  String one = ones + wav;
  //convert them to char array
  char chat_ten[8];
  char chat_one[8];
  ten.toCharArray(chat_ten, 8);
  one.toCharArray(chat_one, 8);
  
  if(i>19||i<11)//if the number is 11-19 the condition is special you have to seperate them.
  {
    if(tens != 0)
    {
      
      playcomplete(chat_ten);
    }
    if(ones!=0)
    {
     playcomplete(chat_one);
    }
  }
  else if (i==11){playcomplete("11.wav");}
  else if (i==12){playcomplete("12.wav");}
  
  else if (i==13){playcomplete("13.wav");}
  else if (i==14){playcomplete("14.wav");}
  else if (i==15){playcomplete("15.wav");}
  else if (i==16){playcomplete("16.wav");}
  else if (i==17){playcomplete("17.wav");}
  else if (i==18){playcomplete("18.wav");}
  else if (i==19){playcomplete("19.wav");}
  
}
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
  
}


time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}
 
