#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// ============================================================================
// Configurações de Hardware - Placa BitDogLab (Raspberry Pi Pico)
// ============================================================================

// Pinos do Joystick (ADC)
#define JOYSTICK_X_PIN 26 // ADC0 - Simula Umidade
#define JOYSTICK_Y_PIN 27 // ADC1 - Simula Temperatura
#define JOYSTICK_SW_PIN 22 // Botão do Joystick (não usado nesta simulação)

// Pinos dos Botões
#define BUTTON_A_PIN 5 // Simula consumo de água
#define BUTTON_B_PIN 6 // Simula alternância Dia/Noite

// Pinos do Display OLED (I2C)
#define I2C_PORT i2c1
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15
#define OLED_ADDR 0x3C

// Pino da Matriz de LEDs WS2812B
#define LED_MATRIX_PIN 7
#define NUM_LEDS 25

// ============================================================================
// Variáveis de Estado da Simulação
// ============================================================================

// Valores simulados
float temperatura_simulada = 25.0; // Em graus Celsius
int umidade_simulada = 60;         // Em porcentagem (0-100%)
int nivel_agua_simulado = 100;     // Em porcentagem (0-100%)
int luz_simulada = 100;            // Em porcentagem (0-100%, 100=Dia, 0=Noite)

// Status dos Atuadores (0 = Desligado, 1 = Ligado)
int status_ventilacao = 0;
int status_nebulizador = 0;
int status_bomba_agua = 0;
int status_grow_lights = 0;

// Limites Ideais
const float TEMP_MAX = 28.0;
const int UMIDADE_MIN = 75;
const int NIVEL_AGUA_CRITICO = 20;
const int LUZ_MINIMA = 30;

// ============================================================================
// Funções de Inicialização
// ============================================================================

void setup_hardware() {
    stdio_init_all();

    // Inicializa ADC para o Joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Inicializa Botões com pull-up interno
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // Nota: A inicialização do I2C para o OLED e PIO para WS2812B 
    // dependeria de bibliotecas específicas (ex: ssd1306.h e ws2812.pio.h)
    // que seriam incluídas no projeto completo.
}

// ============================================================================
// Funções de Leitura (Simulação de Sensores)
// ============================================================================

void ler_sensores_simulados() {
    // 1. Ler Joystick Y para simular Temperatura (Mapear ADC 0-4095 para 15-40°C)
    adc_select_input(1); // ADC1 (Pino 27)
    uint16_t adc_y = adc_read();
    temperatura_simulada = 15.0 + ((float)adc_y / 4095.0) * 25.0;

    // 2. Ler Joystick X para simular Umidade (Mapear ADC 0-4095 para 0-100%)
    adc_select_input(0); // ADC0 (Pino 26)
    uint16_t adc_x = adc_read();
    umidade_simulada = (int)(((float)adc_x / 4095.0) * 100.0);

    // 3. Ler Botão A para simular consumo de água
    static bool last_button_a = true;
    bool current_button_a = gpio_get(BUTTON_A_PIN);
    if (last_button_a && !current_button_a) { // Detecta borda de descida (pressionado)
        nivel_agua_simulado -= 10;
        if (nivel_agua_simulado < 0) nivel_agua_simulado = 0;
        sleep_ms(50); // Debounce simples
    }
    last_button_a = current_button_a;

    // 4. Ler Botão B para simular alternância Dia/Noite
    static bool last_button_b = true;
    bool current_button_b = gpio_get(BUTTON_B_PIN);
    if (last_button_b && !current_button_b) { // Detecta borda de descida (pressionado)
        if (luz_simulada == 100) {
            luz_simulada = 0; // Muda para Noite
        } else {
            luz_simulada = 100; // Muda para Dia
        }
        sleep_ms(50); // Debounce simples
    }
    last_button_b = current_button_b;
}

// ============================================================================
// Lógica de Decisão (O Cérebro)
// ============================================================================

void processar_decisoes() {
    // Gestão de Clima: Temperatura
    if (temperatura_simulada > TEMP_MAX) {
        status_ventilacao = 1; // Liga ventilação
    } else {
        status_ventilacao = 0; // Desliga ventilação
    }

    // Gestão de Clima: Umidade
    if (umidade_simulada < UMIDADE_MIN) {
        status_nebulizador = 1; // Liga nebulizador
    } else {
        status_nebulizador = 0; // Desliga nebulizador
    }

    // Gestão Hídrica: Nível de Água
    if (nivel_agua_simulado < NIVEL_AGUA_CRITICO) {
        status_bomba_agua = 1; // Liga bomba para reabastecer
        // Simula o reabastecimento automático após ligar a bomba
        nivel_agua_simulado = 100; 
    } else {
        status_bomba_agua = 0; // Desliga bomba
    }

    // Gestão de Iluminação: Luz Ambiente
    if (luz_simulada < LUZ_MINIMA) {
        status_grow_lights = 1; // Liga luzes de cultivo
    } else {
        status_grow_lights = 0; // Desliga luzes de cultivo
    }
}

// ============================================================================
// Funções de Feedback Visual (Display e LEDs)
// ============================================================================

void atualizar_display_oled() {
    // Em um ambiente real, usaríamos funções da biblioteca SSD1306
    // Exemplo conceitual do que seria exibido:
    printf("\n--- Status FarmTech ---\n");
    printf("Temp: %.1f C | Umid: %d%%\n", temperatura_simulada, umidade_simulada);
    printf("Agua: %d%%   | Luz: %d%%\n", nivel_agua_simulado, luz_simulada);
    printf("-----------------------\n");
    printf("Ventilador: %s\n", status_ventilacao ? "LIGADO" : "DESLIGADO");
    printf("Nebulizador: %s\n", status_nebulizador ? "LIGADO" : "DESLIGADO");
    printf("Bomba Agua: %s\n", status_bomba_agua ? "LIGADA" : "DESLIGADA");
    printf("Grow Lights: %s\n", status_grow_lights ? "LIGADAS" : "DESLIGADAS");
    printf("=======================\n");
}

void atualizar_matriz_leds() {
    // Em um ambiente real, usaríamos PIO para controlar os WS2812B
    // Aqui definimos as cores conceitualmente para os ícones
    
    // Se Grow Lights ligadas -> Desenha um "Sol" (Amarelo)
    // Se Nebulizador ligado -> Desenha uma "Gota" (Azul)
    // Se Ventilação ligada -> Desenha "Vento" (Branco)
    // Se Bomba ligada -> Pisca em Vermelho/Azul
    
    // (A implementação real exigiria mapear o array de 25 LEDs)
}

// ============================================================================
// Loop Principal
// ============================================================================

int main() {
    setup_hardware();
    
    printf("Iniciando Sistema Autonomo FarmTech...\n");
    sleep_ms(2000);

    while (true) {
        // 1. Ler (Simular entradas)
        ler_sensores_simulados();
        
        // 2. Decidir (Processar lógica)
        processar_decisoes();
        
        // 3. Agir (Atualizar saídas visuais)
        atualizar_display_oled();
        atualizar_matriz_leds();
        
        // Aguarda antes do próximo ciclo
        sleep_ms(1000); // Atualiza a cada 1 segundo
    }

    return 0;
}
