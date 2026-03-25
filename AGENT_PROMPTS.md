# 🤖 Prompts para o Agent do Google AntiGravity

Use estes prompts com o Agent integrado do AntiGravity para acelerar o desenvolvimento e melhorar o código.

## 📋 Prompts Básicos

### 1. Analisar o Código

**Prompt:**
```
Analise o arquivo farmtech_sim.c e explique:
1. Como o sistema toma decisões para ligar/desligar cada atuador?
2. Qual é o ciclo de operação principal?
3. Quais são os limites de operação (setpoints)?
4. Há alguma oportunidade de otimização?
```

**Resultado esperado:** O Agent fornecerá uma análise detalhada da lógica de controle.

---

### 2. Gerar Testes Automatizados

**Prompt:**
```
Gere um arquivo de testes em C que valide o comportamento do sistema FarmTech:
- Teste 1: Verificar se o ventilador liga quando temperatura > 28°C
- Teste 2: Verificar se o nebulizador liga quando umidade < 75%
- Teste 3: Verificar se a bomba liga quando água < 20%
- Teste 4: Verificar se as grow lights ligam quando luz < 30%

Inclua funções de setup, teardown e assertions.
```

**Resultado esperado:** Um arquivo de testes completo com estrutura de framework (ex: Unity, CUnit).

---

### 3. Documentar o Código

**Prompt:**
```
Gere uma documentação técnica completa do arquivo farmtech_sim.c incluindo:
- Descrição de cada função
- Parâmetros de entrada/saída
- Exemplos de uso
- Diagrama de fluxo
- Tabela de pinos GPIO
```

**Resultado esperado:** Documentação em Markdown pronta para publicação.

---

## 🔧 Prompts de Otimização

### 4. Otimizar Performance

**Prompt:**
```
Revise o código farmtech_sim.c para:
1. Reduzir consumo de energia
2. Melhorar tempo de resposta
3. Adicionar tratamento de erros
4. Implementar debounce mais robusto para botões

Forneça o código otimizado com explicações.
```

**Resultado esperado:** Versão melhorada do código com melhor eficiência.

---

### 5. Adicionar Logging e Debug

**Prompt:**
```
Adicione ao código farmtech_sim.c:
1. Sistema de logging com níveis (DEBUG, INFO, WARNING, ERROR)
2. Timestamps para cada evento
3. Histórico de mudanças de estado
4. Função para exibir relatório de diagnóstico

Inclua macros para facilitar o debug.
```

**Resultado esperado:** Código com logging completo para facilitar troubleshooting.

---

## 🌱 Prompts de Extensão

### 6. Integrar Sensores Reais

**Prompt:**
```
Estou usando sensores reais na placa BitDogLab:
- DHT22 para temperatura e umidade (pino GP4)
- LDR para luz ambiente (pino GP28)
- Sensor capacitivo para nível de água (pino GP29)

Modifique o código farmtech_sim.c para:
1. Ler dados dos sensores reais
2. Aplicar calibração
3. Filtrar ruído
4. Manter compatibilidade com simulação

Forneça o código atualizado.
```

**Resultado esperado:** Código que suporta tanto simulação quanto sensores reais.

---

### 7. Adicionar Controle de Atuadores

**Prompt:**
```
Quero controlar atuadores reais com relés:
- Ventilador (pino GP8)
- Nebulizador (pino GP9)
- Bomba d'água (pino GP10)
- Grow Lights com PWM (pino GP11, frequência 1kHz)

Adicione ao código farmtech_sim.c:
1. Funções para controlar cada atuador
2. PWM para controle de intensidade
3. Proteção contra acionamento simultâneo
4. Feedback de status

Forneça o código completo.
```

**Resultado esperado:** Código com suporte a atuadores reais e PWM.

---

### 8. Criar Dashboard Web

