#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"

// ============================================================================
// Configurações de Hardware - FarmTech Domo Geodésico (BitDogLab v3)
// ============================================================================

// --- Sensores (Entradas) ---
#define DHT22_PIN 4          // Temperatura e Umidade
#define LDR_PIN 28           // ADC2 - Sensor de Luz Ambiente
#define WATER_LEVEL_PIN 29   // ADC3 - Sensor de Nível d'Água

// --- Atuadores (Saídas via Relés) ---
#define RELAY_VENT_PIN 8     // Exaustão de ar quente
#define RELAY_INTAKE_PIN 9   // Entrada de ar frio
#define RELAY_MIST_PIN 10    // Sistema de nebulização
#define RELAY_LIGHTS_PIN 11  // Grow Lights (LEDs de cultivo)
#define RELAY_PUMP_PIN 12    // Bomba de solução nutritiva

// --- Comunicação e Telemetria ---
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// --- Interface Local (BitDogLab) ---
#define BUTTON_A_PIN 5       // Modo Manual/Auto
#define BUTTON_B_PIN 6       // Forçar Bomba
#define BUZZER_PIN 13        // Alertas sonoros
#define LED_MATRIX_PIN 7     // Matriz WS2812B
#define I2C_PORT i2c1
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

// --- Simulação (Joystick) ---
#define JOYSTICK_X_PIN 26    // ADC0 - Simula Umidade (se sensor falhar)
#define JOYSTICK_Y_PIN 27    // ADC1 - Simula Temp (se sensor falhar)

// ============================================================================
// Variáveis de Estado do Sistema
// ============================================================================

// Valores dos Sensores
float temperatura_atual = 25.0; // Em graus Celsius
int umidade_atual = 60;         // Em porcentagem (0-100%)
int nivel_agua_atual = 100;     // Em porcentagem (0-100%)
int luz_ambiente_atual = 100;   // Em porcentagem (0-100%)

// Status dos Atuadores (0 = Desligado, 1 = Ligado)
bool status_vent_exaustao = false;
bool status_vent_entrada = false;
bool status_nebulizador = false;
bool status_bomba_agua = false;
bool status_grow_lights = false;

// Modos de Operação
bool modo_automatico = true;
bool alerta_critico = false;

// Limites Ideais (Setpoints para o Sertão)
const float TEMP_MAX = 28.0;
const float TEMP_CRITICA = 35.0;
const int UMIDADE_MIN = 75;
const int NIVEL_AGUA_MIN = 20;
const int LUZ_MINIMA = 30;

// ============================================================================
// Funções de Inicialização
// ============================================================================

