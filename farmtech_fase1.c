// ============================================================================
// FarmTech - FASE 1: O Cérebro na Bancada
// ============================================================================
// Objetivo: Validar 100% da lógica de controle usando apenas os componentes
//           da placa BitDogLab. Sem sensores externos. Sem relés. Pura lógica.
//
// Entradas (Simuladas):
//   - Joystick Y (ADC1) → Temperatura simulada (15°C a 40°C)
//   - Joystick X (ADC0) → Umidade simulada (0% a 100%)
//   - Botão A (GP5)     → Reduz nível de água (-10% por pressionamento)
//   - Botão B (GP6)     → Alterna Dia (100%) / Noite (0%)
//
// Saídas (Feedback Visual):
//   - Display OLED      → Valores atuais + status dos sistemas
//   - Matriz LEDs 5x5   → Ícones: Sol, Gota, Vento, Bomba
//   - Buzzer            → Alerta crítico
// ============================================================================

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// ============================================================================
// Pinout - BitDogLab v3
// ============================================================================

// Entradas de Simulação
#define JOYSTICK_X_PIN  26   // ADC0 → Umidade simulada
#define JOYSTICK_Y_PIN  27   // ADC1 → Temperatura simulada
#define BUTTON_A_PIN     5   // Simula consumo de água (-10%)
#define BUTTON_B_PIN     6   // Alterna Dia / Noite

// Interface Visual
#define OLED_SDA_PIN    14   // I2C1 SDA → Display OLED 128x64
#define OLED_SCL_PIN    15   // I2C1 SCL → Display OLED 128x64
#define LED_MATRIX_PIN   7   // PIO → Matriz WS2812B 5x5
#define BUZZER_PIN      21   // Buzzer passivo

// ============================================================================
// Setpoints (Limites Ideais do Domo)
// ============================================================================

#define TEMP_MAX          28.0f  // °C → acima disso, liga ventilação
#define TEMP_CRITICA      35.0f  // °C → alerta crítico
#define UMIDADE_MIN          75  // %  → abaixo disso, liga nebulizador
#define NIVEL_AGUA_CRITICO   20  // %  → abaixo disso, liga bomba
#define LUZ_MINIMA           30  // %  → abaixo disso, liga grow lights

// ============================================================================
// Variáveis de Estado
// ============================================================================

float temperatura = 25.0f;
int   umidade     = 60;
int   nivel_agua  = 100;
int   luz         = 100;   // 100 = Dia, 0 = Noite

// Status dos Atuadores (apenas lógico nesta fase — sem relés físicos)
bool vent_ligada    = false;
bool nebul_ligado   = false;
bool bomba_ligada   = false;
bool grow_ligada    = false;
bool alerta_critico = false;

// ============================================================================
// Inicialização de Hardware
// ============================================================================

void setup() {
    stdio_init_all();

    // ADC (Joystick)
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Botões com pull-up
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // Buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    // I2C para OLED
    i2c_init(i2c1, 400000);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);

    // Nota: PIO para WS2812B e biblioteca SSD1306 são inicializados
    // nas suas respectivas funções de setup (ws2812_init, oled_init).
}

// ============================================================================
// Leitura das Entradas (Simulação via Joystick e Botões)
// ============================================================================

void ler_entradas() {
    // Joystick Y → Temperatura (15°C a 40°C)
    adc_select_input(1);
    uint16_t raw_y = adc_read();
    temperatura = 15.0f + ((float)raw_y / 4095.0f) * 25.0f;

    // Joystick X → Umidade (0% a 100%)
    adc_select_input(0);
    uint16_t raw_x = adc_read();
    umidade = (int)(((float)raw_x / 4095.0f) * 100.0f);

    // Botão A → Reduz nível de água (borda de descida)
    static bool prev_a = true;
    bool curr_a = gpio_get(BUTTON_A_PIN);
    if (prev_a && !curr_a) {
        nivel_agua -= 10;
        if (nivel_agua < 0) nivel_agua = 0;
        sleep_ms(50); // debounce
    }
    prev_a = curr_a;

    // Botão B → Alterna Dia / Noite (borda de descida)
    static bool prev_b = true;
    bool curr_b = gpio_get(BUTTON_B_PIN);
    if (prev_b && !curr_b) {
        luz = (luz == 100) ? 0 : 100;
        sleep_ms(50); // debounce
    }
    prev_b = curr_b;
}

// ============================================================================
// Lógica de Decisão (O Cérebro)
// ============================================================================

void processar_logica() {
    // Gestão de Clima: Temperatura
    vent_ligada = (temperatura > TEMP_MAX);

    // Gestão de Clima: Umidade
    nebul_ligado = (umidade < UMIDADE_MIN);

    // Gestão Hídrica: Nível de Água
    if (nivel_agua < NIVEL_AGUA_CRITICO) {
        bomba_ligada = true;
        nivel_agua = 100; // simula reabastecimento automático
    } else {
        bomba_ligada = false;
    }

    // Gestão de Iluminação: Luz Ambiente
    grow_ligada = (luz < LUZ_MINIMA);

    // Alerta Crítico
    alerta_critico = (temperatura > TEMP_CRITICA || nivel_agua < 5);
}

// ============================================================================
// Saídas Visuais (Display OLED + Matriz de LEDs + Buzzer)
// ============================================================================

void atualizar_display() {
    // Representação do que será exibido no OLED 128x64.
    // Em produção: usar biblioteca ssd1306 para desenhar as linhas abaixo.
    printf("\033[2J\033[H");
    printf("====== FARMTECH  F1 ======\n");
    printf("Temp : %5.1f C  Umid : %3d%%\n", temperatura, umidade);
    printf("Agua : %3d%%     Luz  : %3d%%\n", nivel_agua, luz);
    printf("--------------------------\n");
    printf("Vent  : %-8s Nebul : %s\n",
           vent_ligada  ? "[ON]" : "[OFF]",
           nebul_ligado ? "[ON]" : "[OFF]");
    printf("Bomba : %-8s Grow  : %s\n",
           bomba_ligada ? "[ON]" : "[OFF]",
           grow_ligada  ? "[ON]" : "[OFF]");
    if (alerta_critico)
        printf(">>> ALERTA CRITICO! <<<\n");
    printf("==========================\n");
}

void atualizar_leds() {
    // Mapeamento de ícones na Matriz WS2812B 5x5.
    // Em produção: usar PIO + array uint32_t[25] com cores GRB.
    //
    // Ícones definidos (linha x coluna):
    //   grow_ligada  → Sol amarelo   (pixels centrais)
    //   nebul_ligado → Gota azul     (coluna central, linhas 1-4)
    //   vent_ligada  → Seta branca   (linha 0, todos os pixels)
    //   bomba_ligada → Pisca vermelho (borda do grid)
    //   alerta_critico → Todos vermelhos
}

void atualizar_buzzer() {
    gpio_put(BUZZER_PIN, alerta_critico ? 1 : 0);
}

// ============================================================================
// Loop Principal
// ============================================================================

int main() {
    setup();

    printf("FarmTech - Fase 1 iniciada.\n");
    sleep_ms(1000);

    while (true) {
        ler_entradas();       // 1. LER
        processar_logica();   // 2. DECIDIR
        atualizar_display();  // 3. AGIR (OLED)
        atualizar_leds();     // 3. AGIR (LEDs)
        atualizar_buzzer();   // 3. AGIR (Buzzer)
        sleep_ms(1000);
    }

    return 0;
}
