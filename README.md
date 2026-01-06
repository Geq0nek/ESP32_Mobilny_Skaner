# ESP32 Wi‑Fi & BLE Scanner

**Krótko:** ESP32 firmware do skanowania sieci Wi‑Fi i urządzeń BLE, budowania JSON z wynikami i wysyłania ich na serwer HTTPS; wyświetla też podstawowe informacje na ekranie SSD1306 (jeśli istnieje) oraz pobiera konfigurację przez MQTT.

---

## Wymagania

- Windows / macOS / Linux
- Git
- Python 3.8+ (dla ESP‑IDF i PlatformIO)
- PlatformIO (zalecane) lub ESP‑IDF (framework użyty w projekcie)
- Sterowniki USB 

Plik konfiguracyjny PlatformIO: `platformio.ini` (środowisko: `adafruit_feather_esp32s3_tft`, framework: `espidf`).

---

## Szybki start (PlatformIO, VSCode)

1. Otwórz projekt w VSCode z zainstalowanym rozszerzeniem PlatformIO.
2. Skonfiguruj połączenie USB do modułu (port COM).
3. (Opcjonalnie) Edytuj poświadczenia Wi‑Fi i inne ustawienia w `src/main.c`:

```c
#define WIIF_SSID "Twoja_SSID"
#define WIFI_PASSWORD "Twoje_haslo"
```

4. Zbuduj projekt:

- Z konsoli PlatformIO: `pio run -e adafruit_feather_esp32s3_tft`
- Aby wgrać: `pio run -e adafruit_feather_esp32s3_tft -t upload`
- Monitor portu szeregowego: `pio device monitor -b 115200`

---

## Alternatywnie: ESP‑IDF (idF.py)

1. Zainstaluj ESP‑IDF zgodnie z oficjalną dokumentacją: https://docs.espressif.com/
2. W katalogu projektu:

```powershell
idf.py build
idf.py -p COM3 flash monitor
```

(Użyj właściwego portu zamiast `COM3`.)

> Uwaga: projekt używa `CMakeLists.txt` i jest zgodny z ESP‑IDF.

---

## Konfiguracja działania

- Wi‑Fi: ustaw `WIIF_SSID` i `WIFI_PASSWORD` w `src/main.c` lub zmodyfikuj kod do korzystania z bezpiecznych zmiennych środowiskowych.
- Endpoint HTTPS: URL serwera znajduje się w `src/main.c` w zmiennej `url`.
- Certyfikat SSL: `src/cert.pem` jest dołączany do obrazu (patrz `platformio.ini` — `board_build.embed_txtfiles`).
- MQTT: domyślny broker to `mqtt://broker.emqx.io:1883`; klient subskrybuje temat `esp32/config` i oczekuje JSON-a typu:

```json
{
  "frequency": 60,
  "time_interval": 10,
  "scan_interval": 5
}
```

Parametry te są zapisywane w zmiennych `frequency`, `scan_interval`, `scan_window`.

---

## Co robi urządzenie

- Cyklicznie skanuje Wi‑Fi i BLE, buduje JSON z wynikami
- Wysyła dane na endpoint HTTPS (jeżeli dostępna sieć)
- Odbiera konfigurację przez MQTT
- Obsługuje odbiór danych GPS przez UART oraz wyświetla podstawowe liczby na wyświetlaczu SSD1306

---

## Troubleshooting

- Brak połączenia Wi‑Fi: sprawdź SSID/hasło i siłę sygnału.
- Brak konfiguracji MQTT: sprawdź ustawienia brokera i czy urządzenie ma połączenie z MQTT.
- Problemy z certyfikatem HTTPS: upewnij się, że serwer używa certyfikatu zgodnego z `cert.pem` albo zaktualizuj certyfikat.

---

## Uwagi dla deweloperów

- Ścieżki i wartości konfiguracyjne są w `platformio.ini` oraz w `src/main.c`.
- Zmienne globalne i struktury komunikują się przez kolejki i semafory FreeRTOS.

