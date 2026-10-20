# RFNETHM auf Waveshare ESP32-P4-Module-DEV-KIT

Der P4-Port nutzt den RPI-RF-MOD über den 40-Pin-Header und das onboard IP101-Ethernet am RJ45. Das bestehende S3-Ziel bleibt unverändert.

## Firmware bauen und flashen (Windows)

Benötigt wird ESP-IDF 5.5.5 mit installierten Tools. Im ESP-IDF-PowerShell-Terminal im Repository:

```powershell
cd firmware
.\build_p4.ps1 build
.\build_p4.ps1 flash -Port COM7
.\build_p4.ps1 monitor -Port COM7
```

`COM7` ist der beim Portieren erkannte USB-Seriell-Port und kann sich nach erneutem Anstecken ändern. Das Skript setzt als Ziel ESP32-P4 und verwendet die Firmware-Defaults für den erkannten Chip Revision 1.3. Ethernet bezieht seine Adresse per DHCP; danach sind WebUI und RFNetHM-Dienste unter dieser IP erreichbar.

## Header-Belegung

| RPI-RF-MOD 40-Pin | Waveshare P4 | Funktion |
| --- | --- | --- |
| 2 oder 4 | 2 oder 4 | 5 V Versorgung |
| 6 | 6 | GND |
| 8 | GPIO37 | P4 UART TX → Modul RX |
| 10 | per Drahtbrücke zu P4-Pin 15 | Modul TX → P4 UART RX |
| 15 | GPIO6 | UART-RX-Eingang des P4; Modul-Pin 15 bleibt isoliert |
| 35 | GPIO53 | Reset, aktiv HIGH |
| 36 | GPIO46 | Modul-LED rot (Ausgang, 1 = an) |
| 38 | GPIO27 | Modul-LED grün (Ausgang, 1 = an) |
| 40 | GPIO45 | Modul-LED blau (Ausgang, 1 = an); am DEV-KIT zugleich SD-Karten-Power-Enable (Q1), ohne SD-Karte folgenlos |
| 32 | GPIO54 | Modul-Taster (BTN) — nicht angesteuert; am DEV-KIT ist GPIO54 der ESP32-C6-Enable |

Die Firmware konfiguriert UART1 mit 115200 Baud. Die Belegung ist für den Header des DEV-KIT bestimmt.

## RGB-LED des RPI-RF-MOD

Die LED auf dem Funkmodul hängt an den Header-Pins 36/38/40 und wird vom
Host getrieben (wie bei piVCCU/RaspberryMatic über `rpi_rf_mod_led.ko`,
Pegel 1 = an). Seit v0.17.0 bedient RFNETHM sie:

| Zustand | LED | Quelle |
| --- | --- | --- |
| Boot, kein HB-RF-ETH-Client | aus | lokal |
| Client verbunden, noch kein StartConn | gelb (rot+grün) | lokal |
| Client verbunden und StartConn (Frames fließen) | grün | lokal |
| Keepalive-Timeout, kein Client mehr | rot | lokal |
| CCU schreibt `/sys/class/leds/rpi_rf_mod:*` | exakt wie bestellt | HB-RF-ETH Typ 3 |

Hat die CCU einmal einen Zustand gesetzt, überschreiben lokale Ereignisse
ihn nicht mehr (außer „alle Clients weg“). OpenCCU (`hss_led`) schreibt die
drei LEDs laufend neu, etwa zwei Typ-3-Pakete pro Sekunde; im Normalbetrieb
bestellt es „aus“, so wie die LED der CCU3 im Betrieb dunkel bleibt. Die
lokalen Farben sind deshalb nur bis zum ersten CCU-Kommando zu sehen.

Hardware-Test: `POST /api/led` mit `{"r":true,"g":false,"b":false,"hold_ms":5000}`
oder ein Klick auf einen der drei Punkte in der HB-RF-ETH-Kachel der WebUI.
Die Firmware hält die Testfarbe `hold_ms` lang (Default 5 s, max 30 s),
puffert solange die CCU-Kommandos und wendet danach das letzte an.
`/api/status` zeigt unter `led` Zustand, Quelle (`boot`/`local`/`host`/`api`),
`hold` und die Zahl der CCU-Kommandos (`host_cmds`).

## Direkt aufstecken: zwei Pins anpassen, eine Drahtbrücke löten

Ein Zwischenadapter ist nicht nötig. Am **RPI-RF-MOD** nur diese zwei Pins
vom Kontakt zum P4-Header weg umbiegen:

1. **Modul-Pin 10 (UART-TX):** vom P4-Pin 10 fernhalten und mit einer kurzen
   isolierten Drahtbrücke an **P4-Pin 15 (GPIO6 / UART-RX)** anlöten.
2. **Modul-Pin 15:** ebenfalls so umbiegen, dass er **P4-Pin 15** nicht
   berührt. Dadurch bleibt GPIO6 ausschließlich mit Modul-Pin 10 verbunden.

Danach kann das RPI-RF-MOD direkt auf den 40-Pin-Header des P4-Boards gesteckt
werden. **Alle übrigen Pins bleiben unverändert und werden direkt aufgesteckt.**
P4-Pin 8 / GPIO37 bleibt UART-TX zum Modul; Reset bleibt P4-Pin 35 / GPIO53.
Beim Umbiegen und Löten Board und Funkmodul vollständig von USB und Netzteil
trennen.

![Pinzählung und direkte Zwei-Pin-Anpassung zwischen RPI-RF-MOD und ESP32-P4](esp32p4-header-adapter.svg)
