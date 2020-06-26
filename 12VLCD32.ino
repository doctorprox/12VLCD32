

#define TEXT "aA MWyz~12" // Text that will be printed on screen in any font

#include "Free_Fonts.h" // Include the header file attached to this sketch

#include "TFT_eSPI.h"
#include <ESP32CAN.h>
#include <CAN_config.h>

CAN_device_t CAN_cfg;               // CAN Config
unsigned long previousMillis = 0;   // will store last time a CAN Message was send
const int interval = 500;          // interval at which send CAN Messages (milliseconds)
const int rx_queue_size = 20;       // Receive Queue size
CAN_frame_t rx_frame;

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

unsigned long drawTime = 0;
float voltage = 0;
unsigned long watts = 0;
int temp;
float pcbtemp;
float current = 0;
float v1=0;
float v2=0;
float v3=0;
float v4=0;
const float gain = 0.381;
const int offset = 1;
int pack_status = 0;
int soc=0;
int bms_state = 0;  //1 is DSG OFF, 2= CHG OFF, 3 = BOTH OFF
int sys_state = 0;
#define RELAY_ON 0
#define RELAY_OFF 1
#define relay_chg 33
#define relay_grid 26
#define relay_dsg 25
#define SYS_OFF 0
#define SYS_OG 1
#define SYS_CHG 2
bool button_og = 0;
bool button_chg = 0;


void setup(void) {
  Serial.begin(921600);
  Serial.println("HI");
  init_can();
  Serial.println("CANOK"); 
 
  pinMode(relay_chg,OUTPUT);     //relay
  pinMode(relay_dsg,OUTPUT);     //relay
  pinMode(relay_grid,OUTPUT);     //relay
  pinMode(12,INPUT_PULLDOWN); // BUTTON CHG
  pinMode(14,INPUT_PULLDOWN); // BUTTON OG
  digitalWrite(relay_chg,HIGH);
  digitalWrite(relay_dsg,HIGH);
  digitalWrite(relay_grid,HIGH);
  tft.begin();

  tft.setRotation(0);
  setup_lcd();
  Serial.println("SETUP COMPLETE"); 
}

void init_can(){
  CAN_cfg.speed = CAN_SPEED_500KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_32;
  CAN_cfg.rx_pin_id = GPIO_NUM_35;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();
}

void setup_lcd(){
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setFreeFont(FF18);                 // Select the font
//  tft.setTextSize(2);
  tft.drawString("Voltage:", 10, 70, GFXFF);// Print the string name of the font
  tft.drawString("Current:", 10, 100, GFXFF);// Print the string name of the font
  tft.drawString("Temp:", 10, 130, GFXFF);// Print the string name of the font
  tft.drawString("Power:", 10, 160, GFXFF);// Print the string name of the font
  tft.drawString("Battery:", 10, 190, GFXFF);// Print the string name of the font
  tft.drawString("System:", 10, 220, GFXFF);// Print the string name of the font

//batt bar
  tft.drawRoundRect(10,10,210,45,7,TFT_WHITE);
//  tft.fillRoundRect(15,15,200,35,6,TFT_GREEN);
  tft.drawRoundRect(219,22,10,20,2,TFT_WHITE);


  //power bar
  tft.drawRoundRect(10,290,220,20,7,TFT_WHITE);
//  tft.fillRoundRect(11,291,109,18,7,TFT_SKYBLUE);


}

void loop() {

  // try to parse packet
if(millis()-previousMillis > 10000){
  bms_state=04;
  pack_status = 4;
  updatelcd();
}

  
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {

    if (rx_frame.FIR.B.FF == CAN_frame_std) {
      //printf("New standard frame");
    }
    else {
      //printf("New extended frame");
    }

    if (rx_frame.FIR.B.RTR == CAN_RTR) {
      printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
    }
    else {

      switch (rx_frame.MsgID){
        case 0x18080A00:
            bms_state = rx_frame.data.u8[0];
            updatelcd();
            break;
        case 0x18080200:
            //bms_state = rx_frame.data.u8[1];
            voltage = rx_frame.data.u8[0] << 8;
            voltage += rx_frame.data.u8[1];
            voltage *= gain;
            voltage += offset;
            voltage/=1000;
          break;
       case 0x18080100:
            v1 = rx_frame.data.u8[0] << 8;
            v1 += rx_frame.data.u8[1];
            v1 *= gain;
            v1 += offset;
            v1/=1000;
            v2 = rx_frame.data.u8[2] << 8;
            v2 += rx_frame.data.u8[3];
            v2 *= gain;
            v2 += offset;
            v2/=1000;
            v3 = rx_frame.data.u8[4] << 8;
            v3 += rx_frame.data.u8[5];
            v3 *= gain;
            v3 += offset;  
            v3/=1000;
            v4 = rx_frame.data.u8[6] << 8;
            v4 += rx_frame.data.u8[7];
            v4 *= gain;
            v4 += offset;    
            v4/=1000;                  
            break;
        case 0x18080D00:
            /*printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
            for (int i = 0; i < rx_frame.FIR.B.DLC; i++) {
              printf("0x%02X ", rx_frame.data.u8[i]);
            }
            printf("\n");*/
            temp = rx_frame.data.u8[0] << 24;
            if(temp != 0){
              break;
            }
            temp += rx_frame.data.u8[1] << 16;
            temp += rx_frame.data.u8[2] << 8;
            temp += rx_frame.data.u8[3];   
                     
            current = (float)temp/500;
            watts = current * voltage;
            temp = rx_frame.data.u8[4];
            pack_status = temp;
            soc = rx_frame.data.u8[5];  
          break;   
          case 0x18080600:
            temp = rx_frame.data.u8[5] << 8;
            temp += rx_frame.data.u8[6];
            pcbtemp=((float)temp/10)-20;
          break;         
        default:
          break;
      }
    }
  }

}

