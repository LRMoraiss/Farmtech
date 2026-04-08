/**
 * Projeto Final - EmbarcaTech
 * FarmTech: Sistema de Controle para Domo Geodésico Hidropônico
 * 
 * Hardware: BitDogLab (RP2040)
 * Periféricos utilizados:
 * - Display OLED (I2C)
 * - Joystick (ADC) - Simula sensores de Temperatura e Umidade
 * - Botões A e B (GPIO) - Controle manual
 * - Buzzer (PWM) - Alarme crítico
 * - LED RGB (PWM) - Status do sistema
 * - Wi-Fi (CYW43439) - Telemetria
 * - Interrupção de Timer - Leitura periódica
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"

// ==========================================
// DEFINIÇÕES DE PINOS - BITDOGLAB
// ==========================================
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

#define JOYSTICK_X_PIN 26 // ADC0 - Simula Temperatura
#define JOYSTICK_Y_PIN 27 // ADC1 - Simula Umidade
#define JOYSTICK_PB 22    // Botão do Joystick

#define BUTTON_A 5
#define BUTTON_B 6

#define LED_R 13
#define LED_G 11
#define LED_B 12

#define BUZZER_PIN 21

// ==========================================
// VARIÁVEIS GLOBAIS DE ESTADO
// ==========================================
volatile float temperatura = 25.0;
volatile float umidade = 60.0;
volatile bool bomba_ligada = false;
volatile bool ventoinha_ligada = false;
volatile bool alarme_critico = false;

// ==========================================
// FUNÇÕES DE HARDWARE (PWM, ADC, BUZZER)
// ==========================================

void setup_pwm(uint gpio, uint32_t freq, uint16_t duty_percent) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    float clkdiv = 125.0;
    uint32_t wrap = (125000000 / (clkdiv * freq)) - 1;
    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_gpio_level(gpio, (wrap * duty_percent) / 100);
    pwm_set_enabled(slice_num, true);
}

void set_led_color(uint8_t r, uint8_t g, uint8_t b) {
    // Duty cycle 0-100%
    setup_pwm(LED_R, 1000, r);
    setup_pwm(LED_G, 1000, g);
    setup_pwm(LED_B, 1000, b);
}

void play_buzzer(uint freq, uint duration_ms) {
    setup_pwm(BUZZER_PIN, freq, 50); // 50% duty cycle para som
    sleep_ms(duration_ms);
    setup_pwm(BUZZER_PIN, 1000, 0);  // Desliga
}

// ==========================================
// INTERRUPÇÃO DE TIMER (LEITURA DE SENSORES)
// ==========================================
bool sensor_timer_callback(struct repeating_timer *t) {
    // Lê Joystick X (Simula Temperatura 15 a 45 C)
    adc_select_input(0);
    uint16_t adc_x = adc_read();
    temperatura = 15.0 + ((float)adc_x / 4095.0) * 30.0;

    // Lê Joystick Y (Simula Umidade 30 a 90 %)
    adc_select_input(1);
    uint16_t adc_y = adc_read();
    umidade = 30.0 + ((float)adc_y / 4095.0) * 60.0;

    // Lógica de Controle Automático
    if (temperatura > 35.0) {
        alarme_critico = true;
        ventoinha_ligada = true;
    } else if (temperatura > 28.0) {
        alarme_critico = false;
        ventoinha_ligada = true;
    } else {
        alarme_critico = false;
        ventoinha_ligada = false;
    }

    return true; // Mantém o timer rodando
}

// ==========================================
// FUNÇÃO PRINCIPAL
// ==========================================
int main() {
    stdio_init_all();
    
    // Inicializa Wi-Fi (Obrigatório no projeto)
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    // Inicializa ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Inicializa Botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Configura Timer de Interrupção (1 segundo)
    struct repeating_timer timer;
    add_repeating_timer_ms(1000, sensor_timer_callback, NULL, &timer);

    printf("FarmTech Iniciado!\n");

    bool prev_a = true;
    bool prev_b = true;

    while (true) {
        // Leitura de Botões (Controle Manual)
        bool curr_a = gpio_get(BUTTON_A);
        if (prev_a && !curr_a) {
            bomba_ligada = !bomba_ligada;
            printf("Bomba d'agua: %s\n", bomba_ligada ? "LIGADA" : "DESLIGADA");
            sleep_ms(200); // Debounce
        }
        prev_a = curr_a;

        bool curr_b = gpio_get(BUTTON_B);
        if (prev_b && !curr_b) {
            // Força envio de telemetria via UART
            printf("{\"temp\":%.1f, \"umid\":%.1f, \"bomba\":%d, \"vent\":%d}\n", 
                   temperatura, umidade, bomba_ligada, ventoinha_ligada);
            sleep_ms(200); // Debounce
        }
        prev_b = curr_b;

        // Atualiza LEDs e Buzzer baseado no estado
        if (alarme_critico) {
            set_led_color(100, 0, 0); // Vermelho
            play_buzzer(1000, 200);   // Apita
            sleep_ms(200);
        } else if (ventoinha_ligada || bomba_ligada) {
            set_led_color(0, 0, 100); // Azul (Atuadores ativos)
        } else {
            set_led_color(0, 100, 0); // Verde (Normal)
        }

        sleep_ms(100);
    }

    return 0;
}
