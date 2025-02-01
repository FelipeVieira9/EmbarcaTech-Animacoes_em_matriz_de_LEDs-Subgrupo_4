#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "pio_matrix.pio.h"
#include "funcoes/mudar_LED.c"
#include "funcoes/scan_keypad.c"


#define Led_RED 11
#define Led_GREEN 12
#define Led_BLUE 13

#define Botao_A 5
#define Botao_B 6

double numeros[10][25] = {
                        {0.8, 0.8, 0.8, 0.8, 0.8, // 0
                        0.8, 0.0, 0.0, 0.0, 0.8, 
                        0.8, 0.0, 0.0, 0.0, 0.8,
                        0.8, 0.0, 0.0, 0.0, 0.8,
                        0.8, 0.8, 0.8, 0.8, 0.8},
                        
                        {0.0, 0.0, 0.8, 0.0, 0.0, // 1
                        0.0, 0.0, 0.8, 0.8, 0.0, 
                        8.0, 0.0, 0.8, 0.0, 0.0,
                        0.0, 0.0, 0.8, 0.0, 0.0,
                        0.0, 0.0, 0.8, 0.0, 0.0},

                        {0.8, 0.8, 0.8, 0.8, 0.8, // 2
                        0.8, 0.0, 0.0, 0.0, 0.0, 
                        0.8, 0.8, 0.8, 0.8, 0.8,
                        0.0, 0.0, 0.0, 0.0, 0.8,
                        0.8, 0.8, 0.8, 0.8, 0.8},

                        {0.8, 0.8, 0.8, 0.8, 0.8, // 3
                        0.8, 0.0, 0.0, 0.0, 0.0, 
                        0.8, 0.8, 0.8, 0.8, 0.8,
                        0.8, 0.0, 0.0, 0.0, 0.0,
                        0.8, 0.8, 0.8, 0.8, 0.8},

                        {0.8, 0.0, 0.0, 0.0, 0.8, // 4
                        0.8, 0.0, 0.0, 0.0, 0.8, 
                        0.8, 0.8, 0.8, 0.8, 0.8,
                        0.8, 0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0, 0.8},

                        {0.8, 0.8, 0.8, 0.8, 0.8, // 5
                        0.0, 0.0, 0.0, 0.0, 0.8, 
                        0.8, 0.8, 0.8, 0.8, 0.8,
                        0.8, 0.0, 0.0, 0.0, 0.0,
                        0.8, 0.8, 0.8, 0.8, 0.8},

                        {0.8, 0.8, 0.8, 0.8, 0.8, // 6
                        0.0, 0.0, 0.0, 0.0, 0.8, 
                        0.8, 0.8, 0.8, 0.8, 0.8,
                        0.8, 0.0, 0.0, 0.0, 0.8,
                        0.8, 0.8, 0.8, 0.8, 0.8},

                        {0.8, 0.8, 0.8, 0.8, 0.8, // 7
                        0.0, 0.8, 0.0, 0.0, 0.0, 
                        0.0, 0.0, 0.8, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.8, 0.0,
                        0.8, 0.0, 0.0, 0.0, 0.0},

                        {0.8, 0.8, 0.8, 0.8, 0.8, // 8
                        0.8, 0.0, 0.0, 0.0, 0.8, 
                        0.8, 0.8, 0.8, 0.8, 0.8,
                        0.8, 0.0, 0.0, 0.0, 0.8,
                        0.8, 0.8, 0.8, 0.8, 0.8},

                        {0.8, 0.8, 0.8, 0.8, 0.8, // 9
                        0.8, 0.0, 0.0, 0.0, 0.8, 
                        0.8, 0.8, 0.8, 0.8, 0.8,
                        0.8, 0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0, 0.8}

                        };

static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
volatile int contador = 0; // Contador que vai mudar o valor nas interrupções, por isso voltatile

// Função que vai pra interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

//função principal
int main()
{
    PIO pio = pio0; 
    uint16_t i;
    bool ok;
    uint32_t valor_led;
    double r,b,g;

    //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    ok = set_sys_clock_khz(128000, false);

    // Inicializa todos os códigos stdio padrão que estão ligados ao binário.
    stdio_init_all();
    //Configurações da PIO
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    // INICIAR LEDS
    gpio_init(Led_RED);
    gpio_set_dir(Led_RED, GPIO_OUT);
    gpio_put(Led_RED, 0);

    gpio_init(Led_GREEN);
    gpio_set_dir(Led_GREEN, GPIO_OUT);
    gpio_put(Led_GREEN, 0);

    gpio_init(Led_BLUE);
    gpio_set_dir(Led_BLUE, GPIO_OUT);
    gpio_put(Led_BLUE, 0);

    // INICIAR BOTOES
    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);

    gpio_init(Botao_B);
    gpio_set_dir(Botao_B, GPIO_IN);
    gpio_pull_up(Botao_B);

    // INTERRUPÇÔES
    gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Estado anterior do led
    int estado_anterior = 0;
    while (true) {
        desenho_pio(numeros[contador], valor_led, pio, sm, 0, 0, 1); // Desenha na matriz de led
        if (estado_anterior == 0) {
          gpio_put(Led_RED, 1);
          estado_anterior = 1;
        } else {
          gpio_put(Led_RED, 0);
          estado_anterior = 0;
        }
        sleep_ms(20);
        }
    
}

static void gpio_irq_handler(uint gpio, uint32_t events) {
  // Tempo atual em (ms)
  uint32_t current_time = to_us_since_boot(get_absolute_time()); 

  // Verificar o tempo que passou
  if ((current_time - last_time) > 200000) { // 200ms de deboucing
    last_time = current_time;
    if (gpio == 5) { // Se botão A
    contador++;
    
    if (contador > 9) {
      contador = 0;
    }

    } else { // Se B
    contador--;
    
    if (contador < 0) {
      contador = 9;
    }

    }
    
    printf("Numero: %d\n", contador);
  }
}
