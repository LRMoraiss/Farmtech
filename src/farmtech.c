/**
 * ============================================================================
 * FARMTECH — Sistema Autônomo de Cultivo Hidropônico em Domo Geodésico
 * ============================================================================
 * Plataforma: BitDogLab v3 (Raspberry Pi Pico W — RP2040 + CYW43)
 * Linguagem:  C com Pico SDK 2.x
 * Curso:      EmbarcaTech — Projeto Final
 *
 * Firmware monolítico: sensores, atuadores, menu OLED, LED RGB (PWM),
 * matriz WS2812B (PIO), buzzer (PWM) e servidor HTTP via WiFi nativo.
 * ============================================================================
 */

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"
#include "config.h"

/* ============================================================================
 * 1. DEFINES — Pinos, setpoints, temporização, WiFi
 * ============================================================================
 */

/* --- Pinos nativos da BitDogLab --- */
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15
#define JOYSTICK_X_PIN 26  /* ADC0 */
#define JOYSTICK_Y_PIN 27  /* ADC1 */
#define JOYSTICK_SW_PIN 22 /* Pull-up, ativo LOW */
#define BUTTON_A_PIN 5     /* Pull-up, ativo LOW */
#define BUTTON_B_PIN 6     /* Pull-up, ativo LOW */
#define LED_R_PIN 13       /* PWM catodo comum */
#define LED_G_PIN 11       /* PWM catodo comum */
#define LED_B_PIN 12       /* PWM catodo comum */
#define LED_MATRIX_PIN 7   /* PIO WS2812B 5x5 */
#define BUZZER_A_PIN 21    /* PWM passivo */
#define BUZZER_B_PIN 10    /* PWM secundário */

/* --- Sensores externos --- */
#define DHT22_PIN 4        /* 1-Wire digital */
#define LDR_PIN 29         /* ADC3 — luminosidade */
#define WATER_LEVEL_PIN 28 /* ADC2 — nível d'água */

/* --- Periféricos --- */
#define I2C_PORT i2c1
#define OLED_ADDR 0x3C
#define I2C_FREQ 400000

/* --- Setpoints de controle --- */
#define TEMP_MAX 28.0f
#define TEMP_CRITICA 35.0f
#define UMIDADE_MIN 75
#define NIVEL_AGUA_MIN 20
#define LUZ_MINIMA 30

/* --- WiFi (credenciais em include/config.h) --- */
#define WIFI_TIMEOUT_MS 10000
#define HTTP_PORT 80

/* --- Temporização --- */
#define DEBOUNCE_MS 50
#define PRESS_CURTO_MS 500
#define PRESS_LONGO_MS 2000
#define MENU_TIMEOUT_MS 10000
#define JOY_POLL_MS 200
#define JOY_THRESHOLD 800
#define JOY_CENTER 2048
#define LOOP_DELAY_MS 50
#define DHT_TIMEOUT 10000 /* iterações máx. no bit-bang */
#define SENSOR_READ_INTERVAL_MS 2000

/* --- Alertas --- */
#define MAX_ALERTAS 5

/* --- Buzzer frequências --- */
#define BUZZ_FREQ_CRIT 1000
#define BUZZ_FREQ_ATENCAO 500
#define BUZZ_FREQ_CONFIRM 800
#define BUZZ_BOOT_F1 523
#define BUZZ_BOOT_F2 659
#define BUZZ_BOOT_F3 784

/* --- LED RGB PWM --- */
#define LED_PWM_WRAP 255

/* ============================================================================
 * 2. TYPEDEFS
 * ============================================================================
 */

typedef enum {
  TELA_DASHBOARD = 0,
  TELA_MENU_PRINCIPAL,
  TELA_SENSORES,
  TELA_DADOS_LIVE,
  TELA_ATUADORES,
  TELA_ALERTAS
} TelaAtual;

typedef struct {
  uint32_t timestamp_ms;
  char msg[28];
} Alerta;

/* ============================================================================
 * 3. VARIÁVEIS GLOBAIS (volatile onde acessadas por IRQ)
 * ============================================================================
 */

/* --- Menu --- */
static TelaAtual tela_atual = TELA_DASHBOARD;
static int cursor_menu = 0;
static uint32_t ultimo_input_ms = 0;
static int scroll_alertas = 0;

/* --- Sensores --- */
static float temperatura_atual = 25.0f;
static int umidade_atual = 60;
static int nivel_agua_atual = 100;
static int luz_ambiente_atual = 100;

static bool sensor_dht22_ativo = true;
static bool sensor_ldr_ativo = true;
static bool sensor_agua_ativo = true;
static bool dht22_ok = false;

/* --- Atuadores --- */
static bool status_vent = false;
static bool status_neb = false;
static bool status_bomba = false;
static bool status_grow = false;

/* --- Controle global --- */
static volatile bool modo_automatico = true;
static bool alerta_critico = false;
static bool wifi_conectado = false;
static char ip_str[20] = "0.0.0.0";

/* --- Alertas (buffer circular) --- */
static Alerta historico_alertas[MAX_ALERTAS];
static int qtd_alertas = 0;
static int idx_alerta_write = 0;

/* --- IRQ flags — SEMPRE volatile --- */
static volatile bool irq_a_curto = false, irq_a_longo = false;
static volatile bool irq_b_curto = false, irq_b_longo = false;
static volatile bool irq_sw_curto = false;
static volatile uint32_t ts_press_a = 0, ts_press_b = 0, ts_press_sw = 0;
static volatile uint32_t ts_debounce_a = 0, ts_debounce_b = 0,
                         ts_debounce_sw = 0;

/* --- PIO WS2812 --- */
static PIO ws_pio = pio1;
static uint ws_sm = 0;

/* --- PWM slices para LED RGB e buzzer --- */
static uint slice_r, slice_g, slice_b, slice_buzz;
static uint chan_r, chan_g, chan_b, chan_buzz;

/* --- Temporização de sensores --- */
static uint32_t ultima_leitura_sensor_ms = 0;

/* --- Buzzer estado --- */
static uint32_t buzz_next_toggle_ms = 0;
static bool buzz_on_phase = false;

/* --- TCP server --- */
static struct tcp_pcb *http_pcb = NULL;

/* ============================================================================
 * 4. OLED — Framebuffer + Fonte 5x7 + Driver SSD1306
 * ============================================================================
 */

static uint8_t oled_buf[128 * 8]; /* 1024 bytes framebuffer */

