# FarmTech: Domo Geodésico Autônomo para o Sertão

![Domo Geodésico FarmTech - Arquitetura Completa](domo_farmtech.png)

## Introdução

O **FarmTech** é um sistema autônomo de controle ambiental projetado para cultivo hidropônico em **domos geodésicos no Sertão Brasileiro**. Implementado na placa **BitDogLab v3** (baseada em Raspberry Pi Pico), o sistema funciona como a Unidade Central de Controle que monitora continuamente o ambiente e toma decisões autônomas para manter condições ideais de cultivo, protegendo as plantas do clima extremo.

Este projeto demonstra uma arquitetura completa de hardware e software para agricultura de precisão em ambientes hostis, utilizando energia solar e telemetria remota.

## Arquitetura do Domo Geodésico

O sistema foi projetado para operar em uma estrutura física otimizada para o sertão:

- **Estrutura Geodésica**: Cobertura externa em lona PVC impermeável e resistente a UV.
- **Massa Térmica**: Base em concreto reforçado semi-enterrada para estabilidade térmica.
- **Energia Renovável**: Painéis solares com banco de baterias em bunker protegido.
- **Cultivo Vertical**: Torres hidropônicas com fluxo contínuo de solução nutritiva.

## Componentes do Sistema (Hardware)

A BitDogLab v3 atua como o cérebro do sistema, conectada aos seguintes periféricos:

### Sensores (Entradas)
- **DHT22 (GP4)**: Monitoramento de temperatura e umidade interna.
- **LDR (GP28)**: Sensor de luz ambiente para controle de fotossíntese.
- **Sensor de Nível (GP29)**: Monitoramento do reservatório de água/nutrientes.

### Atuadores (Saídas via Relés)
- **Exaustor (GP8)**: Liberação de ar quente acumulado no topo do domo.
- **Entrada de Ar (GP9)**: Captação de ar fresco filtrado.
- **Nebulizador (GP10)**: Sistema de *misting* para controle de umidade e resfriamento evaporativo.
- **Grow Lights (GP11)**: LEDs Full Spectrum para iluminação suplementar.
- **Bomba d'Água (GP12)**: Circulação da solução nutritiva nas torres.

### Comunicação
- **Módulo Wi-Fi (UART0 - GP0/GP1)**: Telemetria remota enviando dados em formato JSON.

## Lógica de Controle Autônomo

O sistema executa um ciclo contínuo de **Ler → Decidir → Agir** a cada 1 segundo:

### 1. Gestão de Clima (Resfriamento)
- **Se Temp > 28°C**: Liga exaustor e entrada de ar.
- **Se Temp > 28°C E Umidade < 85%**: Liga nebulizador (resfriamento evaporativo).
- **Se Umidade < 75%**: Liga nebulizador para manter hidratação foliar.

### 2. Gestão Hídrica (Nutrição)
- **Se Nível < 20%**: Liga bomba d'água para reabastecer as torres.
- **Se Nível > 90%**: Desliga bomba d'água.

### 3. Gestão de Iluminação (Fotossíntese)
- **Se Luz < 30%**: Liga Grow Lights (LEDs Full Spectrum).

### 4. Alertas Críticos
- **Se Temp > 35°C OU Nível < 5%**: Aciona alarme sonoro (Buzzer) e envia flag de criticidade via telemetria.

## Estrutura do Código

O código principal (`farmtech_sim.c`) está estruturado da seguinte forma:

```c
// 1. Configurações de Pinos (Sensores, Relés, UART)
// 2. Variáveis de Estado (Temp, Umidade, Status dos Atuadores)
// 3. setup_hardware(): Inicializa GPIOs, ADC e UART
// 4. ler_sensores(): Lê DHT22, LDR e Nível de Água
// 5. processar_decisoes(): Aplica a lógica de controle (Setpoints)
// 6. atualizar_atuadores(): Aciona os relés correspondentes
// 7. enviar_telemetria_wifi(): Envia JSON via UART
// 8. main(): Loop infinito de 1Hz
```

## Compilação e Instalação

### Pré-requisitos
- Raspberry Pi Pico SDK instalado
- CMake 3.13+
- Compilador ARM GCC

### Passos para Compilar
```bash
mkdir build
cd build
cmake ..
make
```

### Upload
1. Conecte a BitDogLab v3 via USB segurando o botão BOOTSEL.
2. Copie o arquivo `farmtech_sim.uf2` gerado para a unidade RPI-RP2.

## Formato de Telemetria (JSON)

O sistema envia dados continuamente via UART (115200 bps) para o módulo Wi-Fi no seguinte formato:

```json
{
  "temp": 26.5,
  "umid": 80,
  "agua": 100,
  "luz": 45,
  "vent": 0,
  "mist": 0,
  "pump": 0,
  "grow": 0,
  "auto": 1
}
```

## Modos de Operação

- **Modo AUTO**: O sistema toma todas as decisões baseado nos sensores.
- **Modo MANUAL**: Alternado via Botão A (GP5). Permite controle manual para manutenção.

## Licença e Autores

Projeto desenvolvido para a iniciativa **EmbarcaTech**.
Arquitetura otimizada para o Sertão Brasileiro.

**Licença**: MIT
