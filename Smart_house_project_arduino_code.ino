#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <SPI.h>
#include <MFRC522.h>

#include <Servo.h>

#define BUTTON_PIN 3
#define BUZZER 4
#define SERVO_PIN 5
#define RAIN_PIN 6
#define LED_R 8
#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);

Servo servo;

LiquidCrystal_I2C lcd(0x27, 16, 2);

char rfid_card[] = "90 E4 84 26";
char rfid_tag[] = "90 E4 84 26";

char Incoming_value;

int Analog_Temp_Sensor_Pin = A0;
int Analog_Gas_Sensor_Pin = A1;
int Analog_Flame_Sensor_Pin = A2;

int Rain_Sensor_Reading;
int Gas_Sensor_Reading;
int Flame_Sensor_Reading;
int Temp_Sensor_Reading;
int Button_Reading;

int GasDetection;
int FlameDetection;
int TempDetection;
int ButtonDetection;
int GateDetection;

int Notification_point_Gas = 0;
int Notification_point_Flame = 0;
int Notification_point_Button = 0;

int Notification_point_Gas_Threshold = 9;
int Notification_point_Flame_Threshold = 9;
int Notification_point_Button_Threshold = 9;


int AvgTemp;
int AvgTempCnt=0;
int TemperatureC = 0;
int Temperature_Threshold = 21;
int TempSampling = 5;

int Rain_count = 0;
int Rain_threeshold=5;

int Window_State = 0;
String ThresholdString;

int servo_pos=0;

void setup() 
{
  delay(2000);
  pinMode(RAIN_PIN, INPUT);

  Serial.begin(9600);
  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  pinMode(LED_R, OUTPUT);

  
  lcd.begin();
	lcd.backlight();

  servo.attach(SERVO_PIN);
  servo.write(0);
  delay(5000);
}

void loop()
{

  if (mfrc522.PICC_IsNewCardPresent()) 
  {

    if (mfrc522.PICC_ReadCardSerial()) 
    {
      String content= "";
      byte letter;
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      content.toUpperCase();
  
      if (content.substring(1) == rfid_card) //change here the UID of the card/cards that you want to give access
      {
        GateDetection = 1;
        digitalWrite(LED_R, HIGH);
        tone(BUZZER, 450);
        delay(1000);
        noTone(BUZZER);
        digitalWrite(LED_R, LOW);
      }
    }
    
  }
  else 
  {
    if (!mfrc522.PICC_IsNewCardPresent())
    
    if(Serial.available()) //Window_State = 0-Off,1-Closed,2-Sensors,3-Open 
    {
      Incoming_value = Serial.read();

      if(Incoming_value == '0')
      {
        Window_State = 0;
      }
      else if(Incoming_value == '1')
      {
        Window_State = 1;
      }
      else if(Incoming_value == '2')
      {
        Window_State = 2;
      }
      else if(Incoming_value == '3')
      {
        Window_State = 3;
      }
      else
      {
        ThresholdString = Serial.readString();
        Temperature_Threshold = ThresholdString.toInt();
      }
    }
  }


  Gas_Sensor_Reading = analogRead(Analog_Gas_Sensor_Pin);
  Flame_Sensor_Reading = analogRead(Analog_Flame_Sensor_Pin);
  Temp_Sensor_Reading = ((analogRead(Analog_Temp_Sensor_Pin) * 5.0) / 1024.0)  * 100;

  Rain_Sensor_Reading = digitalRead(RAIN_PIN);
  Button_Reading = digitalRead(BUTTON_PIN);
  
  AvgTemp += Temp_Sensor_Reading;
  AvgTempCnt++;

  if(AvgTempCnt == TempSampling)
  {
    AvgTempCnt = 0;
    TemperatureC = AvgTemp/TempSampling;
    AvgTemp = 0;
  }

  if(Flame_Sensor_Reading<800)
  {
    FlameDetection = 1;
    Notification_point_Flame_Threshold++;
  }
  else
    FlameDetection = 0;

  if(Gas_Sensor_Reading>100)
  {
    GasDetection = 1;
    Notification_point_Gas_Threshold++;
  }
  else
    GasDetection = 0;

  if(Button_Reading == 1)
  {
    ButtonDetection = 1;
    Notification_point_Button_Threshold++;
  }
  else
    ButtonDetection = 0;


  if(Window_State != 0)
  {
    if(Rain_Sensor_Reading == 1 && Rain_count <= Rain_threeshold)
    Rain_count++;
    else if(Rain_count >= 1)
    Rain_count--;

    if((Rain_count >= Rain_threeshold && servo_pos >= 1 && Window_State == 2) || (servo_pos >= 1 && Window_State == 2 && TemperatureC <= Temperature_Threshold))
    {
      servo_pos = servo_pos - 10;
      servo.write(servo_pos);
    }

    if(servo_pos <= 59 && Window_State == 3)
    {
      servo_pos=servo_pos + 10;
      servo.write(servo_pos);
    }

    if(Rain_count == 0 && servo_pos <= 59 && Window_State == 2 && TemperatureC >= Temperature_Threshold)
    {
      servo_pos=servo_pos + 10;
      servo.write(servo_pos);
    }

    if(Window_State == 1 && servo_pos >= 1)
    {
      servo_pos = servo_pos - 10;
      servo.write(servo_pos);
    }
  }

  lcd.clear();

  lcd.setCursor(9,0);
  lcd.print("Temp:");
  lcd.setCursor(14,0);
  lcd.print(TemperatureC);

  lcd.setCursor(0,0);
  lcd.print("CO:");
  lcd.setCursor(3,0);
  if(GasDetection == 1)
  lcd.print("Det");
  else
  lcd.print("Und");

  lcd.setCursor(0,1);
  lcd.print("Flame:");
  lcd.setCursor(6,1);
  if(FlameDetection == 1)
  lcd.print("Det");
  else
  lcd.print("Und");

  lcd.setCursor(10,1);
  lcd.print("Win:");
  lcd.setCursor(14,1);
  if(Window_State == 0)
  lcd.print("Of");
  else if(Window_State == 1)
  lcd.print("Cl");
  else if(Window_State == 2)
  lcd.print("Se");
  else if(Window_State == 3)
  lcd.print("Op");

  if(Notification_point_Gas_Threshold==10)
    Notification_point_Gas = 1;

  if(Notification_point_Flame_Threshold==10)
    Notification_point_Flame = 1;

  if(Notification_point_Button_Threshold==10)
    Notification_point_Button = 1;
  
  Serial.print(FlameDetection);
  Serial.print("#");
  Serial.print(GasDetection);
  Serial.print("#");
  Serial.print(GateDetection);
  Serial.print("#");
  Serial.print(ButtonDetection);
  Serial.print("#");
  Serial.print(Window_State);
  Serial.print("#");
  Serial.print(TemperatureC);
  Serial.print("#");
  Serial.print(Temperature_Threshold);
  Serial.print("#");
  Serial.print(Notification_point_Gas);
  Serial.print("#");
  Serial.print(Notification_point_Flame);
  Serial.print("#");
  Serial.println(Notification_point_Button);

  delay(1000);

  GateDetection = 0;

  if(Notification_point_Gas_Threshold==10)
  {
  Notification_point_Gas_Threshold=0;
  Notification_point_Gas=0; 
  }

  if(Notification_point_Flame_Threshold==10)
  {
  Notification_point_Flame_Threshold=0;
  Notification_point_Flame=0;
  }

  if(Notification_point_Button_Threshold==10)
  {
  Notification_point_Button_Threshold=0;
  Notification_point_Button=0;
  }

} 