/* Fonte 5x7 — ASCII 32 (' ') até 93 (']'), cobre A-Z, 0-9, pontuação */
static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, /* 32 ' ' */
    0x00, 0x00, 0x2f, 0x00, 0x00, /* 33 '!' */
    0x00, 0x07, 0x00, 0x07, 0x00, /* 34 '"' */
    0x14, 0x7f, 0x14, 0x7f, 0x14, /* 35 '#' */
    0x24, 0x2a, 0x7f, 0x2a, 0x12, /* 36 '$' */
    0x23, 0x13, 0x08, 0x64, 0x62, /* 37 '%' */
    0x36, 0x49, 0x55, 0x22, 0x50, /* 38 '&' */
    0x00, 0x05, 0x03, 0x00, 0x00, /* 39 ''' */
    0x00, 0x1c, 0x22, 0x41, 0x00, /* 40 '(' */
    0x00, 0x41, 0x22, 0x1c, 0x00, /* 41 ')' */
    0x14, 0x08, 0x3e, 0x08, 0x14, /* 42 '*' */
    0x08, 0x08, 0x3e, 0x08, 0x08, /* 43 '+' */
    0x00, 0x50, 0x30, 0x00, 0x00, /* 44 ',' */
    0x08, 0x08, 0x08, 0x08, 0x08, /* 45 '-' */
    0x00, 0x60, 0x60, 0x00, 0x00, /* 46 '.' */
    0x20, 0x10, 0x08, 0x04, 0x02, /* 47 '/' */
    0x3e, 0x51, 0x49, 0x45, 0x3e, /* 48 '0' */
    0x00, 0x42, 0x7f, 0x40, 0x00, /* 49 '1' */
    0x42, 0x61, 0x51, 0x49, 0x46, /* 50 '2' */
    0x21, 0x41, 0x45, 0x4b, 0x31, /* 51 '3' */
    0x18, 0x14, 0x12, 0x7f, 0x10, /* 52 '4' */
    0x27, 0x45, 0x45, 0x45, 0x39, /* 53 '5' */
    0x3c, 0x4a, 0x49, 0x49, 0x30, /* 54 '6' */
    0x01, 0x71, 0x09, 0x05, 0x03, /* 55 '7' */
    0x36, 0x49, 0x49, 0x49, 0x36, /* 56 '8' */
    0x06, 0x49, 0x49, 0x29, 0x1e, /* 57 '9' */
    0x00, 0x36, 0x36, 0x00, 0x00, /* 58 ':' */
    0x00, 0x56, 0x36, 0x00, 0x00, /* 59 ';' */
    0x08, 0x14, 0x22, 0x41, 0x00, /* 60 '<' */
    0x14, 0x14, 0x14, 0x14, 0x14, /* 61 '=' */
    0x00, 0x41, 0x22, 0x14, 0x08, /* 62 '>' */
    0x02, 0x01, 0x51, 0x09, 0x06, /* 63 '?' */
    0x3e, 0x41, 0x5d, 0x55, 0x1e, /* 64 '@' */
    0x7e, 0x11, 0x11, 0x11, 0x7e, /* 65 'A' */
    0x7f, 0x49, 0x49, 0x49, 0x36, /* 66 'B' */
    0x3e, 0x41, 0x41, 0x41, 0x22, /* 67 'C' */
    0x7f, 0x41, 0x41, 0x41, 0x3e, /* 68 'D' */
    0x7f, 0x49, 0x49, 0x49, 0x41, /* 69 'E' */
    0x7f, 0x09, 0x09, 0x09, 0x01, /* 70 'F' */
    0x3e, 0x41, 0x49, 0x49, 0x7a, /* 71 'G' */
    0x7f, 0x08, 0x08, 0x08, 0x7f, /* 72 'H' */
    0x00, 0x41, 0x7f, 0x41, 0x00, /* 73 'I' */
    0x20, 0x40, 0x41, 0x3f, 0x01, /* 74 'J' */
    0x7f, 0x08, 0x14, 0x22, 0x41, /* 75 'K' */
    0x7f, 0x40, 0x40, 0x40, 0x40, /* 76 'L' */
    0x7f, 0x02, 0x0c, 0x02, 0x7f, /* 77 'M' */
    0x7f, 0x04, 0x08, 0x10, 0x7f, /* 78 'N' */
    0x3e, 0x41, 0x41, 0x41, 0x3e, /* 79 'O' */
    0x7f, 0x09, 0x09, 0x09, 0x06, /* 80 'P' */
    0x3e, 0x41, 0x51, 0x21, 0x5e, /* 81 'Q' */
    0x7f, 0x09, 0x19, 0x29, 0x46, /* 82 'R' */
    0x46, 0x49, 0x49, 0x49, 0x31, /* 83 'S' */
    0x01, 0x01, 0x7f, 0x01, 0x01, /* 84 'T' */
    0x3f, 0x40, 0x40, 0x40, 0x3f, /* 85 'U' */
    0x1f, 0x20, 0x40, 0x20, 0x1f, /* 86 'V' */
    0x3f, 0x40, 0x30, 0x40, 0x3f, /* 87 'W' */
    0x63, 0x14, 0x08, 0x14, 0x63, /* 88 'X' */
    0x07, 0x08, 0x70, 0x08, 0x07, /* 89 'Y' */
    0x61, 0x51, 0x49, 0x45, 0x43, /* 90 'Z' */
    0x00, 0x7f, 0x41, 0x41, 0x00, /* 91 '[' */
    0x02, 0x04, 0x08, 0x10, 0x20, /* 92 '\' */
    0x00, 0x41, 0x41, 0x7f, 0x00, /* 93 ']' */
};

