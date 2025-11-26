# Projeto: Fade PWM com DMA e Exibição de Ângulo no SSD1306 (Raspberry Pi Pico)

Este projeto demonstra o uso do **DMA** (Direct Memory Access) para alimentar automaticamente o registrador de comparação de um canal **PWM** no Raspberry Pi Pico, produzindo um efeito de fade suave em um LED. Além disso, calcula-se um "ângulo" proporcional ao duty cycle e o valor é exibido em um display OLED **SSD1306** via I2C.

> Observação: O "ângulo" exibido é apenas uma conversão do nível de duty cycle (0–100%) para uma escala de 0–180°. Caso queira controlar um servo real, será necessário ajustar o período e largura do pulso para o protocolo típico (50 Hz, pulsos de ~500–2500 µs).

## Funcionalidades
- Geração de fade suave usando buffer pré-calculado com valores quadráticos.
- Transferência contínua desses valores ao PWM via DMA de forma circular (reiniciado na interrupção).
- Exibição em tempo real do duty convertido em ângulo no display SSD1306.
- Código modular para o display (arquivo `lib/ssd1306.c` e `lib/ssd1306.h`).

## Arquivos Principais
- `DMA_PWM_LED.c`: Inicialização de PWM, DMA, cálculo do "ângulo" e atualização do display.
- `lib/ssd1306.c` / `lib/ssd1306.h`: Funções de controle do display OLED.
- `lib/font.h`: Fonte utilizada para renderização de texto.

## Hardware Necessário
- Raspberry Pi Pico (ou compatível).
- LED + resistor (por exemplo 330 Ω) conectado ao pino GPIO 8 (LED_PIN).
- Display OLED SSD1306 (I2C, 128x64, endereço 0x3C).
- Ligações I2C:
  - SDA → GPIO 14
  - SCL → GPIO 15
  - VCC → 3V3
  - GND → GND

## Lógica do PWM/DMA
1. O buffer `fade[]` é preenchido com valores quadráticos para suavizar o efeito.
2. O DMA é configurado para transferir cada valor para o registrador `cc` do slice PWM correspondente.
3. Ao final da transferência de `PWM_WRAP` elementos, a interrupção de DMA reinicia a leitura (efeito contínuo).
4. O duty atual é lido e convertido para "ângulo":
   \[ \text{angulo} = \text{dutyRatio} \times 180 \]
   onde `dutyRatio = compare_a / 65535.0`.

## Como Compilar (VS Code Tasks)
Certifique-se de ter o **Pico SDK** configurado no ambiente.

### Passos
1. Abra o workspace no VS Code.
2. Execute a task `Compile Project` (gera `Led_pwm.elf` e `.uf2`).
3. Execute a task `Run Project` para fazer o flash via `picotool`.

Alternativamente, linha de comando (ajuste caminhos conforme instalação):
```powershell
# Entrar na pasta de build
cd c:\Users\lferr\Desktop\EmbarcaTechResU3Ex19\build
# Compilar
ninja
# Gerar UF2 (se não existir automaticamente)
# Carregar para o Pico
picotool load Led_pwm.elf -fx
```

## Estrutura Simplificada
```
CMakeLists.txt
DMA_PWM_LED.c
lib/
  ssd1306.c
  ssd1306.h
  font.h
build/ (gerado após compilação)
```

## Possíveis Extensões
- Adicionar controle por botão para iniciar/parar o fade.
- Ajustar mapeamento para controle real de servo.
- Exibir gráfico de barra representando o duty.
- Permitir alteração dinâmica da curva (linear, exponencial, etc.).

## Solução de Problemas
- Display não acende: verifique SDA/SCL e endereço (0x3C). Confirme pull-ups (internos já habilitados).
- LED não varia: confirme conexão no GPIO 8 e que o slice PWM foi inicializado.
- Pico não programa: confirme modo BOOTSEL ou cabo USB funcional.

## Licença
Projeto educacional. Sem licença explícita definida; adapte conforme necessidade.

## Autor Original
Comentado no código: Wilton Lacerda Silva (20/10/2023). Adaptações e README gerados sob demanda.

---
Sinta-se livre para solicitar melhorias ou novas funcionalidades.
