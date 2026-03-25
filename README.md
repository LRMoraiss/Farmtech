# FarmTech — Fase 1: O Cérebro na Bancada

## Status: EM DESENVOLVIMENTO (EmbarcaTech)

Esta branch contém a **Fase 1** do projeto FarmTech: a prova de conceito completa da lógica de controle, executada inteiramente na placa **BitDogLab v3**, sem nenhum sensor ou atuador externo.

> **Objetivo:** Provar que o "cérebro" do sistema funciona. Toda a inteligência de decisão é validada aqui, usando os próprios componentes da placa como simuladores de entrada.

---

## Como Funciona

O sistema executa um ciclo contínuo de **Ler → Decidir → Agir** a cada 1 segundo.

### Entradas Simuladas

| Componente | Pino | O que Simula |
|-----------|------|-------------|
| Joystick Y | GP27 (ADC1) | Temperatura (15°C a 40°C) |
| Joystick X | GP26 (ADC0) | Umidade (0% a 100%) |
| Botão A | GP5 | Consumo de água (-10% por toque) |
| Botão B | GP6 | Alterna Dia (luz=100%) / Noite (luz=0%) |

### Saídas Visuais

| Componente | Pino | O que Mostra |
|-----------|------|-------------|
| Display OLED | GP14/GP15 (I2C1) | Valores atuais + status de cada sistema |
| Matriz LEDs 5x5 | GP7 (PIO) | Ícones: Sol, Gota, Vento, Bomba |
| Buzzer | GP21 | Alerta sonoro em condição crítica |

---

## Lógica de Decisão (Setpoints)

| Condição | Ação |
|---------|------|
| Temperatura > 28°C | Liga Ventilação |
| Umidade < 75% | Liga Nebulizador |
| Nível de Água < 20% | Liga Bomba (reabastece para 100%) |
| Luz < 30% (Noite) | Liga Grow Lights |
| Temperatura > 35°C ou Água < 5% | Alerta Crítico (Buzzer) |

---

## Estrutura de Arquivos

```
fase-1/
├── farmtech_fase1.c   ← Código principal
├── CMakeLists.txt     ← Configuração de compilação
├── diagram.json       ← Simulação Wokwi
└── README.md          ← Este arquivo
```

---

## Compilação

```bash
mkdir build && cd build
cmake ..
make
# Gera: farmtech_fase1.uf2
```

---

## Próximas Fases

| Branch | Fase | Descrição |
|--------|------|-----------|
| `fase-1` | **Você está aqui** | Simulação pura na bancada |
| `fase-2` | Protótipo Funcional | Sensores reais + relés + mini-atuadores |
| `fase-3` | Domo Completo | Estrutura real + energia solar + telemetria Wi-Fi |
