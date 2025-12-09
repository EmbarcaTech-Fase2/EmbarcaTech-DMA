#include "lib/ssd1306.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include <stdio.h>

#define SERVO_PIN 8

// Limites de pulso do servo (em microssegundos)
#define SERVO_MIN_US 1000
#define SERVO_MAX_US 2000

// Parâmetros do PWM para 50 Hz (20 ms)
#define CLK_DIV 64.f
#define PWM_WRAP 39062  // Período de 20 ms com clkdiv 64

// Buffer de animação "fade" (variação do pulso)
static uint32_t fade[200]; // 200 passos é mais que suficiente
static int dma_chan;

ssd1306_t ssd;

// Rotina de interrupção do DMA
void dma_handler() {
    dma_hw->ints0 = 1u << dma_chan;   // limpa a interrupção
    dma_channel_set_read_addr(dma_chan, fade, true); // reinicia leitura do buffer
}

int main() {
    stdio_init_all();
    display_init(&ssd);

    // Configuração do PWM
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, CLK_DIV);
    pwm_config_set_wrap(&cfg, PWM_WRAP);
    pwm_init(slice, &cfg, true);

    // --- CONSTRUÇÃO DO BUFFER DE PULSOS DO SERVO (1000 → 2000 us) ---
    for (int i = 0; i < 200; i++) {
        float t = i / 199.0f; // varia de 0 a 1
        float us = SERVO_MIN_US + t * (SERVO_MAX_US - SERVO_MIN_US);  // 1000 → 2000 us

        // Converte microssegundos para valor de PWM
        uint32_t cc = (uint32_t)((us * PWM_WRAP) / 20000.0f);

        // Escreve no canal A (16 bits inferiores). Canal B = 0
        fade[i] = cc;
    }

    // --- CONFIGURAÇÃO DO DMA (escrita de 32 bits) ---
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dc = dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);  // escreve 32 bits
    channel_config_set_read_increment(&dc, true);             // incrementa leitura
    channel_config_set_write_increment(&dc, false);           // não incrementa escrita
    channel_config_set_dreq(&dc, DREQ_PWM_WRAP0 + slice);     // sincroniza com PWM

    dma_channel_configure(
        dma_chan,
        &dc,
        &pwm_hw->slice[slice].cc,   // registrador CC de 32 bits
        fade,                       // buffer fonte
        200,                        // quantidade de elementos
        false                       // não iniciar ainda
    );

    // Interrupção do DMA
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(dma_chan); // inicia DMA

    // --- LOOP PRINCIPAL: LER PULSO ATUAL + EXIBIR NO OLED ---
    while (true) {
        uint32_t cc = pwm_hw->slice[slice].cc & 0xFFFF; // lê canal A do PWM

        // Converte o valor PWM → microssegundos
        float us = (cc / (float)PWM_WRAP) * 20000.0f;

        // Converte microssegundos → ângulo do servo
        int angle = (int)(((us - SERVO_MIN_US) / (SERVO_MAX_US - SERVO_MIN_US)) * 180.0f);

        char buf[32];
        snprintf(buf, sizeof(buf), "Angulo: %3d", angle);

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, buf, 20, 28);
        ssd1306_send_data(&ssd);

        sleep_ms(150);
    }
}