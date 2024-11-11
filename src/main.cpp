#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vector.h"

#define adminID "testID"

#define LED_RED 2
#define LED_GREEN 3
#define RST_PIN 9
#define SS_PIN 10

// task handlers
TaskHandle_t checkAuthorizationHandler = NULL;
TaskHandle_t serialSectionHandler = NULL;

// task functions
void checkAuthorization(void *pvParameters);
void serialSection(void *pvParameters);

// read modules
String sRead(); // serial read
int sReadInt(); // serial read int
String rRead(); // rfid read

// authorized ID
Vector<String> authorizedID;

// MFRC522 instance
MFRC522 rfid(SS_PIN, RST_PIN);

void setup()
{
  // initialize serial
  Serial.begin(115200);
  // initialize led pin
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  // initialize rfid
  SPI.begin();
  rfid.PCD_Init();

  authorizedID.push_back(adminID);

  // create tasks
  xTaskCreate(checkAuthorization, "check", 128, NULL, 1, &checkAuthorizationHandler);
  xTaskCreate(serialSection, "serial", 128, NULL, 1, &serialSectionHandler);

  // start tasks
  vTaskSuspend(serialSectionHandler);
  vTaskStartScheduler();
}

void loop()
{
}

void checkAuthorization(void *pvParameters)
{
  (void)pvParameters;
  while (1)
  {
    String input = rRead();
    bool authorized = false;
    for (uint16_t i = 0; i < authorizedID.size(); i++)
    {
      if (input == authorizedID[i])
      {
        authorized = true;
        break;
      }
    }
    if (authorized)
    {
      break;
    }
    else
    {
      Serial.println("Access denied");
      digitalWrite(LED_RED, HIGH);
      delay(1000);
      digitalWrite(LED_RED, LOW);
    }
  }
  Serial.println("Access granted");
  vTaskResume(serialSectionHandler);
  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  vTaskSuspend(checkAuthorizationHandler);
}

void serialSection(void *pvParameters)
{
  (void)pvParameters;

  while (1)
  {
    String input = sRead();
    if (input == "ADD")
    {
      input = rRead();
      bool exists = false;
      for (uint16_t i = 0; i < authorizedID.size(); i++)
      {
        if (input == authorizedID[i])
        {
          Serial.println("ID already exists");
          exists = true;
          break;
        }
      }
      if (!exists)
        authorizedID.push_back(input);
    }

    if (input == "LIST")
    {
      for (uint16_t i = 0; i < authorizedID.size(); i++)
      {
        Serial.print(i + ": ");
        Serial.println(authorizedID[i]);
      }
    }

    if (input == "REMOVE")
    {
      uint16_t index = sReadInt();
      if (index < authorizedID.size())
      {
        authorizedID.remove(index);
      }
      else
      {
        Serial.println("Invalid index");
      }
    }
    // stop this and start checkAuthorization
    if (false) // will change to design condition, now will always not enter
      break;
  }
  vTaskResume(checkAuthorizationHandler);
  vTaskSuspend(serialSectionHandler);
}

String sRead()
{
  String input = "";
  while (Serial.available() == 0)
  {
    vTaskDelay(10);
  }
  input = Serial.readString();
  return input;
}

int sReadInt()
{
  int input = 0;
  while (Serial.available() == 0)
  {
    vTaskDelay(10);
  }
  input = Serial.parseInt();
  return input;
}

String rRead()
{
  String input = "";
  while(!rfid.PICC_IsNewCardPresent()||!rfid.PICC_ReadCardSerial())
  {
    vTaskDelay(10);
  }
  for (uint8_t i = 0; i < rfid.uid.size; i++)
  {
    input += String(rfid.uid.uidByte[i], HEX);
  } 
  input.toUpperCase();
  rfid.PICC_HaltA();
  return input;
}