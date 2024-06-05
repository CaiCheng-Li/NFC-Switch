#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <SoftwareSerial.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
String tagId = "None";
byte nuidPICC[4];

SoftwareSerial mySerial(2, 3); // RX, TX (use different pins)

void setup(void) 
{
    Serial.begin(115200);  // Initialize serial communication with the PC
    mySerial.begin(9600);  // Initialize software serial communication with the Uno
    Serial.println("System initialized");
    nfc.begin();
}

void loop() 
{
    readNFC();
}

void readNFC() 
{
    if (nfc.tagPresent())
    {
        NfcTag tag = nfc.read();
        Serial.println("Card Found!");

        // Extract the NDEF message if present
        if (tag.hasNdefMessage()) 
        {
            NdefMessage message = tag.getNdefMessage();
            for (int i = 0; i < message.getRecordCount(); i++)
            {
                NdefRecord record = message.getRecord(i);
                int payloadLength = record.getPayloadLength();
                byte payload[payloadLength];
                record.getPayload(payload);
                
                // Convert payload to string
                String payloadString = "";
                for (int j = 3; j < payloadLength; j++) 
                {
                    payloadString += (char)payload[j];
                }
                
                // Print the payload string
                Serial.print("Payload: ");
                Serial.println(payloadString);
                
                // Send the payload string to the Uno
                mySerial.print(payloadString);
                mySerial.print("\n");  // Optional: send a newline character to indicate the end of the message
            }
        }

        tagId = tag.getUidString();
        
        // Delay for 2 seconds to allow the user to remove the card
        delay(2000);
    }
    delay(1000);
}
