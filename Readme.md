# Controle de LED PWM via DMA no RP2040 utilizando a BitDogLab

Este projeto demonstra uma implementação avançada de **PWM (Pulse Width Modulation)** utilizando **DMA (Direct Memory Access)** no microcontrolador RP2040 (Raspberry Pi Pico), focado na placa de desenvolvimento **BitDogLab**.

O código cria um efeito de *fading* (respiração) suave em um LED. A principal vantagem desta abordagem é que, após a configuração inicial, o processo é **majoritariamente** gerenciado pelo controlador de DMA, com intervenção mínima da CPU apenas para reiniciar o ciclo via interrupção.

## Funcionalidades

* **Baixo CPU Overhead:** O loop `main` permanece vazio (`tight_loop_contents()`). A CPU atua apenas momentaneamente através de uma interrupção rápida (`dma_handler`) ao final de cada ciclo de brilho.
* **DMA com Interrupção (IRQ):** Diferente do encadeamento (chaining) de canais, este método utiliza apenas **um canal DMA**. O sistema dispara uma interrupção ao fim da transferência do buffer, onde o endereço de leitura é reiniciado via software.
* **Fading Suave:** Implementa uma curva quadrática (`i * i * 2`) para o cálculo do brilho, resultando em uma transição visual mais natural para a percepção humana do que uma transição linear.

## Detalhes Técnicos

A implementação deste projeto baseia-se na sincronização precisa entre o periférico PWM e o controlador DMA, seguindo o fluxo abaixo:

1.  **Geração do Buffer (Quadrático):**
    * Um array `fade[]` é preenchido durante a inicialização.
    * Utiliza-se a fórmula matemática `(i * i * 2)` para preencher os valores. Isso é necessário porque o olho humano percebe a luz de forma logarítmica; um aumento linear no duty cycle pareceria "rápido demais" no início. A curva quadrática compensa isso, criando um efeito visual linear e suave.

2.  **Configuração do PWM:**
    * O PWM é configurado no **GPIO 13** (Led na BitDogLab).
    * O *wrap* (topo da contagem) é definido em 256.
    * O divisor de clock é ajustado para `16.f`, definindo a velocidade da frequência do PWM e, consequentemente, a velocidade do efeito de fade.

3.  **Sincronização DMA (Pacing via DREQ):**
    * O canal DMA não "despeja" os dados de uma vez. Ele é configurado com um **Data Request (DREQ)** vinculado ao `DREQ_PWM_WRAP0 + slice_num`.
    * Isso significa que o DMA transfere exatamente **um valor** (16 bits) do buffer para o registrador `CC` (Counter Compare) do PWM a cada vez que o ciclo do PWM termina (wrap). Isso garante que a mudança de brilho esteja perfeitamente sincronizada com a frequência do hardware.

4.  **Mecanismo de Loop via Interrupção:**
    * Quando o DMA termina de transferir todos os valores do buffer `fade`, ele gera a interrupção `DMA_IRQ_0`.
    * A função de callback `dma_handler()` é acionada imediatamente.
    * Dentro desta função, duas ações ocorrem:
        1.  A flag de interrupção é limpa (`dma_hw->ints0 = 1u << dma_chan`).
        2.  O endereço de leitura do canal é reconfigurado para o início do array `fade` (`dma_channel_set_read_addr`).
    * Isso cria um loop infinito, permitindo que o efeito de respiração continue indefinidamente sem travar o loop principal (`main`).