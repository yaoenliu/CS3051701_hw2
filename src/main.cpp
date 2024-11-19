#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vector.h"
#include "AdminId.h"

// pin definitions
#define LED_RED A4
#define LED_GREEN A2
#define RST_PIN 9
#define SS_PIN 10

// can hadle up to 100 authorized ID
#define miximumAuthorizedID 100

// task functions
void checkAuthorization();
void serialSection();

// read modules
String sRead(); // serial read
String rRead(); // rfid read

// authorized ID storage
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
  // set digital pins as ground
  pinMode(A5, OUTPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A5, LOW);
  digitalWrite(A3, LOW);
  // initialize rfid
  SPI.begin();
  rfid.PCD_Init();
  // initialize authorized ID vector
  authorizedIDVector.setStorage(authorizedID);
  // add the admin ID to the authorized ID list
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
    Serial.println("[Place your card to authorize]");
    String input = rRead();
    bool authorized = false;
    for (uint16_t i = 0; i < authorizedIDVector.size(); i++)
    {
      if (input == authorizedIDVector[i]) // check if the ID is in the authorized ID list
      {
        authorized = true;
        break;
      }
    }
    if (authorized) // if authorized, break the loop
    {
      break;
    }
    else // if not authorized, blink the red led and retry
    {
      Serial.println("[Access denied]\n");
      digitalWrite(LED_RED, HIGH);
      delay(1000);
      digitalWrite(LED_RED, LOW);
    }
  }
  // if authorized, blink the green led and enter the admin mode
  Serial.println("[Access granted]\n");
  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  serialSection();
}

void serialSection()
{
  Serial.println("[Enter admin mode]");
  while (1)
  {
    Serial.println("[Commands: ADD, LIST, REMOVE[index]]");
    String input = sRead(); // read command from serial

    if (input == "ADD")
    {
      if (authorizedIDVector.full()) // check if the authorized ID list is full
      {
        Serial.println("[The authorized ID list is full]");
        break;
      }
      Serial.println("[Place your card to add]");
      input = rRead();
      bool exists = false;
      for (uint16_t i = 0; i < authorizedIDVector.size(); i++) // check if the ID already exists
      {
        if (input == authorizedID[i]) // if exists, break the loop
        {
          Serial.print("[ID: ");
          Serial.print(input);
          Serial.println(" already exists]");
          exists = true;
          break;
        }
      }
      if (!exists) // if not exists, add the ID to the authorized ID list
      {
        authorizedIDVector.push_back(input);
        Serial.print("[ID: ");
        Serial.print(input);
        Serial.println(" added]");
      }
      Serial.println("");
      break;
    }

    if (input == "LIST")
    {
      // draw the authorized ID list
      Serial.println("   +----+---------------+");
      Serial.println("   | No | Authorized ID |");

      for (uint16_t i = 0; i < authorizedIDVector.size(); i++)
      {
        Serial.println("   +----+---------------+");
        Serial.print("   | ");
        if (i + 1 < 10)
          Serial.print(" "); // fix the alignment
        Serial.print(i + 1); // ID index
        Serial.print(" | ");
        for (int j = 0; j < 3; j++)
          Serial.print(" ");
        Serial.print(authorizedID[i]); // ID
        for (int j = 0; j < 3; j++)
          Serial.print(" ");
        Serial.println("|");
      }
      Serial.println("   +----+---------------+");
      Serial.println("");
      break;
    }

    if (input.startsWith("REMOVE[") && input.endsWith("]"))
    {
      if (authorizedIDVector.size() == 1) // check if the ID list has only the admin ID
      {
        Serial.println("[Cannot remove the only ID]");
        break;
      }
      // read the index from the input format
      int startIdx = input.indexOf('[') + 1;
      int endIdx = input.indexOf(']');
      String indexStr = input.substring(startIdx, endIdx);

      // check if the index is a number
      bool validIndex = true;
      for (uint16_t i = 0; i < indexStr.length(); i++)
      {
        if (!isDigit(indexStr[i]))
        {
          Serial.print("[Invalid index, input: 1-");
          Serial.print(authorizedIDVector.size());
          Serial.println(" only]");
          validIndex = false;
          break;
        }
      }

      uint16_t index = indexStr.toInt();
      if (index - 1 < authorizedIDVector.size() && index > 0 && validIndex) // check if the index is valid
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
        Serial.print("[Invalid index, input: 1-");
        Serial.print(authorizedIDVector.size());
        Serial.println(" only]");
      }
      Serial.println("");
      break;
    }
    // invalid command
    Serial.println("[Invalid command]");
  }
}

String sRead()
{
  String input = "";
  // drop all serial input before reading
  while (Serial.available())
  {
    Serial.read();
  }
  // wait for input
  while (!Serial.available())
  {
  }
  input = Serial.readStringUntil('\n');
  input.trim();
  return input;
}

String rRead()
{
  String input = "";
  // wait for card
  while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
  }
  for (uint8_t i = 0; i < rfid.uid.size; i++)
  {
    input += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();
  input.toUpperCase();
  Serial.println("[RFID read: " + input + "]");
  return input;
}