# RFNETHM — HomeMatic-Funk über Ethernet mit ESP32-P4

<p align="center">
  <img src="logos/Lab-Setup.jpeg" alt="RFNETHM Lab-Aufbau der ursprünglichen ESP32-S3-Plattform" width="420">
</p>

Dieser Fork ergänzt RFNETHM um ein Firmware-Ziel für das **Waveshare
ESP32-P4-Module-DEV-KIT**. Dieses verbindet ein **RPI-RF-MOD** über den
40-Pin-Header mit dem lokalen Netzwerk über den eingebauten Ethernet-Port
(RJ45/IP101). So wird das Funkmodul für OpenCCU/RaspberryMatic, piVCCU,
FHEM oder Homegear über LAN erreichbar, ohne einen Raspberry Pi am
Funkstandort zu benötigen.

> [!IMPORTANT]
> **RPI-RF-MOD direkt auf das ESP32-P4-Board stecken – ohne Zwischenadapter.**
> Dafür nur **zwei Pins am RPI-RF-MOD umbiegen** und eine kurze Drahtbrücke
> anlöten: Modul-Pin **10 (UART-TX)** vom Kontakt zu P4-Pin 10 wegführen und
> mit **P4-Pin 15 (GPIO6 / UART-RX)** verbinden; Modul-Pin **15** so umbiegen,
> dass er P4-Pin 15 nicht berührt. Die übrigen Header-Pins können direkt
> aufgesteckt werden. Eine kleine, schnell erledigte Anpassung – Pinzählung
> und Skizze findest Du in der
> [P4-Anschlussdokumentation](docs/esp32p4_port.md).

Der 40-Pin-Header des P4-Boards hat eine Raspberry-Pi-ähnliche mechanische
Anordnung, **die GPIO-Belegung ist aber nicht 1:1 kompatibel**. Die zwei
genannten Pins müssen vor dem Aufstecken angepasst werden. Board und Modul
beim Biegen und Löten vollständig von der Stromversorgung trennen.

Das ursprüngliche **ESP32-S3-Ziel** bleibt im Repository enthalten. Es nutzt
andere Hardware und unterstützt unter anderem den HmIP-RFUSB-Stick. Die
Anleitungen und vorgebackenen Images dafür sind ausdrücklich als S3-Ziel
gekennzeichnet. Der P4-Port wird separat mit ESP-IDF gebaut und geflasht.

---

## Datenfluss

```
   RPI-RF-MOD           RFNETHM                dein Server
   (40-Pin-Header)      (ESP32-P4 + RJ45)      (OpenCCU, RaspberryMatic,
       UART ──────────► P4-UART                  FHEM, Homegear)
                        │
                        └─── Ethernet ────────►  hb_rf_eth.ko
                                                 /dev/raw-uart
                                                 oder TCP-Socket
```

Der ESP32-P4-Port in diesem Fork ist für das **RPI-RF-MOD** dokumentiert.
Der ursprüngliche ESP32-S3-Port kann zusätzlich einen HmIP-RFUSB-Stick über
USB verwenden; das ist ein separates Hardware-Ziel.

---

## Wozu das gut ist

- **Funkstandort frei wählbar.** Stick steht da, wo der Empfang gut ist
  (Estrich, Garage, Pegel-Sweet-Spot); die Hausautomation läuft woanders.
- **Drop-in für alle gängigen Stacks.** Aus Sicht des Servers ist es
  ein normales `hb_rf_eth`-Gerät (Alex Reinerts Kernel-Modul) **oder**
  eine HM-MOD-RPI-PCB-Bridge (TCP/2330). Beide Wege gleichzeitig nutzbar.
- **Kein zusätzlicher Pi nötig.** Wer bisher einen Pi nur als Funkträger
  nutzt, kann ihn ersetzen.
- **Kein Custom-RF-Stack.** Der Funk-Teil bleibt der echte eq-3-Stick mit
  eq-3-Firmware — alle BidCoS-/HmIP-Eigenheiten, AES, Pegelhandling
  sind genauso wie im Original.

---

## Netzwerk-Schnittstellen (parallel offen)

