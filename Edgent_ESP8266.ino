#define BLYNK_FIRMWARE_VERSION    "0.1.0"
// #define BLYNK_PRINT               Serial
// #define BLYNK_DEBUG

#define BLYNK_TEMPLATE_ID "TMPLdWBtr92H"
#define BLYNK_TEMPLATE_NAME "EduTic SmartRoom 1"
#include "BlynkEdgent.h"
#include "DHT.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

const uint16_t kIrLed = 12;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
#define celeng 59
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Example of data captured by IRrecvDumpV2.ino
static const uint16_t LGon[celeng] =  {3412, 9958,  574, 1546,  494, 546,  478, 564,  498, 542,  498, 1582,  498, 550,  520, 518,  472, 574,  524, 530,  498, 558,  498, 550,  498, 542,  522, 518,  522, 516,  524, 516,  496, 558,  524, 1556,  498, 542,  496, 542,  546, 1534,  522, 518,  522, 1570,  526, 516,  498, 542,  522, 1566,  496, 1604,  500, 548,  526, 1556,  524};  // LG2 880094D
static const uint16_t LGoff[celeng] = {3180, 9954,  530, 1572,  528, 512,  500, 538,  528, 512,  502, 1576,  552, 486,  500, 538,  526, 514,  524, 1556, 498, 1604, 498, 540,  498, 542,  522, 516,  522, 516,  498, 550,  520, 518,  522, 516,  522, 516,  520, 520,  520, 520,  522, 518,  520, 1558,  520, 528,  520, 1556,  538, 518,  520, 518,  522, 518,  522, 1566,  522};  // LG2 88C0051

int a, b;
#define DHTPIN 14 
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
float t, h;

BlynkTimer timer;

// IRsend irsend(12);  // An IR LED is controlled by GPIO pin 14 (D5)

// 0 : TOWER
// 1 : WALL
const unsigned int kAc_Type  = 1;

// 0 : cooling
// 1 : heating
unsigned int ac_heat = 1;

// 0 : off
// 1 : on
unsigned int ac_power_on = 0;

// 0 : off
// 1 : on --> power on
unsigned int ac_air_clean_state = 0;

// temperature : 18 ~ 30
unsigned int ac_temperature = 24;

// 0 : low
// 1 : mid
// 2 : high
// if kAc_Type = 1, 3 : change
unsigned int ac_flow = 0;

const uint8_t kAc_Flow_Tower[3] = {0, 4, 6};
const uint8_t kAc_Flow_Wall[4] = {0, 2, 4, 5};

uint32_t ac_code_to_sent;

void Ac_Send_Code(uint32_t code) {
  Serial.print("code to send : ");
  Serial.print(code, BIN);
  Serial.print(" : ");
  Serial.println(code, HEX);
  irsend.sendLG2(code, 28);
}

void Ac_Activate(unsigned int temperature, unsigned int air_flow,
                 unsigned int heat) {
  ac_heat = heat;
  unsigned int ac_msbits1 = 8;
  unsigned int ac_msbits2 = 8;
  unsigned int ac_msbits3 = 0;
  unsigned int ac_msbits4;
  if (ac_heat == 1)
    ac_msbits4 = 4;  // heating
  else
    ac_msbits4 = 0;  // cooling
  unsigned int ac_msbits5 =  (temperature < 15) ? 0 : temperature - 15;
  unsigned int ac_msbits6 = 0;

  if (air_flow <= 2) {
    if (kAc_Type == 0)
      ac_msbits6 = kAc_Flow_Tower[air_flow];
    else
      ac_msbits6 = kAc_Flow_Wall[air_flow];
  }

  // calculating using other values
  unsigned int ac_msbits7 = (ac_msbits3 + ac_msbits4 + ac_msbits5 +
                             ac_msbits6) & B00001111;
  ac_code_to_sent = ac_msbits1 << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits2) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits3) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits4) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits5) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits6) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits7);

  Ac_Send_Code(ac_code_to_sent);

  ac_power_on = 1;
  ac_temperature = temperature;
  ac_flow = air_flow;
}

