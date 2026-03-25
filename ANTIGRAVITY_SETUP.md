# Guia de Configuração: FarmTech no Google AntiGravity

## Visão Geral

Este projeto implementa um **sistema autônomo de gestão de ambiente** para cultivo hidropônico, simulado na placa **BitDogLab (Raspberry Pi Pico)** usando o ambiente **Google AntiGravity**.

## Estrutura do Projeto

```
farmtech-sim/
├── farmtech_sim.c          # Código principal em C
├── CMakeLists.txt          # Configuração de compilação (Pico SDK)
├── diagram.json            # Configuração da simulação Wokwi
├── ANTIGRAVITY_SETUP.md    # Este arquivo
└── README.md               # Documentação detalhada
```

## Passos para Usar no Google AntiGravity

### 1. Acessar o Google AntiGravity

- Navegue até [https://antigravity.google/](https://antigravity.google/)
- Faça login com sua conta Google
- Crie um novo projeto ou workspace

### 2. Importar os Arquivos do Projeto

1. Crie uma nova pasta no workspace chamada `farmtech-sim`
2. Faça upload dos seguintes arquivos:
   - `farmtech_sim.c`
   - `CMakeLists.txt`
   - `diagram.json`

### 3. Configurar o Ambiente de Desenvolvimento

#### Opção A: Usar Wokwi (Simulador Online)

1. No AntiGravity, abra o arquivo `diagram.json`
2. Clique em "Simular" ou "Run Simulation"
3. O Wokwi abrirá com a placa BitDogLab e os componentes conectados
4. Use o Agent do AntiGravity para executar o código

#### Opção B: Compilar com Pico SDK

1. No terminal do AntiGravity, execute:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

2. Isso gerará o arquivo `farmtech_sim.uf2` para upload na placa física

### 4. Usar o Agent do AntiGravity

O AntiGravity possui um Agent integrado que pode:

- **Analisar o código**: Peça ao Agent para revisar a lógica de decisão
- **Gerar testes**: Solicite testes automatizados para cada subsistema
- **Otimizar**: Peça sugestões de otimização de performance
- **Documentar**: Gere documentação automática do código

**Exemplo de prompt para o Agent:**

> "Analise o arquivo `farmtech_sim.c` e explique como o sistema toma decisões para ligar/desligar cada atuador. Depois, gere um arquivo de testes que valide cada condição."

## Componentes da Simulação

### Entradas (Simuladas via Controles)

| Componente | Pino GPIO | Função | Simulação |
|-----------|-----------|--------|-----------|
| Joystick X | GP26 (ADC0) | Umidade | Mover esquerda/direita: 0-100% |
| Joystick Y | GP27 (ADC1) | Temperatura | Mover cima/baixo: 15-40°C |
| Botão A | GP5 | Consumo de Água | Pressionar: -10% água |
| Botão B | GP6 | Dia/Noite | Pressionar: alterna 100%/0% luz |

### Saídas (Feedback Visual)

| Componente | Pino GPIO | Função | Indicação |
|-----------|-----------|--------|-----------|
| Display OLED | GP14/GP15 (I2C) | Valores Atuais | Mostra temp, umidade, água, luz |
| Matriz LEDs | GP7 (PIO) | Status Atuadores | Ícones: Sol, Gota, Vento, Bomba |

### Lógica de Decisão

**Gestão de Clima:**
- Se Temperatura > 28°C → Ventilação LIGADA
- Se Umidade < 75% → Nebulizador LIGADO

**Gestão Hídrica:**
- Se Nível de Água < 20% → Bomba LIGADA (reabastece para 100%)

**Gestão de Iluminação:**
- Se Luz < 30% (Noite) → Grow Lights LIGADAS

## Fluxo de Execução (Ciclo Contínuo)

```
┌─────────────────────────────────────┐
│  1. LER (Sensores Simulados)        │
│     - Joystick (Temp, Umidade)      │
│     - Botões (Água, Luz)            │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  2. DECIDIR (Lógica de Controle)    │
│     - Comparar com limites ideais   │
│     - Determinar ações              │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  3. AGIR (Atualizar Saídas)         │
│     - Display OLED                  │
│     - Matriz de LEDs                │
└──────────────┬──────────────────────┘
               │
               │ Aguarda 1 segundo
               │
               └──────────────┐
                              │
                    ┌─────────▼─────────┐
                    │  Volta ao início  │
                    └───────────────────┘
```

## Testando a Simulação

### Teste 1: Gestão de Temperatura
1. Mova o Joystick para CIMA (aumenta temperatura)
2. Quando passar de 28°C, o Ventilador deve ligar
3. Observe no Display OLED: "Ventilador: LIGADO"

### Teste 2: Gestão de Umidade
1. Mova o Joystick para ESQUERDA (diminui umidade)
2. Quando cair abaixo de 75%, o Nebulizador deve ligar
3. Observe no Display OLED: "Nebulizador: LIGADO"

### Teste 3: Gestão de Água
1. Pressione o Botão A várias vezes (cada pressionamento = -10%)
2. Quando o nível cair abaixo de 20%, a Bomba deve ligar
3. O nível deve voltar para 100% automaticamente
4. Observe no Display OLED: "Bomba Agua: LIGADA"

### Teste 4: Gestão de Iluminação
1. Pressione o Botão B para simular Noite (Luz = 0%)
2. As Grow Lights devem ligar
3. Pressione novamente para simular Dia (Luz = 100%)
4. As Grow Lights devem desligar

## Próximos Passos

### Melhorias Futuras

1. **Integração com Sensores Reais**: Conectar sensores DHT22 (temperatura/umidade), capacitivo (água), LDR (luz)

2. **Controle de Atuadores**: Implementar controle de relés para ventilador, bomba, nebulizador e luzes

3. **Persistência de Dados**: Armazenar histórico de leitura em EEPROM ou SD card

4. **Interface Web**: Criar dashboard com WebSocket para monitoramento remoto

5. **Machine Learning**: Usar IA para otimizar automaticamente os limites de decisão

### Recursos Úteis

- [Documentação Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [Repositório BitDogLab](https://github.com/BitDogLab/BitDogLab)
- [Simulador Wokwi](https://wokwi.com/)
- [Google AntiGravity Docs](https://antigravity.google/docs/home)

## Suporte e Contribuições

Este projeto é parte da iniciativa **EmbarcaTech** e está aberto para contribuições. Para dúvidas ou sugestões, consulte a documentação do projeto ou entre em contato com a equipe de desenvolvimento.

---

**Versão**: 1.0  
**Data**: Março 2026  
**Plataforma**: Google AntiGravity + Wokwi  
**Hardware**: BitDogLab (Raspberry Pi Pico)
