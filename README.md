# Morse Code Translator - ESP32 Bluetooth Bridge

The project uses four ESP32-WROOM-32 boards: two tester/controllers and two Bluetooth bridges. Each tester encodes outgoing text as Morse timing bits and decodes received bit frames. Each bridge provides a UART-to-Bluetooth Classic SPP link.

## System layout

```text
System A                                      System B
[Tester ESP32] <--UART--> [Bridge ESP32] ~~Bluetooth SPP~~ [Bridge ESP32] <--UART--> [Tester ESP32]
                 9600 baud (slave)                          (master) 9600 baud
```

The bridge firmware is identical on both systems. Configure one as the Bluetooth slave and the other as master using AT commands.

## Files

| File | Purpose |
|---|---|
| `src/bt_tester.cpp` | Tester firmware (`env:bt_tester`) |
| `src/morse_codec.cpp`, `include/morse_codec.h` | Morse text/OOK timing-bit encoder and decoder |
| `src/bt_bridge.cpp` | Bluetooth SPP to UART bridge (`env:bt_bridge`) |
| `platformio.ini` | PlatformIO environments |

## Morse data path

Enter a message in the tester's USB serial monitor and submit the line. Supported characters are A-Z and 0-9. The tester applies the timing-bit rules from the supplied `morse.c`: dot = `1`, dash = `111`, intra-letter gap = `0`, letter gap = `000`, and word gap = `0000000`.

The tester sends an ASCII frame in the form `M:<bits>`. The bridges forward the line over Bluetooth SPP and UART. The receiving tester recognizes the `M:` frame and decodes the bit stream back into text. This transports digital timing data; it does not generate a GPIO pulse, sound, or light waveform.

Use `AT`, `AT+NAME=...`, `AT+ROLE=...`, `AT+INQ`, and `AT+LINK=...` from the tester serial monitor to configure its attached bridge. Ordinary text is encoded and sent as a Morse frame.

## Wiring

Connect UART2 with crossed TX/RX lines and common ground (3.3 V logic):

| Bridge ESP32 | Tester ESP32 |
|---|---|
| GPIO17 (TX2) | GPIO16 (RX2) |
| GPIO16 (RX2) | GPIO17 (TX2) |
| GND | GND |

UART is 9600 baud; USB serial monitoring is 115200 baud.

## Build and upload

```sh
pio run -e bt_bridge -t upload     # upload to both bridge boards
pio run -e bt_tester -t upload     # upload to both tester boards
pio device monitor -b 115200
```

## Two-system Bluetooth setup

On System A's tester, configure its bridge as the slave:

```text
AT+NAME=HC05-A
AT+ROLE=0
```

On System B's tester, configure its bridge as master, scan, then connect to the slave:

```text
AT+ROLE=1
AT+INQ
AT+LINK=0
```

After `+CONNECTED`, type text on either tester to send it. Bridge commands must be submitted as uppercase `AT` or `AT+...` lines; other text is encoded as Morse.

## Limitations

- A bridge cannot be master and slave simultaneously; the master initiates the connection.
- Inquiry blocks the bridge for about 8 seconds.
- Transport uses newline-delimited ASCII frames and the current bridge firmware's line buffer is unbounded.
- The full four-board Morse path still needs verification on the physical boards.
