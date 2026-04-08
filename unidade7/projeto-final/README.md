# FarmTech - Sistema de Controle para Domo Geodésico Hidropônico

**Projeto Final - EmbarcaTech | Unidade 7**

Sistema embarcado desenvolvido em C para a placa **BitDogLab (RP2040)** que monitora e controla o microclima de um domo geodésico hidropônico no sertão brasileiro.

![Domo FarmTech](domo_farmtech.png)

---

## Objetivo

Automatizar o controle de temperatura, umidade e irrigação de um ambiente hidropônico fechado, utilizando os periféricos nativos da BitDogLab e comunicação Wi-Fi para telemetria remota.

---

## Periféricos Utilizados

| Periférico | Pino | Função |
|-----------|------|--------|
| Joystick X (ADC0) | GP26 | Simula Temperatura (15°C–45°C) |
| Joystick Y (ADC1) | GP27 | Simula Umidade (30%–90%) |
| Botão A | GP5 | Liga/Desliga Bomba d'água |
| Botão B | GP6 | Envia Telemetria |
| LED Vermelho | GP13 | Alarme Crítico |
| LED Verde | GP11 | Sistema Normal |
| LED Azul | GP12 | Atuadores Ativos |
| Buzzer | GP21 | Alerta Sonoro |
| Display OLED | GP14/GP15 | Interface Visual (I2C) |
| Wi-Fi CYW43439 | — | Telemetria Remota |

---

## Lógica de Controle

```
Temperatura > 35°C → Alarme Crítico (LED Vermelho + Buzzer)
Temperatura > 28°C → Ventoinha Liga (LED Azul)
Temperatura ≤ 28°C → Sistema Normal (LED Verde)
Botão A → Alterna Bomba d'água
Botão B → Envia pacote JSON via UART/Wi-Fi
```

---

## Como Compilar e Gravar

### Pré-requisitos
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) instalado
- CMake 3.13+
- Compilador ARM GCC

### Compilação
```bash
mkdir build
cd build
cmake ..
make
```

### Gravação na BitDogLab
1. Segure o botão **BOOTSEL** da placa
2. Conecte o cabo USB ao computador
3. Copie o arquivo `farmtech_final.uf2` para a unidade **RPI-RP2**

---

## Telemetria JSON
O sistema envia os dados no seguinte formato via UART (115200 baud) e Wi-Fi:
```json
{"temp": 26.5, "umid": 72.3, "bomba": 0, "vent": 1}
```

---

## Estrutura do Repositório
```
projeto-final-farmtech/
├── farmtech_core.c      # Firmware principal
├── CMakeLists.txt       # Configuração de build
├── fluxograma.png       # Diagrama do sistema
├── Relatorio_Tecnico.pdf# Relatório técnico completo
└── README.md            # Este arquivo
```

---

## Licença
MIT License — Livre para uso educacional.

## Créditos
- **Pico SDK** — Raspberry Pi Foundation
- **BitDogLab** — Embarca Tech / Instituto Federal
- **Manus AI** — Assistência no desenvolvimento
