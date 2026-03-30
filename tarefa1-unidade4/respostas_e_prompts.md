# Tarefa 1 - Unidade 4 (EmbarcaTech)

## Resposta da Questão 5
**Pergunta:** Como o ADC converte sinais analógicos do joystick em valores digitais no exemplo 02?

**Resposta:** 
O ADC (Conversor Analógico-Digital) do RP2040 (microcontrolador da BitDogLab) converte a tensão analógica contínua gerada pelos potenciômetros do joystick (que varia de 0V a 3.3V dependendo da posição) em um valor digital discreto. Ele faz isso utilizando um conversor de aproximações sucessivas (SAR) com resolução de 12 bits. 

Isso significa que a faixa de tensão de 0 a 3.3V é dividida em 4096 níveis discretos ($2^{12}$). Quando o joystick é movido, a resistência interna do potenciômetro muda, alterando a tensão no pino GPIO configurado como entrada ADC (ex: GP26 para o eixo X ou GP27 para o eixo Y). O módulo ADC amostra essa tensão e a converte em um número inteiro proporcional entre 0 e 4095:
- **0** representa 0V (joystick totalmente para um lado).
- **4095** representa 3.3V (joystick totalmente para o outro lado).
- **~2048** representa 1.65V (joystick na posição central de repouso).

---

## Prompts para o Google AntiGravity

Copie e cole os prompts abaixo no Agent do Google AntiGravity para gerar os arquivos e simulações rapidamente.

### Prompt 1: Questões 1 e 2 (Temporizador e Botões)
```text
Crie um projeto C para o Raspberry Pi Pico (BitDogLab) que resolva as questões 1 e 2 da Tarefa 1 - Unidade 4.
Requisitos:
1. O Botão A (GP5) deve ser pressionado 5 vezes. Use interrupção ou polling com debounce.
2. Ao atingir 5 pressões, um LED (GP13) deve piscar por 10 segundos.
3. O piscar deve ser controlado por um temporizador (repeating_timer).
4. A frequência inicial de pisca deve ser 10 Hz.
5. O Botão B (GP6) deve alterar a frequência do LED para 1 Hz quando pressionado.
Gere o código C completo e o arquivo diagram.json para simulação no Wokwi com os botões e o LED.
```

### Prompt 2: Questão 3 (PWM e Interrupções RGB)
```text
Crie um projeto C para o Raspberry Pi Pico (BitDogLab) que resolva a questão 3 da Tarefa 1 - Unidade 4.
Requisitos:
1. Controle os LEDs RGB (Vermelho GP13, Azul GP12, Verde GP11) usando PWM.
2. O LED vermelho deve ter PWM de 1kHz. O duty cycle inicia em 5% e incrementa 5% a cada 2 segundos até o máximo, retornando a 5%.
3. A atualização do duty cycle do LED vermelho deve ser feita dentro de uma interrupção de temporizador (repeating_timer_callback).
4. O LED azul deve ter PWM fixo de 10kHz.
Gere o código C completo e o arquivo diagram.json para simulação no Wokwi com o LED RGB.
```

### Prompt 3: Questão 4 (ADC Temperatura em Fahrenheit)
```text
Crie um projeto C para o Raspberry Pi Pico que resolva a questão 4 da Tarefa 1 - Unidade 4.
Requisitos:
1. Leia o sensor de temperatura interno do RP2040 (ADC canal 4).
2. Converta o valor lido (0-4095) para tensão.
3. Calcule a temperatura em graus Celsius.
4. Converta a temperatura de Celsius para Fahrenheit.
5. Imprima o resultado no console (printf) a cada 1 segundo.
Gere o código C completo.
```
