

// pico_time timestamp to delay serial transmissions

// --- Cross platform serial port library for host:
// https://github.com/thuanalg/libserialmodule : multithreaded, no device enum
// https://github.com/GCY/SerialPortLibrary : no Linux, very primitive
// https://github.com/wjwwood/serial : huge dependencies, >1000 stars
// https://github.com/alextoind/serial : library above refactored for C++20(?)
// https://github.com/JonathSpirit/serial : two above, but less dependencies!

// https://github.com/andreyplus/SerIO : great looking serial terminal

// --- MNP options:
// Newton Framework : huge, difficult to compile, base of NCX (Simon Bell)
// DYneTK : old, not much testing (Matt)
// DCL
// Escale: huge, quality, complete, no Windows support (Paul Guyot, search for TDCLCTBMNPSerial.cpp)


// =============================================================================
// SPI:

#include "hardware/spi.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 4
#define PIN_CS   27
#define PIN_SCK  2
#define PIN_MOSI 3

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

// =============================================================================
// PIO:

#include "hardware/pio.h"

#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

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

// =============================================================================
// Clocks and Alarm:

#include "hardware/timer.h"
#include "hardware/clocks.h"

#if 0
int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}
#endif

#if 0
// Timer example code - This example fires off the callback after 2000ms
add_alarm_in_ms(2000, alarm_callback, NULL, false);
// For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer
#endif

printf("System Clock Frequency is %d Hz\n", clock_get_hz(clk_sys));
printf("USB Clock Frequency is %d Hz\n", clock_get_hz(clk_usb));
// For more examples of clocks use see https://github.com/raspberrypi/pico-examples/tree/master/clocks


// =============================================================================
// USB MSC Mass Storage Device:

#include "tusb.h"

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


// =============================================================================
// USB CDC Set Descriptor:

static char usbd_serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static const char *const usbd_desc_str[] = {
    [USBD_STR_MANUF] = "Raspberry Pi",
    [USBD_STR_PRODUCT] = "Pico",
    [USBD_STR_SERIAL] = usbd_serial_str,
    [USBD_STR_CDC] = "Board CDC",
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    [USBD_STR_RPI_RESET] = "Reset",
#endif
};
...
const uint16_t *tud_descriptor_string_cb(uint8_t index, __unused uint16_t langid) {
...
    // Assign the SN using the unique flash id
    if (!usbd_serial_str[0]) {
        pico_get_unique_board_id_string(usbd_serial_str, sizeof(usbd_serial_str));
    }
...
}