void setup_hardware() {
    stdio_init_all();

    // 1. Inicializa ADC para Sensores Analógicos
    adc_init();
    adc_gpio_init(LDR_PIN);
    adc_gpio_init(WATER_LEVEL_PIN);
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // 2. Inicializa Pinos Digitais (Sensores e Botões)
    gpio_init(DHT22_PIN);
    gpio_set_dir(DHT22_PIN, GPIO_IN);
    gpio_pull_up(DHT22_PIN);

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // 3. Inicializa Relés (Atuadores)
    uint reles[] = {RELAY_VENT_PIN, RELAY_INTAKE_PIN, RELAY_MIST_PIN, RELAY_LIGHTS_PIN, RELAY_PUMP_PIN};
    for (int i = 0; i < 5; i++) {
        gpio_init(reles[i]);
        gpio_set_dir(reles[i], GPIO_OUT);
        gpio_put(reles[i], 0); // Garante que começam desligados
    }

    // 4. Inicializa Buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    // 5. Inicializa UART (Módulo Wi-Fi)
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

// ============================================================================
// Funções de Leitura de Sensores
// ============================================================================

void ler_sensores() {
    // Em um ambiente real, leríamos o DHT22 aqui.
    // Para a simulação/fallback, usamos o Joystick Y (ADC1)
    adc_select_input(1); 
    uint16_t adc_y = adc_read();
    temperatura_atual = 15.0 + ((float)adc_y / 4095.0) * 25.0; // 15 a 40°C

    // Simulação/fallback de Umidade via Joystick X (ADC0)
    adc_select_input(0); 
    uint16_t adc_x = adc_read();
    umidade_atual = (int)(((float)adc_x / 4095.0) * 100.0);

    // Leitura do Sensor de Luz (LDR - ADC2)
    adc_select_input(2);
    uint16_t adc_ldr = adc_read();
    luz_ambiente_atual = 100 - (int)(((float)adc_ldr / 4095.0) * 100.0); // Invertido: mais luz = menor resistência

    // Leitura do Sensor de Nível de Água (ADC3)
    adc_select_input(3);
    uint16_t adc_water = adc_read();
    nivel_agua_atual = (int)(((float)adc_water / 4095.0) * 100.0);

    // Verifica Botões de Controle Manual
    static bool last_button_a = true;
    bool current_button_a = gpio_get(BUTTON_A_PIN);
    if (last_button_a && !current_button_a) {
        modo_automatico = !modo_automatico; // Alterna modo Auto/Manual
        sleep_ms(50); // Debounce
    }
    last_button_a = current_button_a;
}

// ============================================================================
// Lógica de Decisão (O Cérebro do Domo)
// ============================================================================

void processar_decisoes() {
    if (!modo_automatico) return; // Se manual, ignora lógica automática

    // 1. Gestão de Clima (Temperatura e Umidade)
    if (temperatura_atual > TEMP_MAX) {
        status_vent_exaustao = true; // Tira ar quente
        status_vent_entrada = true;  // Puxa ar fresco
        
        // Se muito quente e seco, liga nebulizador para resfriamento evaporativo
        if (umidade_atual < 85) {
            status_nebulizador = true;
        } else {
            status_nebulizador = false;
        }
    } else {
        status_vent_exaustao = false;
        status_vent_entrada = false;
        
        // Controle normal de umidade
        if (umidade_atual < UMIDADE_MIN) {
            status_nebulizador = true;
        } else {
            status_nebulizador = false;
        }
    }

    // 2. Gestão Hídrica (Solução Nutritiva)
    if (nivel_agua_atual < NIVEL_AGUA_MIN) {
        status_bomba_agua = true; // Liga bomba para reabastecer as torres
    } else if (nivel_agua_atual > 90) {
        status_bomba_agua = false; // Desliga quando cheio
    }

    // 3. Gestão de Iluminação (Fotossíntese)
    if (luz_ambiente_atual < LUZ_MINIMA) {
        status_grow_lights = true; // Liga luzes suplementares
    } else {
        status_grow_lights = false;
    }

    // 4. Verificação de Alertas Críticos
    if (temperatura_atual > TEMP_CRITICA || nivel_agua_atual < 5) {
        alerta_critico = true;
    } else {
        alerta_critico = false;
    }
}

// ============================================================================
// Funções de Ação (Atuadores e Telemetria)
// ============================================================================

void atualizar_atuadores() {
    gpio_put(RELAY_VENT_PIN, status_vent_exaustao);
    gpio_put(RELAY_INTAKE_PIN, status_vent_entrada);
    gpio_put(RELAY_MIST_PIN, status_nebulizador);
    gpio_put(RELAY_LIGHTS_PIN, status_grow_lights);
    gpio_put(RELAY_PUMP_PIN, status_bomba_agua);
    
    // Alarme sonoro se crítico
    if (alerta_critico) {
        gpio_put(BUZZER_PIN, 1);
    } else {
        gpio_put(BUZZER_PIN, 0);
    }
}

void enviar_telemetria_wifi() {
    // Formata pacote JSON para enviar via UART para o módulo Wi-Fi (ESP8266/ESP32)
    char json_payload[256];
    snprintf(json_payload, sizeof(json_payload), 
        "{\"temp\":%.1f,\"umid\":%d,\"agua\":%d,\"luz\":%d,\"vent\":%d,\"mist\":%d,\"pump\":%d,\"grow\":%d,\"auto\":%d}\n",
        temperatura_atual, umidade_atual, nivel_agua_atual, luz_ambiente_atual,
        status_vent_exaustao, status_nebulizador, status_bomba_agua, status_grow_lights, modo_automatico);
    
    uart_puts(UART_ID, json_payload);
}

void atualizar_display_local() {
    // Feedback no console serial (simulando o display OLED)
    printf("\033[2J\033[H"); // Limpa tela do terminal
    printf("=== FARMTECH DOMO GEODESICO ===\n");
    printf("Modo: %s | Status: %s\n", 
           modo_automatico ? "AUTO" : "MANUAL",
           alerta_critico ? "CRITICO!" : "OK");
    printf("-------------------------------\n");
    printf("Temp: %.1f C   | Umid: %d%%\n", temperatura_atual, umidade_atual);
    printf("Agua: %d%%      | Luz:  %d%%\n", nivel_agua_atual, luz_ambiente_atual);
    printf("-------------------------------\n");
    printf("Exaustao: %s | Entrada: %s\n", status_vent_exaustao ? "ON " : "OFF", status_vent_entrada ? "ON " : "OFF");
    printf("Nebuliza: %s | Bomba:   %s\n", status_nebulizador ? "ON " : "OFF", status_bomba_agua ? "ON " : "OFF");
    printf("Grow LED: %s |\n", status_grow_lights ? "ON " : "OFF");
    printf("===============================\n");
}

// ============================================================================
// Loop Principal
// ============================================================================

int main() {
    setup_hardware();
    
    printf("Iniciando Sistema Autonomo FarmTech (Domo Geodesico)...\n");
    sleep_ms(2000);

    while (true) {
        // 1. LER (Sensores)
        ler_sensores();
        
        // 2. DECIDIR (Lógica)
        processar_decisoes();
        
        // 3. AGIR (Atuadores)
        atualizar_atuadores();
        
        // 4. COMUNICAR (Telemetria e Display)
        enviar_telemetria_wifi();
        atualizar_display_local();
        
        // Ciclo de 1 segundo
        sleep_ms(1000); 
    }

    return 0;
}