/* --- Converte para maiúsculas (fonte só suporta A-Z) --- */
static char to_upper(char c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

/* --- Envia comando I2C ao SSD1306 --- */
static void oled_cmd(uint8_t cmd) {
  uint8_t buf[2] = {0x00, cmd};
  i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

/* --- Inicializa o display OLED SSD1306 128x64 --- */
static void oled_init(void) {
  const uint8_t cmds[] = {
      0xAE,       /* display off */
      0xD5, 0x80, /* clock div */
      0xA8, 0x3F, /* multiplex 64 */
      0xD3, 0x00, /* display offset 0 */
      0x40,       /* start line 0 */
      0x8D, 0x14, /* charge pump enable */
      0x20, 0x00, /* horizontal addressing */
      0xA1,       /* segment remap */
      0xC8,       /* COM scan decrement */
      0xDA, 0x12, /* COM pins config */
      0x81, 0xCF, /* contrast */
      0xD9, 0xF1, /* precharge */
      0xDB, 0x40, /* VCOMH deselect */
      0xA4,       /* resume display from RAM */
      0xA6,       /* normal display (not inverted) */
      0xAF        /* display on */
  };
  for (int i = 0; i < (int)sizeof(cmds); i++) {
    oled_cmd(cmds[i]);
  }
}

/* --- Limpa framebuffer --- */
static void oled_clear(void) { memset(oled_buf, 0, sizeof(oled_buf)); }

/* --- Envia framebuffer ao display --- */
static void oled_flush(void) {
  oled_cmd(0x21);
  oled_cmd(0);
  oled_cmd(127); /* coluna 0-127 */
  oled_cmd(0x22);
  oled_cmd(0);
  oled_cmd(7); /* página 0-7 */
  uint8_t chunk[129];
  chunk[0] = 0x40; /* data mode */
  for (int p = 0; p < 8; p++) {
    memcpy(&chunk[1], &oled_buf[p * 128], 128);
    i2c_write_blocking(I2C_PORT, OLED_ADDR, chunk, 129, false);
  }
}

/* --- Escreve string na posição (col, page). Converte para maiúsculas. --- */
static void oled_str(int col, int page, const char *s) {
  if (page > 7 || col > 127)
    return;
  int idx = page * 128 + col;
  int limit = page * 128 + 128;
  while (*s && idx < limit) {
    char c = to_upper(*s++);
    if (c < 32 || c > 93) {
      idx += 6;
      continue;
    }
    int fi = (c - 32) * 5;
    for (int i = 0; i < 5 && idx < 1024; i++) {
      oled_buf[idx++] = font5x7[fi + i];
    }
    if (idx < 1024)
      idx++; /* espaço entre caracteres */
  }
}

/* --- Desenha linha horizontal no pixel y --- */
static void oled_hline(int y) {
  if (y > 63)
    return;
  int page = y / 8;
  int bit = y % 8;
  for (int col = 0; col < 128; col++) {
    oled_buf[page * 128 + col] |= (1 << bit);
  }
}

/* --- Seta/limpa pixel individual --- */
static void oled_pixel(int x, int y, bool on) {
  if (x < 0 || x > 127 || y < 0 || y > 63)
    return;
  int idx = (y / 8) * 128 + x;
  if (on)
    oled_buf[idx] |= (1 << (y % 8));
  else
    oled_buf[idx] &= ~(1 << (y % 8));
}

/* ============================================================================
 * 5. DHT22 — Leitura bit-bang (protocolo 1-Wire real, sem biblioteca)
 * ============================================================================
 */

static bool dht22_read(float *temp, float *hum) {
  uint8_t data[5] = {0};

  /* Sinal de start: LOW 1ms, HIGH 30µs, depois input com pull-up */
  gpio_set_dir(DHT22_PIN, GPIO_OUT);
  gpio_put(DHT22_PIN, 0);
  sleep_ms(1);
  gpio_put(DHT22_PIN, 1);
  sleep_us(30);
  gpio_set_dir(DHT22_PIN, GPIO_IN);
  gpio_pull_up(DHT22_PIN);

  /* Aguarda resposta do DHT22: LOW ~80µs */
  uint32_t cnt = 0;
  while (gpio_get(DHT22_PIN) == 1) {
    if (++cnt > DHT_TIMEOUT)
      return false;
  }
  cnt = 0;
  while (gpio_get(DHT22_PIN) == 0) {
    if (++cnt > DHT_TIMEOUT)
      return false;
  }
  /* HIGH ~80µs */
  cnt = 0;
  while (gpio_get(DHT22_PIN) == 1) {
    if (++cnt > DHT_TIMEOUT)
      return false;
  }

  /* Lê 40 bits (5 bytes) */
  for (int i = 0; i < 40; i++) {
    /* LOW ~50µs (início de cada bit) */
    cnt = 0;
    while (gpio_get(DHT22_PIN) == 0) {
      if (++cnt > DHT_TIMEOUT)
        return false;
    }
    /* HIGH: duração determina bit 0 (26µs) ou bit 1 (70µs) */
    /* Amostra após ~40µs: se ainda HIGH → bit 1 */
    uint32_t high_cnt = 0;
    while (gpio_get(DHT22_PIN) == 1) {
      high_cnt++;
      if (high_cnt > DHT_TIMEOUT)
        return false;
    }
    data[i / 8] <<= 1;
    if (high_cnt > 40) {
      data[i / 8] |= 1; /* bit 1 (HIGH longo) */
    }
  }

  /* Checksum */
  uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
  if (data[4] != checksum)
    return false;

  /* Decodifica umidade e temperatura */
  *hum = (float)((data[0] << 8) | data[1]) / 10.0f;
  *temp = (float)(((data[2] & 0x7F) << 8) | data[3]) / 10.0f;
  if (data[2] & 0x80)
    *temp = -(*temp); /* temperatura negativa */
  return true;
}

/* ============================================================================
 * 6. IRQ — Handler dos botões A, B e Joystick SW (debounce + curto/longo)
 * ============================================================================
 */

static void gpio_irq_callback(uint gpio, uint32_t events) {
  uint32_t agora = to_ms_since_boot(get_absolute_time());

  if (gpio == BUTTON_A_PIN) {
    if ((events & GPIO_IRQ_EDGE_FALL) &&
        (agora - ts_debounce_a > DEBOUNCE_MS)) {
      ts_press_a = agora;
      ts_debounce_a = agora;
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
      uint32_t dur = agora - ts_press_a;
      if (ts_press_a > 0 && dur < PRESS_CURTO_MS)
        irq_a_curto = true;
      if (ts_press_a > 0 && dur >= PRESS_LONGO_MS)
        irq_a_longo = true;
      ts_press_a = 0;
    }
  } else if (gpio == BUTTON_B_PIN) {
    if ((events & GPIO_IRQ_EDGE_FALL) &&
        (agora - ts_debounce_b > DEBOUNCE_MS)) {
      ts_press_b = agora;
      ts_debounce_b = agora;
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
      uint32_t dur = agora - ts_press_b;
      if (ts_press_b > 0 && dur < PRESS_CURTO_MS)
        irq_b_curto = true;
      if (ts_press_b > 0 && dur >= PRESS_LONGO_MS)
        irq_b_longo = true;
      ts_press_b = 0;
    }
  } else if (gpio == JOYSTICK_SW_PIN) {
    if ((events & GPIO_IRQ_EDGE_FALL) &&
        (agora - ts_debounce_sw > DEBOUNCE_MS)) {
      ts_press_sw = agora;
      ts_debounce_sw = agora;
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
      uint32_t dur = agora - ts_press_sw;
      if (ts_press_sw > 0 && dur < PRESS_CURTO_MS)
        irq_sw_curto = true;
      ts_press_sw = 0;
    }
  }
}

/* ============================================================================
 * 7. WIFI — Inicialização CYW43 nativo + Servidor HTTP TCP (lwIP poll mode)
 * ============================================================================
 */

/* Dashboard HTML embutido — servido em GET / */
static const char HTML_PAGE[] =
    "<!DOCTYPE html>\n<html>\n<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
    "<title>FarmTech</title>\n"
    "<style>\n"
    "*{box-sizing:border-box;margin:0;padding:0}\n"
    "body{font-family:monospace;background:#0d0d0d;color:#00ff88;padding:16px}"
    "\n"
    "h1{text-align:center;margin-bottom:16px;font-size:1.3em}\n"
    ".card{border:1px solid "
    "#00ff88;border-radius:6px;padding:12px;margin-bottom:12px}\n"
    ".card "
    "h2{font-size:.9em;opacity:.6;margin-bottom:8px;text-transform:uppercase}\n"
    ".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}\n"
    ".val{font-size:1.6em;font-weight:bold}\n"
    ".unit{font-size:.8em;opacity:.7}\n"
    ".btn{display:inline-block;padding:8px 14px;margin:3px;border:none;"
    "border-radius:4px;cursor:pointer;font-family:monospace;font-size:.9em;"
    "background:#00ff88;color:#000;font-weight:bold}\n"
    ".btn.off{background:#333;color:#00ff88;border:1px solid #00ff88}\n"
    ".btn.red{background:#ff4444;color:#fff}\n"
    ".status{text-align:center;padding:8px;font-size:.85em;opacity:.8}\n"
    ".crit{color:#ff4444;animation:blink 1s infinite}\n"
    "@keyframes blink{50%{opacity:0}}\n"
    "</style>\n</head>\n<body>\n"
    "<h1>&#127807; FARMTECH DOMO</h1>\n"
    "<div class=\"card\"><h2>Sensores</h2><div class=\"grid\">\n"
    "<div><span class=\"val\" id=\"temp\">--</span><span class=\"unit\"> "
    "C</span></div>\n"
    "<div><span class=\"val\" id=\"umid\">--</span><span class=\"unit\"> % "
    "Umid</span></div>\n"
    "<div><span class=\"val\" id=\"agua\">--</span><span class=\"unit\"> % "
    "Agua</span></div>\n"
    "<div><span class=\"val\" id=\"luz\">--</span><span class=\"unit\"> % "
    "Luz</span></div>\n"
    "</div></div>\n"
    "<div class=\"card\"><h2>Atuadores</h2>\n"
    "<button class=\"btn\" onclick=\"cmd('vent',1)\">Vent ON</button>"
    "<button class=\"btn off\" onclick=\"cmd('vent',0)\">Vent "
    "OFF</button><br>\n"
    "<button class=\"btn\" onclick=\"cmd('neb',1)\">Neb ON</button>"
    "<button class=\"btn off\" onclick=\"cmd('neb',0)\">Neb OFF</button><br>\n"
    "<button class=\"btn\" onclick=\"cmd('pump',1)\">Bomba ON</button>"
    "<button class=\"btn off\" onclick=\"cmd('pump',0)\">Bomba "
    "OFF</button><br>\n"
    "<button class=\"btn\" onclick=\"cmd('grow',1)\">Grow ON</button>"
    "<button class=\"btn off\" onclick=\"cmd('grow',0)\">Grow OFF</button>\n"
    "</div>\n"
    "<div class=\"card\"><h2>Modo</h2>\n"
    "<button class=\"btn\" onclick=\"cmd('mode','auto')\">AUTO</button>\n"
    "<button class=\"btn red\" "
    "onclick=\"cmd('mode','manual')\">MANUAL</button>\n"
    "<div class=\"status\" id=\"modo\">--</div></div>\n"
    "<div class=\"status\" id=\"status\">Conectando...</div>\n"
    "<div class=\"status\" id=\"ip\"></div>\n"
    "<script>\n"
    "function cmd(a,v){fetch('/cmd?act='+a+'&val='+v).then(r=>r.json())"
    ".then(d=>{document.getElementById('status').textContent="
    "d.ok?'OK: '+a+' = '+v:'ERRO: '+d.erro;}).catch(()=>{"
    "document.getElementById('status').textContent='Sem conexao';});}\n"
    "function atualizar(){fetch('/status').then(r=>r.json()).then(d=>{"
    "document.getElementById('temp').textContent=d.temp.toFixed(1);"
    "document.getElementById('umid').textContent=d.umid;"
    "document.getElementById('agua').textContent=d.agua;"
    "document.getElementById('luz').textContent=d.luz;"
    "document.getElementById('modo').textContent='Modo: '+d.modo+(d.crit?' "
    "CRITICO':'');"
    "document.getElementById('ip').textContent='IP: '+d.ip;"
    "document.getElementById('status').textContent="
    "'Alertas: '+d.alertas+' | DHT22: '+(d.dht22?'OK':'SIM');"
    "}).catch(()=>{});}\n"
    "setInterval(atualizar,2000);atualizar();\n"
    "</script>\n</body>\n</html>";

/* --- Monta resposta JSON do /status --- */
static int build_status_json(char *buf, int sz) {
  return snprintf(buf, sz,
                  "{\"temp\":%.1f,\"umid\":%d,\"agua\":%d,\"luz\":%d,"
                  "\"vent\":%d,\"neb\":%d,\"pump\":%d,\"grow\":%d,"
                  "\"modo\":\"%s\",\"crit\":%s,\"alertas\":%d,"
                  "\"dht22\":%s,\"ip\":\"%s\"}",
                  temperatura_atual, umidade_atual, nivel_agua_atual,
                  luz_ambiente_atual, status_vent, status_neb, status_bomba,
                  status_grow, modo_automatico ? "AUTO" : "MANUAL",
                  alerta_critico ? "true" : "false", qtd_alertas,
                  dht22_ok ? "true" : "false", ip_str);
}

/* --- Executa comando recebido via /cmd --- */
static bool execute_http_cmd(const char *act, const char *val, char *err,
                             int esz) {
  if (strcmp(act, "mode") == 0) {
    modo_automatico = (strcmp(val, "auto") == 0);
    return true;
  }
  if (modo_automatico) {
    snprintf(err, esz, "Modo AUTO ativo");
    return false;
  }
  int v = atoi(val);
  if (strcmp(act, "pump") == 0)
    status_bomba = (v == 1);
  else if (strcmp(act, "vent") == 0)
    status_vent = (v == 1);
  else if (strcmp(act, "neb") == 0)
    status_neb = (v == 1);
  else if (strcmp(act, "grow") == 0)
    status_grow = (v == 1);
  return true;
}

/* --- Envia resposta HTTP completa pela conexão TCP --- */
static err_t http_send_response(struct tcp_pcb *pcb, const char *content_type,
                                const char *body, int body_len) {
  char header[128];
  int hlen = snprintf(header, sizeof(header),
                      "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n"
                      "Content-Length: %d\r\nConnection: close\r\n\r\n",
                      content_type, body_len);
  tcp_write(pcb, header, hlen, TCP_WRITE_FLAG_COPY);
  tcp_write(pcb, body, body_len, TCP_WRITE_FLAG_COPY);
  tcp_output(pcb);
  return ERR_OK;
}

/* --- Callback de recepção TCP — despacho de rotas HTTP --- */
static err_t http_recv_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p,
                          err_t err) {
  if (p == NULL) {
    /* Cliente fechou conexão */
    tcp_close(pcb);
    return ERR_OK;
  }

  char req[512];
  int len = (p->tot_len < 511) ? p->tot_len : 511;
  pbuf_copy_partial(p, req, len, 0);
  req[len] = '\0';
  tcp_recved(pcb, p->tot_len);
  pbuf_free(p);

  /* Rota: GET /status */
  if (strstr(req, "GET /status")) {
    char json[256];
    int jlen = build_status_json(json, sizeof(json));
    http_send_response(pcb, "application/json", json, jlen);
  }
  /* Rota: GET /cmd?act=...&val=... */
  else if (strstr(req, "GET /cmd?")) {
    char act[16] = {0}, val[16] = {0};
    const char *pa = strstr(req, "act=");
    if (pa)
      sscanf(pa, "act=%15[^& ]", act);
    const char *pv = strstr(req, "val=");
    if (pv)
      sscanf(pv, "val=%15[^& ]", val);

    char errmsg[32] = "";
    bool ok = execute_http_cmd(act, val, errmsg, sizeof(errmsg));
    char resp[128];
    int rlen;
    if (ok)
      rlen = snprintf(resp, sizeof(resp),
                      "{\"ok\":true,\"acao\":\"%s\",\"val\":\"%s\"}", act, val);
    else
      rlen = snprintf(resp, sizeof(resp), "{\"ok\":false,\"erro\":\"%s\"}",
                      errmsg);
    http_send_response(pcb, "application/json", resp, rlen);
  }
  /* Rota: GET / — dashboard HTML */
  else if (strstr(req, "GET / ") || strstr(req, "GET /\r")) {
    http_send_response(pcb, "text/html", HTML_PAGE, strlen(HTML_PAGE));
  } else {
    const char *n = "404";
    http_send_response(pcb, "text/plain", n, 3);
  }

  tcp_close(pcb);
  return ERR_OK;
}

/* --- Callback de nova conexão TCP aceita --- */
static err_t http_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err) {
  tcp_recv(newpcb, http_recv_cb);
  return ERR_OK;
}

