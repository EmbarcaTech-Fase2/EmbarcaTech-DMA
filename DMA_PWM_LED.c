#include "lib/ssd1306.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include <stdio.h>

// Definições de pinos
#define SERVO_PIN 8
#define PWM_WRAP 256

// Buffer de fade
static uint16_t fade[PWM_WRAP];
static int dma_chan;
ssd1306_t ssd;

// Função de tratamento de DMA
void dma_handler(){
    dma_hw->ints0 = 1u << dma_chan; // Limpa a interrupção
    dma_channel_set_read_addr(dma_chan, fade, true);    // Configura o endereço de leitura
}


int main(){
    // Inicializações
    stdio_init_all();
    display_init(&ssd);
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    
    // Configuração do PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 16.f);
    pwm_init(slice_num, &config, true);

    // Configuração do buffer de fade
    for(int i = 0; i < PWM_WRAP/2; i++){
        fade[i] = (uint16_t)((i * i *2));
        fade[PWM_WRAP - 1 - i] = fade[i];
    }

    // Configuração do DMA
    dma_chan = dma_claim_unused_channel(true);  // Atribui um canal de DMA livre
    dma_channel_config c = dma_channel_get_default_config(dma_chan);    // Configuração padrão do DMA
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);             // Tamanho dos dados: 16 bits
    channel_config_set_read_increment(&c, true);                        // Incrementa o endereço de leitura
    channel_config_set_write_increment(&c, false);                       // Não incrementa o endereço de escrita
    channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + slice_num);            // Define o DREQ do PWM

    // Configuração do PWM e DMA
    dma_channel_configure(
        dma_chan,
        &c,
        &pwm_hw->slice[slice_num].cc,  // endereço destino (16 bits)
        fade,                          // endereço fonte
        PWM_WRAP,                      // número de elementos
        false                          // não iniciar ainda
    );

    // Configuração da interrupção
    dma_channel_set_irq0_enabled(dma_chan, true);       // Habilita a interrupção
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);  // Define a função de tratamento
    irq_set_enabled(DMA_IRQ_0, true);                   // Habilita a interrupção
    dma_channel_start(dma_chan);                        // Inicia o DMA

    while(true){
        uint16_t compare_a = (uint16_t)(pwm_hw->slice[slice_num].cc & 0xFFFF);  // Obtem o valor de comparação
        float duty_ratio = (float)compare_a / 65535.0f;                         // Calcula o duty ratio
        int angle = (int)(duty_ratio * 180.0f);                                 // Converte para angulo

        char buf[32];                                                          // Buffer para armazenar a string
        snprintf(buf, sizeof(buf), "Angulo: %3d", angle);                      // Formata a string
        
        ssd1306_fill(&ssd, false);                                        // Limpa o display
        ssd1306_draw_string(&ssd, buf, 24, 24);                           // Escreve uma string no display
        ssd1306_send_data(&ssd);                                          // Atualiza o display

        sleep_ms(100);
    }
}
