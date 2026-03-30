#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

// Definições de pinos para a BitDogLab
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define LED_PIN 13 // LED Vermelho

volatile int button_a_count = 0;
volatile bool is_blinking = false;
volatile int blink_freq = 10; // Frequência inicial: 10 Hz
volatile uint32_t blink_start_time = 0;
volatile bool led_state = false;

// Callback do temporizador para piscar o LED
bool blink_timer_callback(struct repeating_timer *t) {
    if (is_blinking) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        // Verifica se já se passaram 10 segundos (10000 ms)
        if (current_time - blink_start_time >= 10000) {
            is_blinking = false;
            led_state = false;
            gpio_put(LED_PIN, 0);
            button_a_count = 0; // Reseta o contador para permitir nova contagem
        } else {
            // Alterna o estado do LED
            led_state = !led_state;
            gpio_put(LED_PIN, led_state);
        }
    }
    return true;
}

int main() {
    stdio_init_all();

    // Inicializa o LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Inicializa o Botão A com pull-up
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    // Inicializa o Botão B com pull-up
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    struct repeating_timer timer;
    bool timer_active = false;

    bool prev_a = true;
    bool prev_b = true;

    printf("Sistema Iniciado. Pressione o Botao A 5 vezes.\n");

    while (true) {
        bool curr_a = gpio_get(BUTTON_A_PIN);
        bool curr_b = gpio_get(BUTTON_B_PIN);

        // Detecta borda de descida do Botão A (pressionado)
        if (prev_a && !curr_a) {
            sleep_ms(50); // Debounce
            if (!gpio_get(BUTTON_A_PIN)) {
                if (!is_blinking) {
                    button_a_count++;
                    printf("Botao A pressionado: %d/5\n", button_a_count);
                    
                    if (button_a_count >= 5) {
                        printf("Iniciando pisca-pisca por 10s a %d Hz\n", blink_freq);
                        is_blinking = true;
                        blink_start_time = to_ms_since_boot(get_absolute_time());
                        
                        if (timer_active) cancel_repeating_timer(&timer);
                        
                        // Frequência f Hz -> Período T = 1/f segundos = 1000/f ms.
                        // Para piscar, inverte a cada T/2 ms.
                        int delay_ms = 1000 / (blink_freq * 2);
                        add_repeating_timer_ms(-delay_ms, blink_timer_callback, NULL, &timer);
                        timer_active = true;
                    }
                }
            }
        }
        prev_a = curr_a;

        // Detecta borda de descida do Botão B (pressionado)
        if (prev_b && !curr_b) {
            sleep_ms(50); // Debounce
            if (!gpio_get(BUTTON_B_PIN)) {
                // Alterna a frequência entre 10 Hz e 1 Hz
                blink_freq = (blink_freq == 10) ? 1 : 10;
                printf("Frequencia alterada para: %d Hz\n", blink_freq);
                
                // Se estiver piscando, atualiza o timer imediatamente
                if (is_blinking) {
                    if (timer_active) cancel_repeating_timer(&timer);
                    int delay_ms = 1000 / (blink_freq * 2);
                    add_repeating_timer_ms(-delay_ms, blink_timer_callback, NULL, &timer);
                    timer_active = true;
                }
            }
        }
        prev_b = curr_b;

        sleep_ms(10); // Pequeno delay para não sobrecarregar a CPU
    }

    return 0;
}