| Port | Format | Was redet damit |
|---|---|---|
| **UDP 3008** | HB-RF-ETH wire-format | `hb_rf_eth.ko` → `/dev/raw-uart` für piVCCU / RaspberryMatic |
| **TCP 2330** | HMUARTLGW | FHEM `CUL_HM`, Homegear, jeder Stack der eine HM-MOD-RPI-PCB-Bridge erwartet |
| **TCP 2329** | Raw-Bytestream | bmcond, eigene Tools, Reverse-Engineering |
| **HTTP 80** | WebUI + REST | Status, OTA-Update, Live-Log; WLAN-Setup auf dem S3-Ziel |
| **mDNS** | `_raw-uart._udp:3008` | Auto-Discovery (`rfnethm-XXXX.local` + Alias `rfnethm.local`) |

Eingehende Funk-Frames werden gleichzeitig an alle verbundenen Clients
gespiegelt.  Senden ist mit einem **TX-Master-Soft-Lock** gegen
Mehrfach-Schreiber abgesichert (erster Sender bekommt den Stick für
5 s, danach übernimmt der nächste; per WebUI festpinnbar).

---

## Was Du brauchst (ESP32-P4-Ziel dieses Forks)

- **Waveshare ESP32-P4-Module-DEV-KIT** mit RJ45-Buchse und IP101-Ethernet.
  Die P4-Firmware bezieht die IP-Adresse per DHCP über Ethernet; eine
  WLAN-Konfiguration ist für dieses Ziel nicht erforderlich.
- **RPI-RF-MOD** auf dem 40-Pin-Header. Kein Zwischenadapter nötig: Modul-Pin
  10 und 15 werden wie oben beschrieben umgebogen; eine Drahtbrücke führt
  Modul-Pin 10 (TX) zu P4-Pin 15 (GPIO6 / RX). Alle anderen Pins bleiben
  direkt aufgesteckt. Die [P4-Anschlussdokumentation](docs/esp32p4_port.md)
  zeigt Pinzählung und Verdrahtung.
- **5-V-Versorgung** für das DEV-KIT über die Hohlbuchse gemäß
  Waveshare-Anleitung. Das RPI-RF-MOD benötigt zusätzlich 5 V auf Header-Pin
  2 oder 4. Für den normalen
  Betrieb Ethernet und die Hohlbuchsenversorgung verwenden; USB nach dem
  Flashen abziehen und nicht als parallele Stromversorgung verwenden.
