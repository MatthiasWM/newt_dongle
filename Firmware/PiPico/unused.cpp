#if 0

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

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}

// Timer example code - This example fires off the callback after 2000ms
add_alarm_in_ms(2000, alarm_callback, NULL, false);
// For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

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

#endif



// dock, name, dinf, ninf, wicn, dres, pass, gsto, stor, dsnc, cfgn, cres, 
// stme, opdn, helo, rtbr, dbth, path, gfin, ffin, lpfl, lpkg, gfil, file,



#if 0

CDC Terminal Disconnected
CDC Terminal Connected

t[35.999.581]

B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]CDC Terminal Disconnected
CDC Terminal Connected
CDC Terminal Disconnected
CDC Terminal Disconnected
CDC Terminal Connected

t[4.040.406]

B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
t[0.119.728]

B[9600]
B[9600]
B[9600]
B[9600]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]

- MNP / V.42 error correction T-REC-V.42-199303, Annex A
- http://rfc.nop.hu/itu7/V/V0042e.pdf

Link request LR 1
Link disconnect LD 2
Link transfer LT 4
Link acknowledgement LA 5
Link attention LN 6
Link attention acknowledgement LNA 7

- A modem sends a Link Request (LR) frame.
  - Protocol version (MNP class supported)
  - Feature bits (e.g., whether the modem supports compression, synchronous/asynchronous, etc.)
  - Maximum frame size
- The other side responds with a Link Acknowledge (LA) or Link Reject (RJ) frame.

- The modems exchange additional feature negotiation frames.
- If both modems support MNP5, they indicate willingness to use it via a feature negotiation frame or embedded bit flags in the control frames.
- If both agree, they enable compression, starting immediately on the next data frame.

- If MNP5 is enabled, data is compressed per-frame, using Microcom’s proprietary compression (usually a mix of Run-Length Encoding and Lempel-Ziv).

https://github.com/Aleksandar98/MNP5-Data-Compression

t[5.886.027]

	Send a LR (Link Request) frame.

		header len		x
		frame type		1		kLRFrameType
	-constant parameter 1-
							2
	-constant parameter 2-
		type				1
		length			6
		fixed				1 0 0 0 0 255
	-framing mode-
		type				2
		length			1
		mode				2 | 3
	-max number of outstanding LT frames, k-
		type				3
		length			1
		k					x
	-max information field length, N401-
		type				4
		length			2
		max length		x x
	-data phase optimisation-
		type				8
		length			1
		facilities		x


	Send LA (Link Acknowledge) frame.
	Frame header length and type are fixed:
		header len		3(optimised) | 7(non-optimised)
		frame type		5		kLAFrameType
	-optimised data-
		N(R)				xx		rx sequence number
		N(k)				xx		rx credit number
	-non-optimised data-
		type				1		1 => N(R)
		length			1
		N(R)				xx		rx sequence number
		type				2		2 => N(k)
		length			1
		N(k)				xx		rx credit number
	Args:		inFlags
	Return:	true => success



>16>10>02&>01 (LR) >02>01>06>01>00>00>00>00>FF>02>01>02>03>01>08 // fMaxNumOfLTFramesk
  >04>02
  @>00 // LSB, MSB: 0x0040
  >08>01>03
  >09>01>01
  >0E>04>03>04>00>FA>C5>06>01>04>00>00>E1>00>10>03>B9>BF
t[0.013.120]
<16<10<02<1D<01 (LR) <02<01<06<01<00<00<00<00<FF
  <02<01<02
  <03<01<08   // (unixnpi = 1)
  <04<02@<00
  <08<01<03
  <0E<04<03<04<00<FA   // (this block not in unixnpi!)
  <10<03<8A.
DyneTK (same!):
>16<10<02<1d<01 (LR) <02<01<06<01<00<00<00<00<FF, // constant parameter 2
  <02<01<02, // id=Framing mode, size=1, value=2
  <03<01<08, // maximum number of outstanding LT frames
  <04<02<40<00, // maximum information field length
  <08<01<03, // data phase optimisation
  <0E<04<02<04<00<FA, // ?? Is this the compression request? Would make sense. Or is it encryption? Sigh.
D[4,2480]
t[0.017.430]
>16>10>02>03>05 (LA) >00>08>10>03>03>0D
t[0.004.009]
"Request TO Dock"
>16>10>02>02>04 (LT) >01newtdockrtdk>00>01>00>00>04> 00>00>00>09 >10>03V>00
t[0.010.292]
<16<10<02<03<05 (LA) <01<08<10<03R<CD
D[4,2480]
B[9600]
B[9600]
B[9600]
B[9600]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
t[0.006.916]
>16>10>02>03>05 (LA) >00>08>10>03>03>0D
t[0.041.674]
"Initiate Docking"
<16<10<02<02<04 (LT) <01newtdockdock<00<01<00<00<04<00<00<00<01<10<03<97<C6
D[4,2480]
t[0.016.895]
>16>10>02>03>05 (LA) >01>08>10>03R>CD
t[0.005.988]
"Length, Version, Info, Name"
>16>10>02>02>04 (LT) >02newtdockname>00>00>00\>00>00>00H>ED|>17>E0>01>00>00>00>10>10>000>00>00>02>00>02>00>00>80>00>00>>D0>00>00>003>00>04>C69$0$b>C4>01>04>08>A4>A3>82 >E2>99>9F>11!:4>80sg>CF>88>0E>06>14>19>B01>CE>80>11>03>AE4>1D>00>C7a>00>10>03>04]
t[0.014.626]
<16<10<02<03<05 (LA) <02<07<10<03<A7=
D[4,2480]
t[0.076.982]
"Desktop Info: TDCLDockCmdDesktopInfo, kDDesktopInfo, fEncryptedKey: length, protocol version, desktopType, encrypted key,
		session type, allowSelectiveSync, desktopApps"
