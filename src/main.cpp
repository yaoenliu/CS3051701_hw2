#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "Vector.h"

#define adminID "testID"

#define LED_RED 2
#define LED_GREEN 3

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

void setup()
{
  // initialize serial
  Serial.begin(115200);

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
    }
  }
  Serial.println("Access granted");
  vTaskResume(serialSectionHandler);
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
  // read rfid (implement later)
  return input;
}