/* --- Inicializa WiFi CYW43 nativo e servidor TCP na porta 80 --- */
static bool wifi_init(void) {
  /* Pula WiFi se SSID não foi configurado (placeholder) */
  if (strcmp(WIFI_SSID, "SUA_REDE_AQUI") == 0) {
    printf("[WIFI] SSID nao configurado — WiFi desabilitado\n");
    return false;
  }

  if (cyw43_arch_init()) {
    printf("[WIFI] Falha ao inicializar CYW43\n");
    return false;
  }
  cyw43_arch_enable_sta_mode();

  printf("[WIFI] Conectando a '%s' (Async)...\n", WIFI_SSID);
  cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_SENHA, CYW43_AUTH_WPA2_AES_PSK);
  strcpy(ip_str, "Conectando...");

  /* Inicia servidor TCP na porta 80 */
  struct tcp_pcb *pcb = tcp_new();
  if (!pcb)
    return false;
  err_t berr = tcp_bind(pcb, IP_ADDR_ANY, HTTP_PORT);
  if (berr != ERR_OK) {
    tcp_close(pcb);
    return false;
  }
  http_pcb = tcp_listen(pcb);
  if (!http_pcb)
    return false;
  tcp_accept(http_pcb, http_accept_cb);

  printf("[WIFI] Servidor HTTP ativo na porta %d\n", HTTP_PORT);
  return true;
}

/* ============================================================================
 * 8. SETUP HARDWARE — Inicializa todos os periféricos
 * ============================================================================
 */

/* Configura um pino como PWM e retorna slice/channel */
static void setup_pwm_pin(uint pin, uint *slice_out, uint *chan_out,
                          uint16_t wrap) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  *slice_out = pwm_gpio_to_slice_num(pin);
  *chan_out = pwm_gpio_to_channel(pin);
  pwm_set_wrap(*slice_out, wrap);
  pwm_set_chan_level(*slice_out, *chan_out, 0);
  pwm_set_enabled(*slice_out, true);
}

static void setup_hardware(void) {
  stdio_init_all();
  sleep_ms(500); /* aguarda USB serial estabilizar */

  /* I2C para OLED SSD1306 */
  i2c_init(I2C_PORT, I2C_FREQ);
  gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA_PIN);
  gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SCL_PIN);
  oled_init();

  /* ADC (Joystick X/Y, LDR, Nível de Água) */
  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);  /* ADC0 */
  adc_gpio_init(JOYSTICK_Y_PIN);  /* ADC1 */
  adc_gpio_init(WATER_LEVEL_PIN); /* ADC2 */
  adc_gpio_init(LDR_PIN);         /* ADC3 */

  /* Botões com pull-up */
  gpio_init(BUTTON_A_PIN);
  gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_A_PIN);

  gpio_init(BUTTON_B_PIN);
  gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_B_PIN);

  gpio_init(JOYSTICK_SW_PIN);
  gpio_set_dir(JOYSTICK_SW_PIN, GPIO_IN);
  gpio_pull_up(JOYSTICK_SW_PIN);

  /* IRQ — registra callback com AMBAS bordas em todos os botões */
  gpio_set_irq_enabled_with_callback(BUTTON_A_PIN,
                                     GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                     true, &gpio_irq_callback);
  gpio_set_irq_enabled(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                       true);
  gpio_set_irq_enabled(JOYSTICK_SW_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                       true);

  /* DHT22 — pino configurado como entrada com pull-up */
  gpio_init(DHT22_PIN);
  gpio_set_dir(DHT22_PIN, GPIO_IN);
  gpio_pull_up(DHT22_PIN);

  /* LED RGB — PWM nos 3 canais (catodo comum, duty 0-255) */
  setup_pwm_pin(LED_R_PIN, &slice_r, &chan_r, LED_PWM_WRAP);
  setup_pwm_pin(LED_G_PIN, &slice_g, &chan_g, LED_PWM_WRAP);
  setup_pwm_pin(LED_B_PIN, &slice_b, &chan_b, LED_PWM_WRAP);

  /* Buzzer A — PWM */
  setup_pwm_pin(BUZZER_A_PIN, &slice_buzz, &chan_buzz, 0);

  /* PIO para WS2812B (Matriz 5x5) */
  uint offset = pio_add_program(ws_pio, &ws2812_program);
  ws2812_program_init(ws_pio, ws_sm, offset, LED_MATRIX_PIN, 800000, false);

  printf("[SETUP] Hardware inicializado com sucesso\n");
}

/* ============================================================================
 * 9. LER SENSORES — DHT22 real ou fallback joystick
 * ============================================================================
 */

static void ler_sensores(void) {
  uint32_t agora = to_ms_since_boot(get_absolute_time());

  /* Limita leitura do DHT22 a cada 2 segundos (recomendado pelo datasheet) */
  if (sensor_dht22_ativo &&
      (agora - ultima_leitura_sensor_ms > SENSOR_READ_INTERVAL_MS)) {
    ultima_leitura_sensor_ms = agora;
    float t = 0.0f, h = 0.0f;
    if (dht22_read(&t, &h)) {
      dht22_ok = true;
      temperatura_atual = t;
      umidade_atual = (int)h;
    } else {
      /* Fallback: joystick Y simula temperatura, joystick X simula umidade */
      dht22_ok = false;
      adc_select_input(1); /* Y */
      temperatura_atual = 15.0f + ((adc_read() / 4095.0f) * 25.0f);
      adc_select_input(0); /* X */
      umidade_atual = (int)((adc_read() / 4095.0f) * 100.0f);
    }
  }

  /* LDR — luminosidade (inversão: mais luz = menor resistência) */
  if (sensor_ldr_ativo) {
    adc_select_input(3); /* ADC3 = GP29 */
    luz_ambiente_atual = 100 - (int)((adc_read() / 4095.0f) * 100.0f);
    if (luz_ambiente_atual < 0)
      luz_ambiente_atual = 0;
  }

  /* Nível de água capacitivo */
  if (sensor_agua_ativo) {
    adc_select_input(2); /* ADC2 = GP28 */
    nivel_agua_atual = (int)((adc_read() / 4095.0f) * 100.0f);
  }
}