- **LAN-Kabel** zum Router oder Switch mit DHCP.
- Zum Bauen und Flashen unter Windows: **ESP-IDF 5.5.5** und das mitgelieferte
  Skript `firmware/build_p4.ps1`. Die genaue Folge steht unter
  [P4 bauen und flashen](#p4-bauen-und-flashen).

Das ursprüngliche **ESP32-S3-Devkit-Ziel** mit HmIP-RFUSB-Stick ist weiterhin
vorhanden, aber nicht das oben beschriebene P4-Setup. Seine Hardware- und
Flash-Anleitungen stehen weiter unten im Abschnitt
[ESP32-S3-Ziel (Bestandsplattform)](#esp32-s3-ziel-bestandsplattform).

---

## P4 bauen und flashen

Die P4-Firmware wird unter Windows mit ESP-IDF **5.5.5** gebaut. Installiere
ESP-IDF einschließlich der zugehörigen Tools und öffne das
ESP-IDF-PowerShell-Terminal. Im Repository ausführen:

```powershell
cd firmware
.\build_p4.ps1 build
.\build_p4.ps1 flash -Port COM7
.\build_p4.ps1 monitor -Port COM7
```

`COM7` durch den Port des Boards ersetzen. Für den normalen Betrieb die
5-V-Versorgung über die Hohlbuchse wie in der Waveshare-Anleitung verwenden;
das RPI-RF-MOD benötigt 5 V auf Header-Pin 2 oder 4. Das Board per RJ45 an
ein Netzwerk mit DHCP anschließen. Die vom Router vergebene IP-Adresse
öffnet das WebUI. Die genaue Header-Belegung und die einfache
Zwei-Pin-Anpassung stehen in [docs/esp32p4_port.md](docs/esp32p4_port.md).

Danach die Server-Seite konfigurieren:
   - piVCCU / RaspberryMatic: `hb_rf_eth.ko` mit der IP des RFNETHM laden
     (Schritt-Screenshot weiter unten unter
     [RaspberryMatic / OpenCCU einbinden](#raspberrymatic--openccu-einbinden-p4-ziel)).
   - FHEM: `define hmusb HMUARTLGW rfnethm-XXXX.local:2330`.

---

## ESP32-S3-Ziel (Bestandsplattform)

Die folgenden Hinweise gelten ausschließlich für das ursprüngliche
ESP32-S3-Devkit-Ziel. Dieses Ziel verwendet den HmIP-RFUSB-Stick über USB
und WLAN; sein Webflasher und seine Firmware-Images sind nicht für das
Waveshare ESP32-P4-DEV-KIT bestimmt.

Das S3-Ziel lässt sich über den **Browser-Webflasher** unter
[`https://install.busware.de/rfnethm/`](https://install.busware.de/rfnethm/)
flashen und danach per OTA aktualisieren. WLAN wird auf dem S3-Ziel über
Improv-Serial oder den Captive-AP-Fallback eingerichtet.

### Vorgebackene S3-Binaries (Flash via esptool)

Für CLI-Nutzer, die keinen Web-Serial-fähigen Browser haben oder den
Flash skripten wollen, liegen unter [`webflasher/`](webflasher/) im
Repo dieselben Images, die auch der Webflasher serviert:

- **`webflasher/factory_rfnethm_esp32s3.bin`** — Komplett-Image
  (bootloader + partition-table + ota_data + Applikation).  Erst-Flash
  für ein jungfräuliches oder anders belegtes ESP32-S3.
- **`webflasher/firmware_rfnethm_esp32s3.bin`** — nur die Applikation
  (offset `0x10000`).  Für OTA-Updates über die WebUI, oder als
  gezielter Re-Flash auf bereits provisioniertes Gerät.

Image-Inhalt ist deckungsgleich mit dem aktuellen Release-Tag und der
Version, die der Webflasher unter
[`https://install.busware.de/rfnethm/`](https://install.busware.de/rfnethm/)
ausliefert (siehe `webflasher/manifest.json`).

```sh
# 1) Erst-Flash via USB (ESP32-S3 im Download-Mode — BOOT-Button halten,
#    RESET kurz, BOOT loslassen).  Port-Pfad gegebenenfalls anpassen.
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
    write_flash 0x0 webflasher/factory_rfnethm_esp32s3.bin

# 2) OTA-Update via WebUI (empfohlen sobald das Gerät im Netz ist —
#    kein Druck auf BOOT-Button nötig, keine USB-Verbindung):
curl -X POST --data-binary @webflasher/firmware_rfnethm_esp32s3.bin \
    http://rfnethm-XXXX.local/api/ota

# 3) Reiner Re-Flash der Anwendung über USB (ohne Partition-Tabelle
#    anzufassen — z.B. wenn nur ein neuer Build aufgespielt werden soll):
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
    write_flash 0x10000 webflasher/firmware_rfnethm_esp32s3.bin
```

`esptool.py` liegt bei PlatformIO bei (`~/.platformio/penv/bin/esptool.py`)
oder kommt out-of-the-box aus `pip install esptool`.  Auf macOS / Windows
ist der Port-Pfad entsprechend `/dev/cu.usbmodem*` bzw. `COMx`.

---

## WebUI

Die eingebaute Web-Oberfläche ist im P4-Aufbau nach dem Ethernet-Start unter
`http://rfnethm.local/` erreichbar (kurzer Alias) bzw. unter dem eindeutigen
`http://rfnethm-XXXX.local/` (XXXX = letzte 4 Hex-Stellen der MAC, stabiler
Pfad). Alternativ die vom DHCP-Server vergebene IP-Adresse verwenden. Sie zeigt
Sources, Sinks und System-Status als Kachel-Dashboard — inklusive
Reset-Reason, Heap- und Stack-HWM-Indikator für den Dauerlauf-Betrieb,
TX-Master-Soft-Lock pro Sink, Live-Log und OTA-Update. Die HB-RF-ETH-Kachel
zeigt zusätzlich den Zustand der RPI-RF-MOD-LED (rot/grün/blau) und wer ihn
gesetzt hat; ein Klick auf einen der drei Punkte schaltet die Farbe für 5 s
zum Test um (`POST /api/led`), danach übernimmt wieder die CCU. WLAN-
Provisioning gehört zum ESP32-S3-Ziel.

> Falls mehrere RFNETHM-Sticks am gleichen LAN hängen: `rfnethm.local`
> wird von beiden gleichzeitig beworben — welcher tatsächlich antwortet,
> hängt vom mDNS-Race ab.  Wer eindeutig ansprechen will, nimmt
> `rfnethm-XXXX.local`.
Light- und Dark-Theme sind per Toggle in der Headbar umschaltbar, die
Wahl wird pro Browser persistiert; ohne explizite Wahl folgt das
Theme der OS-Präferenz (`prefers-color-scheme`).

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="logos/webUIDark.png">
    <img alt="RFNETHM WebUI (Light/Dark — folgt der OS-Präferenz beim Anzeigen auf GitHub)" src="logos/webUI.png" width="460">
  </picture>
</p>

---

## RaspberryMatic / OpenCCU einbinden (P4-Ziel)

In RaspberryMatic bzw. OpenCCU unter **System-Optionen → Erweiterte
Einstellungen** trägst Du die IP-Adresse (oder den mDNS-Namen
`rfnethm-XXXX.local`) des RFNETHM in das Feld
**„IP-Adresse (HB-RF-ETH)"** ein und bestätigst mit *Änderungen
speichern*. Danach lädt RaspberryMatic den `hb_rf_eth.ko`-Pfad neu und das
am ESP32-P4 angeschlossene RPI-RF-MOD erscheint als lokales `/dev/raw-uart`.
Für das alte S3-Ziel gelten die entsprechenden USB- und Funkmodul-Hinweise
aus dessen Bestandsplattform-Dokumentation.

<p align="center">
  <img src="logos/CCU-Config.png" alt="OpenCCU / RaspberryMatic — IP-Adresse (HB-RF-ETH) eintragen" width="640">
</p>

---

## Status — was funktioniert

**Verifiziert (live gegen reale Hardware):**

- ESP32-P4-Ziel auf dem Waveshare DEV-KIT: Ethernet-Link über RJ45,
  DHCP-Adresse und WebUI erreichbar; OpenCCU erkennt das angeschlossene
  RPI-RF-MOD über HB-RF-ETH
- USB-Host-Init und Mode-Switch BL → App des HmIP-RFUSB-Sticks
  (ESP32-S3-Ziel)
- Beide HM-Modul-Familien am Pin-Header (HM-MOD-RPI-PCB / Co_CPU
  und RPI-RF-MOD / DualCoPro), Auto-Erkennung beim Boot
  (ESP32-S3-Ziel)
- HMUARTLGW-Codec byte-genau gegen Live-Captures
- `hb_rf_eth.ko`-Pfad bis zur Aktor-Toggle-Schaltung in RaspberryMatic
- FHEM `CUL_HM` / HMUARTLGW-Bridge bis Phase D (TX, RX, AES-Key-Storage)
- Coprocessor-Firmware-Flash von beiden Modul-Familien über bmcond
  via UDP-Transport (RPI-RF-MOD 4.4.22 ⇆ 4.2.14, HM-MOD 2.8.6 ⇆ 2.2.1)
- Multi-Client-Fanout, TX-Master-Lock, mDNS und OTA (beide Ziele);
  Captive-AP-Fallback (S3-Ziel)
- HB-RF-ETH-Reconnect des `hb_rf_eth.ko` (neuer Source-Port, gleiche
  Endpoint-ID) übernimmt seit v0.16.0 die laufende Session inklusive
  StartConn-Zustand — vorher blieb der RX-Pfad Richtung CCU nach jedem
  Kernel-Reconnect still, bis die CCU neu gestartet wurde
- RGB-LED des RPI-RF-MOD (Header 36/38/40) seit v0.17.0: gelb bei
  Verbindung, grün sobald Frames fließen, rot nach Keepalive-Timeout, und
  die CCU steuert sie über HB-RF-ETH Typ 3 (`/sys/class/leds/rpi_rf_mod:*`)
  wie beim Original-HB-RF-ETH; Test per `POST /api/led`. Einschränkung:
  OpenCCU bis 3.89.11 auf Home Assistant OS schickt nur „aus“, weil dem
  HAOS-Kernel die LED-Trigger `timer` und `default-on` fehlen, die
  `hss_led` 2.3 braucht. Die LED bleibt dort auch bei Alarm- und
  Servicemeldungen dunkel, bis ein OpenCCU-Release mit dem neuen `hss_led`
  erscheint (Details in
  [docs/esp32p4_port.md](docs/esp32p4_port.md#rgb-led-des-rpi-rf-mod)).
  Seit v0.19.0 setzt RFNETHM den letzten CCU-Zustand selbst wieder, wenn
  `hb_rf_eth.ko` die Session nach einem Keepalive-Timeout oder einem
  Software-Neustart von RFNETHM (Reboot, OTA-Update) wieder aufnimmt;
  nach einem Kaltstart nicht. Nach einem Stromausfall meldet OpenCCU
  „WatchDog: hb-rf-eth-connection“ — erwartet, wie beim Original-HB-RF-ETH
  (siehe
  [docs/esp32p4_port.md](docs/esp32p4_port.md#stromausfall-und-watchdog-alarm-in-openccu))
- Modul-Reset der CCU (HB-RF-ETH Typ 4) seit v0.18.0 wie beim
  Original-HB-RF-ETH als reiner Reset-Puls; die Verbindung zum Modul
  bleibt offen und OpenCCU bringt das Modul selbst in die App. Vorher
  lief ein eigener Bootloader→App-Ablauf, während dessen RFNETHM rund
  3,5 s lang Befehle der CCU verwarf

**Nicht fertig:**

- AES-Authentifizierung in der Legacy-Bridge (Phase E — mehrtägig,
  kein Blocker für AES-aware Devices).
- Eigenes PCB (Schaltplan und Pin-Mapping stehen, liegen aber nicht im
  Repo).

---

## Was RFNETHM **nicht** ist

- **Kein** Drop-in-Replacement für den HmIP-RFUSB am unmodifizierten
  RaspberryMatic-Kernel-Treiber: der USB-Pfad ist via ECDSA gesperrt
  (bewusste Design-Entscheidung von Alex Reinert, wird respektiert).
  Variante B des Projekts (CP2102N mit OTP-Brand `1B1F:C020`) ist eine
  separate Bauform und ohne ESP32.
- **Kein** eigenes Wireformat.  UDP/3008 mit HB-RF-ETH-Frames und
  TCP/2330 mit HMUARTLGW sind die Default-Sprachen.
- **Keine** BidCoS-/HmIP-Protokoll-Interpretation in der Stick-Firmware
  — reiner Transport, alle HM-Logik bleibt downstream.

---

## Ist das was für mich?

- **Ja**, wenn Du HmIP-Funk in einer Linux-Hausautomation (FHEM,
  Homegear, piVCCU, RaspberryMatic) nutzt und Funk-Standort vom
  Server-Standort entkoppeln willst.
- **Ja**, wenn Du einen alten Pi loswerden willst der nur den
  HmIP-Stick / das RPI-RF-MOD trägt.
- **Nein**, wenn Du eine vollständige CCU-Funktionalität ohne
  Server-Stack willst — dafür ist RaspberryMatic die richtige Adresse,
  RFNETHM ist nur der Funk-Adapter.
- **Nein**, wenn Du Klassik-Homematic-Funk (BidCoS, AskSinPP) ohne
  HmIP brauchst — da ist ein CUL/COC der bessere Match.

---

## Mehr lesen

- [`docs/esp32p4_port.md`](docs/esp32p4_port.md) — P4-Board, RJ45,
  Header-Belegung und direkte Zwei-Pin-Anpassung
- [`docs/intro.md`](docs/intro.md) — ursprüngliches ESP32-S3-Ziel
- [`docs/breadboard_wiring.md`](docs/breadboard_wiring.md) — Verkabelung
  des S3-Devkits
- [`docs/ethernet_addition.md`](docs/ethernet_addition.md) — ursprüngliche
  S3/W5500-Erweiterungsplanung (nicht erforderlich für das P4-Board)
- [`docs/diagrams/`](docs/diagrams/) — Architektur- und
  Message-Flow-Diagramme

---

## Lizenz

GPL-2.0-or-later (analog PIIF / CULFW32).  Listener-Code ist eigene
Reimplementierung gegen die HB-RF-ETH-Spec — kein Copy aus dem
CC-BY-NC-SA-4.0-lizenzierten Original-Repo.

---

<p align="center">
  <img src="logos/busmatic.png" alt="busmatic" width="160">
</p>

<p align="center"><sub>RFNETHM ist ein Lab-Projekt aus dem Tostmann-RF-Lab.</sub></p>
