#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define BUZZER_PIN 21   // Pino do buzzer
#define BUTTON_B_PIN 6   // Pino do botão B
#define BUTTON_A_PIN 5   // Pino do botão A
#define ALERT_FREQUENCY_B 1200  // Frequência ajustada para o botão B (1.2 kHz, menos grave)
#define BEEP_DURATION 500   // Duração do bipe (0.5s)
#define PAUSE_DURATION 500  // Pausa entre os bipes (0.5s)

// Configuração da frequência do buzzer (em Hz)
#define BUZZER_FREQUENCY 100

// Nova melodia de encerramento com notas mais suaves e menos graves
int melody[] = { 1200, 1400, 1600, 1800, 2000, 2200 };
int melody_durations[] = { 180, 180, 180, 180, 180, 180 };

// Função para tocar um bipe com frequência ajustada
void play_bipe(uint pin, int frequency, int duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_clkdiv_int_frac(slice_num, 1, 0);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, (float)clock_get_hz(clk_sys) / (frequency * 4096));

    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 1024);  // Volume reduzido (25%)

    sleep_ms(duration_ms);  // Mantém o som por 0.5s

    pwm_set_gpio_level(pin, 0);  // Desliga o buzzer
}

// Função para tocar a música de encerramento ajustada
void play_closing_melody(uint pin) {
    for (int i = 0; i < 6; i++) {
        play_bipe(pin, melody[i], melody_durations[i]);
        sleep_ms(50);  // Pequena pausa entre as notas
    }
}

// Definição de uma função para inicializar o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(pin, 0);
}

// Definição de uma função para emitir um beep com duração especificada
void beep(uint pin, uint duration_ms) {
    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o duty cycle para 50% (ativo)
    pwm_set_gpio_level(pin, 2048);

    // Temporização
    sleep_ms(duration_ms);

    // Desativar o sinal PWM (duty cycle 0)
    pwm_set_gpio_level(pin, 0);

    // Pausa entre os beeps
    sleep_ms(100); // Pausa de 100ms
}

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int main()
{
    stdio_init_all();   // Inicializa os tipos stdio padrão presentes ligados ao binário

    // Inicializar o PWM no pino do buzzer
    pwm_init_buzzer(BUZZER_PIN);

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Configuração dos pinos
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);  // Ativa pull-up interno para o botão A

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);  // Ativa pull-up interno para o botão B

    // Configuração dos LEDs RGB como saída
    const uint BLUE_LED_PIN= 12;   // LED azul no GPIO 12
    const uint RED_LED_PIN  = 13; // LED vermelho no GPIO 13
    const uint GREEN_LED_PIN = 11;  // LED verde no GPIO 11
    gpio_init(RED_LED_PIN);
    gpio_init(GREEN_LED_PIN);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);

    // Inicialmente, desligar o LED RGB
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);
    gpio_put(BLUE_LED_PIN, 0);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

restart:

    while (true) {
        // Verifica se o botão A está pressionado (lógica invertida devido ao pull-up)
        bool button_a_pressed = gpio_get(BUTTON_A_PIN) == 0;
        bool button_b_pressed = gpio_get(BUTTON_B_PIN) == 0;

        if (button_a_pressed && button_b_pressed) {
            // Se ambos os botões forem pressionados, acende o LED verde e vermelho (amarelo)
            gpio_put(RED_LED_PIN, 1);   // Liga o LED vermelho
            gpio_put(GREEN_LED_PIN, 1); // Liga o LED verde
            gpio_put(BLUE_LED_PIN, 0);  // Desliga o LED azul

            // Exibe mensagem no display
            char *text[] = {
                "Instabilidade  ",
                "  de Rede      "};
            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 24;  // Aumenta o espaçamento entre as linhas
            }
            render_on_display(ssd, &frame_area);
        } 
        else if (button_a_pressed) {
            // Se apenas o botão A for pressionado, acende o LED verde
            gpio_put(RED_LED_PIN, 0);   // Desliga o LED vermelho
            gpio_put(GREEN_LED_PIN, 1); // Liga o LED verde
            gpio_put(BLUE_LED_PIN, 0);  // Desliga o LED azul

            // Exibe mensagem no display
            char *text[] = {
                "      Rede             ",
                "    Estavel "};
            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 24;  // Aumenta o espaçamento entre as linhas
            }
            render_on_display(ssd, &frame_area);
        } 
        else if (button_b_pressed) {
            // Se apenas o botão B for pressionado, acende o LED vermelho
            gpio_put(RED_LED_PIN, 1);   // Liga o LED vermelho
            gpio_put(GREEN_LED_PIN, 0); // Desliga o LED verde
            gpio_put(BLUE_LED_PIN, 0);  // Desliga o LED azul

            // Exibe mensagem no display
            char *text[] = {
                "Rede em Estado  ",
                "   Perigoso   "};
            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 24;  // Aumenta o espaçamento entre as linhas
            }
            render_on_display(ssd, &frame_area);

            play_bipe(BUZZER_PIN, ALERT_FREQUENCY_B, BEEP_DURATION);
            sleep_ms(PAUSE_DURATION);  // Pausa de 0.5s
        } 
        else {
            // Se nenhum botão for pressionado, desliga todos os LEDs
            gpio_put(RED_LED_PIN, 0);   // Desliga o LED vermelho
            gpio_put(GREEN_LED_PIN, 0); // Desliga o LED verde
            gpio_put(BLUE_LED_PIN, 1); // Liga o LED azul

            // Exibe mensagem no display
            char *text[] = {
                "Estabilizando ",
                "   a Rede      "};
            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 24;  // Aumenta o espaçamento entre as linhas
            }
            render_on_display(ssd, &frame_area);
        }

        sleep_ms(10); // Pequeno atraso para evitar debounce
    }

    return 0;
}
