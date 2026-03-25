# 🚀 Quick Start - FarmTech no Google AntiGravity

## 5 Minutos para Começar

### 1️⃣ Acesse o Google AntiGravity

Navegue até [https://antigravity.google/](https://antigravity.google/) e faça login com sua conta Google.

### 2️⃣ Crie um Novo Projeto

- Clique em "New Project"
- Nomeie como "FarmTech-Sim"
- Selecione "C/C++" como linguagem

### 3️⃣ Importe os Arquivos

Faça upload dos seguintes arquivos para o workspace:

```
farmtech_sim.c       ← Código principal
CMakeLists.txt       ← Configuração de compilação
diagram.json         ← Configuração do simulador
```

### 4️⃣ Compile o Projeto

No terminal do AntiGravity, execute:

```bash
mkdir build && cd build && cmake .. && make
```

### 5️⃣ Simule no Wokwi

- Abra o arquivo `diagram.json`
- Clique em "Simulate"
- O Wokwi abrirá com a placa BitDogLab

### 6️⃣ Teste os Controles

| Ação | Resultado |
|------|-----------|
| Mover Joystick CIMA | Aumenta temperatura |
| Mover Joystick ESQUERDA | Diminui umidade |
| Pressionar Botão A | Consome água (-10%) |
| Pressionar Botão B | Alterna Dia/Noite |

---

## 🎮 Interagindo com a Simulação

### Display OLED

Mostra em tempo real:
- Temperatura (°C)
- Umidade (%)
- Nível de Água (%)
- Luz Ambiente (%)

### Status dos Atuadores

```
Ventilador:  LIGADO/DESLIGADO
Nebulizador: LIGADO/DESLIGADO
Bomba Agua:  LIGADA/DESLIGADA
Grow Lights: LIGADAS/DESLIGADAS
```

### Matriz de LEDs

Exibe ícones visuais:
- ☀️ Sol amarelo = Grow Lights ligadas
- 💧 Gota azul = Nebulizador ligado
- 💨 Vento branco = Ventilador ligado
- 🔴 Vermelho piscante = Bomba ligada

---

## 🧪 Teste Rápido (2 minutos)

### Teste 1: Ligar Ventilador

1. Mova o Joystick para CIMA
2. Quando a temperatura passar de 28°C, o ventilador liga
3. Observe "Ventilador: LIGADO" no display

### Teste 2: Ligar Nebulizador

1. Mova o Joystick para ESQUERDA
2. Quando a umidade cair abaixo de 75%, o nebulizador liga
3. Observe "Nebulizador: LIGADO" no display

### Teste 3: Acionar Bomba

1. Pressione o Botão A 9 vezes (cada vez = -10%)
2. Quando o nível cair abaixo de 20%, a bomba liga
3. O nível volta para 100% automaticamente

### Teste 4: Ligar Grow Lights

1. Pressione o Botão B para simular noite
2. As luzes de cultivo devem ligar
3. Pressione novamente para simular dia
4. As luzes devem desligar

---

## 💡 Dicas

- **Usar o Agent do AntiGravity**: Peça ao Agent para gerar testes ou documentação
- **Modificar Limites**: Edite as constantes no início do `farmtech_sim.c`
- **Adicionar Sensores**: Integre sensores reais (DHT22, LDR, etc.)
- **Debugar**: Use `printf()` para ver logs no console do AntiGravity

---

## 📚 Próximas Etapas

1. Leia o `README.md` para entender a arquitetura completa
2. Consulte `ANTIGRAVITY_SETUP.md` para configurações avançadas
3. Explore o código em `farmtech_sim.c` e adicione suas próprias funcionalidades

---

**Pronto para começar? Boa sorte! 🌱**
