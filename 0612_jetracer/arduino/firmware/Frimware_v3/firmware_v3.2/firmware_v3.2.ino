//FaBo JetRacer v3.2 Beta
//2022/09/30
//Rev2.0.21は定数BOARDVERを２１とすること。
//SPI-LED点灯を6個から7個へ変更(#405 JetRacer Rev2.0.2)

#define BOARDVER 21               //Rev21は21         
#define SWITCHINGTREDSHOLD 1500   //信号切り替え1500u秒
#define NUMBERMEASURE 10          //判定計測回数

//ピンセッティング
#if BOARDVER < 21
  #define ST_SIGNAL_INPUT_PIN       A0
  #define TH_SIGNAL_INPUT_PIN       A1
  #define FSW_SIGNAL_INPUT_PIN      A3
  #define SELECT_OUTPUT_PIN         2
  #define RC_LED_PIN                3
  #define JETSON_LED_PIN            4
  
#elif BOARDVER == 21
  #define ST_SIGNAL_INPUT_PIN       2   //INT0
  #define TH_SIGNAL_INPUT_PIN       3   //INT1
  #define FSW_SIGNAL_INPUT_PIN      A3  
  #define MODE_SWITCH_INPUT_PIN     5   //JetRacer mode or Donkey mode.
  #define SELECT_OUTPUT_PIN         9
  #define RC_LED_PIN                10
  #define JETSON_LED_PIN            4
  
#endif

#include "SPI.h"
#include <Wire.h>

uint8_t registerIndex = 0;

typedef union
{
    uint32_t    before;
    struct
    {
        uint8_t d;
        uint8_t c;
        uint8_t b;
        uint8_t a;
    };
} Transfer;

Transfer transfer1;
Transfer transfer2;
Transfer transfer3;

void receiveEvent() {
  while(Wire.available() > 0){
    registerIndex = Wire.read();
  }
}

void requestEvent() {
  if(registerIndex == 0x01)
  {
    //ステアリング信号
    Wire.write(transfer1.a);
    Wire.write(transfer1.b);
    Wire.write(transfer1.c);
    Wire.write(transfer1.d);
    //スロットル信号
    Wire.write(transfer2.a);
    Wire.write(transfer2.b);
    Wire.write(transfer2.c);
    Wire.write(transfer2.d);
    #if BOARDVER == 21
    //切り替え信号
    Wire.write(transfer3.a);
    Wire.write(transfer3.b);
    Wire.write(transfer3.c);
    Wire.write(transfer3.d);
    #endif    
  }
}

void setup(){
  pinMode(SELECT_OUTPUT_PIN, OUTPUT);
  pinMode(RC_LED_PIN, OUTPUT);
  pinMode(JETSON_LED_PIN, OUTPUT);
  pinMode(ST_SIGNAL_INPUT_PIN, INPUT);
  pinMode(TH_SIGNAL_INPUT_PIN, INPUT);
  pinMode(FSW_SIGNAL_INPUT_PIN, INPUT);

  #if BOARDVER == 21
    pinMode(MODE_SWITCH_INPUT_PIN, INPUT_PULLUP);
  #elif BOARDVER == 20
    pinMode(MODE_SWITCH_INPUT_PIN, INPUT_PULLUP);
  #endif
  
  SPI.begin();
  Wire.begin(8);  //i2cデバイスアドレスは0x08
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}


void startBit() {
  byte start = 0x00;
  SPI.transfer(start);
  SPI.transfer(start);
  SPI.transfer(start);
  SPI.transfer(start);
}

void endBit() {
  byte end = 0x00;
  SPI.transfer(end);
  SPI.transfer(end);
  SPI.transfer(end);
  SPI.transfer(end);
}

void setRGB(short r, short g, short b) {
  SPI.transfer(0xEF);
  SPI.transfer(r);
  SPI.transfer(g);
  SPI.transfer(b);
}


