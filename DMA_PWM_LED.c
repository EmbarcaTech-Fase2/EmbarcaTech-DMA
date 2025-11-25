/*
   Por: Wilton Lacerda Silva
   Data: 20/10/2023

   Este exemplo demonstra como usar o DMA para controlar um LED com PWM no Raspberry Pi Pico.
   O LED realizará o fade in/out suavemente, utilizando um buffer de fade preenchido com valores quadráticos.
   O DMA é configurado para transferir os valores do buffer para o PWM, criando um efeito de fade suave.
   Uma INTERRUPÇÃO é configurada para reiniciar a transferência DMA quando ela termina, 
   permitindo que o efeito de fade continue indefinidamente.
*/



#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

#define LED_PIN 13
#define PWM_WRAP 256

static uint16_t fade[PWM_WRAP];
static int dma_chan;

void dma_handler() {
    // Limpa a interrupção do canal DMA
    dma_hw->ints0 = 1u << dma_chan;

    // Reinicia a transferência DMA para o fade
    dma_channel_set_read_addr(dma_chan, fade, true);
}


int main()
{

    stdio_init_all();

    // Configura PWM no pino do LED
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 16.f);
    pwm_init(slice_num, &config, true);

    // Preenche o buffer fade com valores quadráticos para suavidade
    for (int i = 0; i < PWM_WRAP/2; i++) {
        fade[i] = (uint16_t)((i * i *2));
        fade[PWM_WRAP - 1 - i] = fade[i];

    }

    // Configura canal DMA
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + slice_num);

    // Configura o canal DMA para transferir o buffer fade para o PWM CC
    dma_channel_configure(
        dma_chan,
        &c,
        &pwm_hw->slice[slice_num].cc,  // endereço destino (16 bits)
        fade,                          // endereço fonte
        PWM_WRAP,                      // número de elementos
        false                          // não iniciar ainda
    );

    // Configura interrupção DMA para reiniciar a transferência
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Inicia a transferência DMA
    dma_channel_start(dma_chan);

    // Loop principal vazio
    while (true) {
        //Usado para garantir que o processador não entre em modo de baixo consumo
        tight_loop_contents();
    }
}
