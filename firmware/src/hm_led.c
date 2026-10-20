// SPDX-License-Identifier: GPL-2.0-or-later
//
// hm_led.c — Pin-Treiber für die RGB-LED des RPI-RF-MOD.  Siehe hm_led.h.

#include "hm_led.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include <string.h>

static const char *TAG = "hm-led";

#ifndef RFNETHM_LED_RED_GPIO
#define RFNETHM_LED_RED_GPIO   4    // S3-Breadboard → eQ3-Pin 36
#endif
#ifndef RFNETHM_LED_GREEN_GPIO
#define RFNETHM_LED_GREEN_GPIO 5    // S3-Breadboard → eQ3-Pin 38
#endif
#ifndef RFNETHM_LED_BLUE_GPIO
#define RFNETHM_LED_BLUE_GPIO  6    // S3-Breadboard → eQ3-Pin 40
#endif

// Nach den ersten Host-Kommandos nur noch DEBUG loggen: OpenCCU schreibt
// die LEDs laufend neu (~2 Pakete/s), das würde den 60-Zeilen-Log-Ring in
// Sekunden leerfegen.  Echte Zustandswechsel bleiben bei INFO.
#define HOST_LOG_VERBOSE_CMDS 10

static hm_led_state_t    S;
static portMUX_TYPE      s_lock = portMUX_INITIALIZER_UNLOCKED;
static esp_timer_handle_t s_hold_timer;
static bool              s_pending_host;   // Host-Kommando während Hold gesehen
static uint8_t           s_pending_bits;

static const char *rgb_name(bool r, bool g, bool b)
{
    if (!r && !g && !b) return "aus";
    if (r && g && b)    return "weiss";
    if (r && g)         return "gelb";
    if (r && b)         return "magenta";
    if (g && b)         return "cyan";
    if (r)              return "rot";
    if (g)              return "gruen";
    return "blau";
}

const char *hm_led_src_str(hm_led_src_t src)
{
    switch (src) {
    case HM_LED_SRC_BOOT:  return "boot";
    case HM_LED_SRC_LOCAL: return "local";
    case HM_LED_SRC_HOST:  return "host";
    case HM_LED_SRC_API:   return "api";
    }
    return "?";
}

static void drive(int gpio, bool on)
{
    if (gpio >= 0) gpio_set_level((gpio_num_t)gpio, on ? 1 : 0);
}

// Test-Hold abgelaufen (esp_timer-Task): letztes Host-Kommando nachholen.
static void hold_expired(void *arg)
{
    (void)arg;
    bool    apply;
    uint8_t bits;
    portENTER_CRITICAL(&s_lock);
    S.hold         = false;
    apply          = s_pending_host;
    bits           = s_pending_bits;
    s_pending_host = false;
    portEXIT_CRITICAL(&s_lock);
    if (apply) {
        hm_led_set(bits & 0x01, bits & 0x02, bits & 0x04, HM_LED_SRC_HOST);
    } else {
        ESP_LOGI(TAG, "Test-Hold vorbei, kein Host-Kommando ausstehend");
    }
}

void hm_led_init(void)
{
    memset(&S, 0, sizeof(S));
    S.gpio_red   = RFNETHM_LED_RED_GPIO;
    S.gpio_green = RFNETHM_LED_GREEN_GPIO;
    S.gpio_blue  = RFNETHM_LED_BLUE_GPIO;

    uint64_t mask = 0;
    if (S.gpio_red   >= 0) mask |= 1ULL << S.gpio_red;
    if (S.gpio_green >= 0) mask |= 1ULL << S.gpio_green;
    if (S.gpio_blue  >= 0) mask |= 1ULL << S.gpio_blue;
    if (!mask) {
        ESP_LOGW(TAG, "keine LED-Pins konfiguriert — Modul-LED bleibt unbedient");
        return;
    }

    gpio_config_t io = {
        .pin_bit_mask = mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t e = gpio_config(&io);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config fehlgeschlagen: %s", esp_err_to_name(e));
        return;
    }
    drive(S.gpio_red, false);
    drive(S.gpio_green, false);
    drive(S.gpio_blue, false);
    S.enabled = true;

    esp_timer_create_args_t a = {
        .callback        = hold_expired,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "hm-led-hold",
    };
    if (esp_timer_create(&a, &s_hold_timer) != ESP_OK) {
        s_hold_timer = NULL;
        ESP_LOGW(TAG, "Hold-Timer nicht verfügbar — Test ohne Hold");
    }

    ESP_LOGI(TAG, "RPI-RF-MOD-LED bereit: rot=GPIO%d gruen=GPIO%d blau=GPIO%d (aus)",
             S.gpio_red, S.gpio_green, S.gpio_blue);
}

void hm_led_set(bool red, bool green, bool blue, hm_led_src_t src)
{
    int64_t  now = esp_timer_get_time();
    bool     changed;
    uint32_t host_cmds;

    portENTER_CRITICAL(&s_lock);
    changed = (S.red != red) || (S.green != green) || (S.blue != blue);
    S.red = red; S.green = green; S.blue = blue; S.src = src;
    if (changed) {
        S.changes++;
        S.last_change_us = now;
    }
    host_cmds = S.host_cmds;
    portEXIT_CRITICAL(&s_lock);

    if (S.enabled) {
        drive(S.gpio_red,   red);
        drive(S.gpio_green, green);
        drive(S.gpio_blue,  blue);
    }

    if (!changed) return;
    if (src == HM_LED_SRC_HOST && host_cmds > HOST_LOG_VERBOSE_CMDS) {
        ESP_LOGD(TAG, "LED %s (%s)", rgb_name(red, green, blue), hm_led_src_str(src));
    } else {
        ESP_LOGI(TAG, "LED %s (%s)", rgb_name(red, green, blue), hm_led_src_str(src));
    }
}

void hm_led_set_bits(uint8_t bits, hm_led_src_t src)
{
    if (src == HM_LED_SRC_HOST) {
        bool held;
        portENTER_CRITICAL(&s_lock);
        S.host_cmds++;
        held = S.hold;
        if (held) {
            s_pending_host = true;
            s_pending_bits = bits;
        }
        portEXIT_CRITICAL(&s_lock);
        if (held) return;   // nach Ablauf des Holds nachgeholt
    }
    hm_led_set(bits & 0x01, bits & 0x02, bits & 0x04, src);
}

void hm_led_test(bool red, bool green, bool blue, uint32_t hold_ms)
{
    if (hold_ms > HM_LED_HOLD_MAX_MS) hold_ms = HM_LED_HOLD_MAX_MS;
    if (!s_hold_timer) hold_ms = 0;
    if (s_hold_timer) esp_timer_stop(s_hold_timer);   // ESP_ERR_INVALID_STATE wenn nicht aktiv — egal

    portENTER_CRITICAL(&s_lock);
    S.hold         = hold_ms > 0;
    s_pending_host = false;
    portEXIT_CRITICAL(&s_lock);

    hm_led_set(red, green, blue, HM_LED_SRC_API);
    if (hold_ms > 0) {
        esp_timer_start_once(s_hold_timer, (uint64_t)hold_ms * 1000ULL);
        ESP_LOGI(TAG, "Test-Hold %u ms — Host-Kommandos werden solange gepuffert",
                 (unsigned)hold_ms);
    }
}

void hm_led_get_state(hm_led_state_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&s_lock);
    *out = S;
    portEXIT_CRITICAL(&s_lock);
}
