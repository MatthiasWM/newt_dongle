#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "bsp/board_api.h"
#include "tusb.h"

#include "PicoUARTEndpoint.h"
#include "common/Pipe.h"

nd::PicoUARTEndpoint uart_endpoint;
nd::Pipe uart_test_pipe;


void cdc_task(void);

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 4
#define PIN_CS   27
#define PIN_SCK  2
#define PIN_MOSI 3

#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

#if 0
int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}
#endif

// // UART defines
// // By default the stdout UART is `uart0`, so we will use the second one
// #define UART_ID uart0
// #define BAUD_RATE 19200

// // Use pins 4 and 5 for UART1
// // Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
// #define UART_TX_PIN 0
// #define UART_RX_PIN 1

void init_tusb() {
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
    
    if (board_init_after_tusb) {
        board_init_after_tusb();
    }    
}

int main()
{
    stdio_uart_init_full(uart1, 115200, 8, 9);

    init_tusb();

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    // PIO Blinking example
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    printf("Loaded program at %d\n", offset);
    
    #ifdef PICO_DEFAULT_LED_PIN
    blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);
    #else
    blink_pin_forever(pio, 0, offset, 6, 3);
    #endif
    // For more pio examples see https://github.com/raspberrypi/pico-examples/tree/master/pio

    #if 0
    // Timer example code - This example fires off the callback after 2000ms
    add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer
    #endif

    printf("System Clock Frequency is %d Hz\n", clock_get_hz(clk_sys));
    printf("USB Clock Frequency is %d Hz\n", clock_get_hz(clk_usb));
    // For more examples of clocks use see https://github.com/raspberrypi/pico-examples/tree/master/clocks

    uart_endpoint.init();
    uart_test_pipe.connect_from(uart_endpoint).connect_to(uart_endpoint);

    // uart_test_pipe.putc('a');
    // uart_test_pipe.putc('b');
    // uart_test_pipe.putc('c');
    // uart_test_pipe.putc('d');
    // uart_test_pipe.putc('e');
    // uart_test_pipe.put_ctrl(1, 4711, 1501);
    // uart_test_pipe.putc('f');
    // uart_test_pipe.putc('g');
    // uart_test_pipe.putc('h');
    // uart_test_pipe.putc('i');
    // uart_test_pipe.put_ctrl(2, 4711, 1501);
    // uart_test_pipe.put_ctrl(3, 4711, 1501);
    // uart_test_pipe.putc('j');
    // uart_test_pipe.putc('k');
    // uart_test_pipe.putc('l');
    // uart_test_pipe.putc('m');
    // uart_test_pipe.putc('n');
    // uart_test_pipe.putc('o');
    // uart_test_pipe.putc('p');
    // uart_test_pipe.putc('q');
    // uart_test_pipe.putc('r');

    // for (;;) {
    //     if (uart_test_pipe.data_available() > 0) {
    //         char c = uart_test_pipe.getc();
    //         printf("Rcvd: %c\n", c);
    //     }
    //     if (uart_test_pipe.ctrl_available() > 0) {
    //         nd::CtrlBlock *ctrl_block = uart_test_pipe.peek_ctrl();
    //         printf("Ctrl: %d %d %d %d %d\n", ctrl_block->cmd(), ctrl_block->d0(), ctrl_block->d1(), ctrl_block->d2(), ctrl_block->d3());
    //         uart_test_pipe.pop_ctrl();
    //     }
    // }

    // // Set up our UART
    // uart_init(UART_ID, BAUD_RATE);
    // // Set the TX and RX pins by using the function select on the GPIO
    // // Set datasheet for more information on function select
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    // uart_puts(UART_ID, " Hello, UART!\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    while (true) {
        #if 0
        printf("Hello, world!\n");
        sleep_ms(1000);
        #elif 1
        tud_task(); // tinyusb device task    
        cdc_task();
        uart_endpoint.task();
        #else
        char c = uart_getc(uart0);
        printf("Rcvd: %c\n", c);
        #endif
    }
}


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    puts("TUD Mount CB\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    puts("TUD Unmount CB\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
  printf("TUD Suspend CB (%d)\n", remote_wakeup_en);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  printf("TUD Resume CB (%d)\n", tud_mounted());
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void) {
  // connected() check for DTR bit
  // Most but not all terminal client set this when making connection
  // if ( tud_cdc_connected() )
  {
    // connected and there are data available
    if (tud_cdc_available()) {
      // read data
      char buf[64];
      uint32_t count = tud_cdc_read(buf, sizeof(buf));
      (void)count;

      // Echo back
      // Note: Skip echo by commenting out write() and write_flush()
      // for throughput test e.g
      //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
      tud_cdc_write(buf, count);
      tud_cdc_write_flush();
    }
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;

  // TODO set some indicator
  if (dtr) {
    // Terminal connected
    puts("CDC Terminal Connected\n");
  } else {
    // Terminal disconnected
    puts("CDC Terminal Disconnected\n");
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;
}
