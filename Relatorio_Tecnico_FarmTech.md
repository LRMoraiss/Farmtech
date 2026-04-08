# Relatório Técnico: FarmTech - Domo Geodésico Autônomo para o Sertão

**Autor:** Luciano Morais  
**Programa:** EmbarcaTech - Projeto Final  
**Data:** Abril de 2026  
**Plataforma:** BitDogLab v3 (Raspberry Pi Pico W - RP2040)

---

## a. Apresentação

O **FarmTech** é um sistema embarcado autônomo de controle ambiental projetado para cultivo hidropônico em domos geodésicos no Sertão Brasileiro. 

A agricultura no semiárido nordestino enfrenta desafios severos: escassez hídrica, altas temperaturas, baixa umidade relativa do ar e intensa radiação solar. O cultivo tradicional a céu aberto frequentemente resulta em perda de safra e uso ineficiente da pouca água disponível. 

O FarmTech resolve esse problema criando um microclima controlado dentro de um domo geodésico. O sistema atua como a Unidade Central de Controle, monitorando continuamente o ambiente e tomando decisões autônomas para manter condições ideais de cultivo. Ele protege as plantas do clima extremo e otimiza o uso de água através da hidroponia de circuito fechado, garantindo resiliência e eficiência na produção agrícola.

## b. Objetivos

### Objetivo Geral
Desenvolver um sistema embarcado de baixo custo e alta eficiência para automação de estufas hidropônicas no semiárido, utilizando a plataforma RP2040 (BitDogLab v3).

### Objetivos Específicos
- Monitorar em tempo real variáveis críticas: temperatura, umidade, luminosidade e nível de água.
- Controlar atuadores (ventilação, nebulização, iluminação e bombas) de forma autônoma baseada em setpoints pré-definidos.
- Fornecer interface local (Display OLED e Matriz de LEDs) para diagnóstico rápido.
- Transmitir dados de telemetria via comunicação serial/Wi-Fi para monitoramento remoto (IoT).
- Garantir resiliência do sistema com modos de fallback em caso de falha de sensores.

## c. Requisitos Funcionais

O sistema foi projetado para atender aos seguintes requisitos:
- **Leitura de Sensores**: Aquisição de dados analógicos (ADC) e digitais a cada 1 segundo.
- **Controle Autônomo**: Acionamento de relés baseado em regras lógicas de clima e hidratação.
- **Modo Manual/Auto**: Alternância de modo de operação via interrupção de botão (Botão A).
- **Interface Visual**: Exibição de status na Matriz de LEDs WS2812B e Display Gráfico OLED (I2C).
- **Alertas Críticos**: Acionamento de alarme sonoro (Buzzer) em condições extremas (Temp > 35°C ou Água < 5%).
- **Telemetria**: Envio de dados formatados em JSON via interface UART.

## d. Arquitetura de Hardware

A BitDogLab v3 atua como o cérebro do sistema, integrando os seguintes componentes:

- **Microcontrolador**: Raspberry Pi Pico W (RP2040) - Processador dual-core ARM Cortex-M0+, responsável por toda a lógica de controle.
- **Sensores (Entradas)**:
  - **DHT22 (GP4)**: Monitoramento digital de temperatura e umidade interna. *(Fallback implementado via Joystick ADC0/ADC1 para simulação/resiliência)*.
  - **LDR (GP28 - ADC2)**: Sensor analógico de luz ambiente para controle de fotossíntese.
  - **Sensor de Nível (GP29 - ADC3)**: Monitoramento analógico do reservatório de água/nutrientes.
  - **Botões (GP5/GP6)**: Entradas digitais com *pull-up* para controle de interface.
- **Atuadores e Interfaces (Saídas)**:
  - **Relés de Controle (GP8 a GP12)**: Controle de potência para Exaustor, Entrada de Ar, Nebulizador, Grow Lights e Bomba d'Água.
  - **Display OLED (GP14/GP15 - I2C1)**: Interface gráfica local para exibição de telemetria.
  - **Matriz WS2812B (GP7)**: Feedback visual rápido do status do sistema.
  - **Buzzer (GP13)**: Alertas sonoros de criticidade.
- **Comunicação**:
  - **Interface UART (GP0/GP1 - UART0)**: Transmissão de telemetria para módulos externos ou debug.

## e. Arquitetura do Firmware

O software embarcado foi desenvolvido em **C/C++ utilizando o Pico SDK**, estruturado em um loop infinito de controle (Padrão *Super Loop* com Interrupções):

1. **Inicialização (`setup_hardware`)**: Configuração de GPIOs, inicialização do ADC, I2C (Display), PIO (Matriz LED) e UART.
2. **Aquisição de Dados (`ler_sensores`)**: Leitura dos canais ADC e pinos digitais. Implementa lógica de *debounce* para botões.
3. **Processamento Lógico (`processar_decisoes`)**: O "Cérebro". Avalia os dados contra os *setpoints* (ex: TEMP_MAX = 28.0°C) e define o estado futuro dos atuadores.
4. **Atuação (`atualizar_atuadores`)**: Aplica os estados calculados aos pinos de saída (Relés, Buzzer, LEDs).
5. **Comunicação (`enviar_telemetria_wifi` / `atualizar_display_local`)**: Formata os dados em JSON e envia via UART, além de atualizar o display I2C.

## f. Fluxograma

```mermaid
graph TD
    A[Início] --> B[setup_hardware]
    B --> C[Loop Principal 1Hz]
    C --> D[ler_sensores]
    D --> E{Modo Auto?}
    E -- Sim --> F[processar_decisoes]
    E -- Não --> G[Controle Manual]
    F --> H[atualizar_atuadores]
    G --> H
    H --> I[enviar_telemetria_wifi]
    I --> J[atualizar_display_local]
    J --> C
```

## g. Indicação do uso de IA

Ferramentas de Inteligência Artificial (Manus AI / Google AntiGravity) foram utilizadas neste projeto para:
- Auxílio na estruturação do código C e resolução de conflitos de pinagem (ADC).
- Geração de documentação técnica (README e Relatório Técnico).
- Revisão de boas práticas de programação para sistemas embarcados (Pico SDK).
- O código lógico central e a arquitetura do sistema são de autoria própria, baseados nos requisitos do programa EmbarcaTech.

## h. Conclusão

O projeto FarmTech demonstrou com sucesso a viabilidade de um sistema embarcado de baixo custo para controle ambiental em estufas hidropônicas. A utilização da placa BitDogLab v3 (RP2040) provou ser robusta o suficiente para gerenciar múltiplos sensores analógicos e digitais, além de controlar atuadores de forma autônoma.

**Resultados Alcançados:**
- Leitura estável de sensores e atuação correta dos relés baseada na lógica de controle.
- Interface local funcional (Display OLED e Matriz de LEDs).
- Telemetria JSON formatada corretamente via UART.

**Dificuldades Enfrentadas:**
- Conflitos de pinagem do ADC na placa BitDogLab, resolvidos através de realocação de pinos (GP28/GP29).
- Implementação de *fallback* para sensores ausentes (uso do Joystick para simular temperatura e umidade).

**Melhorias Futuras:**
- Integração completa do módulo Wi-Fi (CYW43439) para envio direto de dados via MQTT.
- Implementação de um dashboard web para visualização remota da telemetria.
- Adição de sensores de pH e Condutividade Elétrica (EC) para controle preciso da solução nutritiva.
