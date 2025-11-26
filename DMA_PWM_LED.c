#include "lib/ssd1306.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include <stdio.h>

#define LED_PIN 8
#define PWM_WRAP 256

static uint16_t fade[PWM_WRAP];
static int dma_chan;
ssd1306_t ssd;

void dma_handler() {
    dma_hw->ints0 = 1u << dma_chan;
    dma_channel_set_read_addr(dma_chan, fade, true);
}


int main(){

    stdio_init_all();
    display_init(&ssd);
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 16.f);
    pwm_init(slice_num, &config, true);

    for (int i = 0; i < PWM_WRAP/2; i++) {
        fade[i] = (uint16_t)((i * i *2));
        fade[PWM_WRAP - 1 - i] = fade[i];

    }

    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + slice_num);

    dma_channel_configure(
        dma_chan,
        &c,
        &pwm_hw->slice[slice_num].cc,  // endereço destino (16 bits)
        fade,                          // endereço fonte
        PWM_WRAP,                      // número de elementos
        false                          // não iniciar ainda
    );

    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(dma_chan);

    while (true) {
        uint16_t compare_a = (uint16_t)(pwm_hw->slice[slice_num].cc & 0xFFFF);
        float duty_ratio = (float)compare_a / 65535.0f;
        int angle = (int)(duty_ratio * 180.0f);

        char buf[32];
        snprintf(buf, sizeof(buf), "Angulo: %3d", angle);

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, buf, 24, 24);
        ssd1306_send_data(&ssd);

        sleep_ms(100);
    }
}