/* ============================================================================
 * 10. PROCESSAR DECISÕES — Setpoints + Geração de alertas
 * ============================================================================
 */

static void registrar_alerta(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  uint32_t ms = to_ms_since_boot(get_absolute_time());
  vsnprintf(historico_alertas[idx_alerta_write].msg,
            sizeof(historico_alertas[0].msg), fmt, args);
  historico_alertas[idx_alerta_write].timestamp_ms = ms;
  idx_alerta_write = (idx_alerta_write + 1) % MAX_ALERTAS;
  if (qtd_alertas < MAX_ALERTAS)
    qtd_alertas++;
  va_end(args);
}

static void processar_decisoes(void) {
  /* Verifica alertas críticos */
  bool novo_critico =
      (temperatura_atual > TEMP_CRITICA || nivel_agua_atual < 5);
  if (!alerta_critico && novo_critico) {
    if (temperatura_atual > TEMP_CRITICA)
      registrar_alerta("TEMP>35 %.1fC", temperatura_atual);
    if (nivel_agua_atual < 5)
      registrar_alerta("AGUA<5 %d%%", nivel_agua_atual);
  }
  alerta_critico = novo_critico;

  /* Modo automático: aplica regras de setpoints */
  if (modo_automatico) {
    if (temperatura_atual > TEMP_MAX) {
      status_vent = true;
      status_neb = (umidade_atual < 85);
    } else {
      status_vent = false;
      status_neb = (umidade_atual < UMIDADE_MIN);
    }
    status_bomba = (nivel_agua_atual < NIVEL_AGUA_MIN);
    status_grow = (luz_ambiente_atual < LUZ_MINIMA);
  }
}

/* ============================================================================
 * 11. ATUALIZAR ATUADORES — LED RGB (PWM) + Buzzer (PWM)
 * ============================================================================
 */

/* Define cor do LED RGB via PWM (0-255 cada canal) */
static void led_rgb_set(uint8_t r, uint8_t g, uint8_t b) {
  pwm_set_chan_level(slice_r, chan_r, r);
  pwm_set_chan_level(slice_g, chan_g, g);
  pwm_set_chan_level(slice_b, chan_b, b);
}

/* Configura buzzer para frequência (0 = silêncio) */
static void buzzer_set_freq(uint32_t freq) {
  if (freq == 0) {
    pwm_set_chan_level(slice_buzz, chan_buzz, 0);
    return;
  }
  uint32_t clk = clock_get_hz(clk_sys);
  uint32_t wrap = clk / freq - 1;
  if (wrap > 65535)
    wrap = 65535;
  pwm_set_wrap(slice_buzz, (uint16_t)wrap);
  pwm_set_chan_level(slice_buzz, chan_buzz,
                     (uint16_t)(wrap / 2)); /* 50% duty */
}

static void atualizar_atuadores(void) {
  uint32_t agora = to_ms_since_boot(get_absolute_time());

  /* --- LED RGB: status do sistema --- */
  static bool manual_blink = false;

  if (alerta_critico) {
    /* Vermelho = crítico */
    led_rgb_set(255, 0, 0);
  } else if (!modo_automatico) {
    /* Azul piscando = modo MANUAL */
    manual_blink = !manual_blink;
    led_rgb_set(0, 0, manual_blink ? 180 : 0);
  } else if (temperatura_atual > 25.0f && temperatura_atual <= TEMP_MAX) {
    /* Amarelo = atenção */
    led_rgb_set(255, 180, 0);
  } else if (!wifi_conectado) {
    /* Branco fraco = sem WiFi */
    led_rgb_set(40, 40, 40);
  } else {
    /* Verde = normal, tudo OK */
    led_rgb_set(0, 255, 0);
  }

  /* --- Buzzer: alertas sonoros --- */
  if (alerta_critico) {
    /* Bip 1000Hz: 200ms on / 200ms off */
    if (agora >= buzz_next_toggle_ms) {
      buzz_on_phase = !buzz_on_phase;
      buzzer_set_freq(buzz_on_phase ? BUZZ_FREQ_CRIT : 0);
      buzz_next_toggle_ms = agora + 200;
    }
  } else if (temperatura_atual > TEMP_MAX) {
    /* Bip 500Hz: 100ms on / 900ms off */
    if (agora >= buzz_next_toggle_ms) {
      buzz_on_phase = !buzz_on_phase;
      buzzer_set_freq(buzz_on_phase ? BUZZ_FREQ_ATENCAO : 0);
      buzz_next_toggle_ms = agora + (buzz_on_phase ? 100 : 900);
    }
  } else {
    buzzer_set_freq(0);
  }
}

/* ============================================================================
 * 12. ATUALIZAR MATRIZ LEDS — WS2812B 5x5 via PIO
 * ============================================================================
 */

/* Cores GRB 24 bits para WS2812B */
#define WS_VERDE 0x00FF0000u
#define WS_VERMELHO 0x0000FF00u
#define WS_AZUL 0x000000FFu
#define WS_AMARELO 0x00FFFF00u
#define WS_BRANCO 0x00FFFFFFu
#define WS_CIANO 0x00FF00FFu
#define WS_LARANJA 0x007FFF00u
#define WS_APAGADO 0x00000000u

static void ws2812_put(uint32_t grb) {
  pio_sm_put_blocking(ws_pio, ws_sm, grb << 8u);
}

static void atualizar_matriz_leds(void) {
  static bool blink = false;
  static uint8_t fade_val = 0;
  static int8_t fade_dir = 5;
  blink = !blink;

  uint32_t grid[25] = {0};

  /* 1. Alerta crítico: todos VERMELHO piscando */
  if (alerta_critico) {
    uint32_t cor = blink ? WS_VERMELHO : WS_APAGADO;
    for (int i = 0; i < 25; i++)
      grid[i] = cor;
  } else {
    /* 2. Modo MANUAL: LED[0][0] laranja fixo */
    if (!modo_automatico)
      grid[0] = WS_LARANJA;
    /* 3. Ventilação ativa: linha 0 branco */
    if (status_vent) {
      for (int i = 0; i < 5; i++)
        grid[i] = WS_BRANCO;
    }
    /* 4. Nebulizador ativo: linha 1 azul */
    if (status_neb) {
      for (int i = 5; i < 10; i++)
        grid[i] = WS_AZUL;
    }
    /* 5. Grow lights ativas: linha 2 amarelo */
    if (status_grow) {
      for (int i = 10; i < 15; i++)
        grid[i] = WS_AMARELO;
    }
    /* 6. Bomba ativa: linha 3 ciano */
    if (status_bomba) {
      for (int i = 15; i < 20; i++)
        grid[i] = WS_CIANO;
    }
    /* 7. Idle: LED central [2][2] verde pulsando (fade) */
    if (!status_vent && !status_neb && !status_grow && !status_bomba) {
      fade_val += fade_dir;
      if (fade_val >= 200 || fade_val <= 5)
        fade_dir = -fade_dir;
      grid[12] = ((uint32_t)fade_val << 16); /* G channel only */
    }
  }

  /* Envia os 25 pixels ao PIO */
  for (int i = 0; i < 25; i++) {
    ws2812_put(grid[i]);
  }
}

/* ============================================================================
 * 13. ATUALIZAR MENU — Máquina de estados + timeout 10s
 * ============================================================================
 */

static uint32_t last_joy_ms = 0;

/* Lê joystick Y e ajusta cursor */
static bool poll_joystick(uint32_t agora) {
  if (agora - last_joy_ms < JOY_POLL_MS)
    return false;
  last_joy_ms = agora;
  adc_select_input(1); /* Y */
  uint16_t jy = adc_read();
  if (jy < (JOY_CENTER - JOY_THRESHOLD)) {
    cursor_menu--;
    return true;
  }
  if (jy > (JOY_CENTER + JOY_THRESHOLD)) {
    cursor_menu++;
    return true;
  }
  return false;
}

/* Retorna o cursor máximo conforme a tela atual */
static int menu_max_cursor(void) {
  switch (tela_atual) {
  case TELA_MENU_PRINCIPAL:
    return 4;
  case TELA_SENSORES:
    return 2;
  case TELA_ATUADORES:
    return 3;
  case TELA_ALERTAS:
    return (qtd_alertas > 1) ? qtd_alertas - 1 : 0;
  default:
    return 0;
  }
}

