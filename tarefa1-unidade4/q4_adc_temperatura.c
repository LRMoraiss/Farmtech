#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

int main() {
    stdio_init_all();
    
    // Inicializa o módulo ADC
    adc_init();
    
    // Habilita o sensor de temperatura interno (conectado ao canal 4 do ADC)
    adc_set_temp_sensor_enabled(true);
    
    // Seleciona o canal 4 (sensor de temperatura)
    adc_select_input(4);
    
    printf("Leitura de Temperatura Interna (Fahrenheit)\n");
    
    while (true) {
        // Lê o valor digital do ADC (12 bits: 0 a 4095)
        uint16_t raw = adc_read();
        
        // Converte o valor digital para tensão (considerando VREF de 3.3V)
        const float conversion_factor = 3.3f / (1 << 12);
        float voltage = raw * conversion_factor;
        
        // Calcula a temperatura em Celsius
        // O sensor tem uma tensão típica de 0.706V a 27 graus Celsius
        // com uma inclinação de -1.721mV (0.001721V) por grau.
        float temp_celsius = 27.0f - (voltage - 0.706f) / 0.001721f;
        
        // Converte a temperatura de Celsius para Fahrenheit
        float temp_fahrenheit = (temp_celsius * 9.0f / 5.0f) + 32.0f;
        
        printf("Tensao: %.3f V | Temperatura: %.2f F\n", voltage, temp_fahrenheit);
        
        sleep_ms(1000);
    }
    
    return 0;
}
