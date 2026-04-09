# 🌱 FarmTech — Sistema Autônomo de Cultivo Hidropônico

[![EmbarcaTech](https://img.shields.io/badge/EmbarcaTech-2026-green)]()
[![Pico W](https://img.shields.io/badge/Raspberry_Pi-Pico_W-red)]()
[![C](https://img.shields.io/badge/Linguagem-C-blue)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)]()

### Descrição

O projeto FarmTech é um sistema embarcado autônomo projetado para otimizar o cultivo hidropônico em domos geodésicos no Sertão Brasileiro. A agricultura no semiárido enfrenta desafios climáticos severos, e esta solução tecnológica visa criar um microclima controlado para garantir resiliência e eficiência na produção agrícola.

O sistema atua como a Unidade Central de Controle (UCC), monitorando continuamente variáveis ambientais críticas como temperatura, umidade, luminosidade e nível d'água. Com base nessas leituras, o FarmTech toma decisões autônomas para acionar atuadores (ventilação, nebulização, iluminação e bombas), mantendo as condições ideais de cultivo.

Desenvolvido para a plataforma BitDogLab v3 (baseada no Raspberry Pi Pico W), o firmware foi escrito inteiramente em linguagem C utilizando o Pico SDK 2.x. O sistema oferece uma interface local completa (Display OLED, Matriz de LEDs, Buzzer) e controle remoto via WiFi através de um dashboard web responsivo.

### Funcionalidades

- Menu interativo no OLED 128x64 com 7 telas navegáveis por joystick
- Lógica de controle automático com 3 receitas de cultivo (Morango, Alface, Tomate)
- Servidor HTTP com dashboard web responsivo — acesse pelo celular na mesma rede
- Controle remoto de atuadores via navegador (GET /cmd)
- IRQ nos botões com detecção de pressão curta e longa
- Matriz WS2812B 5x5 com padrões visuais por estado do sistema
- LED RGB PWM com 5 estados de cor
- Buzzer com alertas sonoros e melodia de boot
- DHT22 real com fallback automático para joystick em caso de falha
- Histórico circular de até 5 alertas com timestamp
- WiFi com fallback offline (sistema funciona sem rede)

### Hardware

**Componentes nativos da BitDogLab:**

| Componente | Pino | Função |
|---|---|---|
| Display OLED SSD1306 | GP14/GP15 (I2C1) | Menu interativo e dashboard local |
| Joystick analógico X/Y | GP26/GP27 (ADC0/1) | Navegação + simulação de sensores |
| Botão A | GP5 (IRQ) | Selecionar/confirmar |
| Botão B | GP6 (IRQ) | Voltar/cancelar |
| Joystick SW | GP22 (IRQ) | Entrar no menu |
| Matriz WS2812B 5x5 | GP7 (PIO) | Indicação visual dos atuadores |
| LED RGB | GP11/GP12/GP13 (PWM) | Status geral do sistema |
| Buzzer passivo | GP21 (PWM) | Alertas e melodia de boot |
| WiFi CYW43 | interno | Servidor HTTP e controle remoto |

**Sensores externos (conectar via expansão IDC):**

| Sensor | Pino | Função |
|---|---|---|
| DHT22 | GP4 (1-Wire) | Temperatura e umidade reais |
| LDR + resistor 10kΩ | GP26 (ADC0) | Luminosidade ambiente |
| Sensor capacitivo nível | GP28 (ADC2) | Nível do reservatório d'água |

### Setpoints de Controle

| Parâmetro | Limite | Ação |
|---|---|---|
| Temperatura | > 28°C | Liga ventilação + nebulização |
| Temperatura crítica | > 35°C | Buzzer + alerta vermelho |
| Umidade | < 75% | Liga nebulizador |
| Nível d'água | < 20% | Liga bomba |
| Luminosidade | < 30% | Liga Grow Lights |

### Receitas de Cultivo

| Receita | Temp. Máx | Umidade Mín | Luz Mín | Água Mín |
|---|---|---|---|---|
| Morango | 28°C | 75% | 30% | 20% |
| Alface | 25°C | 85% | 50% | 30% |
| Tomate | 32°C | 60% | 80% | 15% |

### Dashboard Web

```
Acesse: http://<IP-da-placa>/
O IP aparece no display OLED após conexão WiFi.
```
- Mostra temperatura, umidade, nível d'água e luminosidade em tempo real
- Botões para ligar/desligar cada atuador (modo MANUAL)
- Alterna entre modo AUTO e MANUAL
- Atualiza automaticamente a cada 2 segundos

### Instalação e Compilação

```bash
# 1. Clone o repositório
git clone https://github.com/LRMoraiss/FarmTech.git
cd FarmTech

# 2. Configure as credenciais WiFi
cp include/config.h.example include/config.h
# Edite include/config.h com seu SSID e senha

# 3. Compile (requer Pico SDK 2.x e arm-none-eabi-gcc)
mkdir build && cd build
cmake .. -DPICO_BOARD=pico_w
make

# 4. Grave na placa
# Segure BOOTSEL, conecte USB, solte BOOTSEL
# Arraste build/farmtech.uf2 para a unidade RPI-RP2
```

### Dependências

- Raspberry Pi Pico SDK 2.x
- arm-none-eabi-gcc (ARM cross-compiler)
- CMake 3.13+
- Ninja ou Make

### Estrutura do Projeto

```
FarmTech/
├── src/farmtech.c      # Firmware principal (~900 linhas)
├── include/
│   ├── lwipopts.h      # Configuração lwIP (TCP/IP)
│   └── config.h.example # Template de credenciais WiFi
├── pio/ws2812.pio      # Programa PIO para matriz de LEDs
├── CMakeLists.txt      # Configuração de build
└── README.md
```

### Arquitetura do Firmware

Breve descrição das 17 seções do código:
- Seções 1-3: Configuração, tipos e variáveis globais
- Seção 4: Driver OLED SSD1306 (framebuffer + fonte 5x7, sem biblioteca externa)
- Seção 5: DHT22 bit-bang com timeout por time_us_32()
- Seção 6: IRQ dos botões (pressão curta/longa, debounce por tempo)
- Seção 7: Servidor HTTP WiFi (lwIP poll mode, rotas /, /status, /cmd)
- Seções 8-10: Setup hardware, leitura de sensores, lógica de decisão
- Seções 11-12: LED RGB PWM, Matriz WS2812B PIO
- Seções 13-14: Menu OLED (FSM 7 estados), renderização
- Seções 15-17: Serial debug, melodia boot, main loop

### Uso de Inteligência Artificial

```
Este projeto foi desenvolvido por Luciano Rodrigues de Morais com suporte
de ferramentas de IA como assistentes de código:

- Claude (Anthropic) — suporte em código C embarcado e documentação
- Google Gemini — pesquisa técnica e geração de imagens
- Manus — reorganização e estruturação do repositório
- Google AntiGravity — ambiente de desenvolvimento e compilação

A autoria intelectual, decisões de arquitetura, testes físicos na placa
e validação do sistema são de responsabilidade do autor.
```

### Licença

```
MIT License — veja LICENSE para detalhes.
```

### Autor

```
Luciano Rodrigues de Morais
EmbarcaTech — Expansão em Sistemas Embarcados
Fortaleza, CE — Abril 2026
```