static void clamp_cursor(void) {
  if (cursor_menu < 0)
    cursor_menu = 0;
  int mx = menu_max_cursor();
  if (cursor_menu > mx)
    cursor_menu = mx;
}

/* Bip de confirmação (800Hz, 50ms) */
static void bip_confirma(void) {
  buzzer_set_freq(BUZZ_FREQ_CONFIRM);
  sleep_ms(50);
  buzzer_set_freq(0);
}

static void atualizar_menu(void) {
  uint32_t agora = to_ms_since_boot(get_absolute_time());
  bool teve_input = false;

  teve_input |= poll_joystick(agora);

  /* --- Botão A curto --- */
  if (irq_a_curto) {
    irq_a_curto = false;
    teve_input = true;
    bip_confirma();
    switch (tela_atual) {
    case TELA_DASHBOARD:
      tela_atual = TELA_MENU_PRINCIPAL;
      cursor_menu = 0;
      break;
    case TELA_MENU_PRINCIPAL:
      switch (cursor_menu) {
      case 0:
        tela_atual = TELA_SENSORES;
        cursor_menu = 0;
        break;
      case 1:
        tela_atual = TELA_DADOS_LIVE;
        cursor_menu = 0;
        break;
      case 2:
        tela_atual = TELA_ATUADORES;
        cursor_menu = 0;
        break;
      case 3:
        tela_atual = TELA_ALERTAS;
        cursor_menu = 0;
        break;
      case 4:
        modo_automatico = !modo_automatico;
        break;
      }
      break;
    case TELA_SENSORES:
      if (cursor_menu == 0)
        sensor_dht22_ativo = !sensor_dht22_ativo;
      if (cursor_menu == 1)
        sensor_ldr_ativo = !sensor_ldr_ativo;
      if (cursor_menu == 2)
        sensor_agua_ativo = !sensor_agua_ativo;
      break;
    case TELA_ATUADORES:
      if (!modo_automatico) {
        if (cursor_menu == 0)
          status_vent = !status_vent;
        if (cursor_menu == 1)
          status_neb = !status_neb;
        if (cursor_menu == 2)
          status_bomba = !status_bomba;
        if (cursor_menu == 3)
          status_grow = !status_grow;
      }
      break;
    default:
      break;
    }
  }

  /* --- Botão A longo → volta para Dashboard --- */
  if (irq_a_longo) {
    irq_a_longo = false;
    teve_input = true;
    tela_atual = TELA_DASHBOARD;
  }

  /* --- Botão B curto → volta um nível --- */
  if (irq_b_curto) {
    irq_b_curto = false;
    teve_input = true;
    if (tela_atual != TELA_DASHBOARD) {
      tela_atual = TELA_MENU_PRINCIPAL;
      cursor_menu = 0;
    }
  }

  /* --- Botão B longo → limpa histórico de alertas --- */
  if (irq_b_longo) {
    irq_b_longo = false;
    teve_input = true;
    if (tela_atual == TELA_ALERTAS) {
      qtd_alertas = 0;
      idx_alerta_write = 0;
    }
  }

  /* --- Joystick SW curto → entrar no menu ou confirmar --- */
  if (irq_sw_curto) {
    irq_sw_curto = false;
    teve_input = true;
    bip_confirma();
    if (tela_atual == TELA_DASHBOARD) {
      tela_atual = TELA_MENU_PRINCIPAL;
      cursor_menu = 0;
    }
  }

  if (teve_input)
    ultimo_input_ms = agora;

  /* Timeout: 10s sem input → Dashboard */
  if (agora - ultimo_input_ms > MENU_TIMEOUT_MS &&
      tela_atual != TELA_DASHBOARD) {
    tela_atual = TELA_DASHBOARD;
  }

  clamp_cursor();
}

/* ============================================================================
 * 14. ATUALIZAR OLED — Renderiza conforme tela atual
 * ============================================================================
 */

/* Converte ms desde boot para string HH:MM:SS */
static void ms_to_hms(uint32_t ms, char *buf, int sz) {
  uint32_t s = ms / 1000;
  snprintf(buf, sz, "%02d:%02d:%02d", (int)(s / 3600) % 24, (int)(s / 60) % 60,
           (int)(s % 60));
}

static void render_dashboard(void) {
  char b[32];
  oled_str(12, 0, "== FARMTECH DOMO ==");
  oled_hline(9);
  snprintf(b, sizeof(b), "T:%.1fC   U:%d%%", temperatura_atual, umidade_atual);
  oled_str(0, 2, b);
  snprintf(b, sizeof(b), "H2O:%d%%   Luz:%d%%", nivel_agua_atual,
           luz_ambiente_atual);
  oled_str(0, 3, b);
  snprintf(b, sizeof(b), "V:%d N:%d B:%d G:%d", status_vent, status_neb,
           status_bomba, status_grow);
  oled_str(0, 4, b);
  snprintf(b, sizeof(b), "%s %s DHT:%s", modo_automatico ? "AUTO" : "MAN",
           alerta_critico ? "!CRIT" : "OK", dht22_ok ? "OK" : "SIM");
  oled_str(0, 5, b);
  oled_hline(54);
  oled_str(0, 7, wifi_conectado ? "[SW/A] Menu" : "[SW/A] Menu WiFi:OFF");
}

static void render_menu_principal(void) {
  char b[32];
  oled_str(30, 0, "=== MENU ===");
  snprintf(b, sizeof(b), "%c 1.Sensores Ativos", cursor_menu == 0 ? '>' : ' ');
  oled_str(0, 1, b);
  snprintf(b, sizeof(b), "%c 2.Dados Live", cursor_menu == 1 ? '>' : ' ');
  oled_str(0, 2, b);
  snprintf(b, sizeof(b), "%c 3.Atuadores", cursor_menu == 2 ? '>' : ' ');
  oled_str(0, 3, b);
  snprintf(b, sizeof(b), "%c 4.Historico", cursor_menu == 3 ? '>' : ' ');
  oled_str(0, 4, b);
  snprintf(b, sizeof(b), "%c 5.Modo:%s", cursor_menu == 4 ? '>' : ' ',
           modo_automatico ? "AUTO" : "MANUAL");
  oled_str(0, 5, b);
  oled_str(0, 7, "[Joy]Nav [A]OK [B]Volta");
}

static void render_sensores(void) {
  char b[32];
  oled_str(18, 0, "=== SENSORES ===");
  snprintf(b, sizeof(b), "%c DHT22 Temp/Umid %s", cursor_menu == 0 ? '>' : ' ',
           sensor_dht22_ativo ? "ON" : "OFF");
  oled_str(0, 2, b);
  snprintf(b, sizeof(b), "%c LDR Luminosidade %s", cursor_menu == 1 ? '>' : ' ',
           sensor_ldr_ativo ? "ON" : "OFF");
  oled_str(0, 3, b);
  snprintf(b, sizeof(b), "%c Nivel d Agua %s", cursor_menu == 2 ? '>' : ' ',
           sensor_agua_ativo ? "ON" : "OFF");
  oled_str(0, 4, b);
  oled_str(0, 7, "[A]Liga/Des [B]Volta");
}

static void render_dados_live(void) {
  char b[32];
  oled_str(12, 0, "=== DADOS LIVE ===");
  snprintf(b, sizeof(b), "Temp:  %.1f C", temperatura_atual);
  oled_str(0, 1, b);
  snprintf(b, sizeof(b), "Umid:  %d %%", umidade_atual);
  oled_str(0, 2, b);
  snprintf(b, sizeof(b), "Agua:  %d %%", nivel_agua_atual);
  oled_str(0, 3, b);
  snprintf(b, sizeof(b), "Luz:   %d %%", luz_ambiente_atual);
  oled_str(0, 4, b);
  snprintf(b, sizeof(b), "Modo: %s DHT:%s", modo_automatico ? "AUTO" : "MAN",
           dht22_ok ? "OK" : "SIM");
  oled_str(0, 5, b);
  oled_str(0, 7, "[B] Voltar");
}

static void render_atuadores(void) {
  char b[32];
  oled_str(12, 0, "=== ATUADORES ===");
  if (modo_automatico) {
    oled_str(72, 0, "[Modo AUTO]");
  }
  snprintf(b, sizeof(b), "%c Ventilacao  %s", cursor_menu == 0 ? '>' : ' ',
           status_vent ? "ON" : "OFF");
  oled_str(0, 2, b);
  snprintf(b, sizeof(b), "%c Nebulizador %s", cursor_menu == 1 ? '>' : ' ',
           status_neb ? "ON" : "OFF");
  oled_str(0, 3, b);
  snprintf(b, sizeof(b), "%c Bomba Agua  %s", cursor_menu == 2 ? '>' : ' ',
           status_bomba ? "ON" : "OFF");
  oled_str(0, 4, b);
  snprintf(b, sizeof(b), "%c Grow Lights %s", cursor_menu == 3 ? '>' : ' ',
           status_grow ? "ON" : "OFF");
  oled_str(0, 5, b);
  oled_str(0, 7,
           modo_automatico ? "(so leitura) [B]Volta" : "[A]Alterna [B]Volta");
}