void loop(){
  
  #if BOARDVER == 21
  //チャッタリング防止対策カウンタ
  static uint16_t counta;
  static uint16_t countb;
  #endif

  //信号計測
  uint32_t duration = pulseInLong(FSW_SIGNAL_INPUT_PIN, HIGH,25000);
  uint32_t pwm1 = pulseInLong(TH_SIGNAL_INPUT_PIN, HIGH,25000);
  uint32_t pwm0 = pulseInLong(ST_SIGNAL_INPUT_PIN, HIGH,25000);
  

  transfer1.before=pwm0;
  transfer2.before=pwm1;
  transfer3.before=duration;
  
  #if BOARDVER == 21 | BOARDVER == 20
    int val = digitalRead(MODE_SWITCH_INPUT_PIN);
  #endif

 //Rev21
 #if BOARDVER == 21
    //スライドスイッチの状態により、スイッチのRCモードにはしない。
    if (val == LOW){
      //Donkey Car mode.
      digitalWrite(SELECT_OUTPUT_PIN, HIGH);
      digitalWrite(RC_LED_PIN, HIGH);
      digitalWrite(JETSON_LED_PIN, LOW);
      startBit();
      //橙発光　7個点灯
      setRGB(0,   255,   128);
      setRGB(0,   255,   128);
      setRGB(0,   255,   128);
      setRGB(0,   255,   128);
      setRGB(0,   255,   128);
      setRGB(0,   255,   128);
      setRGB(0,   255,   128);
      endBit();

    }else{
      //JetRacer Mode
      if(duration > SWITCHINGTREDSHOLD){
        //チャッタリング防止
        counta++;
        countb = 0;
        if (counta > NUMBERMEASURE){
            //AIモード
            digitalWrite(SELECT_OUTPUT_PIN, HIGH);
            digitalWrite(RC_LED_PIN, HIGH);
            digitalWrite(JETSON_LED_PIN, LOW);
            startBit();
            //レインボー発光 7個点灯
            setRGB(80,   0,   10);
            setRGB(80,   0,   40);
            setRGB(80,   0,   90);
            setRGB(80,   0,   130);
            setRGB(80,   0,   170);
            setRGB(80,   0,   210);
            setRGB(80,   0,   255);
            endBit();
            counta = 0;//カウンタリセット
          }
        } 
        else{
          //チャッタリング防止
          countb++;
          counta = 0;
          if  (countb > NUMBERMEASURE){
            //RCカーモード
            digitalWrite(SELECT_OUTPUT_PIN, LOW);
            digitalWrite(RC_LED_PIN, LOW);
            digitalWrite(JETSON_LED_PIN, HIGH);
            startBit();
            //緑色発光　7個点灯
            setRGB(0,   255,   0);
            setRGB(0,   255,   0);
            setRGB(0,   255,   0);
            setRGB(0,   255,   0);
            setRGB(0,   255,   0);
            setRGB(0,   255,   0);
            setRGB(0,   255,   0);
            endBit();
            //カウンタリセット
            countb = 0;
          }
          
        }
    }

 //Rev20以前
 #elif BOARDVER < 21
  if(duration > SWITCHINGTREDSHOLD){
    digitalWrite(SELECT_OUTPUT_PIN, HIGH);
    digitalWrite(RC_LED_PIN, HIGH);
    digitalWrite(JETSON_LED_PIN, LOW);
    startBit();
    //レインボー発光
    setRGB(80,   0,   10);
    setRGB(80,   0,   40);
    setRGB(80,   0,   90);
    setRGB(80,   0,   130);
    setRGB(80,   0,   170);
    setRGB(80,   0,   210);
    endBit();
  } 
  else{
    digitalWrite(SELECT_OUTPUT_PIN, LOW);
    digitalWrite(RC_LED_PIN, LOW);
    digitalWrite(JETSON_LED_PIN, HIGH);
    startBit();
    //緑色発光
    setRGB(0,   255,   0);
    setRGB(0,   255,   0);
    setRGB(0,   255,   0);
    setRGB(0,   255,   0);
    setRGB(0,   255,   0);
    setRGB(0,   255,   0);
    endBit();
    
  } 
 #endif
}