<16<10<02<02<04 (LT) <02newtdockdinf <00<00<00z <00<00<00<0A <00<00<00<00 <E4J <F0<00<00<02<84 <00<00<00<01 <00<00<00<00 <02<05<01<063<00<07<14<1C<88C<06<0E<1A<05<05<D8<9CQ<A0 <0F<9A:v<D8<C8<89<A3`<C1A4v<88<E0<B9#g<C1<8E<01Q<06<A0<19<A0g<C0<9D<01r<06<C4<190b<80<91<AAW<AF><1D`f<EA<006]<B1j<C5"<96<EC<9B<B1i<BF<DA<81<B8<C0<E1<80<0E<0D<03<00<10<03<F2<CA
D[4,2480]
t[0.014.190]
>16>10>02>03>05 (LA) >02>07>10>03>A7=
>16>10>02>02>04 (LT) >03>14>0D"t>13>87M>9C4>11>1FDl p>96>13%>03>948b>16>00>10>03>A1#
t[0.009.858]
<16<10<02<03<05 (LA) <03<08<10<03<F3<0D
D[4,2480]
t[0.042.719]
<16<10<02<02<04 (LT) <03<13<0D"t<A3<87<8D<19<AC<0D<1F6<14<12<00<10<03'T
D[4,2480]
t[0.017.300]
>16>10>02>03>05> (LA) 03>08>10>03>F3>0D
>16>10>02>02>04>04X->BA9S>07>8D>1D>A18>03>00>10>03c0
t[0.008.645]
<16<10<02<03<05<04<08<10<03B<CC
D[4,2480]
t[0.037.292]
<16<10<02<02<04<04v+<DA<B9<C3<06<0E<C4<BF<03n<05<00<10<03<DF<F1
D[4,2480]
t[0.017.477]
>16>10>02>03>05>04>08>10>03B>CC
>16>10>02>02>04>05l>B5>CE!cG>AE>C3>05>BD"@Zf>ADE>92;>01>00>10>03>B5l
t[0.010.237]
<16<10<02<03<05<05<08<10<03<13<0C
D[4,2480]
t[0.058.226]
<16<10<02<02<04<05<82<F1<CE!c<E7m<C3<05><1C\<01w<05A?M<01<00<10<03t[
D[4,2480]
t[0.017.043]
>16>10>02>03>05>05>08>10>03>13>0C
t[2.394.162]
>16>10>02>03>05>05>08>10>03>13>0C
t[0.502.413]
<16<10<02<03<05<05<08<10<03<13<0C
D[4,2480]
t[2.133.799]
>16>10>02>02>04>06u>13>AEA>F3F>0E>CE>00>10>03G>E4
t[0.010.348]
<16<10<02<03<05<06<08<10<03<E3<0C
D[4,2480]
t[0.344.989]
>16>10>02>03>05>05>08>10>03>13>0C
t[2.588.027]
<16<10<02<03<05<06<08<10<03<E3<0C
D[4,2480]
t[0.409.930]
>16>10>02>03>05>05>08>10>03>13>0C
t[1.289.600]
>16>10>02>02>04>07>85>15>DE)S'i>01>05>0B>EE&$>A3&>EE>80>00>10>03w|
t[0.014.867]
<16<10<02<03<05<07<08<10<03<B2<CC
D[4,2480]
t[0.078.939]
<16<10<02<02<04<06<8C<13<9E<A9S<14<B1<C7<00<10<03<AD<A8
D[4,2480]
t[0.018.269]
>16>10>02>03>05>06>08>10>03>E3>0C
t[0.094.432]
>16>10>02>02>04>08>8B>CF>CC>B9>B3>06g>00>10>03>D1>D2
t[0.006.113]
<16<10<02<03<05<08<08<10<03<82<CF
D[4,2480]
t[0.056.964]
<16<10<02<02<04<07<9C<DD8<BE<B3<06")<95<06<12<14x<19s<E6<CB;|<E6<A0Y0a<C0<11<B7<03<DC<88<B5:<C7cK<06<05<18<18<10<10z<86<8D<1D7<B2i<0B<D5<B3f<EF<9A<r<DE,<D80<00<CA<002`<07H<19`%<EB<00<1D<03b<0C<C01<BE<<C4<07r<07<B0<16N|i<D3<AFR<A9Z5<BB<B5<ACW<A8a<A9<92<AD<0F<1E-<FF<01k<C9<0A<08<10<10<AA<C8<06<88<AB<A1<00<00<10<03<13<0C
D[4,2480]
t[0.015.799]
>16>10>02>03>05>07>07>10>03>B7<
t[0.032.448]
>16>10>02>02>04>09>8B>D5>A4a>F3>06g>00>10>03>FB>B5
t[0.013.464]
<16<10<02<03<05<09<08<10<03<D3<0F
D[4,2480]
t[0.088.238]
<16<10<02<02<04<08<A3<A5<C1<C6<1BP5<A4<84J<05<B0<E6<9AL4<1D<B0\m<B7<15<91<DDv<BC<E11<C0o<07.<D0<DEp<06T7<00<13^Q<05<D5Z<03<0E<F0<04[X<F1&<D9<00<01<00<10<03)<97
D[4,2480]
t[0.009.237]
>16>10>02>03>05>08>07>10>03>87?
t[2.213.498]
>16>10>02>03>05>08>08>10>03>82>CF
t[0.567.564]
<16<10<02<03<05<09<08<10<03<D3<0F
D[4,2480]
t[2.430.211]
>16>10>02>03>05>08>08>10>03>82>CF
t[0.023.000]
>16>10>02>02>04>0A>8B>0F'>C6>19>00>10>03>B1>CB
t[0.009.024]
<16<10<02<03<05<0A<08<10<03#<0F
D[4,2480]
t[2.914.592]
<16<10<02<03<05<0A<08<10<03#<0F
D[4,2480]
t[0.045.388]
>16>10>02>03>05>08>08>10>03>82>CF
t[2.580.769]
>16>10>02>02>04>0B>8B>EDp>F6>EC>10>10C>01>04>04>16>98<BUn>C3;>03>14>CF>C19 >00>10>03>16>F2
t[0.008.256]
<16<10<02<03<05<0B<08<10<03r<CF
D[4,2480]
t[0.027.324]
<16<10<02<02<04<09<0A2<E8<E0<00<98<A8<D6<12<85<B0]8<1BM<0B8<07<9Dto<18<87<9Cr?<9E<88<9Dvau<F7<9DV<E2<91g<9E<94<E9!<16\<89<0C<1C<C0<00<02<0B<94<10<10<1FT<F3<E1<A7UnP<E5<F1<D5ZZ<D1p<9E<9A<0D<A9<E7<D7<95<C4e<B9<E5<02<1D<A1<85<15Y{T<E9<17D<01<00<10<03<D6;
D[4,2480]
t[0.021.693]
>16>10>02>03>05>09>07>10>03>D6>FF
t[2.339.895]
>16>10>02>03>05>09>08>10>03>D3>0F
t[0.518.920]
<16<10<02<03<05<0B<08<10<03r<CF
D[4,2480]
t[1.116.409]
>16>10>02>02>04>0C>A7SG>84>D2>BA>00>EC>01>B27>D2>1E`>1B>B7>C3>04>91>0F`>D4(Y>CF>1A6f>D6d>91>F3fA>89>01Q>A8>EA1~;3T>E1T>F3P>1D>1D>95>C6>80>18>03>D0>EB>1E>10>10>00>10>03k>1D
t[0.009.645]
<16<10<02<03<05<0C<07<10<03<C6<FE
D[4,2480]
t[0.038.311]
<16<10<02<02<04<0A56H<12<07*E0!L<15<C6<96<E4<0B<03<10<10<01"<88<02~5fWae<C1<16<98_<D5<01<9E<9A<E4]:<00<9B<08<92<F8^<06<03lh<95U<BCiuEWV<AD%b<A7&<C6<C5<95<0B<03<BC*<D9<AA'r<B5<96vo<BDEVX<8A<FE<16"\<B3V<80<1B<8A/<0E<F0<96V5<9C7<C3<AF<C3<95H<A7<B0V<85<A5*<B3<9E<0E<90<04~T<95z*<A3<D2<BAg"SNMJ<9FWZq<E5<1FV<FA<B5<E5<DFYm<B9X`<B1<CBv<BB@\N<A5<88<04<BC<CD^<10<10<9F<A9NXz^<0E<E7<DDp/<B5N<82<B7<05x)<86<E5<1F<B7<CDZ<F7<9Dv<87<FD<B6<16T<E4vu<C6W<B2Nk<E2<0D#x<E9h<A5<BE<81<A7<AEw<03<F0<F1<EE<8A<03<1C<A6<95<B5<A8<0E`1T<95<8E<C0<B1<C0<DE<CA<AC<D5<14<03|<88m<8C<E0<95l<D5<870/Z)T<19<BD<FC<15Va<10<038<94
D[4,2480]
t[0.010.803]
<16<10<02<02<04<0B<19<98<95<CC46<14<00<10<03X<C4
D[4,2480]
t[0.007.973]
>16>10>02>03>05>0A>07>10>03&>FF
>16>10>02>03>05>0B>08>10>03r>CF
t[2.207.384]
>16>10>02>03>05>0B>08>10>03r>CF
t[0.608.237]
<16<10<02<03<05<0C<08<10<03<C3<0E
D[4,2480]
t[2.389.070]
>16>10>02>03>05>0B>08>10>03r>CF
t[0.599.337]
<16<10<02<03<05<0C<08<10<03<C3<0E
D[4,2480]
t[2.397.967]
>16>10>02>03>05>0B>08>10>03r>CF
t[0.590.194]
<16<10<02<03<05<0C<08<10<03<C3<0E
D[4,2480]
t[2.407.091]
>16>10>02>03>05>0B>08>10>03r>CF
t[0.488.164]
>16>10>02>02>04>0D>A4>11+>8E>18>00>10>03{0
t[0.012.840]
<16<10<02<03<05<0D<08<10<03<92<CE
D[4,2480]
t[2.492.454]
>16>10>02>03>05>0B>08>10>03r>CF
t[0.472.688]
<16<10<02<03<05<0D<08<10<03<92<CE
D[4,2480]
t[1.844.623]
>16>10>02>02>04>0E>BB;GT>D3>DA@>F0>E1>03>8A>1F>1F>90{>80r>05>CC3>A2y>1E}z>F5>EB>D9>B7o>EC>CE>FFi>D4>FC>E47>9A_>9F^@>85>11->B0>C1>00V>0C@FU>FD>0D0>DAFQ>19>F1>1DP>1B9>14>00>10>03>A0b
t[0.011.657]
<16<10<02<03<05<0E<07<10<03g>
D[4,2480]
t[0.009.134]
<16<10<02<02<04<0C}:h<80(*i0<E8k<16b<B8@<0A<89.<1A1[ai<17<A0<C8d<BEK<96<1A^a<1Co<C0<E5<AA8<C0<C7<DB<82<A7<1DV<16k<D5<1DU<A7<15m<F3<DA]<89<BB<F0{t<E5<E65<87Z}'n<A5<DA<91}<E3<88<19/<D0<82<B0I<F3&<EEo<14<9B<FB<D5~l<99I <C93/<E0h<99gv<F5<9B<DD<03<98*j<DBf7<FB<81<B0c<B3E<95vh<82<AA-o{<9B<B8xn<84G<B53<D9Z<BD<DD2xJ<E0<AD<DDo<A7<E9<FAn<E9<EF<E5k/<DC<8BZE<16VT<C5<BE<00]0B<CB<A1<8B<1B<FD<DD_<D9<9C&n<1DvV<B9<8C2x<DFAE<D5<87<BF<11o<E2m<E0<DAN<15<AF<8B<9A<FF<FC<97<EA<E3<F7]X<95<92<F5<1BU<EE<071<C0<CD<C2<F3V<F4[<88<DE<0D2<F9<EF2<C4!<F2r<0F`f<D1<96<CB7|<F6<F1ZW<C5<E4]<91}g<BA<D8<10<03<A0<88
D[4,2480]
t[0.010.799]
<16<10<02<02<04<0D#<1E<EFm)<F9<DC<16Vd<BB<AF<DEw<!<EF<FE<A7<82s<F8u~x<97<EC<FE<0F#<97<FC<D6<C9)<0B<FB<B7>16v>10<B9>02{>03<15>05<B7>0C<EB>07<1F>10<92>03<B5>C6<16>FEU0<EC<AC<15<FB<BE<BA<1F8<E6<80}5X<F8x<8B<84tI85<FB<A9<A9P<E5<A9s<8AX<FD<96s<E6<AB<F9<EE<D5[<BFYUi<F3<99<870<00Zo<81<DE<9BXEg<C4<E4<C5&5<AB^<A4dM*<E2<00<01<00<10<03<83<F4
D[4,2480]
t[0.017.085]
>16>10>02>03>05>0D>07>10>03>97>
t[2.520.269]
>16>10>02>03>05>0D>08>10>03>92>CE
t[0.258.111]
<16<10<02<03<05<0E<08<10<03b<CE
D[4,2480]
t[2.208.200]
>16>10>02>02>04>0F>EE>99>16Q>00>10>03>F9Y
t[0.007.535]
<16<10<02<03<05<0F<08<10<033<0E
D[4,2480]
t[0.518.525]
>16>10>02>03>05>0D>08>10>03>92>CE
t[2.443.096]
<16<10<02<03<05<0F<08<10<033<0E
D[4,2480]
t[0.554.635]
>16>10>02>03>05>0D>08>10>03>92>CE
t[1.461.233]
>16>10>02>02>04>10>10)>C2>E7P>00>10>03>01>DE
t[0.014.714]
<16<10<02<03<05<10<10<08<10<03<02<C8
D[4,2480]
t[0.884.185]
>16>10>02>02>04>11>F2>A96>00!>AD>1D>80>DFl>03>D4>F6>9D>7F>00>0A>E8>DC>02>D0IG>9Du>D8i>C7>9Dw>8A=(>DEF>12>86>18>1Ez>EA>B1>B7>97>86>1CzhUn">86W>A2b'>9>00>14&ReUT>1Cnd>1B>1E >E2>14>00>10>03?>A9
t[0.011.208]
<16<10<02<03<05<11<07<10<03V<F8
D[4,2480]
t[0.065.470]
<16<10<02<02<04<0EMCD<83J<04L]<A8<8F<B4-p:<F7<BBC<05<DAW<E3/Z<19<B2L<0F<10<10<00<10<03<CAO
D[4,2480]
t[0.015.301]
>16>10>02>03>05>0E>08>10>03b>CE
t[2.392.270]
>16>10>02>02>04>12-F$Jk>08>CCH\>8DD>96>99cs>04>F2h>E0>8F>09>0A>C9>A0>9BFFX>DE>92>16:9@>86>1Bv>F8!>95T>91>D8%>8A>81r>89>A5>97>E1>85>E9>1D>99>FE->F0>C1>96>88~>08&Uc>82>08>C3>005D>14>00>10>03m>82
t[0.005.484]
<16<10<02<03<05<12<07<10<03<A6<F8
D[4,2480]
t[0.065.354]
<16<10<02<02<04<0F,6<C4<C3<8C8<C2<A6cm'<B8^<BBF<03<18<AC<15<90<DE<1FF<DEo<BC<FD$WC<01<00<10<03<F7/
D[4,2480]
t[0.014.460]
>16>10>02>03>05>0F>08>10>033>0E
t[2.028.310]
>16>10>02>03>05>0F>08>10>033>0E
t[0.786.564]
<16<10<02<03<05<12<08<10<03<A3<08
D[4,2480]
t[1.640.598]
>16>10>02>02>04>13+>E2>14>00>10>03>D4>C1
t[0.005.697]
<16<10<02<03<05<13<08<10<03<F2<C8
D[4,2480]
t[0.585.489]
>16>10>02>03>05>0F>08>10>033>0E
t[2.349.029]
<16<10<02<03<05<13<08<10<03<F2<C8
D[4,2480]
t[0.129.696]
>16>10>02>02>04>14>9F>85>F6>94C*>00w>C2R>03<>E6>E1S@E>B5Ex>8B>0E>90>A5U>EA>E5>D6>90>1A>11>05>00>10>03.>B2
t[0.008.676]
<16<10<02<03<05<14<08<10<03C<09
D[4,2480]
t[0.046.963]
<16<10<02<02<04<10<105<C6<91QC<E7<14<90<00<02/<B9<C1F<1Cg<BC<E4<D8<1Dk<BCd<07<1B}<D0<A4<80<19u<A0A<C6<1Dh<1C<BA<00<1Cr<9C<C1F<1Al<B8<BA<C0<BF<F7eD<DD<F7<D6<E3<A7<15VV<E9<AC<9D<99<DAA<C6a<19<8CBE<1E<82<BF<A5?_<B2<8C<7F<D4<B9V<97<0E<A0f<0F#?<BC<A8<C4<E0<A5<ED<BD<BFk~<06><918/<AApWm:,<FE<C4<BF<1D9<80<BF<0D<09<D4P<13<9B<0C <90<03<B6<CC<01c<D0C?<11<00<10<03<CC<8F
D[4,2480]
t[0.011.621]
>16>10>02>03>05>10>10>07>10>03>078
t[2.406.405]
>16>10>02>03>05>10>10>08>10>03>02>C8
t[0.435.241]
<16<10<02<03<05<14<08<10<03C<09
D[4,2480]
t[0.619.632]
>16>10>02>02>04>15>8B>BD1G>1A>A3>8DZ>EA>A9>A9>82>C7>EA>00>AEF>05>AB>AC>16>D6:>C0>AD>0E>05>00>10>03>E4>E3
t[0.012.501]
<16<10<02<03<05<15<08<10<03<12<C9
D[4,2480]
t[0.189.807]
<16<10<02<02<04<11<A3<D1<B6<07<FE<06T<10<10<07<1En<A0<E1g<02<0A `<C6<1Bd<D8a<87<02<0F<10<10j<C7<1Du<E0a<C6<1Dl<C8<11<C7<D9<0B<BC<C1F<ABu<90<F1<06<E0d<D4<A1F<12<8A<C3<A6@<D5p<10<10<11<F9<D9<0F<1C<81F<19x<A8<A1<06<1Au0<81<A7U5<18P<F3<00<04d \<09<A7<F8<B1<C1<0E<A8<00<C2A<07-<A4<02<88<07<CA<AB2<80<02%H<CF<E3<0A%(3<00<05<D27 <C6<00H<082@<03'<14#=<03<F23<00<C2<0E<F2+p<8CUD<E8<7F}<09<EFc'<7F<03<08T<94<80U%<94P<FD<FC<03<C8<BF<80<1C<04<10<10<10<10<C8H!<B4 <BD<03<F2;`<D5',!(<C2<F4-<04<B5<00<0BV<1D<12<14<03<1D<F9<11T<03><0C<F0>'9<05<E5<00<0B<03<04<F5<C0<08*<0C<D0<C1V<F3<AB<F5~@AA0RP<11<F0 <FD<03<F2?`<15=<81<04<C2AP<12<18"<FDb<03<10<03\<86
D[4,2480]
t[0.010.799]
<16<10<02<02<04<12<9C<F6@P<13P6@<04<F2G<10<10<14<05;<D8<F4>;t<0DP<01^<BA<05e<81q<03<DC4]H<D3<91m<8D<97A]<D0<D7_<84<9D<E5<16<01<17<04<85<81T<19<F8<15<98Yj<01x<CA<1F<05>16<1C>10<A0>02<01>03q>05<0A>11H>07P>10E>03<AAVg>F8<18!<07<1Cp<90a<E8<11<AC<92<A1<80<03<8E<99<E1<06<19<AA<C7t<B9<12h<DCa<C5<1Br<DC<A1<00<03<98<AA<11<D3<1Dx<90<DAd<91H<DE!<07<A9<A1[i<05<1BP<B2:%M<12X9<05<199<F6<88<06<12q<CC<CDG<95FN<E1<86<1A<18<A6<A1<80<05L<D81E<1DK"$<C7<1Bl<D6qG<EBX<D8Q[<03b<D5<8E<D5<F4<0FhQ<87<E1hl<E8!<8A<87B<80!<1Cr<E4<81<06<9Bo<AA<BE@<A8<16W<9A4<82<15<0Cp<B3va<F1<A6<1D<D9<CD*<A0<00<97r<E01<C7<01<0F<19+T<19h<A8A<A8<13h<D4<99<00<1A<86^]'<03<84<9E<10<03<ED<83
D[4,2480]
t[0.010.806]
<16<10<02<02<04<13<81<C6<1El<DE<B1<C6>16<D6T<E0<B1z<9D<08<D8K'<A0r<D2<99k<AFhP<DE<A1<15>10<E2>02<CE>03<F1>05R>12<14>06<FF>10<DE>03<C1>A7<00h<06%<03<EA<C6<11E<90q<86<15X<92<DAe<98R<92<1Aj<CD<88<9E<0B<C1Kg<94<A1<86<9A<D3/`<EF<19s<C8A(<93<07<1C4<90<B7<03<A3<D1<E2<1D/*<00v<14<15~<BD<06<1A;<C7!<14<D84gI<13<02v<E4LS<03~<EB<FDG<ACu<EC<FF<92<1D<1E7I<C7<C1u<F0a<05mfx<EB<E6<94<8FW:@<00<10<03<02<A1
t[0.009.941]
>16>10>02>03>05>13>05>10>03>F6X
t[0.191.834]
>16>10>02>02>04>16>9A>C1>A5>D7C8>05>00>10>03>ED>C5
t[0.006.219]

D[4,2480]<16<10<02<03<05<16<08<10<03<E2<C9
D[4,2480]
t[0.073.031]
<16<10<02<02<04<14<A3<D1<B6<07<FE<05<B40G<1A<86<FA<99<C0<D6f<BCA<86<1D<A1<13<EAw<1Dx<98q<07<1Br<C4<01<F8<1B<8B<93<F1<06<E0dDNy<DB<0AT<0D<07<11<91<87~<04<1Ae<E0<A1<86<EAu<B4<9E<86U5<18<C0<8Bp%<9C<E2<C7<06;<A0<02<08<07<CA<A7<02<88<07<CAK<AF<C0<07+\/<BF<02Vq<01<C2<0A%(3<00<05<D27<F0<FE<13<15<9DP<8C<F4<0C<E8<1F<C2<0E<0C<1Dc<D5<12<FA__<C2<FBWl<D7<00<02<15%`<15<0B<1Dq<B4<DD<02V<9D<C2<12<82A)<D0<82M9Y<95<89r<0B<B0`<D5"A1<D0<11{j<BDoIP<0E(V<A0[<03<90<12<D4<03<98<CD<FF<D8<00<BA<04<05AP<11<00<D8<D8<00<0B6x<80<04<12Z<98!<AD'<AA<E8<A2<8C7<EE<18$<F4\*<C9<A4<93_FI&<85Vv<9F<06<19x<BCq<A5<DEv<8A<A7<A5<02<0Fp)<9D<98T2<90<A6<9E|<10<030<A1
D[4,2480]
t[0.010.798]
<16<10<02<02<04<15<02*h<A2<8E<9A<01<A9<A4<94<C6<91<E9<A6<A2rX<EAy<AF<C6<0A<9C<AD<C6.<CA<1B<1F<1C"<18<EAo<BCaeU<8C-<E3<DD<EC<00<02<09<14P<13<7F<00<0D<DE<B9<0D <BD<B4<D3BEM5<D6H<07OS<DAo<8B>16<8D>10<B6>02<DA>03<D0>05<DF>14->07]>10<DD>03oF<E7>F9Me<DF<81+0x<E0<89<B7Z<C7<D6<0Dc<DEP<00<10<03<F3[
D[4,2480]
t[0.008.430]
>16>10>02>03>05>15>07>10>03>179>16>10>02>02>04>17>CCb>E6>D0P>0E>05>00>10>03>D8>F6<16<10<02<03<05<17<08<10<03<B3<09
D[4,2480]
t[0.076.997]
<16<10<02<02<04<16<A3<D9<D1o<1A<10<10<0D<10<10<00<10<03n<DA
D[4,2480]
t[0.017.797]
>16>10>02>03>05>16>08>10>03>E2>C9>16>10>02>02>04>18>D0:;>ED>00>01>00>10>03>CEo
t[0.013.934]
<16<10<02<03<05<18<08<10<03<83<0A
D[4,2480]
t[0.055.533]
<16<10<02<02<04<17<A3<BBnqC9<F8I<00rn.go<85<0B<9C0@<11xk<87<BCV[<80<07<A4<F7<87<F9{<C4<00>Y<C1k<00<10<03<03<AF
D[4,2480]
t[0.013.620]
>16>10>02>03>05>17>07>10>03>B6>F9
t[0.009.534]
>16>10>02>02>04>19>8B>CD>B1>15>8B>03>04>00>10>03>E4'
t[0.015.257]
<16<10<02<03<05<19<08<10<03<D2<CA
D[4,2480]
t[0.033.164]
<16<10<02<02<04<18<A3<BD1<87<9E<0DQ<B0Hr=<A2A<C3<1E{<E0[<00<AF<0D<E9<DB<90<99fF<AB<D1"<B2<18r<C9&<FB<EEK<00<A2<0D<11<00<F1<04_4<94<B1wXB<<00!<03t<E2<D0<BE\Y<F5<1B<8B<95<92E<F6<1Abi<C5<0Axj<F20@<CF8<80<A7<E8o<BF<AD<05<95V(<0Fp<D8ox<88e<F7<00<BD<0D<10<10V<0C<E0i<A5<E8Zki<05<F3<002<8B<F5<96<DC_<BDU<B4<99PY<1CvC<D5<D6<8E<ED<00<DAj<D5<EDW<E0<E2F<EE<00<E6f<87<07<96Q<A0<A1<07<96q<B42<C2<15r@<EA<84<E1#<B0<F9<A6<EA#<B02<02<0D<<F0P<03<0C<BF<F3<80<C3<0B#<10<101<C7<1Co<A01<02<ADs<98<8EF<1D<CB<B7n<06<E4<03L<A0(<C5<10<10<01L<00<10<10<0DQ<0F1<01`<F0KBC<07<C0<D3<10<10<02<9C<98<DFP<02<D04t<02<F9 <F3K<B1<C8<03<10<10@<0A<C5<C4P<8C<0E<BF<08<10<03<D2<E9
D[4,2480]
t[0.009.554]
<16<10<02<02<04<194<04<C2<10<10<FB"<80E<15u<18<10<10<8E<01G<C8<11<C5<92<AD<FBM<C6<1Bk<09#<8C0<0D<85<E0<11<AF UA<08%d<B<A8K5YaF<1Dl<CCA<15N<0D<91<D0<13D@Yq<0E<1B<B6<F4<9BGKw>16<C0>10<F4>02<C6>03R>05M>18E>075>10<00>03N>86:>FAe<85<D6<FD"<E5P<81-<F5<D4<95k<AFh<F0<E5<14U<0Dq`<D8<00@<052<08><8C8V<C7<1D<81<02<E6UV\<ED[@A5<F0k<FF<00<06<C0<B0<FF<BE<04<19<14\<01P<0DP<00 <FC<82<C2//<FCb<D3P<06<A1<01<85<82<03<D7$a<C6<1Bd<D8<F1<D6k<1B}$<92!<A9<F4<00<0DRs<E3a<C6<1Dl<04<BF<DEk<A0<C1W<851<C4h0<C6<1B<8B<C3<E4<DF`<01<FE$<92<05<EA<0C<01<0E<19<91S^<A1k<83m<D0<9DH^<FC<E0<03>U<C3AD<E4<07<0E<11<9As<80l<C0<01<0B<C8<90<1DA<07F"<F3><05<00<92<10<03%<11
D[4,2480]
t[0.009.562]
<16<10<02<02<04<1A<ED<81<07O<BE<1F<02<80V<D5<E0<D7<03<1F<80<10<10<C2<0AA)<A0<16<80<FA<96<00<F0<0B<05<99?<9Dn\4d<00<8B<BA>16<E5>10<AF>02<1B>030>05<82>19<82>07C>10<BF>03o>D7<0E:-<EA<D0u<03<F04`/<A5<B4<F0<06<11g<9C<01<85<1C<FD<C2a<05<1Aw<E0<F1<DBk"<A8X<85/<EE<F8<01<8C<12o<C8Q<06LI<C4<81<C4<1El<F8<BD<1EX<AE2@<C3<11<0C\<96<07<1A<A0<8Ej*<AA<AB<BEv)<85U<D0<13J<07<0EXi-<1A<A5<9E<9A<AAk<0D<ED<10<10<E4<01<18<0C<B09<1A<01<00<10<03&<87
D[4,2480]
t[0.007.536]
<16<10<02<02<04<1BO<83g.TTM<FD<9BVZ<BFE<95vn<7FU)D<1A<B8<BA<C1<04<F6<88<E1wSq<90>16=>10<D8>02$>03<F9>05<FE>1A<EB>07<D3>10<CC>03J'<0F:P<C2<00<93shy<E6<01w<E5<FDa1ku<98U><83<87<B8X<09<83G<96<E0WuE<95VaYu<D8o^<8F5@<19<02~<A5<95<1E[S5<B3Vjo<DD<F4<D4X<91EU<F8<03<9C8`,<C3Lr<80<95U<CC<EE<D9<00<17<0Fx<85 <E9P<A3<D2~i<B08<D8<FA<B2<D1v?<04<F7<B3<B0<FF<01<0E5<F4A<90<CF<F9<B1<01%<1C<10<10<D2<01<0B<A7<F8<F1<87<1F<1E<F40<C0<1F<00<7F<B0gt<14<DD<8F<C4<FD<85<12 <D1<00<17<14/Yp<08<00:<C0{<E5<9A!<8C+<CE<90<0AGf<9B<FD<15XC<AD<0E<D8N<06?<E8<83D<1CvL<89F<EBw<A0QGL75<F49<AF<B3Ad<9Bh<D5<89FMp<086<84<C1<EBC<FA<C1<01%<1D<10<03f<BA
D[4,2480]
t[0.009.560]
<16<10<02<02<04<1C0<E2A<0B<05<A8?<00<F1<10<10<17<D0'<02<B9%<90<E2<00<09`<D2P<02<B00<14[D<A1K5<BAT <84<94<FA<00<13<98<14<FB<0E<00O0}Qg=*!D@<FD<10<10J@G@<D1<D2<1D0<DD<F4Z<F4<10<10'<00<06>16<BF>10<08>02<9C>03U>05h>1Be>07<F7>10<8B>03<14vC>FA<17<CF<<E0<D8<C0<C2<CEQ<E9N<1E<B8<EA<835y\<91<86i<A8!<04XC<19<B8z<8B@<1E(<A3c<1Ddt<EE<B9<AB]<94<80O<1E<9D<9Eq<04<1Ai D<EA<19F<A0<06<D1<04nA<94@<A3R<D1<D3<D0j<10<10/<10<10\<02<85n<F0`CM<96H<D6<00<10<10`?<01$<ACO?<12<BF%<A20Y<89<E4<BB><E01A<8CCL<1A<ED<BE<BB-D)<B8<1A<03<10<10<1BHQ<9A<FE<A7<D5<B4a<1Ci<B0QG<A9f4<95<FE`'<908<80<022"<A8<1D<FD<921"<D9<F3<0C<90<D2<D0<03<87<0D<B0<02<8D<1E<DA<A8&.%r<10<03<19<C3
D[4,2480]
t[0.009.562]
<16<10<02<02<04<1D<A4&;<921 <19<8E<08*?@<07<AE.<11<8D<16><94?j<14<15<CE<BB<B7<AB<EE<C8`H<1E<D3<D6a<07>16<1B>10<C1>02C>03<A4>05}>1CC>07<01>10<00>03<10>C7<03;<1A<CE
D[4,2480]<16<10<02<02<04<1E<A2<89<F4<8B.<06<C0"G<E4d<C4<C1F<1Fd<EC<E7+t<AE<0AtF!<E5x<EA<EE<14<A4<A6<B1^C<C1<0AW<C5'<D3<DC<E1J<B8p<8C+<07<98o@u<ED<80<C5<E0#>16<C9>10<1C>02i>03<90>05<B1>1D<07>07<BE>10<02>03<17>96<DC>FB<D0<0A'<D7<BC<F6<CD<03X<AC<95<CE<9BC<85gC9h<AC<E8o<BF<AD<A5<F4<00\Y=<C0oSSU<F4<0B<E0i<1D<16<E4<AF=<04<179<10<10u<90t<D2L7<B4<F4<00<04<81<EC<A7s<0D1<80<DB<00<0C<E0<D55<10<10X<D7<D65 <0D<F1]<EE4<BE<EC<A3<04<1Bg,<09<D1<A5<E5><A2<07<1B5<E0a<07<1AK<90Q<C7<19z<10<10<8E<86<11G<B0:<82<ABGd<91N<0B<7F+<1E<<98jXqGSq<A8<01Q<FB<E5<96q<8F<09<DEX~<04<1B0I<11<D5<AE<FA<937<80<09<AE<8A<03K@<B5<AC<C1<AC<1AX<CCA<C4<1D<B5<DBaF<F0<03<99A<D5`C<A4<C9<00<D2<7F<10<03<0F<95
D[4,2480]
t[0.009.562]
<16<10<02<02<04<1F^<FDtC<0E03<00<01?7<84<FA<00<A3@d<F1<9F<0E<E0<13<12<C0<0F<D0<D0<D0<11|<06<D7@<A4<0D<90<DF<00<D9<0D<10<10<D2P<03u<0E<D0<C0<A2/M<D8<00@<EE<7F<ED<00<0F<EEc<EF@<A1<0E<B8$<DD<80~<8Cr<C9<16v<A4:G<B2w<D8<81<04W<83<F9X<AE<07$<88QFYs<A4U<07[O>16<18>10a>02n>03C>05%>1E<B8>07<BA>10<C6>039f<F4>FBHpF<98hl<EFe<F6wH<11<C7<1B<F35d7D@<E9C<01<08<E1X<1F<07iV<CC<81<06<1Ag<F8<C7<BA<AB<C3<8C<93<8B<0C<C3in<1Cr<FF<0F<90<FF<BA<F6<E1g=<1Cp0{<C6<15l<C0<81<86<1Cxt<CF<90<AB<EB<10<10`F.<E8<EDa<C4<F1<B8N<81<D0<13<84B5<9B<ABj<80<90<0D<18<84<DE<81<90<11p<9Cq<04<1Ao<90!<DB<00N<95{<0B-`<EC`/<13g<80^<E9`<B6<AD<FB<0A<18<FB<DC`<1D<1B<9C<8B<1E<85<1Cq5<10<03<9E|
D[4,2480]
t[0.009.560]
<16<10<02<02<04 <EEj*"<BCA<07<93N<D6<11<97j<B0A<A4<82<AB(<B8<C1N;Y<F6<18<17<11q<9C<91<04je<90a<86<1B<BC<8F9 @<00<01r<CD<9A<A2<D7N<AAg<D7wV<DF<00/L<9F<00&<F4<CF<D2P>16<05>10<C3>024>03<84>05<C2>1F >07<0D>109>03<017<D1;<1C<A3<0E<90<09D<BE<0C<12<00<0D<8D?@<0F)<0Fp<C5<00<CA<CF0@<0A2<0F@<91<B9<14<D50<80<D8<CF<0E<A0<C5<00<D0<1E{<AA<0D<C6<0E@<C5<009<0C0:<AB<BAB|<C0<BF<8A<0AK<EC<B7\5<AB,<B6<D1<FA<0Bm<B5<D9<0Epj<B1<C7<8A[l<BAD<A7<F9<80<8C<0D<FD<D0<90 <FBd<CC1<FD<10<10<A5<1E<F5E<7F>@KC<0Fp4<C0<03<F44<04<01<053<E3<D0P<B0<EBb<10<10<8B<17<92<00^G<1El<98<A1<F8<8Av<F8<E7<B9<AB<A0D<11<C4<03<A23k<07<1C<86<1FA<C6i<D6<9E<EE<AA<01C<E4<81<0D<A3h<B0<91<07<1A^<8F-<87<10<03E<FF
D[4,2480]
t[0.009.558]
<16<10<02<02<04!<C5<83<9D<E0<EA!<A4@#<89<D5<FAo}<C7<12G(&7<19e7<E4<FB<80<ED\<92<C0<16l<BB<8D>16F>10<E1>02<88>03<83>05<BD 8>07<DD>10<BC>03?>07d7>m3<17<0A<01<CBW?<C0<AB<16&<19<B0<81<91<1D<14b<8A<1F<1EPRB2V<15<E1<C7<07<94<9Cr<C2<1F<CB<0F<E0B<F5<CA<0C<E0<80U)<80<80<8A<18<03<AC<10<10B<FC<03<C8 <82<9A<A8<F8<BF<00 [u<ED<7F<03+<F8<EF<C0<0F<F27<E1W<07<0B1<C0C<01<83<FD0}Q<92i7<80<01<8B<0E<10<10A<BA<11<88m<13<16<0DE@<D6<00<12D*<01:<0D<190<A1<02<E4<1FP<D3<04x4<B4<01<8D<1B<04<17A<A4>5<A4<81<ABi<94`<84'b<CCQ<87<1CwX5<18]<03<E6<B2<04<1C<1B<A4<81F<1C<A2w<DE<D0j<FB&@<B6\j<B2<FC<8B<C6<80Z<05<D6<08<1CV<0AUn<DA<951<C0m<03P<95<1BUV<E5F<16VoQ<A5<DD<1B<10<03<C4<92
D[4,2480]
t[0.010.798]
<16<10<02<02<04"<03T<D7<1BxQ<0CKmxU<80<97<C7<00/<0F<F0<16YVa<95<9BUi<80G<D5<1A<E9<81W<C2<00E<0CP<A9vp<83<A7mn<DD<EEwX<7F<B9<1Df<95<CB<03<1C<96<DB[PQ<05<D6<1C<03<84<9D[n\<C5>16W>10<DC>02<00>03<CD>05<0D!<00>07<16>10T>03BV<F0>F7<1A<0Ck<C0<BD<08<9Ev<BB<F18<C0o<C4<1D<97<E2r-<0E<F0\<8A<D4]'<A4|<03H1<C0<13<03<1C1<9Ey<FBU<DA<9E<8ES<DA<87<DF<00<FA<F16@<7F<00<12<B8<E4<00<16<E7<96<E0<00<0BB<D5 x<11<C6<C9 x<19<A6<F8a<88<E0<99<18f<8A.<C28<C0<8C<ACNrr<00<C0<9A<B9_nm<0A<C8<A1<81s<82w<A7<85l<EA<A9c<85<17<B2<B7_<87<03<80<C8<DC<AC<DCU<F7<A3v<16<B3<88k<93<D3EY<1DT<16<8F<08^yP<B9<18<1D<87<D5UGiX<AEb<F0O<07!l<18<07<98u<1C<C6P<9A<05<C0><C0<04<B0<10<03o<D7
D[4,2480]
t[0.010.798]
<16<10<02<02<04#44<016<C1<0D r<0A&<F9a<95<04<BD<B3 <E0<F9<16x@<F1<07 <B0<B0A<08<8C<F8<AF@<0B'<0D>16<F6>10<82>02J>03<A0>05<B0"<F4>07<14>10'>03<0D>A6)>F7<10<10<A9<01<D8S@<C0<FD<85&<80<C9Y<06g<15<F0<C0<14<90<D0<10<10<058(,r<05<0Eo<C0<82<91)g<D6<F2<D1L<1C}T<B0<EBZ"P<12IHQF<1B<9D<DD1<85<1BjT<81<C6};<D9<B6n'<BEx<D2F<DE<E5<E7<81<C6<14d<98<E1<06<19j@<85S<00<10<03<DBZ
t[0.013.831]
>16>10>02>03>05#>07>10>03>F77
t[0.557.021]
>16>10>02>02>04>1A>D4>CE>15Q>00>10>03>CDJ
D[4,2480]<16<10<02<03<05<1A<08<10<03"<CA
D[4,2480]
t[0.059.647]
<16<10<02<02<04$<9C<B9!<C7<1Cg`<05Q<00<10<03<F5y
D[4,2480]
t[0.019.305]
>16>10>02>03>05$>08>10>03C>06
t[2.482.798]
>16>10>02>03>05$>08>10>03C>06
t[0.019.507]
>16>10>02>02>04>1B>9A>B1a>87>198>05>00>10>03>AA>8A
t[0.005.700]
<16<10<02<03<05<1B<08<10<03s<0A
D[4,2480]
t[1.009.010]
>16>10>02>07>02>01>01>00>02>01>00>00>00>10>03>0B>C1CDC Terminal Disconnected
CDC Terminal Connected

t[1.072.582]

B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[9600]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[9600]
B[9600]
B[9600]
B[9600]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]
B[38400]

#endif




#if 0 // PPP attempt:

PPP uses HDLC-like framing. Network package fragments are packed into an additional 8 bytes
The Link Control Protocol (LCP) is used to establish the connection
Configure-Request, Configure-Ack, Configure-Nak and Configure-Reject

   A summary of the Link Control Protocol packet format is shown below.
   The fields are transmitted from left to right.

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Code      |  Identifier   |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Data ...
   +-+-+-+-+

         1       Configure-Request
         2       Configure-Ack
         3       Configure-Nak
         4       Configure-Reject
         5       Terminate-Request
         6       Terminate-Ack
         7       Code-Reject
         8       Protocol-Reject
         9       Echo-Request
         10      Echo-Reply
         11      Discard-Request

7E = Frame delimiter (marks the beginning and end of a PPP frame)
FF = Standard broadcast address
7D = Escape character (used to escape special characters) The byte following 0x7D is XORed with 0x20
0x7D 0x23 = unescaped 0x03 => 03 = Control field (UI frame)
C0 21 = IPv6

7D 21	01
7D 21	01
7D 20	00
7D 34	14
7D 22	02
7D 26	06
7D 20	00
7D 20	00
A0	A0
7D 20	00
7D 25	05
7D 26	06
63	63
BD	BD
7D 25	05
7D 27	07
7D 28	08
7D 22	02
7D 27	07
7D 22	02
FC	FC
A6	A6
7E	(end marker)

Decoded: 01 01 00 14 02 06 00 00 A0 00 05 06 63 BD 05 07 08 02 07 02 FC A6

Or:
01	Code = Configure-Request
01	Identifier
00 14	Length = 20 bytes (decimal 20)
02 06	Option: Type = 2 (MRU), Length = 6
00 00 A0 00	MRU = 40960 (0xA000)
05 06	Option: Type = 5 (Magic Number), Length = 6
63 BD 05 07	Magic Number = 63BD0507
08 02	Option: Unknown type = 8, Length = 2
07 02	Option: Unknown type = 7, Length = 2
FC A6	Possibly start of next packet's header or FCS (Frame Check Sequence)


>7E~ >FF. >7D} >23# >C0. >21! >7D} >21! >7D} >21! >7D} >20  >7D} >344 >7D} >22" >7D} >26& >7D} >20  >7D} >20  >A0. >7D} >20  >7D} >25% >7D} >26& >63c >BD. >7D} >25% >7D} >27' >7D} >28( >7D} >22" >7D} >27' >7D} >22" >FC. >A6. >7E~ 
t[1.554.341]
>FF. >7D} >23# >C0. >21! >7D} >21! >7D} >22" >7D} >20  >7D} >344 >7D} >22" >7D} >26& >7D} >20  >7D} >20  >A0. >7D} >20  >7D} >25% >7D} >26& >63c >BD. >7D} >25% >7D} >27' >7D} >28( >7D} >22" >7D} >27' >7D} >22" >366 >7D} >3B; >7E~ 
t[1.954.889]
>FF. >7D} >23# >C0. >21! >7D} >21! >7D} >23# >7D} >20  >7D} >344 >7D} >22" >7D} >26& >7D} >20  >7D} >20  >A0. >7D} >20  >7D} >25% >7D} >26& >63c >BD. >7D} >25% >7D} >27' >7D} >28( >7D} >22" >7D} >27' >7D} >22" >7F. >88. >7E~ 



================================================================================
dock, name, dinf, ninf, wicn, dres, pass, gsto, stor, dsnc, cfgn, cres, stme, opdn, helo, rtbr, dbth, path, gfin, ffin, lpfl, lpkg, gfil, file,
---- Opened the serial port /dev/tty.usbmodem11302 ----
t[0.014.522]
>16. >10. >02. >26& >01. >02. >01. >06. >01. >00. >00. >00. >00. >FF. >02. >01. >02. >03. >01. >08. >04. >02. >40@ >00. >08. >01. >03. >09. >01. >01. >0E. >04. >03. >04. >00. >FA. >C5. >06. >01. >04. >00. >00. >E1. >00. >10. >03. >B9. >BF. 
<16. <10. <02. <17. <01. <02. <01. <06. <01. <00. <00. <00. <00. <FF. <02. <01. <02. <03. <01. <01. <04. <02. <40@ <00. <08. <01. <03. <10. <03. <9A. <1B. 
D[4,1090]>16. >10. >02. >03. >05. >00. >01. >10. >03. >05. >5D] 
>16. >10. >02. >02. >04. >01. >6En >65e >77w >74t >64d >6Fo >63c >6Bk >72r >74t >64d >6Bk >00. >00. >00. >04. >00. >00. >00. >09. >10. >03. >B4. >89. 
<16. <10. <02. <03. <05. <01. <01. <10. <03. <54T <9D. 
D[4,1090]<16. <10. <02. <02. <04. <01. "newtdockdock" <00. <00. <00. <04. <00. <00. <00. <01. <10. <03. <BA. <4FO 
D[4,1090]>16. >10. >02. >03. >05. >01. >01. >10. >03. >54T >9D. 
>16. >10. >02. >02. >04. >02. >6En >65e >77w >74t >64d >6Fo >63c >6Bk "name" >00. >00. >00. >5C\ >00. >00. >00. >48H >ED. >7C| >17. >E0. >01. >00. >00. >00. >10. >10. >00. >300 >00. >00. >02. >00. >02. >00. >00. >80. >00. >00. >3E> >D0. >00. >00. >00. >01. >E0. >00. >00. >01. >40@ >00. >00. >00. >00. >00. >00. >00. >01. >01. >E6. >51Q >05. >00. >00. >00. >64d >00. >00. >00. >64d >00. >00. >00. >04. >00. >00. >00. >03. >00. >00. >00. >00. >01. >E6. >51Q >05. >00. >00. >00. >0B. >00. >42B >00. >65e >00. >6En >00. >20  >00. >54T >00. >65e >00. >6Dm >00. >00. >10. >03. >F3. >6Bk 
<16. <10. <02. <03. <05. <02. <01. <10. <03. <A4. <9D. 
D[4,1090]<16. <10. <02. <02. <04. <02. "newtdockdinf" <00. <00. <00. <66f <00. <00. <00. <0A. <00. <00. <00. <00. Key: <5F_ <FE. <F6. <6Aj <5B[ <E3. <DA. <62b :key <00. <00. <00. <01. <00. <00. <00. <01. <02. <05. <01. <06. <04. <07. <04. <6En <61a <6Dm <65e <07. <02. <69i <64d <07. <07. <76v <65e <72r <73s <69i <6Fo <6En <07. <08. <64d <6Fo <65e <73s <41A <75u <74t <6Fo <08. <24$ <00. <4EN <00. <65e <00. <77w <00. <74t <00. <6Fo <00. <6En <00. <20  <00. <43C <00. <6Fo <00. <6En <00. <6En <00. <65e <00. <63c <00. <74t <00. <69i <00. <6Fo <00. <6En <00. <00. <00. <08. <00. <04. <00. <1A. <00. <00. <10. <03. <A3. <333 
D[4,1090]>16. >10. >02. >03. >05. >02. >01. >10. >03. >A4. >9D. 
>16. >10. >02. >02. >04. >03. >6En >65e >77w >74t >64d >6Fo >63c >6Bk "ninf" >00. >00. >00. >0C. >00. >00. >00. >0A. >00. >65e >E0. >BC. >FF. >A7. >74t >1B. >10. >03. >C5. >D2. 
<16. <10. <02. <03. <05. <03. <01. <10. <03. <F5. <5D] 
D[4,1090]<16. <10. <02. <02. <04. <03. "newtdockwicn" <00. <00. <00. <04. <00. <00. <00. <2D- <10. <03. <26& <1D. 
D[4,1090]>16. >10. >02. >03. >05. >03. >01. >10. >03. >F5. >5D] 
>16. >10. >02. >02. >04. >04. >6En >65e >77w >74t >64d >6Fo >63c >6Bk "dres" >00. >00. >00. >04. >00. >00. >00. >00. >10. >03. >9C. >3B; 
<16. <10. <02. <03. <05. <04. <01. <10. <03. <44D <9C. 
D[4,1090]<16. <10. <02. <02. <04. <04. "newtdockstim"  <00. <00. <00. <04. <00. <00. <00. <5AZ <10. <03. <C4. <74t 
D[4,1090]>16. >10. >02. >03. >05. >04. >01. >10. >03. >44D >9C. 
>16. >10. >02. >02. >04. >05. >6En >65e >77w >74t >64d >6Fo >63c >6Bk "pass" >00. >00. >00. >08. >40@ >54T >49I >A1. >CA. >56V >6Cl >4EN >10. >03. >7F. >F7. 
<16. <10. <02. <03. <05. <05. <01. <10. <03. <15. <5C\ 
D[4,1090]<16. <10. <02. <02. <04. <05. "newtdockpass"<6En <00. <00. <00. <08. <366 <5AZ <08. <FF. <A1. <D8. <FC. <13. <10. <03. <DA. <3F? 
D[4,1090]>16. >10. >02. >03. >05. >05. >01. >10. >03. >15. >5C\ 
<16. <10. <02. <02. <04. <06. "newtdockgsto" <00. <00. <00. <00. <10. <03. <A0. <FB. 
D[4,1090]>16. >10. >02. >03. >05. >06. >01. >10. >03. >E5. >5C\ 
t[2.097.787]
>16. >10. >02. >02. >04. >06. "newtdockstor" >73s >74t >6Fo >72r >00. >00. >01. >366 >02. >05. >02. >06. >0A. >07. >04. >6En >61a >6Dm >65e >07. >09. >73s >69i >67g >6En >61a >74t >75u >72r >65e >07. >09. >54T >6Fo >74t >61a >6Cl >53S >69i >7Az >65e >07. >08. >55U >73s >65e >64d >53S >69i >7Az >65e >07. >04. >6Bk >69i >6En >64d >07. >04. >69i >6En >66f >6Fo >07. >08. >72r >65e >61a >64d >4FO >6En >6Cl >79y >07. >0D. >73s >74t >6Fo >72r >65e >70p >61a >73s >73s >77w >6Fo >72r >64d >07. >0C. >64d >65e >66f >61a >75u >6Cl >74t >53S >74t >6Fo >72r >65e >07. >0C. >73s >74t >6Fo >72r >65e >76v >65e >72r >73s >69i >6Fo >6En >08. >12. >00. >49I >00. >6En >00. >74t >00. >65e >00. >72r >00. >6En >00. >61a >00. >6Cl >00. >00. >00. >FF. >07. >99. >44D >14. >00. >FF. >00. >DC. >388 >80. >00. >FF. >00. >43C >44D >50P >08. >12. >00. >49I >00. >6En >00. >74t >00. >65e >00. >72r >00. >6En >00. >61a >00. >6Cl >00. >00. >06. >01. >07. >13. >6Cl >61a >73s >74t >72r >65e >73s >74t >6Fo >72r >65e >66f >72r >6Fo >6Dm >63c >61a >72r >64d >00. >FF. >8F. >377 >49I >DC. >0A. >0A. >00. >1A. >00. >10. >10. >06. >09. >09. >02. >09. >03. >09. >04. >09. >05. >09. >06. >09. >07. >09. >08. >09. >09. >09. >0B. >08. >14. >00. >322 >00. >300 >00. >4DM >00. >42B >00. >20  >00. >43C >00. >61a >00. >72r >00. >10. >03. >25% >48H 
<16. <10. <02. <03. <05. <06. <01. <10. <03. <E5. <5C\ 
D[4,1090]>16. >10. >02. >02. >04. >07. >64d >00. >00. >00. >FF. >26& >0D. >79y >14. >00. >FF. >04. >C5. >78x >00. >00. >FF. >00. >18. >07. >A0. >08. >26& >00. >46F >00. >6Cl >00. >61a >00. >73s >00. >68h >00. >20  >00. >73s >00. >74t >00. >6Fo >00. >72r >00. >61a >00. >67g >00. >65e >00. >20  >00. >63c >00. >61a >00. >72r >00. >64d >00. >00. >06. >01. >09. >0A. >0A. >0A. >0A. >00. >10. >10. >00. >00. >10. >03. >CB. >A4. 
<16. <10. <02. <03. <05. <07. <01. <10. <03. <B4. <9C. 
D[4,1090]<16. <10. <02. <02. <04. <07. "newtdockdsnc" <00. <00. <00. <00. <10. <03. <5E^ <FA. 
D[4,1090]>16. >10. >02. >03. >05. >07. >01. >10. >03. >B4. >9C. 
<16. <10. <02. <02. <04. <08. <6En <65e <77w <74t <64d <6Fo <63c <6Bk "cgfn" <FF. <FF. <FF. <FF. <02. <07. <0D. <47G <65e <74t <55U <73s <65e <72r <43C <6Fo <6En <66f <69i <67g <02. <05. <01. <07. <06. <68h <65e <78x <61a <64d <65e <10. <03. <4FO <399 
D[4,1090]>16. >10. >02. >03. >05. >08. >01. >10. >03. >84. >9F. 
t[0.319.595]
>16. >10. >02. >02. >04. >08. "newtdockcres" >00. >00. >00. >02. >02. >0A. >00. >00. >10. >03. >21! >57W 
<16. <10. <02. <03. <05. <08. <01. <10. <03. <84. <9F. 
D[4,1090]<16. <10. <02. <02. <04. <09. "newtdockstme" <00. <00. <00. <04. <00. <00. <00. <00. <10. <03. <76v <97. 
D[4,1090]>16. >10. >02. >03. >05. >09. >01. >10. >03. >D5. >5F_ 
>16. >10. >02. >02. >04. >09. >6En >65e >77w >74t >64d >6Fo >63c >6Bk >74t >69i >6Dm >65e >00. >00. >00. >04. >02. >E6. >F3. >A9. >10. >03. >91. >17. 
<16. <10. <02. <03. <05. <09. <01. <10. <03. <D5. <5F_ 
D[4,1090]<16. <10. <02. <02. <04. <0A. "newtdockcgfn" <FF. <FF. <FF. <FF. <02. <07. <0D. <47G <65e <74t <55U <73s <65e <72r <43C <6Fo <6En <66f <69i <67g <02. <05. <01. <07. <08. <75u <73s <65e <72r <46F <6Fo <6En <74t <10. <03. <9A. <18. 
D[4,1090]>16. >10. >02. >03. >05. >0A. >01. >10. >03. >25% >5F_ 
>16. >10. >02. >02. >04. >0A. >6En >65e >77w >74t >64d >6Fo >63c >6Bk "cres" >00. >00. >00. >07. >02. >00. >FF. >00. >01. >20  >0C. >00. >10. >03. >25% >311 
<16. <10. <02. <03. <05. <0A. <01. <10. <03. <25% <5F_ 
D[4,1090]<16. <10. <02. <02. <04. <0B."newtdockcgfn" <FF. <FF. <FF. <FF. <02. <07. <0D. <47G <65e <74t <55U <73s <65e <72r <43C <6Fo <6En <66f <69i <67g <02. <05. <01. <07. <0B. <75u <73s <65e <72r <46F <6Fo <6Cl <64d <65e <72r <73s <10. <03. <D8. <B2. 
D[4,1090]>16. >10. >02. >03. >05. >0B. >01. >10. >03. >74t >9F. 
>16. >10. >02. >02. >04. >0B. >6En >65e >77w >74t >64d >6Fo >63c >6Bk "cres" >00. >00. >00. >F5. >02. >06. >08. >07. >08. >42B >75u >73s >69i >6En >65e >73s >73s >07. >0D. >4DM >69i >73s >63c >65e >6Cl >6Cl >61a >6En >65e >6Fo >75u >73s >07. >08. >70p >65e >72r >73s >6Fo >6En >61a >6Cl >07. >06. >5F_ >73s >6Fo >75u >70p >73s >07. >0B. >5F_ >65e >78x >74t >65e >6En >73s >69i >6Fo >6En >73s >07. >06. >5F_ >73s >65e >74t >75u >70p >07. >05. >5F_ >68h >65e >6Cl >70p >07. >09. >63c >6Fo >6Dm >70p >6Cl >65e >74t >65e >64d >08. >12. >00. >42B >00. >75u >00. >73s >00. >69i >00. >6En >00. >65e >00. >73s >00. >73s >00. >00. >08. >1C. >00. >4DM >00. >69i >00. >73s >00. >63c >00. >65e >00. >6Cl >00. >6Cl >00. >61a >00. >6En >00. >65e >00. >6Fo >00. >75u >00. >73s >00. >00. >08. >12. >00. >50P >00. >65e >00. >72r >00. >73s >00. >6Fo >00. >6En >00. >61a >00. >6Cl >00. >00. >08. >10. >10. >00. >53S >00. >74t >00. >6Fo >00. >72r >00. >61a >00. >67g >00. >65e >00. >00. >08. >16. >00. >45E >00. >78x >00. >74t >00. >65e >00. >6En >00. >73s >00. >69i >00. >6Fo >00. >6En >00. >73s >00. >00. >08. >0C. >00. >53S >00. >65e >00. >74t >00. >75u >00. >70p >00. >00. >08. >0A. >00. >48H >00. >65e >00. >6Cl >00. >70p >00. >00. >08. >14. >00. >43C >00. >6Fo >00. >6Dm >00. >70p >00. >6Cl >00. >65e >00. >74t >00. >10. >03. >A2. >62b 
<16. <10. <02. <03. <05. <0B. <01. <10. <03. <74t <9F. 
D[4,1090]>16. >10. >02. >02. >04. >0C. >65e >00. >64d >00. >00. >00. >00. >00. >10. >03. >DE. >5D] 
<16. <10. <02. <03. <05. <0C. <01. <10. <03. <C5. <5E^ 
D[4,1090]<16. <10. <02. <02. <04. <0C. "newtdockopdn" <00. <00. <00. <00. <10. <03. <B4. <E4. 
D[4,1090]>16. >10. >02. >03. >05. >0C. >01. >10. >03. >C5. >5E^ 
t[5.105.644]
>16. >10. >02. >02. >04. >0D. "newtdockhelo" >00. >00. >00. >00. >10. >03. >8A. >0A. 
<16. <10. <02. <03. <05. <0D. <01. <10. <03. <94. <9E. 
D[4,1090]
t[4.984.216]
>16. >10. >02. >02. >04. >0E. "newtdockhelo" >00. >00. >00. >00. >10. >03. >7Az >4EN 
<16. <10. <02. <03. <05. <0E. <01. <10. <03. <64d <9E. 
D[4,1090]
t[3.053.092]
>16. >10. >02. >02. >04. >0F. "newtdockrtbr" >00. >00. >00. >0B. >02. >07. >08. >70p >61a >63c >6Bk >61a >67g >65e >73s >00. >10. >03. >FC. >E8. 
<16. <10. <02. <03. <05. <0F. <01. <10. <03. <355 <5E^ 
D[4,1090]<16. <10. <02. <02. <04. <0D. "newtdockdres" <00. <00. <00. <04. <00. <00. <00. <00. <10. <03. <C5. <51Q 
D[4,1090]>16. >10. >02. >03. >05. >0D. >01. >10. >03. >94. >9E. 
t[0.235.361]
>16. >10. >02. >02. >04. >10. >10. "newtdockdpth" >00. >00. >00. >00. >10. >03. >11. >E5. 
<16. <10. <02. <03. <05. <10. <10. <01. <10. <03. <04. <98. 
D[4,1090]<16. <10. <02. <02. <04. <0E. "newtdockpath" <00. <00. <00. <65e <02. <05. <04. <06. <02. <07. <04. <6En <61a <6Dm <65e <07. <04. <74t <79y <70p <65e <08. <18. <00. <4DM <00. <61a <00. <63c <00. <42B <00. <6Fo <00. <6Fo <00. <6Bk <00. <20  <00. <50P <00. <72r <00. <6Fo <00. <00. <00. <00. <06. <02. <09. <02. <09. <03. <08. <04. <00. <2F/ <00. <00. <00. <0C. <06. <02. <09. <02. <09. <03. <08. <0C. <00. <55U <00. <73s <00. <65e <00. <72r <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0A. <00. <6Dm <00. <61a <00. <74t <00. <74t <00. <00. <00. <08. <00. <00. <00. <10. <03. <A5. <63c 
D[4,1090]>16. >10. >02. >03. >05. >0E. >01. >10. >03. >64d >9E. 
>16. >10. >02. >02. >04. >11. >6En >65e >77w >74t >64d >6Fo >63c >6Bk "gfil" >00. >00. >00. >00. >10. >03. >19. >4AJ 
<16. <10. <02. <03. <05. <11. <01. <10. <03. <55U <58X 
D[4,1090]<16. <10. <02. <02. <04. <0F. "newtdockfile" <00. <00. <06. <D1. <02. <05. <3B; <06. <02. <07. <04. <6En <61a <6Dm <65e <07. <04. <74t <79y <70p <65e <08. <0C. <00. <4DM <00. <75u <00. <73s <00. <69i <00. <63c <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <1A. <00. <45E <00. <69i <00. <6En <00. <73s <00. <74t <00. <65e <00. <69i <00. <6En <00. <2E. <00. <61a <00. <70p <00. <70p <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <12. <00. <74t <00. <65e <00. <73s <00. <74t <00. <2E. <00. <70p <00. <6Bk <00. <67g <00. <00. <00. <04. <06. <02. <09. <02. <09. <03. <08. <18. <00. <57W <00. <69i <00. <6En <00. <64d <00. <6Fo <00. <77w <00. <73s <00. <4DM <00. <42B <00. <6Fo <00. <78x <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <1E. <00. <6Cl <00. <6Fo <00. <73s <00. <74t <00. <20  <00. <61a <00. <6En <00. <64d <00. <20  <00. <66f <00. <6Fo <00. <75u <00. <6En <00. <64d <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <12. <00. <69i <00. <6En <00. <76v <00. <6Fo <00. <6Bk <00. <65e <00. <61a <00. <69i <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <1C. <00. <46F <00. <72r <00. <65e <00. <65e <00. <43C <00. <41A <00. <44D <00. <5F_ <00. <54T <00. <6Fo <00. <6Fo <00. <6Cl <00. <73s <00. <00. <00. <08. <06. <02. <09. <10. <03. <8C. <5E^ 
D[4,1090]>16. >10. >02. >03. >05. >0F. >01. >10. >03. >355 >5E^ 
<16. <10. <02. <02. <04. <10. <10. <02. <09. <03. <08. <08. <00. <65e <00. <73s <00. <70p <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <22" <00. <4FO <00. <6En <00. <65e <00. <44D <00. <72r <00. <69i <00. <76v <00. <65e <00. <44D <00. <6Fo <00. <77w <00. <6En <00. <6Cl <00. <6Fo <00. <61a <00. <64d <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <06. <00. <67g <00. <6Fo <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <1C. <00. <4DM <00. <42B <00. <4FO <00. <58X <00. <43C <00. <6Fo <00. <6En <00. <76v <00. <65e <00. <72r <00. <74t <00. <65e <00. <72r <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0C. <00. <45E <00. <69i <00. <66f <00. <65e <00. <6Cl <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <08. <00. <62b <00. <69i <00. <6En <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <16. <00. <43C <00. <79y <00. <62b <00. <65e <00. <72r <00. <56V <00. <69i <00. <65e <00. <77w <00. <58X <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <24$ <00. <48H <00. <6Fo <00. <63c <00. <68h <00. <7Az <00. <65e <00. <69i <00. <74t <00. <20  <00. <41A <00. <6Dm <00. <61a <00. <20  <00. <56V <00. <61a <00. <74t <00. <69i <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0C. <00. <53S <00. <74t <00. <69i <00. <63c <00. <6Bk <00. <00. <00. <08. <06. <02. <09. <10. <03. <311 <C1. 
D[4,1090]>16. >10. >02. >03. >05. >10. >10. >01. >10. >03. >04. >98. 
<16. <10. <02. <02. <04. <11. <02. <09. <03. <08. <0C. <00. <76v <00. <65e <00. <6En <00. <76v <00. <311 <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0E. <00. <50P <00. <75u <00. <74t <00. <65e <00. <75u <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <12. <00. <50P <00. <69i <00. <63c <00. <74t <00. <75u <00. <72r <00. <65e <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <10. <10. <00. <4EN <00. <43C <00. <58X <00. <2E. <00. <61a <00. <70p <00. <70p <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <10. <10. <00. <6Dm <00. <70p <00. <67g <00. <2E. <00. <70p <00. <6Bk <00. <67g <00. <00. <00. <04. <06. <02. <09. <02. <09. <03. <08. <08. <00. <6En <00. <61a <00. <6Fo <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <10. <10. <00. <41A <00. <7Az <00. <75u <00. <72r <00. <65e <00. <75u <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <10. <10. <00. <44D <00. <65e <00. <73s <00. <6Bk <00. <74t <00. <6Fo <00. <70p <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <10. <10. <00. <4CL <00. <69i <00. <62b <00. <72r <00. <61a <00. <72r <00. <79y <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0C. <00. <54T <00. <72r <00. <61a <00. <73s <00. <68h <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <14. <00. <54T <00. <65e <00. <73s <00. <10. <03. <A3. <3C< 
D[4,1090]>16. >10. >02. >03. >05. >11. >01. >10. >03. >55U >58X 
<16. <10. <02. <02. <04. <12. <74t <00. <2E. <00. <6Dm <00. <62b <00. <6Fo <00. <78x <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <18. <00. <41A <00. <70p <00. <6Bk <00. <50P <00. <72r <00. <6Fo <00. <6Aj <00. <65e <00. <63c <00. <74t <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <08. <00. <63c <00. <66f <00. <67g <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <14. <00. <56V <00. <75u <00. <6Cl <00. <6Bk <00. <61a <00. <6En <00. <53S <00. <44D <00. <4BK <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <12. <00. <5F_ <00. <5F_ <00. <4DM <00. <41A <00. <43C <00. <4FO <00. <53S <00. <58X <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0C. <00. <53S <00. <69i <00. <74t <00. <65e <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <366 <00. <51Q <00. <75u <00. <69i <00. <63c <00. <6Bk <00. <6Cl <00. <6Fo <00. <6Fo <00. <6Bk <00. <46F <00. <43C <00. <53S <00. <74t <00. <64d <00. <2E. <00. <71q <00. <6Cl <00. <67g <00. <65e <00. <6En <00. <65e <00. <72r <00. <61a <00. <74t <00. <6Fo <00. <72r <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0E. <00. <77w <00. <69i <00. <6En <00. <6En <00. <74t <00. <6Bk <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <08. <00. <4EN <00. <54T <00. <4BK <00. <00. <00. <08. <06. <02. <09. <10. <03. <87. <52R 
D[4,1090]>16. >10. >02. >03. >05. >12. >01. >10. >03. >A5. >58X 
<16. <10. <02. <02. <04. <13. <02. <09. <03. <08. <1A. <00. <47G <00. <6Fo <00. <6Fo <00. <67g <00. <6Cl <00. <65e <00. <20  <00. <44D <00. <72r <00. <69i <00. <76v <00. <65e <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0E. <00. <50P <00. <75u <00. <62b <00. <6Cl <00. <69i <00. <63c <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0C. <00. <42B <00. <65e <00. <62b <00. <6Fo <00. <70p <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <1A. <00. <55U <00. <62b <00. <75u <00. <6En <00. <74t <00. <75u <00. <2E. <00. <73s <00. <68h <00. <61a <00. <72r <00. <65e <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <10. <10. <00. <41A <00. <6En <00. <64d <00. <72r <00. <6Fo <00. <69i <00. <64d <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <322 <00. <41A <00. <70p <00. <70p <00. <6Cl <00. <69i <00. <63c <00. <61a <00. <74t <00. <69i <00. <6Fo <00. <6En <00. <73s <00. <20  <00. <28( <00. <50P <00. <61a <00. <72r <00. <61a <00. <6Cl <00. <6Cl <00. <65e <00. <6Cl <00. <73s <00. <29) <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <0E. <00. <4DM <00. <6Fo <00. <76v <00. <69i <00. <65e <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <1A. <00. <41A <00. <70p <00. <70p <00. <6Cl <00. <69i <00. <63c <00. <61a <00. <74t <00. <69i <00. <6Fo <00. <6En <00. <10. <03. <19. <00. 
D[4,1090]>16. >10. >02. >03. >05. >13. >01. >10. >03. >F4. >98. 
<16. <10. <02. <02. <04. <14. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <08. <00. <6Cl <00. <69i <00. <62b <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <08. <00. <64d <00. <65e <00. <76v <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <22" <00. <57W <00. <69i <00. <6En <00. <64d <00. <77w <4DM <78x <00. <00. <06. <09. <53S <333 <43C <65e <08. <09. <00. <00. <00. <73s <08. <03. <76v <76v <06. <09. <00. <00. <00. <02. <03. <76v <61a <08. <09. <00. <00. <00. <00. <64d <6Fo <6Fo <65e 
D[4,1090]>16. >10. >02. >03. >05. >14. >01. >10. >03. >45E >59Y 
<16. <10. <02. <02. <04. <15. <63c <00. <74t <00. <73s <00. <00. <00. <08. <06. <02. <09. <02. <09. <03. <08. <1A. <00. <53S <00. <54T <00. <4DM <00. <333 <00. <322 <00. <43C <00. <75u <00. <62b <00. <65e <00. <49I <00. <45E <06. <03. <00. <52R <06. <03. <00. <6En <00. <73s <06. <00. <72r <00. <64d <00. <75u <00. <69i <00. <00. <70p <00. <02. <00. <43C <00. <73s <00. <02. <08. <65e <00. <70p <00. <00. <0A. 
>16. >10. >02. >03. >05. >15. >01. >10. >03. >14. >99. 
t[4.704.005]
>16. >10. >02. >02. >04. >12. "newtdockgfin" >00. >00. >00. >15. >02. >08. >12. >00. >74t >00. >65e >00. >73s >00. >74t >00. >2E. >00. >70p >00. >6Bk >00. >67g >00. >00. >00. >00. >00. >10. >03. >55U >59Y 
<16. <10. <02. <03. <05. <12. <01. <10. <03. <A5. <58X 
D[4,1090]<16. <10. <02. <02. <04. <16. "newtdockfinf" <00. <00. <00. <85. <02. <06. <06. <07. <04. <6Bk <69i <6En <64d <07. <04. <73s <69i <7Az <65e <07. <07. <63c <72r <65e <61a <74t <65e <64d <07. <08. <6Dm <6Fo <64d <69i <66f <69i <65e <64d <07. <04. <70p <61a <74t <68h <07. <04. <69i <63c <6Fo <6En <08. <2E. <00. <49I <00. <6En <00. <73s <00. <74t <00. <61a <00. <6Cl <00. <6Cl <00. <65e <00. <72r <00. <20  <00. <66f <00. <6Cl <00. <61a <00. <74t <00. <20  <00. <70p <00. <61a <00. <63c <00. <6Bk <00. <61a <00. <67g <00. <65e <00. <00. <00. <FF. <00. <00. <26& <E0. <00. <FF. <0E. <92. <27' <44D <00. <FF. <0F. <14. <C0. <A0. <08. <12. <00. <74t <00. <65e <00. <73s <00. <74t <00. <2E. <00. <70p <00. <6Bk <00. <67g <00. <00. <0A. <00. <00. <00. <10. <03. <92. <47G 
D[4,1090]>16. >10. >02. >03. >05. >16. >01. >10. >03. >E4. >99. 
t[4.231.975]
>16. >10. >02. >02. >04. >13. "newtdocklpfl" >00. >00. >00. >15. >02. >08. >12. >00. >74t >00. >65e >00. >73s >00. >74t >00. >2E. >00. >70p >00. >6Bk >00. >67g >00. >00. >00. >00. >00. >10. >03. >05. >17. 
<16. <10. <02. <03. <05. <13. <01. <10. <03. <F4. <98. 
D[4,1090]<16. <10. <02. <02. <04. <17. "newtdocksdef" <00. <00. <00. <00. <10. <03. <4BK <B0. 
D[4,1090]>16. >10. >02. >03. >05. >17. >01. >10. >03. >B5. >59Y 
t[0.574.433]
>16. >10. >02. >02. >04. >14. "newtdockdres" >00. >00. >00. >04. >00. >00. >00. >00. >10. >03. >8D. >366 
<16. <10. <02. <03. <05. <14. <01. <10. <03. <45E <59Y 
D[4,1090]<16. <10. <02. <02. <04. <18. "newtdocklpkg" <00. <00. <09. <B8. <70p <61a <63c <6Bk <61a <67g <65e <300 <78x <78x <78x <78x <00. <00. <00. <00. <00. <00. <00. <01. <00. <00. <00. <44D <00. <44D <00. <14. <00. <00. <09. <B8. <E2. <377 <49I <65e <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <01. <08. <00. <00. <00. <01. <00. <00. <00. <00. <00. <00. <08. <B0. <00. <00. <08. <B0. <66f <6Fo <72r <6Dm <00. <00. <00. <00. <00. <00. <00. <81. <00. <58X <00. <1C. <00. <00. <00. <00. <00. <47G <00. <65e <00. <6En <00. <65e <00. <72r <00. <61a <00. <74t <00. <65e <00. <64d <00. <20  <00. <62b <00. <79y <00. <20  <00. <74t <00. <68h <00. <65e <00. <20  <00. <45E <00. <69i <00. <6En <00. <73s <00. <74t <00. <65e <00. <6En <00. <20  <00. <54T <00. <6Fo <00. <6Fo <00. <6Cl <00. <6Bk <00. <69i <00. <74t <00. <2E. <00. <00. <00. <4DM <00. <50P <00. <47G <00. <3A: <00. <57W <00. <4FO <00. <4EN <00. <4BK <00. <4FO <00. <00. <41A <20  <4EN <65e <77w <74t <6Fo <6En <20  <54T <6Fo <6Fo <6Cl <6Bk <69i <74t <20  <61a <70p <70p <6Cl <69i <63c <61a <74t <69i <6Fo <6En <4EN <65e <77w <74t <6Fo <6En <AA. <20  <54T <6Fo <6Fo <6Cl <4BK <69i <74t <20  <50P <61a <63c <6Bk <61a <67g <65e <20  <A9. <20  <311 <399 <399 <322 <2D- <311 <399 <399 <377 <2C, <20  <41A <70p <70p <10. <03. <C8. <C4. 
D[4,1090]>16. >10. >02. >03. >05. >18. >01. >10. >03. >85. >5AZ 
<16. <10. <02. <02. <04. <19. <6Cl <65e <20  <43C <6Fo <6Dm <70p <75u <74t <65e <72r <2C, <20  <49I <6En <63c <10. <03. <21! <7B{ 
D[4,1090]>16. >10. >02. >03. >05. >19. >01. >10. >03. >D4. >9A. 
<16. <10. <02. <02. <04. <1A. <2E. <00. <BF. <BF. <BF. <BF. <BF. <BF. <00. <00. <10. <10. <41A <00. <00. <00. <00. <00. <00. <00. <02. <00. <00. <01. <19. <00. <00. <1C. <43C <00. <00. <00. <00. <00. <00. <01. <399 <00. <00. <01. <B9. <00. <00. <01. <D9. <00. <00. <02. <09. <00. <00. <04. <399 <BF. <BF. <BF. <BF. <00. <00. <20  <41A <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <02. <00. <00. <01. <59Y <00. <00. <01. <71q <00. <00. <01. <89. <00. <00. <01. <A1. <00. <00. <14. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <0E. <C1. <FB. <99. <61a <70p <70p <00. <BF. <BF. <BF. <BF. <00. <00. <15. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <DC. <6Dm <87. <DD. <74t <65e <78x <74t <00. <BF. <BF. <BF. <00. <00. <15. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <8E. <5C\ <377 <A1. <69i <63c <6Fo <6En <00. <BF. <BF. <BF. <00. <00. <18. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <69i <80. <6En <2D- <74t <68h <65e <46F <6Fo <72r <6Dm <00. <00. <00. <1A. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <BC. <399 <3A: <4CL <4DM <50P <47G <3A: <57W <4FO <4EN <4BK <4FO <00. <BF. <BF. <BF. <BF. <BF. <BF. <00. <00. <14. <40@ <00. <00. <00. <00. <00. <00. <01. <F1. <00. <4DM <00. <50P <00. <47G <00. <00. <BF. <BF. <BF. <BF. <00. <00. <17. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <18. <10. <10. <F3. <5F_ <10. <03. <48H <C6. 
D[4,1090]>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <1B. <73s <74t <72r <69i <6En <67g <00. <BF. <00. <00. <18. <43C <00. <00. <00. <00. <00. <00. <02. <21! <00. <00. <02. <89. <00. <00. <03. <11. <00. <00. <03. <99. <00. <00. <1C. <41A <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <02. <00. <00. <02. <41A <00. <00. <02. <59Y <00. <00. <02. <71q <BF. <BF. <BF. <BF. <00. <00. <15. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <69i <02. <A4. <CC. <6Dm <61a <73s <6Bk <00. <BF. <BF. <BF. <00. <00. <15. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <1E. <4FO <7F. <22" <62b <69i <74t <73s <00. <BF. <BF. <BF. <00. <00. <17. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <AD. <77w <3E> <B3. <62b <6Fo <75u <6En <64d <73s <00. <BF. <00. <00. <88. <40@ <00. <00. <00. <00. <00. <00. <02. <41A <00. <00. <00. <00. <00. <04. <00. <00. <00. <00. <00. <00. <00. <1B. <00. <18. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <1C. <00. <00. <00. <3F? <00. <00. <00. <3F? <00. <1F. <FF. <7F. <00. <3F? <FF. <7E~ <00. <3F? <FF. <FE. <00. <3F? <FF. <FC. <00. <3F? <FF. <FC. <00. <3F? <FF. <F8. <00. <3F? <FF. <F8. <00. <3F? <FF. <F0. <00. <3F? <FF. <F0. <00. <3F? <FF. <E0. <00. <3F? <FF. <E0. <00. <3F? <FF. <C0. <00. <3F? <FF. <C0. <00. <3F? <FF. <80. <00. <3F? <FF. <00. <00. <3F? <FF. <80. <00. <3F? <FF. <80. <00. <3F? <FF. <80. <00. <10. <03. <E9. <09. 
D[4,1090]>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <1C. <1F. <FF. <00. <00. <1F. <FF. <00. <00. <0F. <FE. <00. <00. <00. <00. <00. <00. <00. <00. <88. <40@ <00. <00. <00. <00. <00. <00. <02. <59Y <00. <00. <00. <00. <00. <04. <00. <00. <00. <00. <00. <00. <00. <1B. <00. <18. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <1C. <00. <00. <00. <3F? <00. <00. <00. <333 <00. <1F. <FF. <7B{ <00. <3F? <FF. <6En <00. <300 <00. <CE. <00. <377 <FC. <CC. <00. <377 <FD. <9C. <00. <344 <05. <98. <00. <355 <53S <388 <00. <344 <03. <300 <00. <355 <56V <70p <00. <344 <06. <60` <00. <355 <54T <E0. <00. <344 <0C. <C0. <00. <355 <4FO <C0. <00. <344 <0B. <80. <00. <366 <0F. <00. <00. <377 <FE. <80. <00. <377 <FD. <80. <00. <300 <0B. <80. <00. <18. <03. <00. <00. <1F. <FF. <00. <00. <0F. <FE. <00. <00. <00. <00. <00. <00. <00. <00. <1C. <43C <00. <00. <00. <00. <00. <00. <03. <B9. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <60` <00. <00. <00. <6Cl <BF. <BF. <BF. <BF. <00. <00. <20  <41A <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <02. <00. <00. <03. <D9. <00. <00. <03. <F1. <00. <00. <04. <09. <00. <00. <04. <21! <00. <00. <15. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <CA. <CB. <2B+ <13. <6Cl <65e <66f <74t <00. <BF. <BF. <BF. <00. <00. <14. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <2E. <A8. <8A. <9B. <10. <03. <12. <BC. 
D[4,1090]
t[0.361.906]
>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <1D. <74t <6Fo <70p <00. <BF. <BF. <BF. <BF. <00. <00. <16. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <16. <C7. <A2. <0E. <72r <69i <67g <68h <74t <00. <BF. <BF. <00. <00. <17. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <DB. <A1. <FF. <ED. <62b <6Fo <74t <74t <6Fo <6Dm <00. <BF. <00. <00. <24$ <43C <00. <00. <00. <00. <00. <00. <04. <61a <00. <00. <05. <69i <00. <00. <02. <D3. <00. <00. <05. <A9. <00. <00. <08. <A1. <00. <00. <09. <A1. <00. <00. <00. <02. <BF. <BF. <BF. <BF. <00. <00. <1C. <41A <00. <00. <00. <00. <00. <00. <00. <10. <10. <00. <00. <04. <81. <00. <00. <05. <01. <00. <00. <05. <21! <00. <00. <05. <49I <BF. <BF. <BF. <BF. <00. <00. <1C. <41A <00. <00. <00. <00. <00. <00. <00. <10. <10. <00. <00. <00. <02. <00. <00. <04. <A1. <00. <00. <04. <C1. <00. <00. <04. <D9. <BF. <BF. <BF. <BF. <00. <00. <1B. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <5B[ <BA. <05. <56V <76v <69i <65e <77w <42B <6Fo <75u <6En <64d <73s <00. <BF. <BF. <BF. <BF. <BF. <00. <00. <17. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <66f <22" <43C <9B. <5F_ <70p <72r <6Fo <74t <6Fo <00. <BF. <00. <00. <23# <40@ <00. <00. <00. <00. <00. <05. <55U <52R <88. <CB. <80. <11. <72r <65e <63c <61a <6Cl <63c <75u <6Cl <61a <74t <65e <4DM <69i <6Cl <65e <61a <67g <65e <00. <BF. <BF. <BF. <BF. <BF. <10. <03. <2B+ <A6. 
D[4,1090]
t[1.881.601]
>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <1E. <00. <00. <1D. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <D9. <3F? <68h <1D. <53S <74t <65e <70p <43C <68h <69i <6Cl <64d <72r <65e <6En <00. <BF. <BF. <BF. <00. <00. <24$ <40@ <00. <00. <00. <00. <00. <05. <55U <52R <AD. <59Y <8A. <F6. <53S <74t <65e <70p <41A <6Cl <6Cl <6Fo <63c <61a <74t <65e <43C <6Fo <6En <74t <65e <78x <74t <00. <BF. <BF. <BF. <BF. <00. <00. <1E. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <16. <F7. <AC. <E0. <6Dm <79y <43C <68h <65e <63c <6Bk <42B <75u <74t <74t <6Fo <6En <00. <BF. <BF. <00. <00. <1C. <43C <00. <00. <00. <00. <00. <00. <05. <89. <00. <00. <00. <00. <00. <00. <00. <78x <00. <00. <02. <D0. <00. <00. <03. <70p <BF. <BF. <BF. <BF. <00. <00. <20  <41A <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <02. <00. <00. <03. <D9. <00. <00. <03. <F1. <00. <00. <04. <09. <00. <00. <04. <21! <00. <00. <20  <43C <00. <00. <00. <00. <00. <00. <05. <C9. <00. <00. <06. <81. <00. <00. <06. <A1. <00. <00. <06. <E9. <00. <00. <07. <C9. <00. <00. <00. <00. <00. <00. <24$ <41A <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <02. <00. <00. <05. <F1. <00. <00. <06. <09. <00. <00. <06. <29) <00. <00. <06. <49I <00. <00. <06. <69i <BF. <BF. <BF. <BF. <00. <00. <16. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <25% <0B. <D4. <46F <10. <03. <9A. <A7. 
D[4,1090]
t[3.883.184]
>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <1F. <63c <6Cl <61a <73s <73s <00. <BF. <BF. <00. <00. <1D. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <83. <A6. <3A: <CD. <69i <6En <73s <74t <72r <75u <63c <74t <69i <6Fo <6En <73s <00. <BF. <BF. <BF. <00. <00. <19. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <C3. <C1. <17. <60` <6Cl <69i <74t <65e <72r <61a <6Cl <73s <00. <BF. <BF. <BF. <BF. <BF. <BF. <BF. <00. <00. <19. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <13. <E7. <40@ <DD. <61a <72r <67g <46F <72r <61a <6Dm <65e <00. <BF. <BF. <BF. <BF. <BF. <BF. <BF. <00. <00. <18. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <5B[ <3C< <3B; <F5. <6En <75u <6Dm <41A <72r <67g <73s <00. <00. <00. <1A. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <3F? <FD. <28( <D6. <43C <6Fo <64d <65e <42B <6Cl <6Fo <63c <6Bk <00. <BF. <BF. <BF. <BF. <BF. <BF. <00. <00. <2E. <40@ <00. <00. <00. <00. <00. <00. <06. <D1. <70p <19. <91. <A3. <72r <19. <91. <A4. <27' <0F. <9C. <A5. <7C| <20  <C7. <00. <0B. <6Fo <00. <1A. <7B{ <7C| <C7. <00. <08. <AB. <74t <19. <7D} <1D. <29) <1E. <2B+ <02. <BF. <BF. <00. <00. <17. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <F8. <2A* <64d <5D] <62b <69i <6En <61a <72r <79y <00. <BF. <00. <00. <28( <41A <00. <00. <00. <00. <00. <00. <06. <29) <00. <00. <07. <11. <00. <00. <07. <29) <00. <00. <07. <41A <10. <03. <FD. <A1. 
D[4,1090]
t[3.883.352]
>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <20  <00. <00. <07. <59Y <00. <00. <07. <71q <00. <00. <07. <91. <00. <00. <07. <A9. <00. <00. <16. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <EB. <FB. <0B. <66f <77w <54T <72r <69i <70p <00. <BF. <BF. <00. <00. <16. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <78x <90. <28( <55U <76v <61a <6Cl <75u <65e <00. <BF. <BF. <00. <00. <15. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <1E. <4FO <7F. <22" <77w <47G <61a <73s <00. <BF. <BF. <BF. <00. <00. <18. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <04. <59Y <BD. <54T <6Dm <69i <6Cl <65e <61a <67g <65e <00. <00. <00. <19. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <C9. <344 <1B. <333 <77w <4DM <69i <6Cl <65e <61a <67g <65e <00. <BF. <BF. <BF. <BF. <BF. <BF. <BF. <00. <00. <16. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <8F. <A5. <88. <F2. <66f <6Cl <6Fo <6Fo <72r <00. <BF. <BF. <00. <00. <19. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <53S <B4. <5E^ <E1. <53S <65e <74t <56V <61a <6Cl <75u <65e <00. <BF. <BF. <BF. <BF. <BF. <BF. <BF. <00. <00. <24$ <43C <00. <00. <00. <00. <00. <00. <07. <F1. <00. <00. <00. <02. <00. <00. <00. <02. <00. <00. <00. <02. <00. <00. <FF. <F2. <00. <00. <FF. <F2. <00. <00. <FF. <F2. <BF. <BF. <BF. <BF. <00. <00. <28( <41A <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <02. <10. <03. <81. <9B. 
D[4,1090]
t[3.883.191]
>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <21! <00. <00. <08. <19. <00. <00. <08. <399 <00. <00. <08. <51Q <00. <00. <08. <71q <00. <00. <08. <89. <00. <00. <07. <59Y <00. <00. <1E. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <F1. <9E. <1A. <0B. <5F_ <6En <65e <78x <74t <41A <72r <67g <46F <72r <61a <6Dm <65e <00. <BF. <BF. <00. <00. <18. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <C5. <D5. <F0. <A1. <5F_ <70p <61a <72r <65e <6En <74t <00. <00. <00. <1D. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <55U <7B{ <79y <93. <5F_ <69i <6Dm <70p <6Cl <65e <6Dm <65e <6En <74t <6Fo <72r <00. <BF. <BF. <BF. <00. <00. <15. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <27' <20  <AD. <87. <74t <72r <69i <70p <00. <BF. <BF. <BF. <00. <00. <14. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <59Y <75u <21! <43C <67g <61a <73s <00. <BF. <BF. <BF. <BF. <00. <00. <10. <10. <41A <00. <00. <00. <00. <00. <00. <08. <B1. <00. <00. <08. <C9. <00. <00. <16. <40@ <00. <00. <00. <00. <00. <05. <55U <52R <B4. <FF. <1B. <C7. <61a <72r <72r <61a <79y <00. <BF. <BF. <00. <00. <1C. <43C <00. <00. <00. <00. <00. <00. <08. <E9. <00. <00. <03. <6Bk <00. <00. <09. <49I <00. <00. <09. <61a <00. <00. <05. <49I <BF. <BF. <BF. <BF. <00. <00. <14. <41A <00. <00. <00. <00. <00. <00. <00. <10. <10. <00. <00. <09. <01. <00. <00. <09. <21! <BF. <BF. <BF. <BF. <10. <03. <EA. <53S 
D[4,1090]
t[3.882.266]
>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 
<16. <10. <02. <02. <04. <22" <00. <00. <1C. <41A <00. <00. <00. <00. <00. <00. <00. <10. <10. <00. <00. <00. <02. <00. <00. <04. <C1. <00. <00. <01. <71q <00. <00. <04. <A1. <BF. <BF. <BF. <BF. <00. <00. <24$ <40@ <00. <00. <00. <00. <00. <05. <55U <52R <2B+ <AA. <75u <AD. <50P <72r <65e <61a <6Cl <6Cl <6Fo <63c <61a <74t <65e <64d <43C <6Fo <6En <74t <65e <78x <74t <00. <BF. <BF. <BF. <BF. <00. <00. <18. <40@ <00. <00. <00. <00. <00. <00. <01. <F1. <00. <48H <00. <65e <00. <6Cl <00. <6Cl <00. <6Fo <00. <00. <00. <00. <1C. <43C <00. <00. <00. <00. <00. <00. <09. <81. <00. <00. <01. <24$ <00. <00. <02. <08. <00. <00. <02. <84. <00. <00. <02. <300 <BF. <BF. <BF. <BF. <00. <00. <20  <41A <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <00. <02. <00. <00. <03. <D9. <00. <00. <03. <F1. <00. <00. <04. <21! <00. <00. <04. <09. <00. <00. <14. <41A <00. <00. <00. <00. <00. <00. <08. <B1. <00. <00. <05. <49I <00. <00. <08. <C9. <BF. <BF. <BF. <BF. <10. <03. <9E. <19. 
D[4,1090]
t[3.914.173]
>16. >10. >02. >03. >05. >1A. >01. >10. >03. >24$ >9A. 



#endif
