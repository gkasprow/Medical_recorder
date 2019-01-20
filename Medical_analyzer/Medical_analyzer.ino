//////////////////////////////////////////////////////////////////////////////////////////
//
//   AFE44xx Arduino Firmware
//
//   Copyright (c) 2016 ProtoCentral
//   
//   AFE4490 code based on EVM code from Texas Instruments
//   SpO2 computation based on original code from Maxim Integrated 
//
//   This software is licensed under the MIT License(http://opensource.org/licenses/MIT). 
//   
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   For information on how to use the HealthyPi, visit https://github.com/Protocentral/afe44xx_Oximeter
/////////////////////////////////////////////////////////////////////////////////////////

/* This code displays the computed SpO2 value through the Serial port*/


#include <OneWire.h>
#include <DallasTemperature.h>
// Digital pin the DS18B20 is connected to. Do not use digital pins 0 or 1 since those conflict with the use of Serial.
const int tmpPin = 22;
OneWire oneWire(tmpPin);
DallasTemperature sensors(&oneWire);



//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#define NTP_OFFSET  3600 // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "0.pl.pool.ntp.org"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// WiFi network info.
char ssid[] = "Casper";
char wifiPassword[] = "abbaabbaff.";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "dbeef360-a5c5-11e7-bba6-6918eb39b85e";
char password[] = "4bf5c884c749da17023a9156a1b514beef743dbe";
char clientID[] = "a33c2ae0-1c0e-11e9-a08c-c5a286f8c00d";
unsigned long lastMillis = 0;

#include <Wire.h>
#include "SSD1306.h"
SSD1306 display(0x3c, 16, 17); 

// CONNECTIONS:
// DS1302 CLK/SCLK --> 5
// DS1302 DAT/IO --> 4
// DS1302 RST/CE --> 2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND

#include <ThreeWire.h>  
#include <RtcDS1302.h>
ThreeWire myWire(25,33,32); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

#include <string.h>
#include <SPI.h>
#include <math.h>
#include <mySD.h>

//afe44xx Register definition
#define CONTROL0    0x00
#define LED2STC     0x01
#define LED2ENDC    0x02
#define LED2LEDSTC    0x03
#define LED2LEDENDC   0x04
#define ALED2STC    0x05
#define ALED2ENDC   0x06
#define LED1STC     0x07
#define LED1ENDC    0x08
#define LED1LEDSTC    0x09
#define LED1LEDENDC   0x0a
#define ALED1STC    0x0b
#define ALED1ENDC   0x0c
#define LED2CONVST    0x0d
#define LED2CONVEND   0x0e
#define ALED2CONVST   0x0f
#define ALED2CONVEND  0x10
#define LED1CONVST    0x11
#define LED1CONVEND   0x12
#define ALED1CONVST   0x13
#define ALED1CONVEND  0x14
#define ADCRSTCNT0    0x15
#define ADCRSTENDCT0  0x16
#define ADCRSTCNT1    0x17
#define ADCRSTENDCT1  0x18
#define ADCRSTCNT2    0x19
#define ADCRSTENDCT2  0x1a
#define ADCRSTCNT3    0x1b
#define ADCRSTENDCT3  0x1c
#define PRPCOUNT    0x1d
#define CONTROL1    0x1e
#define SPARE1      0x1f
#define TIAGAIN     0x20
#define TIA_AMB_GAIN  0x21
#define LEDCNTRL    0x22
#define CONTROL2    0x23
#define SPARE2      0x24
#define SPARE3      0x25
#define SPARE4      0x26
#define SPARE4      0x26
#define RESERVED1   0x27
#define RESERVED2   0x28
#define ALARM     0x29
#define LED2VAL     0x2a
#define ALED2VAL    0x2b
#define LED1VAL     0x2c
#define ALED1VAL    0x2d
#define LED2ABSVAL    0x2e
#define LED1ABSVAL    0x2f
#define DIAG      0x30
#define count 60

#define CES_CMDIF_PKT_START_1   0x0A
#define CES_CMDIF_PKT_START_2   0xFA
#define CES_CMDIF_TYPE_DATA   0x02
#define CES_CMDIF_PKT_STOP    0x0B

