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
//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND           866E6
#define LORA_SYNC_WORD 0x34

// Frame and chunking config
const int MAX_FRAME = 1024;
const int CHUNK_SIZE = 128;
const int MAX_CHUNKS = (MAX_FRAME + CHUNK_SIZE - 1) / CHUNK_SIZE; // = 8
const int HEADER_SIZE = 2;

byte frameBuffer[MAX_FRAME];
bool receivedChunks[MAX_CHUNKS];
int chunkRSSI[MAX_CHUNKS];
int totalChunks = 0;
int receivedCount = 0;
int frameLength = 0;


void resetFrame() {
  memset(frameBuffer, 0, sizeof(frameBuffer));
  memset(receivedChunks, 0, sizeof(receivedChunks));
  memset(chunkRSSI, 0, sizeof(chunkRSSI));
  totalChunks = 0;
  receivedCount = 0;
  frameLength = 0;
}

void setup() {
  Serial.begin(115200);

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  LoRa.setSyncWord(LORA_SYNC_WORD);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (true);
  }

  Serial.println("LoRa Receiver Ready");
  resetFrame();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize == 0) return;
  if (packetSize < HEADER_SIZE) return;

  int chunkIndex = LoRa.read();
  totalChunks = LoRa.read();

  if (chunkIndex >= MAX_CHUNKS || totalChunks > MAX_CHUNKS) {
    Serial.println("Too many chunks – skipping");
    resetFrame();
    return;
  }

  int startPos = chunkIndex * CHUNK_SIZE;
  int len = packetSize - HEADER_SIZE;

  if (startPos + len > MAX_FRAME) {
    Serial.println("Frame too large – skipping");
    resetFrame();
    return;
  }

 for (int i = 0; i < len && LoRa.available(); i++) {
    frameBuffer[startPos + i] = LoRa.read();
  }

  if (!receivedChunks[chunkIndex]) {
    receivedChunks[chunkIndex] = true;
    chunkRSSI[chunkIndex] = LoRa.packetRssi();
    receivedCount++;
    frameLength += len;
  }

  if (receivedCount == totalChunks) {
    Serial.println("---- Complete P1 Frame ----");
    for (int i = 0; i < frameLength; i++) {
      if (frameBuffer[i] < 0x10) Serial.print("0");
      Serial.print(frameBuffer[i], HEX);
    }
    Serial.println();

    Serial.println("---- RSSI per Chunk ----");
    for (int i = 0; i < totalChunks; i++) {
      Serial.print("Chunk ");
      Serial.print(i);
      Serial.print(": RSSI = ");
      Serial.println(chunkRSSI[i]);
    }

    Serial.print("Frame length: ");
    Serial.println(frameLength);
    Serial.println("-------------------------");

    resetFrame();
  }
}