**Prompt:**
```
Crie um dashboard web simples para monitorar o FarmTech em tempo real:
1. Gráficos de temperatura, umidade, água e luz
2. Status dos atuadores (ON/OFF)
3. Histórico das últimas 24 horas
4. Botões para simular entradas (temperatura, umidade, etc.)

Use HTML5, CSS3 e JavaScript. Inclua WebSocket para comunicação com a placa.
```

**Resultado esperado:** Código HTML/CSS/JS para um dashboard funcional.

---

## 📊 Prompts de Análise

### 9. Gerar Relatório de Eficiência

**Prompt:**
```
Analise o código farmtech_sim.c e gere um relatório incluindo:
1. Consumo estimado de energia
2. Tempo de resposta de cada subsistema
3. Confiabilidade do sistema (MTBF)
4. Recomendações de melhoria
5. Comparação com sistemas comerciais similares

Forneça o relatório em formato Markdown.
```

**Resultado esperado:** Relatório técnico detalhado com métricas e recomendações.

---

### 10. Comparar com Alternativas

**Prompt:**
```
Compare a abordagem do FarmTech com:
1. Sistemas comerciais de automação agrícola
2. Outras implementações em Raspberry Pi Pico
3. Alternativas usando Arduino ou ESP32

Inclua:
- Vantagens e desvantagens
- Custo-benefício
- Escalabilidade
- Facilidade de uso

Forneça uma análise comparativa.
```

**Resultado esperado:** Análise comparativa com tabelas e gráficos.

---

## 🚀 Prompts Avançados

### 11. Machine Learning para Otimização

**Prompt:**
```
Implemente um algoritmo de Machine Learning simples no FarmTech para:
1. Aprender os padrões de crescimento das plantas
2. Otimizar automaticamente os setpoints (limites)
3. Prever falhas antes que ocorram
4. Adaptar-se a diferentes tipos de cultivo

Use TensorFlow Lite ou biblioteca similar compatível com Pico.
Forneça o código e instruções de treinamento.
```

**Resultado esperado:** Código com ML integrado para otimização automática.

---

### 12. Rede de Múltiplos Domos

**Prompt:**
```
Estou expandindo o FarmTech para controlar 10 domos simultaneamente.
Crie uma arquitetura que:
1. Sincronize dados entre domos
2. Detecte e isole falhas em um domo
3. Otimize recursos compartilhados
4. Forneça visão centralizada via dashboard

Use protocolo MQTT ou similar. Forneça código para:
- Firmware do domo (Pico)
- Servidor central (Python/Node.js)
- Dashboard web
```

**Resultado esperado:** Arquitetura completa para sistema distribuído.

---

## 💬 Dicas para Usar o Agent

1. **Seja Específico**: Quanto mais detalhes você fornecer, melhor será a resposta
2. **Forneça Contexto**: Mencione limitações de hardware, requisitos de performance
3. **Peça Iterações**: "Agora adicione tratamento de erro" ou "Otimize para menor consumo"
4. **Revise o Código**: Sempre revise o código gerado antes de usar em produção
5. **Faça Perguntas**: "Por que você escolheu essa abordagem?" ajuda a aprender

---

## 📝 Template de Prompt Personalizado

Use este template para criar seus próprios prompts:

```
Contexto:
[Descreva a situação atual]

Objetivo:
[O que você quer alcançar]

Restrições:
[Limitações de hardware, software, tempo, etc.]

Requisitos:
1. [Requisito 1]
2. [Requisito 2]
3. [Requisito 3]

Formato de Saída:
[Código, documentação, diagrama, etc.]

Exemplos (opcional):
[Forneça exemplos de entrada/saída esperada]
```

---

## 🎓 Recursos Adicionais

- [Google AntiGravity Docs](https://antigravity.google/docs/home)
- [Pico SDK Documentation](https://github.com/raspberrypi/pico-sdk)
- [BitDogLab Repository](https://github.com/BitDogLab/BitDogLab)
- [Wokwi Simulator](https://wokwi.com/)

---

**Boa sorte com seus prompts! 🚀**
