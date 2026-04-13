# 🌱 FarmTech — Sistema Autônomo de Cultivo Hidropônico

[![EmbarcaTech](https://img.shields.io/badge/EmbarcaTech-2026-green)]()
[![Pico W](https://img.shields.io/badge/Raspberry_Pi-Pico_W-red)]()
[![C](https://img.shields.io/badge/Linguagem-C-blue)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)]()

## Descrição

O **FarmTech** é um sistema embarcado autônomo desenvolvido para controle ambiental de cultivo hidropônico em domos geodésicos no **Sertão Brasileiro**. O sistema monitora temperatura, umidade, luminosidade e nível d'água, tomando decisões autônomas para manter condições ideais de cultivo.

Desenvolvido na plataforma **BitDogLab v3** (Raspberry Pi Pico W), com firmware em **C usando Pico SDK 2.x**. Oferece interface local completa (OLED + menu + LEDs) e controle remoto via **WiFi** com dashboard web responsivo.

## Funcionalidades

- Menu interativo no OLED 128x64 com 7 telas navegáveis por joystick
- Lógica de controle automático com 3 receitas de cultivo (Morango, Alface, Tomate)
- Servidor HTTP com dashboard web responsivo — acesse pelo celular na mesma rede
- Controle remoto de atuadores via navegador (GET /cmd)
- IRQ nos botões com detecção de pressão curta e longa
- Matriz WS2812B 5x5 com padrões visuais por estado do sistema
- LED RGB PWM com 5 estados de cor
- Buzzer com alertas sonoros e melodia de boot
- DHT22 com fallback automático para joystick em caso de falha
- Histórico circular de até 5 alertas com timestamp
- WiFi com fallback offline (sistema funciona sem rede)

## Hardware

### Componentes nativos da BitDogLab

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

### Sensores externos

| Sensor | Pino | Função |
|---|---|---|
| DHT22 | GP4 (1-Wire) | Temperatura e umidade reais |
| LDR + resistor 10kΩ | GP26 (ADC0) | Luminosidade ambiente |
| Sensor capacitivo nível | GP28 (ADC2) | Nível do reservatório d'água |

## Setpoints de Controle

| Parâmetro | Limite | Ação |
|---|---|---|
| Temperatura | > 28°C | Liga ventilação + nebulização |
| Temperatura crítica | > 35°C | Buzzer + alerta vermelho |
| Umidade | < 75% | Liga nebulizador |
| Nível d'água | < 20% | Liga bomba |
| Luminosidade | < 30% | Liga Grow Lights |

## Receitas de Cultivo

| Receita | Temp. Máx | Umidade Mín | Luz Mín | Água Mín |
|---|---|---|---|---|
| Morango | 28°C | 75% | 30% | 20% |
| Alface | 25°C | 85% | 50% | 30% |
| Tomate | 32°C | 60% | 80% | 15% |

## Dashboard Web

Acesse: `http://<IP-da-placa>/`
O IP aparece no display OLED após conexão WiFi.

- Dados em tempo real (atualiza a cada 2 segundos)
- Controle de atuadores remotamente
- Alterna entre modo AUTO e MANUAL

## Instalação e Compilação

```bash
# 1. Clone o repositório
git clone https://github.com/LRMoraiss/Farmtech.git
cd Farmtech

# 2. Configure as credenciais WiFi
cp config.h.exemplo config.h
# Edite config.h com seu SSID e senha

# 3. Compile
mkdir build && cd build
cmake .. -DPICO_BOARD=pico_w
make

# 4. Grave na placa
# Segure BOOTSEL, conecte USB, solte BOOTSEL
# Arraste build/farmtech.uf2 para RPI-RP2
```

## Dependências

- Raspberry Pi Pico SDK 2.x
- arm-none-eabi-gcc
- CMake 3.13+

## Uso de Inteligência Artificial

Projeto desenvolvido por **Luciano Rodrigues de Morais** com suporte de ferramentas de IA:

- **Manus** — pesquisa, análise crítica e organização do repositório
- **Google Gemini** — geração de imagens e pesquisa visual
- **Claude (Anthropic)** — suporte em código C embarcado e documentação
- **Google AntiGravity** — ambiente de desenvolvimento e compilação

A autoria intelectual, decisões de arquitetura, testes físicos na placa e validação do sistema são de responsabilidade do autor.

## Licença

MIT License — veja [LICENSE](LICENSE) para detalhes.

## Autor

**Luciano Rodrigues de Morais**
EmbarcaTech — Expansão em Sistemas Embarcados | IFCE
Fortaleza, CE — Abril 2026