void check_states(){
button_og = digitalRead(14);
button_chg = digitalRead(12);
Serial.println(String(button_og) + " , " + String(button_chg) + " BMS:" + String(bms_state));
if(button_og == 1 && (bms_state == 0 || bms_state==2) && sys_state != SYS_OG) { // button = OG && BMS STATE = DSG EN  && SYS_STATE != SYS_OG;
  sys_state = SYS_OG;
  Serial.println("OG");
  digitalWrite(relay_chg,RELAY_OFF);
  delay(1000);
  digitalWrite(relay_grid,RELAY_ON);
  delay(1000);
  digitalWrite(relay_dsg,RELAY_ON);
}
if((sys_state == SYS_CHG && current < 1 && voltage > 14)  || (bms_state > 1)  || ( button_og == 0 && sys_state != SYS_CHG))  // either we finished a charge, or bms fault, or we turned off OG mode
{
  sys_state = SYS_OFF;
  digitalWrite(relay_dsg,RELAY_OFF);
  delay(1000);  
  digitalWrite(relay_grid,RELAY_OFF); //connects shore supply
  delay(1000);
  digitalWrite(relay_chg,RELAY_OFF);
}

if(button_chg == 1 && bms_state < 2 && sys_state != SYS_CHG){  //chg button pressed and BMS CHG FET ON and sys_state != charging
  sys_state = SYS_CHG;
  digitalWrite(relay_dsg,RELAY_OFF);
  delay(1000);  
  digitalWrite(relay_grid,RELAY_OFF); //connects shore supply
  delay(1000);
  digitalWrite(relay_chg,RELAY_ON);
}
}


  void updatelcd(){

  if(millis() - previousMillis > interval) {
    previousMillis = millis();   
  } else{
    return;
  }
  check_states();
   tft.fillRect(110,70,130,175,TFT_BLACK);
   tft.drawString(String(voltage,2) + " v    ", 110, 70, GFXFF);// Print the string name of the font
   tft.drawString(String(current,2) + " A    ", 110, 100, GFXFF);// Print the string name of the font    
   tft.drawString(String(pcbtemp,2) + " C    ", 110, 130, GFXFF);// Print the string name of the font    
   tft.drawString(String(watts) + " W    ", 110, 160, GFXFF);// Print the string name of the font          
   tft.setFreeFont(FF17);                 // Select the font
    

   tft.drawString(String(v1,2) + "  "+ String(v2,2) +"  "+ String(v3,2)  + "  "+ String(v4,2), 10, 250, GFXFF);// Print the string name of the font       
   tft.setFreeFont(FF18);                 // Select the font

    
    tft.fillRoundRect(15,15,200,35,6,TFT_BLACK);
    if(soc>100){
      soc=100;
    }
    tft.fillRoundRect(15,15,soc*2,35,6,TFT_GREEN);       
    tft.fillRoundRect(11,291,218,18,7,TFT_BLACK);    
    float powerwidth=0;
    powerwidth = (float)watts/1500;
    if(powerwidth>1){
      powerwidth=1;
    }
    powerwidth *=218;
    if(watts>1000){
          tft.fillRoundRect(11,291,(int)powerwidth,18,7,TFT_RED);  
    }else if(watts>500){
          tft.fillRoundRect(11,291,(int)powerwidth,18,7,TFT_YELLOW);  
    }else{
          tft.fillRoundRect(11,291,(int)powerwidth,18,7,TFT_SKYBLUE);  
    }


    switch(pack_status){
        case 0:
                tft.drawString("Idle", 110, 190, GFXFF);// Print the string name of the font
                break;
        case 1:
                tft.drawString("Discharging", 110, 190, GFXFF);// Print the string name of the font
                break;  
        case 2:
                tft.drawString("Charging", 110, 190, GFXFF);// Print the string name of the font
                break; 
        case 4:
                tft.drawString("Timeout", 110, 190, GFXFF);// Print the string name of the font
                break;                     
    }   
switch(sys_state){
    case SYS_OFF:
          tft.drawString("GRID", 110, 220, GFXFF);// Print the string name of the font
          break;
    case SYS_CHG:
          tft.drawString("CHARGE", 110, 220, GFXFF);// Print the string name of the font
          break;
    case SYS_OG:
          tft.drawString("OFFGRID", 110, 220, GFXFF);// Print the string name of the font
          break;
}
  
   /*
    if(bms_state == 0){
      tft.drawString("OK", 110, 220, GFXFF);// Print the string name of the font
    } else {
      tft.drawString("FAULT", 110, 220, GFXFF);// Print the string name of the font
    }
    */
  }

#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif
