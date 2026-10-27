// SPDX-License-Identifier: GPL-2.0-or-later
//
// hm_led.h — RGB-Status-LED des RPI-RF-MOD (eQ3-Header-Pins 36/38/40).
//
// Die LED sitzt auf dem Funkmodul, getrieben wird sie aber vom Host:
// piVCCU/RaspberryMatic legen die drei Leitungen als GPIO-Outputs an
// (rpi_rf_mod_led.ko, Pegel 1 = LED an).  Über HB-RF-ETH kommt der
// gewünschte Zustand als Typ-3-Paket mit einem Byte (Bit0 rot, Bit1 grün,
// Bit2 blau) vom hb_rf_eth.ko, sobald die CCU /sys/class/leds/rpi_rf_mod:*
// beschreibt.  Dieses Modul ist der reine Pin-Treiber plus Zustand; die
// Protokoll-Logik liegt in sink_hbrfeth.c, Test/Anzeige in webui.c.
//
// OpenCCU (hss_led) zeigt wie die CCU3 im Normalbetrieb blau, bei Service-
// bzw. Alarmmeldungen gelb/blau bzw. rot/blau im Wechsel.  hss_led 2.3 (bis
// OpenCCU 3.89.11) braucht dafür die Kernel-Trigger default-on und timer;
// fehlen sie wie unter Home Assistant OS, setzt es die LEDs alle paar
// Sekunden wieder auf aus (beobachtet: Bündel von 4–8 Typ-3-Paketen, immer
// aus).  Neuere hss_led blinken im Userspace und schicken beim Blinken
// laufend Typ-3-Pakete.  Ein Hand-Test wäre damit nach spätestens wenigen
// Sekunden wieder überschrieben — deshalb gibt es den Test-Hold:
// hm_led_test() hält die Farbe hold_ms lang, Host-Kommandos werden solange
// nur gemerkt und danach der letzte Stand angewendet.  Details:
// docs/esp32p4_port.md, Abschnitt RGB-LED.
//
// Pins per Compile-Define (siehe src/CMakeLists.txt):
//   RFNETHM_LED_RED_GPIO / _GREEN_GPIO / _BLUE_GPIO   (-1 = nicht beschaltet)
//   P4 (Waveshare DEV-KIT, Modul direkt auf dem Header): 46 / 27 / 45
//   S3 (Breadboard-Wiring, docs/breadboard_wiring.md):   4 / 5 / 6

#ifndef RFNETHM_HM_LED_H
#define RFNETHM_HM_LED_H

#include <stdbool.h>
#include <stdint.h>

#define HM_LED_HOLD_DEFAULT_MS  5000
#define HM_LED_HOLD_MAX_MS     30000

typedef enum {
    HM_LED_SRC_BOOT = 0,   // seit Boot nicht gesetzt (alles aus)
    HM_LED_SRC_LOCAL,      // Sink-Ereignis: Connect / StartConn / Disconnect / Timeout
    HM_LED_SRC_HOST,       // HB-RF-ETH Typ 3 — die CCU hat es so bestellt
    HM_LED_SRC_API,        // WebUI bzw. POST /api/led (Test)
} hm_led_src_t;

typedef struct {
    bool         enabled;         // Pins konfiguriert
    bool         red, green, blue;
    hm_led_src_t src;             // wer den aktuellen Zustand gesetzt hat
    bool         hold;            // Test-Hold aktiv: Host-Kommandos werden gepuffert
    uint32_t     host_cmds;       // empfangene Typ-3-Pakete seit Boot
    bool         host_known;      // ein CCU-Zustand ist bekannt (host_bits gültig)
    uint8_t      host_bits;       // letzter CCU-Zustand: Bit0 rot, Bit1 grün, Bit2 blau
    bool         restored;        // host_bits stammt von vor einem Software-Reset
    uint32_t     changes;         // tatsächliche Zustandswechsel seit Boot
    int64_t      last_change_us;  // esp_timer-Zeitstempel des letzten Wechsels
    int          gpio_red, gpio_green, gpio_blue;
} hm_led_state_t;

// GPIOs als Output konfigurieren, alle LEDs aus.  Früh in app_main rufen,
// bevor Sinks starten.  Ohne konfigurierte Pins (alle -1) ist alles Noop.
void hm_led_init(void);

// Zustand setzen.  Thread-sicher, blockiert nicht (Spinlock + GPIO-Write).
void hm_led_set(bool red, bool green, bool blue, hm_led_src_t src);

// HB-RF-ETH-Byte: Bit0 rot, Bit1 grün, Bit2 blau.  Zählt host_cmds hoch,
// wenn src == HM_LED_SRC_HOST; während eines Test-Holds wird ein Host-
// Kommando nur gemerkt und nach Ablauf angewendet.
void hm_led_set_bits(uint8_t bits, hm_led_src_t src);

// Letzten bekannten CCU-Zustand erneut anwenden (src HOST), wenn
// hb_rf_eth.ko eine Session wieder aufnimmt.  Der Kernel schickt Typ 3 nur
// bei Änderungen; nach unserem Neustart oder einem Keepalive-Timeout käme
// sonst bis zur nächsten Änderung nur die lokale Farbe.  Der Zustand
// überlebt Software-Resets (OTA, Reboot, Panic, Watchdog) im RTC-Speicher,
// nach einem Kaltstart ist keiner bekannt.  Während eines Test-Holds wird
// er nur vorgemerkt.  false = kein CCU-Zustand bekannt.
bool hm_led_resume_host(void);

// Neue Kernel-Session (V1 oder V2 mit Endpoint 0, z.B. nach dem Neuladen
// von hb_rf_eth.ko): der gemerkte CCU-Zustand gehört zur alten Session und
// wird verworfen, auch im RTC-Speicher.  Eine noch angezeigte CCU-Farbe
// gilt danach als lokal, damit die lokale Anzeige neu entscheidet.
void hm_led_forget_host(void);

// Hardware-Test: Farbe setzen (src API) und hold_ms lang gegen Host-
// Kommandos halten (0 = kein Hold, max HM_LED_HOLD_MAX_MS).
void hm_led_test(bool red, bool green, bool blue, uint32_t hold_ms);

void        hm_led_get_state(hm_led_state_t *out);
const char *hm_led_src_str(hm_led_src_t src);

#endif // RFNETHM_HM_LED_H
