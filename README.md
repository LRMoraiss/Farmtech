# FarmTech: Sistema Autônomo de Gestão de Ambiente

## Introdução

O **FarmTech** é um sistema autônomo de controle ambiental projetado para cultivo hidropônico em ambientes controlados (domos, estufas inteligentes). Implementado na placa **BitDogLab** (baseada em Raspberry Pi Pico), o sistema funciona como um "cérebro" que monitora continuamente o ambiente e toma decisões autônomas para manter condições ideais de cultivo.

Este projeto é desenvolvido como parte da iniciativa **EmbarcaTech** e demonstra conceitos de sistemas embarcados, programação em C, e lógica de controle em tempo real.

## Características Principais

### 1. Gestão de Clima

O sistema monitora e controla temperatura e umidade do ambiente:

- **Sensoriamento**: Lê temperatura e umidade via joystick analógico (simulado)
- **Decisão**: Compara valores com limites ideais (Temp máx: 28°C, Umidade mín: 75%)
- **Ação**: Liga ventilador se temperatura exceder limite; liga nebulizador se umidade cair abaixo do ideal

### 2. Gestão Hídrica

Garante que o sistema de irrigação nunca fique sem água:

- **Sensoriamento**: Monitora nível do reservatório (simulado via botão)
- **Decisão**: Verifica se nível está crítico (< 20%)
- **Ação**: Liga bomba d'água para reabastecer automaticamente

### 3. Gestão de Iluminação

Fornece luz artificial quando necessário para fotossíntese:

- **Sensoriamento**: Detecta luz ambiente (simulado via botão Dia/Noite)
- **Decisão**: Verifica se luz está insuficiente (< 30%)
- **Ação**: Liga luzes de cultivo (Grow Lights) durante a noite

## Arquitetura do Sistema

### Ciclo de Operação (Ler → Decidir → Agir)

```
┌──────────────────────────────────────────────────────────┐
│                   CICLO CONTÍNUO (1s)                    │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  1️⃣  LER (Sensores Simulados)                            │
│      ├─ Joystick Y → Temperatura (15-40°C)              │
│      ├─ Joystick X → Umidade (0-100%)                   │
│      ├─ Botão A → Consumo de Água (-10%)                │
│      └─ Botão B → Alternância Dia/Noite                 │
│                                                          │
│  2️⃣  DECIDIR (Lógica de Controle)                        │
│      ├─ IF Temp > 28°C THEN Ventilador ON               │
│      ├─ IF Umidade < 75% THEN Nebulizador ON            │
│      ├─ IF Água < 20% THEN Bomba ON                     │
│      └─ IF Luz < 30% THEN Grow Lights ON                │
│                                                          │
│  3️⃣  AGIR (Atualizar Saídas)                             │
│      ├─ Display OLED: Mostrar valores atuais            │
│      └─ Matriz LEDs: Ícones de status                   │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### Mapeamento de Hardware

| Função | Pino GPIO | Tipo | Descrição |
|--------|-----------|------|-----------|
| Temperatura | GP27 (ADC1) | Entrada | Joystick Y (analógico) |
| Umidade | GP26 (ADC0) | Entrada | Joystick X (analógico) |
| Água | GP5 | Entrada | Botão A (digital) |
| Luz | GP6 | Entrada | Botão B (digital) |
| Display | GP14/GP15 | I2C | OLED 128×64 |
| LEDs | GP7 | PIO | Matriz WS2812B 5×5 |

## Limites de Operação (Setpoints)

| Parâmetro | Limite | Ação |
|-----------|--------|------|
| Temperatura | > 28°C | Ligar Ventilador |
| Umidade | < 75% | Ligar Nebulizador |
| Nível de Água | < 20% | Ligar Bomba |
| Luz Ambiente | < 30% | Ligar Grow Lights |

## Estrutura do Código

### Arquivo Principal: `farmtech_sim.c`

```c
// Configurações de Hardware
#define JOYSTICK_X_PIN 26    // Umidade
#define JOYSTICK_Y_PIN 27    // Temperatura
#define BUTTON_A_PIN 5       // Água
#define BUTTON_B_PIN 6       // Luz

// Variáveis de Estado
float temperatura_simulada = 25.0;
int umidade_simulada = 60;
int nivel_agua_simulado = 100;
int luz_simulada = 100;

// Status dos Atuadores
int status_ventilacao = 0;
int status_nebulizador = 0;
int status_bomba_agua = 0;
int status_grow_lights = 0;

// Funções Principais
void ler_sensores_simulados()    // Lê entradas
void processar_decisoes()         // Aplica lógica
void atualizar_display_oled()     // Mostra valores
void atualizar_matriz_leds()      // Mostra ícones
```

### Fluxo de Funções

```
main()
  ├─ setup_hardware()           // Inicializa GPIO, ADC, I2C
  └─ while(true)
     ├─ ler_sensores_simulados()
     ├─ processar_decisoes()
     ├─ atualizar_display_oled()
     ├─ atualizar_matriz_leds()
     └─ sleep_ms(1000)
