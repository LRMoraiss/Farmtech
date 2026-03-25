# Mapeamento de Hardware - FarmTech Domo Geodésico

## Visão Geral da Arquitetura

O sistema FarmTech é instalado em um **domo geodésico** no **Sertão Brasileiro**, projetado para resistir ao ambiente árido e extremo da região. A BitDogLab v3 atua como a **Unidade Central de Controle**, gerenciando todos os subsistemas de forma autônoma.

---

## Estrutura Física do Domo

| Componente Estrutural | Descrição |
|-----------------------|-----------|
| Geodesic Dome Structure | Estrutura geodésica principal |
| Outer Waterproof PVC Canvas Skin | Cobertura externa impermeável, resistente a UV |
| Thick Reinforced Concrete Shell | Camada de concreto reforçado (massa térmica) |
| Inner Hygienic Surface | Superfície interna reflexiva e de fácil limpeza |
| Deep Concrete Foundation Ring | Anel de fundação e sistema de ancoragem |

---

## Componentes Eletrônicos e Sensores

### Sensores (Entradas)

| Sensor | Função | Pino GPIO (BitDogLab v3) |
|--------|--------|--------------------------|
| DHT22 | Temperatura e Umidade | GP4 |
| LDR (Light Sensor) | Luz Ambiente | GP28 (ADC2) |
| Water Level Sensor | Nível do Reservatório | GP29 (ADC3) |

### Atuadores (Saídas via Relés)

| Atuador | Função | Pino GPIO (BitDogLab v3) |
|---------|--------|--------------------------|
| Automated Vent (Hot Air Release) | Ventilação / Exaustão de ar quente | GP8 (Relé 1) |
| Cooler Air Intake (Filtered) | Entrada de ar frio filtrado | GP9 (Relé 2) |
| Misting System | Nebulização (Umidade e Resfriamento) | GP10 (Relé 3) |
| Full Spectrum LED Grow Lights | Iluminação suplementar de cultivo | GP11 (Relé 4) |
| Water Pump | Bomba de solução nutritiva | GP12 (Relé 5) |

### Sistema de Energia

| Componente | Função |
|-----------|--------|
| Solar Panel Array | Fonte de energia renovável |
| Charge Controller & Battery Bank | Controle de carga e banco de baterias |
| Protected Bunker Box | Caixa de proteção da fonte de energia |

### Comunicação e Telemetria

| Componente | Função | Interface |
|-----------|--------|-----------|
| Wi-Fi Telemetry Module | Monitoramento e controle remoto | UART/SPI |
| BitDogLab v3 Central Control Unit | Unidade central de processamento | — |

---

## Subsistemas de Cultivo

| Componente | Descrição |
|-----------|-----------|
| Torres Hidropônicas | Cultivo vertical de hortaliças e frutas |
| Nutrient-Rich Water Flow | Circulação de solução nutritiva |
| Water Reservoir & Pump | Reservatório e bomba de nutrientes |
| Plant Roots | Sistema radicular exposto à solução |

---

## Pinout Completo - BitDogLab v3

```
GP0  → UART TX (Wi-Fi Module)
GP1  → UART RX (Wi-Fi Module)
GP2  → I2C SDA (Display OLED)
GP3  → I2C SCL (Display OLED)
GP4  → DHT22 (Temperatura/Umidade)
GP5  → Botão A (Simulação/Manual)
GP6  → Botão B (Simulação/Manual)
GP7  → Matriz LEDs WS2812B (5x5)
GP8  → Relé 1 - Ventilação (Exaustão)
GP9  → Relé 2 - Entrada de Ar Frio
GP10 → Relé 3 - Sistema de Nebulização
GP11 → Relé 4 - Grow Lights (LED)
GP12 → Relé 5 - Bomba d'Água
GP13 → Buzzer (Alertas)
GP14 → I2C SDA (alternativo)
GP15 → I2C SCL (alternativo)
GP26 → ADC0 - Joystick X (Simulação)
GP27 → ADC1 - Joystick Y (Simulação)
GP28 → ADC2 - LDR (Sensor de Luz)
GP29 → ADC3 - Sensor de Nível d'Água
```

---

## Lógica de Controle Completa

### Setpoints (Limites Ideais)

| Parâmetro | Limite Mínimo | Limite Máximo | Ação |
|-----------|--------------|--------------|------|
| Temperatura | 18°C | 28°C | > 28°C → Liga Ventilação + Nebulização |
| Umidade | 75% | 95% | < 75% → Liga Nebulização |
| Nível de Água | 20% | 100% | < 20% → Liga Bomba |
| Luz Ambiente | 30% | 100% | < 30% → Liga Grow Lights |

### Modos de Operação

| Modo | Descrição |
|------|-----------|
| AUTO | Sistema opera de forma totalmente autônoma |
| MANUAL | Controle via botões A/B ou joystick |
| REMOTE | Controle via Wi-Fi (telemetria) |
| ALERT | Condição crítica detectada (buzzer + LED vermelho) |