static void render_alertas(void) {
  char b[32], hms[12];
  snprintf(b, sizeof(b), "=== ALERTAS (%d) ===", qtd_alertas);
  oled_str(6, 0, b);
  if (qtd_alertas == 0) {
    oled_str(0, 2, "Nenhum alerta.");
    oled_str(0, 7, "[B]Volta");
    return;
  }
  /* Mostra até 3 alertas, scroll com joystick */
  int show = (qtd_alertas > 3) ? 3 : qtd_alertas;
  int start = cursor_menu; /* scroll offset */
  if (start + show > qtd_alertas)
    start = qtd_alertas - show;
  if (start < 0)
    start = 0;
  int page = 1;
  for (int i = 0; i < show && page < 7; i++) {
    int pos =
        (idx_alerta_write - 1 - start - i + MAX_ALERTAS * 2) % MAX_ALERTAS;
    ms_to_hms(historico_alertas[pos].timestamp_ms, hms, sizeof(hms));
    snprintf(b, sizeof(b), "%c%s %s", (start + i == cursor_menu) ? '>' : ' ',
             hms, historico_alertas[pos].msg);
    oled_str(0, page, b);
    page += 2; /* espaço entre alertas */
  }
  oled_str(0, 7, "[Blong]Limpar [B]Volta");
}

static void atualizar_oled(void) {
  oled_clear();
  switch (tela_atual) {
  case TELA_DASHBOARD:
    render_dashboard();
    break;
  case TELA_MENU_PRINCIPAL:
    render_menu_principal();
    break;
  case TELA_SENSORES:
    render_sensores();
    break;
  case TELA_DADOS_LIVE:
    render_dados_live();
    break;
  case TELA_ATUADORES:
    render_atuadores();
    break;
  case TELA_ALERTAS:
    render_alertas();
    break;
  }
  oled_flush();
}

/* ============================================================================
 * 15. ATUALIZAR SERIAL — Debug USB
 * ============================================================================
 */

static void atualizar_serial(void) {
  printf("[FARM] T=%.1f U=%d%% H2O=%d%% Luz=%d%% | "
         "V:%d N:%d B:%d G:%d | %s | DHT:%s | Tela:%d | WiFi:%s\n",
         temperatura_atual, umidade_atual, nivel_agua_atual, luz_ambiente_atual,
         status_vent, status_neb, status_bomba, status_grow,
         modo_automatico ? "AUTO" : "MANUAL", dht22_ok ? "OK" : "SIM",
         (int)tela_atual, wifi_conectado ? ip_str : "OFF");
}

/* ============================================================================
 * 16. MELODIA DE BOOT — 3 notas curtas
 * ============================================================================
 */

static void melodia_boot(void) {
  const uint freqs[] = {BUZZ_BOOT_F1, BUZZ_BOOT_F2, BUZZ_BOOT_F3};
  for (int i = 0; i < 3; i++) {
    buzzer_set_freq(freqs[i]);
    sleep_ms(120);
    buzzer_set_freq(0);
    sleep_ms(50);
  }
}

/* ============================================================================
 * 17. MAIN — Loop principal
 * ============================================================================
 */

