// ESP32 #2: plays the Arduino. Talks to the "HC-05" over UART2 and prints results on USB serial.
// Wiring: this GPIO17(TX2) -> bridge GPIO16(RX2),  this GPIO16(RX2) <- bridge GPIO17(TX2),  GND-GND
#include <Arduino.h>

#define HC_RX 16
#define HC_TX 17
#define UART_BAUD 9600
#define AUTO_HELLO 0  // 1: send a hello every 3 s; 0: only forward what you type (use "Both NL & CR")

void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, HC_RX, HC_TX);
  delay(1000);
  Serial.println("Tester ready. Type text in monitor to send it over BT.");
  Serial2.print("AT\r\n");  // link check
}

void loop() {
  // Periodic message to the phone/PC (shows up in a Bluetooth terminal app)
  static uint32_t last = 0;
  if (AUTO_HELLO && millis() - last > 3000) {
    last = millis();
    Serial2.print("hello from tester " + String(millis() / 1000) + "s\r\n");
  }
  // Anything the HC-05 sends back (OK replies, or data typed on phone)
  while (Serial2.available()) Serial.write(Serial2.read());
  // USB monitor -> HC-05
  while (Serial.available()) Serial2.write(Serial.read());
}