void Ac_Change_Air_Swing(int air_swing) {
  if (kAc_Type == 0) {
    if (air_swing == 1)
      ac_code_to_sent = 0x881316B;
    else
      ac_code_to_sent = 0x881317C;
  } else {
    if (air_swing == 1)
      ac_code_to_sent = 0x8813149;
    else
      ac_code_to_sent = 0x881315A;
  }
  Ac_Send_Code(ac_code_to_sent);
}

void Ac_Power_Down() {
  ac_code_to_sent = 0x88C0051;

  Ac_Send_Code(ac_code_to_sent);

  ac_power_on = 0;
}

void Ac_Air_Clean(int air_clean) {
  if (air_clean == '1')
    ac_code_to_sent = 0x88C000C;
  else
    ac_code_to_sent = 0x88C0084;

  Ac_Send_Code(ac_code_to_sent);

  ac_air_clean_state = air_clean;
}

  BLYNK_WRITE(V6) {  // Power button
  // Serial.println("# a : mode or temp    b : air_flow, temp, swing, clean,"
  //                " cooling/heating");
  // Serial.println("# 0 : off             0");
  // Serial.println("# 1 : on              0");
  // Serial.println("# 2 : air_swing       0 or 1");
  // Serial.println("# 3 : air_clean       0 or 1");
  // Serial.println("# 4 : air_flow        0 ~ 2 : flow");
  // Serial.println("# + : temp + 1");
  // Serial.println("# - : temp - 1");

  Serial.println("a=");  // Prompt User for input
  a = param.asInt();  // Read user input into a
 switch (a) {
    case 0:  // off
      Ac_Power_Down();
      // irsend.sendRaw(LGon, celeng, 38);  // Send a raw data capture at 38kHz.
      break;
    case 1:  // on
      Ac_Activate(ac_temperature, ac_flow, ac_heat);
        // irsend.sendRaw(LGoff, celeng, 38);  // Send a raw data capture at 38kHz.
      break;
    case 3:
      if (b == 0)
        Ac_Change_Air_Swing(0);
      else
        Ac_Change_Air_Swing(1);
      break;
    case 4:  // 1  : clean on, power on
      if (b == 0 || b == 1)
        Ac_Air_Clean(b);
      break;
    case 5:
      switch (b) {
        case 1:
          Ac_Activate(ac_temperature, 1, ac_heat);
          break;
        case 2:
          Ac_Activate(ac_temperature, 2, ac_heat);
          break;
        case 3:
          Ac_Activate(ac_temperature, 3, ac_heat);
          break;
        default:
          Ac_Activate(ac_temperature, 0, ac_heat);
      }
      break;
  }
  }
BLYNK_WRITE(V7) {                     // Power button
  Serial.println("ac_temperature=");  // Prompt User for input
  ac_temperature = param.asInt();     // Read user input into a
  }

BLYNK_WRITE(V8) {             // +
  Serial.println("++++++ =");  // Prompt User for input
      if (param.asInt() == 0 && 18 <= ac_temperature && ac_temperature <= 29)
        Ac_Activate((ac_temperature + 1), ac_flow, ac_heat);
  }

BLYNK_WRITE(V9) {              // -
  Serial.println("------ =");  // Prompt User for input
      if (param.asInt() == 0 && 19 <= ac_temperature && ac_temperature <= 30)
        Ac_Activate((ac_temperature - 1), ac_flow, ac_heat);
  }



// This function sends Arduino's up time every second to Virtual Pin (5).
void sendSensor()
{
  h = dht.readHumidity();
  t = dht.readTemperature();  
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V4, t);
  Serial.print("Current humidity = ");
  Serial.print(h);
  Serial.println("%  ");
  Serial.print("temperature = ");
  Serial.print(t); 
  Serial.println("C  ");
}

void ACdata()
{
  Blynk.virtualWrite(V10, ac_temperature);
  Blynk.virtualWrite(V6, ac_power_on);
  Serial.print("Temperature = ");
  Serial.println(ac_temperature);
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  irsend.begin();
  dht.begin();
  BlynkEdgent.begin();
  timer.setInterval(60000L, sendSensor);
  timer.setInterval(1000L, ACdata);
}

void loop() {
  BlynkEdgent.run();
  timer.run();        // Initiates BlynkTimer
}