#define RESET 4 // RESET pin 
#define SPISTE 5 // chip select
#define SPIDRDY 2 // data ready pin 

//int IRheartsignal[count];
//int Redheartsignal[count];
int IRdc[count];
int Reddc[count];
double difIRheartsig_dc;
double difREDheartsig_dc;
double powdifIR;
double powdifRed;
double IRac; 
double Redac;
double SpOpercentage;
double Ratio;

File root;

volatile int drdy_trigger = LOW;

void afe44xxInit (void);
void afe44xxWrite (uint8_t address, uint32_t data);
unsigned long afe44xxRead (uint8_t address);
signed long average_BPM( signed long );
volatile char DataPacketHeader[6];
volatile char DataPacket[10];
volatile char DataPacketFooter[2];
int datalen = 0x09;
//unsigned long time;

volatile static int SPI_RX_Buff_Count = 0;
volatile char *SPI_RX_Buff_Ptr;
volatile int afe44xx_data_ready = false;
volatile unsigned int pckt =0, buff=0,t=0;
unsigned long ueegtemp = 0, ueegtemp2 = 0;
unsigned long IRtemp,REDtemp;
signed long seegtemp=0, seegtemp2=0;
volatile int i;


uint16_t aun_ir_buffer[400]; //infrared LED sensor data ////////////////////////
uint16_t aun_red_buffer[400];  //red LED sensor data /////////////////////////

#define FS            25    //sampling frequency
#define BUFFER_SIZE  (FS*16) ////////////////////////*4
#define MA4_SIZE  4 // DONOT CHANGE
#define min(x,y) ((x) < (y) ? (x) : (y))

const uint8_t uch_spo2_table[184]={ 95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99, 
              99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 
              100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97, 
              97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91, 
              90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 
              80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67, 
              66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 
              49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29, 
              28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5, 
              3, 2, 1 } ;

static  int32_t an_x[ BUFFER_SIZE]; 
static  int32_t an_y[ BUFFER_SIZE]; 

volatile int16_t n_buffer_count; //data length

int32_t n_spo2;  //SPO2 value
int8_t ch_spo2_valid;  //indicator to show if the SPO2 calculation is valid
int32_t n_heart_rate; //heart rate value
int8_t  ch_hr_valid;  //indicator to show if the heart rate calculation is valid

long status_byte=0;
uint8_t LeadStatus=0;
boolean leadoff_deteted = true;
uint8_t spo2_probe_open = false;
int dec=0;



void setup()
{
   Serial.begin(57600);
Cayenne.begin(username, password, clientID, ssid, wifiPassword);
Serial.print("Initializing OLED...");


while (WiFi.status() != WL_CONNECTED)
{
delay(500);
Serial.print(".");
}
Serial.println("");
Serial.println("WiFi connected.");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());
timeClient.begin();

 
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
 // display.drawString(0, 0, "SpO2 analyzer");
  
timeClient.update();
String formattedTime = timeClient.getFormattedTime();
Serial.println(formattedTime);
display.drawString(0, 0, formattedTime);
display.display();



  

  Serial.print("Initializing SD card...");
  /* initialize SD library with SPI pins */
  if (!SD.begin(26, 14, 12, 27)) {
    Serial.println("initialization failed!");
    return;
  }
    Serial.println("initialization done.");
  /* Begin at the root "/" */
  root = SD.open("/");
  if (root) {    
    printDirectory(root, 0);
    root.close();
  } else {
    Serial.println("error opening results.txt");
  }
  /* open "results.txt" for writing */
  root = SD.open("results.txt", FILE_WRITE);
  /* if open succesfully -> root != NULL 
    then write string "Hello world!" to it
  */
  if (root) {
   // root.println("Hello world!");
    root.flush();
   /* close the file */
    root.close();
  } else {
    /* if the file open error, print an error */
    Serial.println("error opening results.txt");
  }
  delay(1000);
  /* after writing then reopen the file and read it */
  root = SD.open("results.txt");
  if (root) {    
    /* read from the file until there's nothing else in it */
    while (root.available()) {
      /* read the file and print to Terminal */
      Serial.write(root.read());
    }
    root.close();
  } else {
    Serial.println("error opening results.txt");
  }

  

   
   Serial.println("Intilazition AFE44xx.. ");   
   
   delay(2000) ;   // pause for a moment
  
   SPI.begin(); 

   // configure and set RESET
   pinMode (RESET,OUTPUT);//Slave Select

   digitalWrite(RESET, LOW);
   delay(500);
   digitalWrite(RESET, HIGH);
   delay(500);    
   
   // set pin directions
   pinMode (SPISTE,OUTPUT);//Slave Select
   digitalWrite(SPISTE, HIGH);
   pinMode (SPIDRDY,INPUT);// data ready 
 
   attachInterrupt(2, afe44xx_drdy_event, RISING ); // Digital2 is attached to Data ready pin of AFE is interrupt0 in ARduino

   // set SPI transmission
   SPI.setClockDivider (SPI_CLOCK_DIV8); // set Speed as 2MHz , 16MHz/ClockDiv
   SPI.setDataMode (SPI_MODE0);          //Set SPI mode as 0
   SPI.setBitOrder (MSBFIRST);           //MSB first

