#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/sync.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 6
#define I2C_SCL 7

static uint8_t addr_ds3231 = 0x68;

static uint8_t dec_to_bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

static void ds3231_set_time(uint8_t sec, uint8_t min, uint8_t hour,
                            uint8_t day_of_week, uint8_t day, uint8_t month, uint8_t year)
{
    uint8_t buf[8];
    buf[0] = 0x00;
    buf[1] = dec_to_bcd(sec);
    buf[2] = dec_to_bcd(min);
    buf[3] = dec_to_bcd(hour);
    buf[4] = dec_to_bcd(day_of_week);
    buf[5] = dec_to_bcd(day);
    buf[6] = dec_to_bcd(month);
    buf[7] = dec_to_bcd(year);

    i2c_write_blocking(i2c1, addr_ds3231, buf, 8, false);
}

static void revisar_sincronizacion_usb(void)
{
    static char rx_buf[32];
    static uint8_t idx = 0;
    int c;

    // Lee todos los caracteres disponibles en el buffer USB sin bloquear el bucle
    while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT)
    {
        if (c == '\n' || c == '\r')
        {
            if (idx > 0)
            {
                rx_buf[idx] = '\0';
                int Y, M, D, h, m, s;

                // Si llega el formato "SYNC:2026-11-25 20:46:30"
                if (sscanf(rx_buf, "SYNC:%d-%d-%d %d:%d:%d", &Y, &M, &D, &h, &m, &s) == 6)
                {
                    s = s + 4;
                    ds3231_set_time((uint8_t)s, (uint8_t)m, (uint8_t)h, 1,
                                    (uint8_t)D, (uint8_t)M, (uint8_t)(Y - 2000));
                    printf("DS3231 sincronizado: %04d/%02d/%02d %02d:%02d:%02d\n", Y, M, D, h, m, s);
                }
                idx = 0; // Reiniciar buffer
            }
        }
        else if (idx < sizeof(rx_buf) - 1)
        {
            rx_buf[idx++] = (char)c;
        }
    }
}

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    sleep_ms(10000);
    printf("DS3231 Esperando sincronización en formato [SYNC:YY-MM-DD hh:mm:ss]:\n");
    while (true) {
        // ****** DESCOMENTAR SI SE REQUIERE ACTUALIZAR LA HORA Y FECHA DEL RTC ds3231
        revisar_sincronizacion_usb();
    }
}
