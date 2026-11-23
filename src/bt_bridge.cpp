// HC-05 emulator on a classic ESP32: Bluetooth SPP <-> UART2, controlled with AT+ commands.
// Wiring (3.3V, cross-over): bridge GPIO17(TX2) -> main RX, bridge GPIO16(RX2) <- main TX, GND-GND
//
// Commands (end with \r\n). Anything else is treated as data and sent over BT when connected.
//   AT              -> OK
//   AT+NAME?        -> +NAME:<name>
//   AT+NAME=<n>     -> set name (restarts BT)
//   AT+ROLE?        -> +ROLE:0|1      (0 = slave/discoverable, 1 = master)
//   AT+ROLE=0|1     -> switch role (restarts BT)
//   AT+INQ          -> master only: scan, prints +INQ:<idx>,<mac>,<rssi>,<name>
//   AT+LINK=<idx|mac|name> -> master only: connect to a device
//   AT+DISC         -> disconnect
//   AT+STATE?       -> +STATE:CONNECTED|DISCONNECTED
// Async events: +CONNECTED / +DISCONNECTED
#include <Arduino.h>
#include <vector>
#include "BluetoothSerial.h"

#define BRIDGE_RX 16
#define BRIDGE_TX 17
#define UART_BAUD 9600

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
BluetoothSerial SerialBT;
#pragma GCC diagnostic pop
String btName = "HC05-A";
bool isMaster = false;
bool wasConnected = false;
std::vector<BTAddress> found;

void startBT() {
  SerialBT.end();
  delay(200);
  SerialBT.begin(btName, isMaster);
  found.clear();
}

void reply(const String &s) { Serial2.print(s + "\r\n"); Serial.println("-> " + s); }

void doInquiry() {
  if (!isMaster) { reply("ERROR:NOT_MASTER"); return; }
  found.clear();
  BTScanResults *res = SerialBT.discover(8000);  // blocking, 8 s
  if (!res) { reply("ERROR:SCAN"); return; }
  for (int i = 0; i < res->getCount(); i++) {
    BTAdvertisedDevice *d = res->getDevice(i);
    found.push_back(d->getAddress());
    String name = d->haveName() ? String(d->getName().c_str()) : String("?");
    reply("+INQ:" + String(i) + "," + String(d->getAddress().toString().c_str()) + "," +
          String(d->haveRSSI() ? d->getRSSI() : 0) + "," + name);
  }
  reply("OK");
}

void doLink(const String &arg) {
  if (!isMaster) { reply("ERROR:NOT_MASTER"); return; }
  bool ok = false;
  bool numeric = arg.length() > 0 && arg.length() < 3 && isDigit(arg[0]);
  if (numeric) {
    size_t idx = arg.toInt();
    if (idx >= found.size()) { reply("ERROR:BAD_INDEX"); return; }
    ok = SerialBT.connect(found[idx]);
  } else if (arg.indexOf(':') == 2) {
    ok = SerialBT.connect(BTAddress(arg));
  } else {
    ok = SerialBT.connect(arg);  // by name
  }
  reply(ok ? "OK" : "ERROR:LINK");
}

void handleAT(String line) {
  line.trim();
  if (line == "AT") reply("OK");
  else if (line == "AT+NAME?") reply("+NAME:" + btName + "\r\nOK");
  else if (line.startsWith("AT+NAME=")) { btName = line.substring(8); startBT(); reply("OK"); }
  else if (line == "AT+ROLE?") reply("+ROLE:" + String(isMaster ? 1 : 0) + "\r\nOK");
  else if (line.startsWith("AT+ROLE=")) { isMaster = line.substring(8).toInt() == 1; startBT(); reply("OK"); }
  else if (line == "AT+INQ") doInquiry();
  else if (line.startsWith("AT+LINK=")) doLink(line.substring(8));
  else if (line == "AT+DISC") { SerialBT.disconnect(); reply("OK"); }
  else if (line == "AT+STATE?") reply(String("+STATE:") + (SerialBT.connected() ? "CONNECTED" : "DISCONNECTED") + "\r\nOK");
  else reply("ERROR:UNKNOWN");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, BRIDGE_RX, BRIDGE_TX);
  startBT();
  Serial.println("Bridge ready: " + btName);
}

void loop() {
  static String line;
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      line.trim();
      if (line.startsWith("AT")) handleAT(line);
      else if (line.length()) { SerialBT.println(line); Serial.println("UART->BT: " + line); }
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
  while (SerialBT.available()) {
    char c = SerialBT.read();
    Serial2.write(c);
    Serial.write(c);
  }
  bool conn = SerialBT.connected();
  if (conn != wasConnected) { wasConnected = conn; reply(conn ? "+CONNECTED" : "+DISCONNECTED"); }
}