// Packet structure
  DataPacketHeader[0] = CES_CMDIF_PKT_START_1;  //packet header1 0x0A
  DataPacketHeader[1] = CES_CMDIF_PKT_START_2;  //packet header2 0xFA
  DataPacketHeader[2] = datalen;                // data length 9
  DataPacketHeader[3] = (uint8_t)(datalen >> 8);
  DataPacketHeader[4] = CES_CMDIF_TYPE_DATA;
  
  DataPacketFooter[0] = 0x00;
  DataPacketFooter[1] = CES_CMDIF_PKT_STOP;

   
   afe44xxInit (); 
   Serial.println("intilazition is done");
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{    

 // Serial.println("loop started");
     if (drdy_trigger == HIGH)  
     {
        detachInterrupt(0);
        afe44xxWrite(CONTROL0,0x000001);  
        IRtemp = afe44xxRead(LED1VAL);
        afe44xxWrite(CONTROL0,0x000001);  
        REDtemp = afe44xxRead(LED2VAL);   
        afe44xx_data_ready = true;
     }  
                    
  if(afe44xx_data_ready == true)
  {
      
      IRtemp = (unsigned long) (IRtemp<<10);
      seegtemp = (signed long) (IRtemp);
      seegtemp = (signed long) (seegtemp>>10);  
                      
      REDtemp = (unsigned long) (REDtemp<<10);
      seegtemp2 = (signed long) (REDtemp);
      seegtemp2 = (signed long) (seegtemp2>>10);
  
      if(dec==20)
      {
        aun_ir_buffer[n_buffer_count]=(uint16_t) (seegtemp>>4);
        aun_red_buffer[n_buffer_count]=(uint16_t) (seegtemp2>>4);
        n_buffer_count++;
        dec=0;
        
      }
      dec++;
     // Serial.print(n_buffer_count);  
      if(n_buffer_count>399) /////////////////99
      {
        
        estimate_spo2(aun_ir_buffer, 400, aun_red_buffer, &n_spo2, &ch_spo2_valid,&n_heart_rate, &ch_hr_valid); //////////////////100
        if(n_spo2 == -999)
        {
          Serial.println("Probe error!!!!");
          display.clear();
          timeClient.update();
          display.setFont(ArialMT_Plain_16);
          String formattedTime = timeClient.getFormattedTime();
          display.drawString(0, 0, formattedTime);
          display.display();

          Cayenne.loop();
        }
        else
        {  

           sensors.requestTemperatures();
           
            Serial.print("calculating sp02...");
            Serial.print(" Sp02 : ");
            Serial.print(n_spo2);
            Serial.print("% ,"); 
            Serial.print("Pulse rate :");
            Serial.print(n_heart_rate);                       
            Serial.println(" bpm");

        display.clear();
        display.setFont(ArialMT_Plain_16);
        display.drawString(0,16,("SpO2 " + String(n_spo2)+"%"));
        display.drawString(0,32,("Pulse " + String(n_heart_rate)+"bpm"));
          timeClient.update();
          String formattedTime = timeClient.getFormattedTime();
          display.drawString(0, 0, formattedTime);
         // display.display();

        Cayenne.virtualWrite(0, n_spo2);
        Cayenne.virtualWrite(1, n_heart_rate);
        
       display.setFont(ArialMT_Plain_16);
    Cayenne.celsiusWrite(2, sensors.getTempCByIndex(0));
    Cayenne.celsiusWrite(3, sensors.getTempCByIndex(1));
    Cayenne.celsiusWrite(4, sensors.getTempCByIndex(2));

      Serial.print("tem0: ");Serial.println(sensors.getTempCByIndex(0));
      Serial.print("tem1: ");Serial.println(sensors.getTempCByIndex(1));
      Serial.print("tem2: ");Serial.println(sensors.getTempCByIndex(2));
      
    display.setFont(ArialMT_Plain_10);
    display.drawString(0,48, ( String(sensors.getTempCByIndex(0)) + "C" ));
    display.drawString(50,48,( String(sensors.getTempCByIndex(1)) + "C" ));
    display.drawString(96,48,( String(sensors.getTempCByIndex(2)) + "C" ));
    display.display();
      
        Cayenne.loop();

 root = SD.open("results.txt", FILE_WRITE);
  /* if open succesfully -> root != NULL 
    then write string "Hello world!" to it
  */
  if (root) {
    root.print(formattedTime);
    root.print("; Sp02:; ");
    root.print(n_spo2);
    root.print("% ;"); 
    root.print("Pulse rate :;");
    root.print(n_heart_rate);
    root.print(" bpm ;");
    
    root.print("T black ; " + String(sensors.getTempCByIndex(0)) + "C ;" );
    root.print("T brown ; " + String(sensors.getTempCByIndex(1)) + "C ;" );
    root.print("T red ; " + String(sensors.getTempCByIndex(2)) + "C ;" );
    
    root.println("");
    
    root.flush();
   /* close the file */
    root.close();
  } else {
    /* if the file open error, print an error */
    Serial.println("error opening results.txt");
  }

            
        }
        n_buffer_count=0;
      }         
     
      afe44xx_data_ready = false;
      drdy_trigger = LOW;
      attachInterrupt(0, afe44xx_drdy_event, RISING );      
       
  }                   
        
}

