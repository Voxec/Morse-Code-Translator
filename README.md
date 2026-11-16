# Morse Code Translator – ESP32 Bluetooth Köprüsü (HC-05 Emülasyonu)

Amaç: Arduino + HC-05 mantığıyla çalışmak. HC-05 yerine **klasik ESP32** kullanılır; ESP32 UART üzerinden komut alan bir Bluetooth modülü gibi davranır (`src/bt_bridge.cpp`). Ana kontrolcü (Arduino rolündeki ESP32) bu köprüyle UART üzerinden konuşur.

> ESP32-S3 sadece BLE destekler, SPP (Bluetooth Classic) yoktur. Köprü için **klasik ESP32 (`esp32dev`)** gerekir. Ana kontrolcü herhangi bir kart olabilir.

## Sistem yapısı

```
 Sistem A                                   Sistem B
 [Ana ESP32] <--UART--> [Köprü ESP32] ~~Bluetooth SPP~~ [Köprü ESP32] <--UART--> [Ana ESP32]
              9600 baud   (slave)                         (master)     9600 baud
```

Her sistemde 2 ESP32 vardır: ana kontrolcü ve köprü. Köprü firmware'i iki sistemde de aynıdır, rol komutla seçilir.

## Dosyalar

| Dosya | Görev |
|---|---|
| `src/bt_bridge.cpp` | Köprü firmware'i (`env:bt_bridge`) |
| `src/bt_tester.cpp` | Test kontrolcüsü; USB monitörden yazdığını köprüye iletir (`env:bt_tester`) |
| `platformio.ini` | Ortam tanımları |

## Kablolama (3.3V mantık, çapraz bağlantı)

| Köprü ESP32 | Ana ESP32 |
|---|---|
| GPIO17 (TX2) | RX pini (testerda GPIO16) |
| GPIO16 (RX2) | TX pini (testerda GPIO17) |
| GND | GND |

Pinler `BRIDGE_RX` / `BRIDGE_TX` (köprü) ve `HC_RX` / `HC_TX` (tester) ile değiştirilebilir. UART hızı 9600'dür.

## Yükleme

```
pio run -e bt_bridge -t upload     # köprü kartı
pio run -e bt_tester -t upload     # test kontrolcü kartı
pio device monitor -b 115200       # "Both NL & CR" ile
```

## AT komutları

Komutlar `\r\n` ile biter. `AT` ile başlayan her satır komut sayılır. Diğer satırlar veri olarak Bluetooth'tan gönderilir.

| Komut | Cevap / Etki |
|---|---|
| `AT` | `OK` |
| `AT+NAME?` / `AT+NAME=<ad>` | adı sorgular / değiştirir |
| `AT+ROLE?` | `+ROLE:0` veya `+ROLE:1` |
| `AT+ROLE=0` / `AT+ROLE=1` | slave (görünür, bağlantı bekler) / master (tarar, bağlanır) |
| `AT+INQ` | Sadece master. Yaklaşık 8 sn tarar, `+INQ:<idx>,<mac>,<rssi>,<ad>` satırları ve `OK` döner |
| `AT+LINK=<idx\|MAC\|ad>` | Sadece master. Cihaza bağlanır (`OK` / `ERROR:LINK`) |
| `AT+DISC` | Bağlantıyı keser |
| `AT+STATE?` | `+STATE:CONNECTED` veya `+STATE:DISCONNECTED` |

Otomatik olaylar: `+CONNECTED`, `+DISCONNECTED`.
Hatalar: `ERROR:NOT_MASTER`, `ERROR:BAD_INDEX`, `ERROR:SCAN`, `ERROR:LINK`, `ERROR:UNKNOWN`.

`AT+NAME` ve `AT+ROLE` komutları Bluetooth'u yeniden başlatır ve eski tarama listesini siler. Ayarlar kalıcı değildir, her açılışta slave ve `HC05-A` adıyla başlar.

## Kullanım senaryoları

### 1. Telefonla test
1. Köprüyü yükle ve çalıştır. Bluetooth adı `HC05-A`.
2. Telefonda eşleş ve "Serial Bluetooth Terminal" uygulamasını aç.
3. Ana ESP32'den gönderilen satırlar telefonda görünür. Telefondan yazılanlar ana ESP32'nin UART'ına gelir.

### 2. İki sistemi haberleştirme
Sistem A (slave), ana ESP32'den köprüye:
```
AT+NAME=HC05-A
AT+ROLE=0
```
Sistem B (master):
```
AT+ROLE=1
AT+INQ            -> +INQ:0,24:6F:28:AA:BB:CC,-55,HC05-A ... OK
AT+LINK=0         -> OK, ardından +CONNECTED
```
Bağlantı kurulunca iki ana ESP32 arasında çift yönlü şeffaf UART hattı oluşur. Bir tarafın yazdığı satır (`AT` ile başlamayan) diğer tarafın UART'ına gelir. Bağlantıyı kesmek için `AT+DISC` gönder.

### 3. Ana ESP32'de tarama listesini okuma
Ana ESP32 `AT+INQ` gönderir, ardından `OK` gelene kadar gelen satırları okur. `+INQ:` ile başlayanları `,` ile ayırarak (idx, mac, rssi, ad) listeye ekler. Seçilen cihaz için `AT+LINK=<idx>` gönderir.

## Bilinen sınırlamalar
- Köprü aynı anda hem master hem slave olamaz. Bağlantıyı başlatan taraf master olmalıdır.
- `AT+INQ` yaklaşık 8 sn bloklar. Bu sürede köprü diğer veriyi işlemez.
- Veri `\n` ile biten satırlar halinde taşınır. `AT` ile başlayan veri satırları komut olarak yorumlanır. Morse protokolünde bu önekten kaçınılmalı.
- Kod henüz kart üzerinde test edilmedi.
