// ESP32-WROOM-32 tester/controller. Text is encoded as OOK timing bits and
// sent through the UART-connected Bluetooth bridge as an M:<bits> line.
#include <Arduino.h>
#include <string.h>
#include "morse_codec.h"

#define HC_RX 16
#define HC_TX 17
#define UART_BAUD 9600
#define INPUT_CAP 96
#define BIT_CAP 2500
#define RX_CAP 2600

static char inputLine[INPUT_CAP];
static size_t inputLength = 0;
static char rxLine[RX_CAP];
static size_t rxLength = 0;

static void sendInput() {
  inputLine[inputLength] = '\0';
  if (!inputLength) return;
  if (strcmp(inputLine, "AT") == 0 || strncmp(inputLine, "AT+", 3) == 0) {
    Serial2.println(inputLine);
    Serial.println(String("Bridge command: ") + inputLine);
  } else {
    static char bits[BIT_CAP];
    if (!morseTextToBits(inputLine, bits, sizeof(bits))) Serial.println("No supported characters (A-Z and 0-9).");
    else {
      Serial2.print("M:"); Serial2.println(bits);
      Serial.println(String("TX Morse bits: ") + bits);
    }
  }
  inputLength = 0;
}

static void receiveLine() {
  rxLine[rxLength] = '\0';
  if (strncmp(rxLine, "M:", 2) == 0) {
    static char decoded[INPUT_CAP];
    if (morseBitsToText(rxLine + 2, decoded, sizeof(decoded))) Serial.println(String("RX decoded text: ") + decoded);
    else Serial.println("Invalid or oversized Morse frame.");
  } else if (rxLength) Serial.println(String("Bridge: ") + rxLine);
  rxLength = 0;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, HC_RX, HC_TX);
  delay(1000);
  Serial.println("Morse tester ready (ESP32-WROOM-32). Enter text to send; AT commands configure the bridge.");
  Serial2.print("AT\r\n");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') sendInput();
    else if (c == '\b' || c == 127) { if (inputLength) --inputLength; }
    else if (inputLength + 1 < sizeof(inputLine)) inputLine[inputLength++] = c;
    else { inputLength = 0; Serial.println("Input too long; line discarded."); }
  }
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') receiveLine();
    else if (c != '\r' && rxLength + 1 < sizeof(rxLine)) rxLine[rxLength++] = c;
    else if (c != '\r') { rxLength = 0; Serial.println("Oversized UART line discarded."); }
  }
}