///////// Gets Fired on DRDY event/////////////////////////////

void afe44xx_drdy_event()
{
  drdy_trigger = HIGH;
}

////////////////AFE44xx initialization//////////////////////////////////////////
void afe44xxInit (void)
{ 
  //  Serial.println("afe44xx Initialization Starts"); 
     afe44xxWrite(CONTROL0,0x000000);

     afe44xxWrite(CONTROL0,0x000008);  

     afe44xxWrite(TIAGAIN,0x000000); // CF = 5pF, RF = 500kR
     afe44xxWrite(TIA_AMB_GAIN,0x000001); 
     
     afe44xxWrite(LEDCNTRL,0x001414);    
     afe44xxWrite(CONTROL2,0x000000); // LED_RANGE=100mA, LED=50mA 
     afe44xxWrite(CONTROL1,0x010707); // Timers ON, average 3 samples  
    
     afe44xxWrite(PRPCOUNT, 0X001F3F);

    afe44xxWrite(LED2STC, 0X001770); 
    afe44xxWrite(LED2ENDC,0X001F3E); 
    afe44xxWrite(LED2LEDSTC,0X001770);
    afe44xxWrite(LED2LEDENDC,0X001F3F);
    afe44xxWrite(ALED2STC, 0X000000); 
    afe44xxWrite(ALED2ENDC, 0X0007CE);
    afe44xxWrite(LED2CONVST,0X000002); 
    afe44xxWrite(LED2CONVEND, 0X0007CF);
    afe44xxWrite(ALED2CONVST, 0X0007D2); 
    afe44xxWrite(ALED2CONVEND,0X000F9F); 

    afe44xxWrite(LED1STC, 0X0007D0); 
    afe44xxWrite(LED1ENDC, 0X000F9E);
    afe44xxWrite(LED1LEDSTC, 0X0007D0);
    afe44xxWrite(LED1LEDENDC, 0X000F9F);
    afe44xxWrite(ALED1STC, 0X000FA0); 
    afe44xxWrite(ALED1ENDC, 0X00176E);
    afe44xxWrite(LED1CONVST, 0X000FA2);
    afe44xxWrite(LED1CONVEND, 0X00176F); 
    afe44xxWrite(ALED1CONVST, 0X001772); 
    afe44xxWrite(ALED1CONVEND, 0X001F3F);

    afe44xxWrite(ADCRSTCNT0, 0X000000);
    afe44xxWrite(ADCRSTENDCT0,0X000000); 
    afe44xxWrite(ADCRSTCNT1, 0X0007D0); 
    afe44xxWrite(ADCRSTENDCT1, 0X0007D0);
    afe44xxWrite(ADCRSTCNT2, 0X000FA0);
    afe44xxWrite(ADCRSTENDCT2, 0X000FA0); 
    afe44xxWrite(ADCRSTCNT3, 0X001770); 
    afe44xxWrite(ADCRSTENDCT3, 0X001770);
  
    delay(1000);
   // Serial.println("afe44xx Initialization Done"); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void afe44xxWrite (uint8_t address, uint32_t data)
{
    digitalWrite (SPISTE, LOW); // enable device
    SPI.transfer (address); // send address to device
    SPI.transfer ((data >> 16) & 0xFF); // write top 8 bits
    SPI.transfer ((data >> 8) & 0xFF); // write middle 8 bits
    SPI.transfer (data & 0xFF); // write bottom 8 bits    
    digitalWrite (SPISTE, HIGH); // disable device
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long afe44xxRead (uint8_t address)
{       
    unsigned long data=0;
    digitalWrite (SPISTE, LOW); // enable device
    SPI.transfer (address); // send address to device
    //SPI.transfer (data);
    data |= ((unsigned long)SPI.transfer (0)<<16); // read top 8 bits data
    data |= ((unsigned long)SPI.transfer (0)<<8); // read middle 8 bits  data
    data |= SPI.transfer (0); // read bottom 8 bits data
    digitalWrite (SPISTE, HIGH); // disable device
    

    return data; // return with 24 bits of read data
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void estimate_spo2(uint16_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint16_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid, int32_t *pn_heart_rate, int8_t *pch_hr_valid)
{
  uint32_t un_ir_mean,un_only_once ;
  int32_t k, n_i_ratio_count;
  int32_t i, s, m, n_exact_ir_valley_locs_count, n_middle_idx;
  int32_t n_th1, n_npks, n_c_min;   
  int32_t an_ir_valley_locs[60] ;  /////////////////////////15
  int32_t n_peak_interval_sum;
  
  int32_t n_y_ac, n_x_ac;
  int32_t n_spo2_calc; 
  int32_t n_y_dc_max, n_x_dc_max; 
  int32_t n_y_dc_max_idx, n_x_dc_max_idx; 
  int32_t an_ratio[30], n_ratio_average; /////////5
  int32_t n_nume, n_denom ;

  Serial.println("DC"); //////////////////////////////
  // calculates DC mean and subtract DC from ir
  un_ir_mean =0; 
  for (k=0 ; k<n_ir_buffer_length ; k++ ) un_ir_mean += pun_ir_buffer[k] ;
  un_ir_mean =un_ir_mean/n_ir_buffer_length ;

    Serial.println(" rem DC"); //////////////////////////////
  // remove DC and invert signal so that we can use peak detector as valley detector
  for (k=0 ; k<n_ir_buffer_length ; k++ )  
    an_x[k] = -1*(pun_ir_buffer[k] - un_ir_mean) ; 

    Serial.println(" mAVG"); //////////////////////////////
  // 4 pt Moving Average
  for(k=0; k< BUFFER_SIZE-MA4_SIZE; k++){
    an_x[k]=( an_x[k]+an_x[k+1]+ an_x[k+2]+ an_x[k+3])/(int)4;        
  }
  // calculate threshold  
  n_th1=0; 
  for ( k=0 ; k<BUFFER_SIZE ;k++){
    n_th1 +=  an_x[k];
  }
  n_th1=  n_th1/ ( BUFFER_SIZE);
  if( n_th1<30) n_th1=30; // min allowed
  if( n_th1>60) n_th1=60; // max allowed

  for ( k=0 ; k<60;k++) an_ir_valley_locs[k]=0;//////////////15
  // since we flipped signal, we use peak detector as valley detector
  find_peak( an_ir_valley_locs, &n_npks, an_x, BUFFER_SIZE, n_th1, 4, 30 );//peak_height, peak_distance, max_num_peaks /////////////////////////15
  n_peak_interval_sum =0;
  if (n_npks>=2){
    for (k=1; k<n_npks; k++) n_peak_interval_sum += (an_ir_valley_locs[k] -an_ir_valley_locs[k -1] ) ;
    n_peak_interval_sum =n_peak_interval_sum/(n_npks-1);
    *pn_heart_rate =(int32_t)( (FS*60)/ n_peak_interval_sum );
    *pch_hr_valid  = 1;
  }
  else  { 
    *pn_heart_rate = -999; // unable to calculate because # of peaks are too small
    *pch_hr_valid  = 0;
  }

  //  load raw value again for SPO2 calculation : RED(=y) and IR(=X)
  for (k=0 ; k<n_ir_buffer_length ; k++ )  {
      an_x[k] =  pun_ir_buffer[k] ; 
      an_y[k] =  pun_red_buffer[k] ; 
  }

  // find precise min near an_ir_valley_locs
  n_exact_ir_valley_locs_count =n_npks; 
  
  //using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
  //finding AC/DC maximum of raw

  n_ratio_average =0; 
  n_i_ratio_count = 0; 
  for(k=0; k< 5; k++) an_ratio[k]=0;
  for (k=0; k< n_exact_ir_valley_locs_count; k++){
    if (an_ir_valley_locs[k] > BUFFER_SIZE ){
      *pn_spo2 =  -999 ; // do not use SPO2 since valley loc is out of range
      *pch_spo2_valid  = 0; 
      return;
    }
  }
  // find max between two valley locations 
  // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2 
  for (k=0; k< n_exact_ir_valley_locs_count-1; k++){
    n_y_dc_max= -16777216 ; 
    n_x_dc_max= -16777216; 
    if (an_ir_valley_locs[k+1]-an_ir_valley_locs[k] >3){
        for (i=an_ir_valley_locs[k]; i< an_ir_valley_locs[k+1]; i++){
          if (an_x[i]> n_x_dc_max) {n_x_dc_max =an_x[i]; n_x_dc_max_idx=i;}
          if (an_y[i]> n_y_dc_max) {n_y_dc_max =an_y[i]; n_y_dc_max_idx=i;}
      }
      n_y_ac= (an_y[an_ir_valley_locs[k+1]] - an_y[an_ir_valley_locs[k] ] )*(n_y_dc_max_idx -an_ir_valley_locs[k]); //red
      n_y_ac=  an_y[an_ir_valley_locs[k]] + n_y_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k])  ; 
      n_y_ac=  an_y[n_y_dc_max_idx] - n_y_ac;    // subracting linear DC compoenents from raw 
      n_x_ac= (an_x[an_ir_valley_locs[k+1]] - an_x[an_ir_valley_locs[k] ] )*(n_x_dc_max_idx -an_ir_valley_locs[k]); // ir
      n_x_ac=  an_x[an_ir_valley_locs[k]] + n_x_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k]); 
      n_x_ac=  an_x[n_y_dc_max_idx] - n_x_ac;      // subracting linear DC compoenents from raw 
      n_nume=( n_y_ac *n_x_dc_max)>>7 ; //prepare X100 to preserve floating value
      n_denom= ( n_x_ac *n_y_dc_max)>>7;
      if (n_denom>0  && n_i_ratio_count <5 &&  n_nume != 0)
      {   
        an_ratio[n_i_ratio_count]= (n_nume*100)/n_denom ; //formular is ( n_y_ac *n_x_dc_max) / ( n_x_ac *n_y_dc_max) ;
        n_i_ratio_count++;
      }
    }
  }
  // choose median value since PPG signal may varies from beat to beat
  Serial.println(" sort"); //////////////////////////////
  sort_ascend(an_ratio, n_i_ratio_count);
  n_middle_idx= n_i_ratio_count/2;

  if (n_middle_idx >1)
    n_ratio_average =( an_ratio[n_middle_idx-1] +an_ratio[n_middle_idx])/2; // use median
  else
    n_ratio_average = an_ratio[n_middle_idx ];

  if( n_ratio_average>2 && n_ratio_average <184){
    n_spo2_calc= uch_spo2_table[n_ratio_average] ;
    *pn_spo2 = n_spo2_calc ;
    *pch_spo2_valid  = 1;//  float_SPO2 =  -45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845 ;  // for comparison with table
  }
  else{
    *pn_spo2 =  -999 ; // do not use SPO2 since signal an_ratio is out of range
    *pch_spo2_valid  = 0; 
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void find_peak( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num )
/**
* \brief        Find peaks
* \par          Details
*               Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE
*
* \retval       None
*/
{
    find_peak_above( pn_locs, n_npks, pn_x, n_size, n_min_height );
    remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );
    *n_npks = min( *n_npks, n_max_num );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void find_peak_above( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height )
/**
* \brief        Find peaks above n_min_height
* \par          Details
*               Find all peaks above MIN_HEIGHT
*
* \retval       None
*/
{
  int32_t i = 1, n_width;
  *n_npks = 0;
  
  while (i < n_size-1){
    if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i-1]){      // find left edge of potential peaks
      n_width = 1;
      while (i+n_width < n_size && pn_x[i] == pn_x[i+n_width])  // find flat peaks
        n_width++;
      if (pn_x[i] > pn_x[i+n_width] && (*n_npks) < 15 ){      // find right edge of peaks
        pn_locs[(*n_npks)++] = i;    
        // for flat peaks, peak location is left edge
        i += n_width+1;
      }
      else
        i += n_width;
    }
    else
      i++;
    //  Serial.println("beat");
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance)
/**
* \brief        Remove peaks
* \par          Details
*               Remove peaks separated by less than MIN_DISTANCE
*
* \retval       None
*/
{
    
  int32_t i, j, n_old_npks, n_dist;
    
  /* Order peaks from large to small */
  sort_indices_descend( pn_x, pn_locs, *pn_npks );

  for ( i = -1; i < *pn_npks; i++ ){
    n_old_npks = *pn_npks;
    *pn_npks = i+1;
    for ( j = i+1; j < n_old_npks; j++ ){
      n_dist =  pn_locs[j] - ( i == -1 ? -1 : pn_locs[i] ); // lag-zero peak of autocorr is at index -1
      if ( n_dist > n_min_distance || n_dist < -n_min_distance )
        pn_locs[(*pn_npks)++] = pn_locs[j];
    }
  }

  // Resort indices int32_to ascending order
  sort_ascend( pn_locs, *pn_npks );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sort_ascend(int32_t  *pn_x, int32_t n_size) 
/**
* \brief        Sort array
* \par          Details
*               Sort array in ascending order (insertion sort algorithm)
*
* \retval       None
*/
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_x[i];
    for (j = i; j > 0 && n_temp < pn_x[j-1]; j--)
        pn_x[j] = pn_x[j-1];
    pn_x[j] = n_temp;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sort_indices_descend(  int32_t  *pn_x, int32_t *pn_indx, int32_t n_size)
/**
* \brief        Sort indices
* \par          Details
*               Sort indices according to descending order (insertion sort algorithm)
*
* \retval       None
*/ 
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_indx[i];
    for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
      pn_indx[j] = pn_indx[j-1];
    pn_indx[j] = n_temp;
  }
}


void printDirectory(File dir, int numTabs) {
  
  while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');   // we'll have a nice indentation
     }
     // Print the name
     Serial.print(entry.name());
     /* Recurse for directories, otherwise print the file size */
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       /* files have sizes, directories do not */
       Serial.print("\t\t");
       Serial.println(entry.size());
     }
     entry.close();
   }
}