int main(void) {
  setup_hardware();

  /* Tela de splash no OLED */
  oled_clear();
  oled_str(12, 2, "FARMTECH DOMO");
  oled_str(18, 4, "Inicializando...");
  oled_flush();

  /* Melodia de boot */
  melodia_boot();

  /* Conecta WiFi (não bloqueia se falhar) */
  wifi_conectado = wifi_init();
  if (!wifi_conectado) {
    printf("[MAIN] WiFi indisponivel — sistema funcionando offline\n");
  }

  /* Marca tempo inicial para timeout do menu */
  ultimo_input_ms = to_ms_since_boot(get_absolute_time());

  printf("[MAIN] FarmTech iniciado. Loop principal ativo.\n");
  if (wifi_conectado) {
    printf("[MAIN] Dashboard: http://%s/\n", ip_str);
  }

  while (true) {
    ler_sensores();          /* [Sensores] DHT22 real ou fallback joystick */
    processar_decisoes();    /* [Decisão]  Setpoints + alertas */
    atualizar_atuadores();   /* [Atuadores] LED RGB + buzzer */
    atualizar_matriz_leds(); /* [Visual]   WS2812B via PIO */
    atualizar_menu();        /* [Menu]     FSM navegação + timeout */
    atualizar_oled();        /* [Display]  Render tela atual */
    if (wifi_conectado) {
      cyw43_arch_poll(); /* [WiFi]     Processa lwIP poll mode */
      if (strcmp(ip_str, "Conectando...") == 0) {
         if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP) {
            const ip4_addr_t *ip = netif_ip4_addr(netif_default);
            snprintf(ip_str, sizeof(ip_str), "%s", ip4addr_ntoa(ip));
            printf("[WIFI] Conectado! IP: %s\n", ip_str);
         }
      }
    }
    atualizar_serial(); /* [Debug]    Printf USB */
    sleep_ms(LOOP_DELAY_MS);
  }

  return 0; /* nunca alcançado */
}
o codigo = e os erros : [{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "fatal_too_many_errors",
	"severity": 8,
	"message": "Too many errors emitted, stopping now",
	"source": "clang",
	"startLineNumber": 1,
	"startColumn": 1,
	"endLineNumber": 1,
	"endColumn": 1,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "pp_file_not_found",
	"severity": 8,
	"message": "'hardware/adc.h' file not found",
	"source": "clang",
	"startLineNumber": 22,
	"startColumn": 10,
	"endLineNumber": 22,
	"endColumn": 26,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "unknown_typename",
	"severity": 8,
	"message": "Unknown type name 'PIO'",
	"source": "clang",
	"startLineNumber": 171,
	"startColumn": 8,
	"endLineNumber": 171,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "undeclared_var_use",
	"severity": 8,
	"message": "Use of undeclared identifier 'pio1'",
	"source": "clang",
	"startLineNumber": 171,
	"startColumn": 21,
	"endLineNumber": 171,
	"endColumn": 25,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "unknown_typename_suggest",
	"severity": 8,
	"message": "Unknown type name 'uint'; did you mean 'int'? (fix available)",
	"source": "clang",
	"startLineNumber": 172,
	"startColumn": 8,
	"endLineNumber": 172,
	"endColumn": 12,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "unknown_typename_suggest",
	"severity": 8,
	"message": "Unknown type name 'uint'; did you mean 'int'? (fix available)",
	"source": "clang",
	"startLineNumber": 175,
	"startColumn": 8,
	"endLineNumber": 175,
	"endColumn": 12,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "unknown_typename_suggest",
	"severity": 8,
	"message": "Unknown type name 'uint'; did you mean 'int'? (fix available)",
	"source": "clang",
	"startLineNumber": 176,
	"startColumn": 8,
	"endLineNumber": 176,
	"endColumn": 12,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'i2c_write_blocking'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 267,
	"startColumn": 3,
	"endLineNumber": 267,
	"endColumn": 21,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "undeclared_var_use",
	"severity": 8,
	"message": "Use of undeclared identifier 'i2c1'",
	"source": "clang",
	"startLineNumber": 267,
	"startColumn": 22,
	"endLineNumber": 267,
	"endColumn": 30,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'i2c_write_blocking'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 310,
	"startColumn": 5,
	"endLineNumber": 310,
	"endColumn": 23,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "undeclared_var_use",
	"severity": 8,
	"message": "Use of undeclared identifier 'i2c1'",
	"source": "clang",
	"startLineNumber": 310,
	"startColumn": 24,
	"endLineNumber": 310,
	"endColumn": 32,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'gpio_set_dir'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 366,
	"startColumn": 3,
	"endLineNumber": 366,
	"endColumn": 15,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "undeclared_var_use",
	"severity": 8,
	"message": "Use of undeclared identifier 'GPIO_OUT'",
	"source": "clang",
	"startLineNumber": 366,
	"startColumn": 27,
	"endLineNumber": 366,
	"endColumn": 35,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'gpio_put'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 367,
	"startColumn": 3,
	"endLineNumber": 367,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'sleep_ms'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 368,
	"startColumn": 3,
	"endLineNumber": 368,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'sleep_us'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 370,
	"startColumn": 3,
	"endLineNumber": 370,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "undeclared_var_use",
	"severity": 8,
	"message": "Use of undeclared identifier 'GPIO_IN'",
	"source": "clang",
	"startLineNumber": 371,
	"startColumn": 27,
	"endLineNumber": 371,
	"endColumn": 34,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'gpio_pull_up'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 372,
	"startColumn": 3,
	"endLineNumber": 372,
	"endColumn": 15,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'gpio_get'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 376,
	"startColumn": 10,
	"endLineNumber": 376,
	"endColumn": 18,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "unknown_typename_suggest",
	"severity": 8,
	"message": "Unknown type name 'uint'; did you mean 'int'? (fix available)",
	"source": "clang",
	"startLineNumber": 432,
	"startColumn": 31,
	"endLineNumber": 432,
	"endColumn": 35,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "_generated_diagnostic_collection_name_#0",
	"code": "-Wimplicit-function-declaration",
	"severity": 8,
	"message": "Call to undeclared function 'to_ms_since_boot'; ISO C99 and later do not support implicit function declarations",
	"source": "clang",
	"startLineNumber": 433,
	"startColumn": 20,
	"endLineNumber": 433,
	"endColumn": 36,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1481",
	"severity": 4,
	"message": "unused variable 'scroll_alertas'",
	"source": "sonarqube",
	"startLineNumber": 132,
	"startColumn": 12,
	"endLineNumber": 132,
	"endColumn": 26,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 163,
	"startColumn": 1,
	"endLineNumber": 163,
	"endColumn": 62,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 164,
	"startColumn": 1,
	"endLineNumber": 164,
	"endColumn": 62,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 166,
	"startColumn": 1,
	"endLineNumber": 166,
	"endColumn": 73,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 167,
	"startColumn": 1,
	"endLineNumber": 168,
	"endColumn": 44,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 175,
	"startColumn": 1,
	"endLineNumber": 175,
	"endColumn": 50,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 176,
	"startColumn": 1,
	"endLineNumber": 176,
	"endColumn": 46,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S886",
	"severity": 4,
	"message": "Refactor this loop so that it is less error-prone. [+1 location]",
	"source": "sonarqube",
	"startLineNumber": 327,
	"startColumn": 5,
	"endLineNumber": 327,
	"endColumn": 8,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1144",
	"severity": 4,
	"message": "unused function 'oled_pixel'",
	"source": "sonarqube",
	"startLineNumber": 347,
	"startColumn": 13,
	"endLineNumber": 347,
	"endColumn": 23,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S3776",
	"severity": 4,
	"message": "Refactor this function to reduce its Cognitive Complexity from 38 to the 25 allowed. [+22 locations]",
	"source": "sonarqube",
	"startLineNumber": 432,
	"startColumn": 13,
	"endLineNumber": 432,
	"endColumn": 30,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S5276",
	"severity": 4,
	"message": "implicit conversion loses integer precision: 'int' to 'u16_t' (aka 'unsigned short')",
	"source": "sonarqube",
	"startLineNumber": 604,
	"startColumn": 26,
	"endLineNumber": 604,
	"endColumn": 30,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S5276",
	"severity": 4,
	"message": "implicit conversion loses integer precision: 'int' to 'u16_t' (aka 'unsigned short')",
	"source": "sonarqube",
	"startLineNumber": 605,
	"startColumn": 24,
	"endLineNumber": 605,
	"endColumn": 32,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1172",
	"severity": 4,
	"message": "Remove the unused parameter \"arg\".",
	"source": "sonarqube",
	"startLineNumber": 611,
	"startColumn": 33,
	"endLineNumber": 611,
	"endColumn": 36,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1172",
	"severity": 4,
	"message": "Remove the unused parameter \"err\".",
	"source": "sonarqube",
	"startLineNumber": 612,
	"startColumn": 33,
	"endLineNumber": 612,
	"endColumn": 36,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S5276",
	"severity": 4,
	"message": "implicit conversion loses integer precision: 'int' to 'u16_t' (aka 'unsigned short')",
	"source": "sonarqube",
	"startLineNumber": 621,
	"startColumn": 29,
	"endLineNumber": 621,
	"endColumn": 32,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 634,
	"startColumn": 5,
	"endLineNumber": 634,
	"endColumn": 38,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1172",
	"severity": 4,
	"message": "Remove the unused parameter \"arg\".",
	"source": "sonarqube",
	"startLineNumber": 667,
	"startColumn": 35,
	"endLineNumber": 667,
	"endColumn": 38,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1172",
	"severity": 4,
	"message": "Remove the unused parameter \"err\".",
	"source": "sonarqube",
	"startLineNumber": 667,
	"startColumn": 70,
	"endLineNumber": 667,
	"endColumn": 73,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1763",
	"severity": 4,
	"message": "code will never be executed",
	"source": "sonarqube",
	"startLineNumber": 676,
	"startColumn": 5,
	"endLineNumber": 676,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 797,
	"startColumn": 5,
	"endLineNumber": 797,
	"endColumn": 29,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S923",
	"severity": 4,
	"message": "Remove use of ellipsis notation.",
	"source": "sonarqube",
	"startLineNumber": 832,
	"startColumn": 13,
	"endLineNumber": 832,
	"endColumn": 29,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S5281",
	"severity": 4,
	"message": "format string is not a string literal",
	"source": "sonarqube",
	"startLineNumber": 837,
	"startColumn": 47,
	"endLineNumber": 837,
	"endColumn": 50,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S3776",
	"severity": 4,
	"message": "Refactor this function to reduce its Cognitive Complexity from 36 to the 25 allowed. [+18 locations]",
	"source": "sonarqube",
	"startLineNumber": 961,
	"startColumn": 13,
	"endLineNumber": 961,
	"endColumn": 34,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S3776",
	"severity": 4,
	"message": "Refactor this function to reduce its Cognitive Complexity from 47 to the 25 allowed. [+21 locations]",
	"source": "sonarqube",
	"startLineNumber": 1069,
	"startColumn": 13,
	"endLineNumber": 1069,
	"endColumn": 27,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S3562",
	"severity": 4,
	"message": "Add a \"default\" case to this switch statement.",
	"source": "sonarqube",
	"startLineNumber": 1086,
	"startColumn": 7,
	"endLineNumber": 1086,
	"endColumn": 13,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S134",
	"severity": 4,
	"message": "Refactor this code to not nest more than 3 if|for|do|while|switch statements. [+3 locations]",
	"source": "sonarqube",
	"startLineNumber": 1118,
	"startColumn": 9,
	"endLineNumber": 1118,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S134",
	"severity": 4,
	"message": "Refactor this code to not nest more than 3 if|for|do|while|switch statements. [+3 locations]",
	"source": "sonarqube",
	"startLineNumber": 1120,
	"startColumn": 9,
	"endLineNumber": 1120,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S134",
	"severity": 4,
	"message": "Refactor this code to not nest more than 3 if|for|do|while|switch statements. [+3 locations]",
	"source": "sonarqube",
	"startLineNumber": 1122,
	"startColumn": 9,
	"endLineNumber": 1122,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S134",
	"severity": 4,
	"message": "Refactor this code to not nest more than 3 if|for|do|while|switch statements. [+3 locations]",
	"source": "sonarqube",
	"startLineNumber": 1124,
	"startColumn": 9,
	"endLineNumber": 1124,
	"endColumn": 11,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1659",
	"severity": 4,
	"message": "Define each identifier in a dedicated statement.",
	"source": "sonarqube",
	"startLineNumber": 1286,
	"startColumn": 3,
	"endLineNumber": 1286,
	"endColumn": 22,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S886",
	"severity": 4,
	"message": "Refactor this loop so that it is less error-prone. [+1 location]",
	"source": "sonarqube",
	"startLineNumber": 1302,
	"startColumn": 3,
	"endLineNumber": 1302,
	"endColumn": 6,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1066",
	"severity": 4,
	"message": "Merge this \"if\" statement with the enclosing one. [+1 location]",
	"source": "sonarqube",
	"startLineNumber": 1409,
	"startColumn": 10,
	"endLineNumber": 1409,
	"endColumn": 12,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S134",
	"severity": 4,
	"message": "Refactor this code to not nest more than 3 if|for|do|while|switch statements. [+3 locations]",
	"source": "sonarqube",
	"startLineNumber": 1409,
	"startColumn": 10,
	"endLineNumber": 1409,
	"endColumn": 12,
	"origin": "extHost1"
},{
	"resource": "/c:/Users/morai/OneDrive/Documentos/FARMTECH/farmtech.c",
	"owner": "sonarlint",
	"code": "c:S1763",
	"severity": 4,
	"message": "'return' will never be executed",
	"source": "sonarqube",
	"startLineNumber": 1420,
	"startColumn": 10,
	"endLineNumber": 1420,
	"endColumn": 11,
	"origin": "extHost1"
}]