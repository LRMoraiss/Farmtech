#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

// Pinos dos LEDs RGB na BitDogLab
#define LED_R_PIN 13
#define LED_B_PIN 12
#define LED_G_PIN 11

volatile uint16_t red_duty_percent = 5;

// Callback do temporizador (Interrupção) para atualizar o duty cycle a cada 2 segundos
bool repeating_timer_callback(struct repeating_timer *t) {
    red_duty_percent += 5;
    if (red_duty_percent > 100) {
        red_duty_percent = 5;
    }
    
    uint slice_num_r = pwm_gpio_to_slice_num(LED_R_PIN);
    uint16_t wrap_r = pwm_hw->slice[slice_num_r].top;
    pwm_set_gpio_level(LED_R_PIN, (wrap_r * red_duty_percent) / 100);
    
    printf("Duty cycle LED Vermelho atualizado para: %d%%\n", red_duty_percent);
    return true;
}

// Função auxiliar para configurar o PWM
void setup_pwm(uint gpio, uint32_t freq, uint16_t duty_percent) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    
    // Frequência do clock do sistema (padrão 125 MHz)
    uint32_t clock_freq = 125000000;
    
    // Calcula o divisor e o wrap para atingir a frequência desejada
    // freq = clock_freq / (clkdiv * (wrap + 1))
    // Fixando clkdiv em 125.0 para simplificar os cálculos
    float clkdiv = 125.0;
    uint32_t wrap = (clock_freq / (clkdiv * freq)) - 1;
    
    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap);
    
    pwm_set_gpio_level(gpio, (wrap * duty_percent) / 100);
    pwm_set_enabled(slice_num, true);
}

int main() {
    stdio_init_all();
    
    // a) LED vermelho com PWM de 1kHz, duty cycle inicial de 5%
    setup_pwm(LED_R_PIN, 1000, 5);
    
    // b) LED azul com PWM de 10kHz (duty cycle fixo em 50% como exemplo)
    setup_pwm(LED_B_PIN, 10000, 50);
    
    // LED verde desligado (duty cycle 0%)
    setup_pwm(LED_G_PIN, 1000, 0);
    
    struct repeating_timer timer;
    // Configura interrupção de tempo a cada 2000 ms (2 segundos)
    add_repeating_timer_ms(2000, repeating_timer_callback, NULL, &timer);
    
    printf("Sistema PWM Iniciado.\n");
    
    while (true) {
        tight_loop_contents();
    }
    
    return 0;
}
