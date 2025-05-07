#include <SPI.h>
#include <LoRa.h> // used LoRa by Sandeep Misty version 0.8.0
#include <Wire.h>

// LoRa SPI pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

// LoRa frequency and sync word
#define BAND           866E6
#define LORA_SYNC_WORD 0x34

// UART pins for P1 data
#define RX_GPIO 4
#define TX_GPIO -1

// Frame and chunking config
const int MAX_FRAME     = 1024;
const int CHUNK_SIZE    = 128;
const int SERIAL_BAUD   = 115200;
const int SEND_DELAY_MS = 10;
const unsigned long FRAME_TIMEOUT = 1000; // ms

byte buffer[MAX_FRAME];
int bufferPos = 0;
unsigned long lastByteTime = 0;

HardwareSerial P1Serial(2);

void setup() {
  // Initialize P1 serial
  P1Serial.begin(SERIAL_BAUD, SERIAL_8N1, RX_GPIO, TX_GPIO);

  // Initialize SPI and LoRa
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  LoRa.setSyncWord(LORA_SYNC_WORD);

  if (!LoRa.begin(BAND)) {
    while (true); // Halt if LoRa init fails
  }
}

void sendChunks() {
  uint16_t totalChunks = (bufferPos + CHUNK_SIZE - 1) / CHUNK_SIZE;

  for (uint16_t i = 0; i < totalChunks; i++) {
    int start = i * CHUNK_SIZE;
    int end = min(start + CHUNK_SIZE, bufferPos);
    int len = end - start;

    // Retry until beginPacket succeeds
    int attempts = 0;
    while (LoRa.beginPacket() == 0) {
      delay(1);
      if (attempts++ >= 1000) return;
    }

    LoRa.write((uint8_t)i);             // Chunk index
    LoRa.write((uint8_t)totalChunks);   // Total chunks
    LoRa.write(buffer + start, len);    // Chunk payload
    LoRa.endPacket();

    delay(SEND_DELAY_MS);
  }
}


void loop() {
  while (P1Serial.available()) {
    byte b = P1Serial.read();
    if (bufferPos < MAX_FRAME) {
      buffer[bufferPos++] = b;
    }
    lastByteTime = millis();
  }

  if (bufferPos > 0 && (millis() - lastByteTime > FRAME_TIMEOUT)) {
    sendChunks();
    bufferPos = 0;
  }
}
