#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vector.h"
#include "AdminId.h"

#define LED_RED A4
#define LED_GREEN A2
#define RST_PIN 9
#define SS_PIN 10

#define miximumAuthorizedID 100

// task functions
void checkAuthorization();
void serialSection();

// read modules
String sRead(); // serial read
String rRead(); // rfid read

// authorized ID
String authorizedID[miximumAuthorizedID];
Vector<String> authorizedIDVector;

// MFRC522 instance
MFRC522 rfid(SS_PIN, RST_PIN);

void setup()
{
  // initialize serial
  Serial.begin(115200);
  // initialize led pin
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A5, LOW);
  digitalWrite(A3, LOW);
  // initialize rfid
  SPI.begin();
  rfid.PCD_Init();
  // initialize authorized ID vector
  authorizedIDVector.setStorage(authorizedID);
  authorizedIDVector.push_back(adminID);
  // print system ready
  Serial.println("[System ready]");
}

void loop()
{
  checkAuthorization();
}

void checkAuthorization()
{
  while (1)
  {
    Serial.println("[Place your card]");
    String input = rRead();
    bool authorized = false;
    for (uint16_t i = 0; i < authorizedIDVector.size(); i++)
    {
      if (input == authorizedIDVector[i])
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
      Serial.println("[Access denied]");
      digitalWrite(LED_RED, HIGH);
      delay(1000);
      digitalWrite(LED_RED, LOW);
    }
  }
  Serial.println("[Access granted]");
  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  serialSection();
}

void serialSection()
{
  Serial.println("[Enter admin mode]");
  Serial.println("[Commands: ADD, LIST, REMOVE[index]]");
  while (1)
  {
    String input = sRead();
    if (input == "ADD")
    {
      if (authorizedIDVector.full())
      {
        Serial.println("[The authorized ID list is full]");
        break;
      }
      Serial.println("[Place your card]");
      input = rRead();
      bool exists = false;
      for (uint16_t i = 0; i < authorizedIDVector.size(); i++)
      {
        if (input == authorizedID[i])
        {
          Serial.print("[ID: ");
          Serial.print(input);
          Serial.println(" already exists]");
          exists = true;
          break;
        }
      }
      if (!exists)
      {
        authorizedIDVector.push_back(input);
        Serial.print("[ID: ");
        Serial.print(input);
        Serial.println(" added]");
      }

      break;
    }

    if (input == "LIST")
    {
      for (uint16_t i = 0; i < authorizedIDVector.size(); i++)
      {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(authorizedID[i]);
      }
      break;
    }

    if (input.startsWith("REMOVE[") && input.endsWith("]"))
    {
      if (authorizedIDVector.size() == 1)
      {
        Serial.println("[Cannot remove the only ID]");
        break;
      }
      // remove the ID
      int startIdx = input.indexOf('[') + 1;
      int endIdx = input.indexOf(']');
      String indexStr = input.substring(startIdx, endIdx);
      uint16_t index = indexStr.toInt();
      if (index - 1 < authorizedIDVector.size() && index > 0)
      {
        Serial.print("[ID: ");
        Serial.print(authorizedID[index - 1]);
        Serial.print(" at index: ");
        Serial.print(index);
        Serial.println(" removed]");
        authorizedIDVector.remove(index - 1);
      }
      else
      {
        Serial.print("[Invalid index, input: 1 - ");
        Serial.print(authorizedIDVector.size());
        Serial.println(" only]");
      }
      break;
    }
  }
}

String sRead()
{
  // drop all serial input before reading
  while (Serial.available())
  {
    Serial.read();
  }
  // wait for input
  while (!Serial.available())
  {
  }
  String input = "";
  input = Serial.readStringUntil('\n');
  input.trim();
  return input;
}

String rRead()
{
  String input = "";
  while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
  }
  for (uint8_t i = 0; i < rfid.uid.size; i++)
  {
    input += String(rfid.uid.uidByte[i], HEX);
  }
  input.toUpperCase();
  Serial.println("RFID read: " + input);
  rfid.PICC_HaltA();
  return input;
}