```

## Compilação e Deployment

### Pré-requisitos

- Raspberry Pi Pico ou BitDogLab
- Pico SDK instalado
- CMake 3.13+
- Compilador ARM GCC

### Compilação

```bash
# Criar diretório de build
mkdir build
cd build

# Configurar e compilar
cmake ..
make

# Resultado: farmtech_sim.uf2
```

### Upload na Placa

1. Conecte a placa BitDogLab via USB
2. Pressione o botão BOOTSEL enquanto conecta (ou durante o upload)
3. A placa aparecerá como unidade de armazenamento
4. Copie o arquivo `farmtech_sim.uf2` para a unidade

## Simulação no Wokwi/AntiGravity

### Arquivo: `diagram.json`

Define a simulação com os seguintes componentes:

- **Raspberry Pi Pico**: Microcontrolador principal
- **Joystick Analógico**: Simula temperatura e umidade
- **Botões A e B**: Simulam consumo de água e alternância dia/noite
- **Display OLED**: Mostra valores em tempo real
- **Matriz de LEDs 5×5**: Exibe ícones de status

### Como Usar

1. Acesse [Wokwi](https://wokwi.com/)
2. Crie novo projeto com Raspberry Pi Pico
3. Importe o arquivo `diagram.json`
4. Carregue o código compilado ou use o simulador online

## Testes e Validação

### Teste 1: Temperatura

**Procedimento:**
1. Mova o Joystick para CIMA (aumenta temperatura simulada)
2. Observe o Display OLED

**Resultado Esperado:**
- Quando Temperatura > 28°C → "Ventilador: LIGADO"
- Quando Temperatura ≤ 28°C → "Ventilador: DESLIGADO"

### Teste 2: Umidade

**Procedimento:**
1. Mova o Joystick para ESQUERDA (diminui umidade simulada)
2. Observe o Display OLED

**Resultado Esperado:**
- Quando Umidade < 75% → "Nebulizador: LIGADO"
- Quando Umidade ≥ 75% → "Nebulizador: DESLIGADO"

### Teste 3: Nível de Água

**Procedimento:**
1. Pressione o Botão A repetidamente (cada pressionamento = -10%)
2. Observe o Display OLED

**Resultado Esperado:**
- Quando Água < 20% → "Bomba Agua: LIGADA"
- Nível volta para 100% automaticamente
- "Bomba Agua: DESLIGADA"

### Teste 4: Iluminação

**Procedimento:**
1. Pressione o Botão B para alternar Dia/Noite
2. Observe o Display OLED

**Resultado Esperado:**
- Noite (Luz = 0%) → "Grow Lights: LIGADAS"
- Dia (Luz = 100%) → "Grow Lights: DESLIGADAS"

## Extensões Futuras

### Curto Prazo

1. **Integração com Sensores Reais**
   - DHT22: Temperatura e umidade
   - Sensor capacitivo: Nível de água
   - LDR: Luz ambiente

2. **Controle de Atuadores**
   - Relés para ventilador, bomba, nebulizador
   - PWM para controle de intensidade de luzes

3. **Persistência de Dados**
   - EEPROM para armazenar histórico
   - SD card para logs detalhados

### Médio Prazo

1. **Interface Web**
   - Dashboard com WebSocket
   - Monitoramento remoto via WiFi (ESP32)
   - Alertas por email/SMS

2. **Machine Learning**
   - Otimização automática de setpoints
   - Previsão de falhas

3. **Múltiplos Domos**
   - Rede de sensores distribuídos
   - Sincronização entre sistemas

### Longo Prazo

1. **Integração com Nuvem**
   - AWS IoT, Google Cloud IoT
   - Big Data analytics

2. **Automação Agrícola**
   - Integração com sistemas de irrigação
   - Controle de fertilizantes
   - Otimização de rendimento

## Referências e Recursos

### Documentação Oficial

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [BitDogLab GitHub](https://github.com/BitDogLab/BitDogLab)
- [Google AntiGravity](https://antigravity.google/)

### Tutoriais

- [Wokwi Simulator](https://wokwi.com/)
- [EmbarcaTech Project](https://www.embarcatech.gov.br/)
- [Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)

### Componentes Utilizados

- **Microcontrolador**: Raspberry Pi Pico (RP2040)
- **Display**: OLED 128×64 (I2C)
- **LEDs**: Matriz WS2812B 5×5 (RGB endereçável)
- **Entrada**: Joystick analógico + 2 botões

## Licença

Este projeto é licenciado sob a licença MIT. Veja o arquivo LICENSE para detalhes.

## Autores

- **Desenvolvido por**: Manus AI
- **Projeto**: EmbarcaTech
- **Plataforma**: Google AntiGravity + BitDogLab
- **Data**: Março 2026

## Suporte

Para dúvidas, sugestões ou contribuições, consulte:

- Repositório GitHub do BitDogLab
- Documentação do EmbarcaTech
- Comunidade Wokwi

---

**Status**: ✅ Pronto para uso  
**Versão**: 1.0  
**Última Atualização**: Março 2026
