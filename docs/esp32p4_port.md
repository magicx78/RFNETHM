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
| 10 | via Zwischenadapter zu GPIO6 | Modul TX → P4 UART RX |
| 15 | GPIO6 | UART-RX-Eingang des P4 (Zwischenadapter-Verbindung) |
| 35 | GPIO53 | Reset, aktiv HIGH |

Die Firmware konfiguriert UART1 mit 115200 Baud. Die Belegung ist für den Header des DEV-KIT bestimmt.

## Vor dem Aufstecken beachten

Der Waveshare-Schaltplan verbindet GPIO37/38 direkt mit dem CH343P-USB-UART. Der Zwischenadapter führt deshalb nur den Modul-TX-Kontakt von Header-Pin 10 auf GPIO6 an Header-Pin 15. Die direkte Verbindung von Modul-Pin 10 zu P4-Pin 10 bleibt offen; Modul-Pin 15 bleibt auf der Oberseite ebenfalls getrennt. Die übrigen Header-Kontakte werden durchverbunden. GPIO37 bleibt UART-TX auf Pin 8; Reset bleibt GPIO53 auf Pin 35. Damit läuft der Radio-RX auf GPIO6 und die CH343-TX-Leitung ist vom Funkmodul getrennt. Der Adapter ist nötig, bevor das Funkmodul aufgesteckt wird.

![Draufsicht und Adapterverdrahtung der beiden 40-Pin-Header](esp32p4-header-adapter.svg)
