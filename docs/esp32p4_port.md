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

Die Firmware konfiguriert UART1 mit 115200 Baud. Die Belegung ist für den Header des DEV-KIT bestimmt.

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
