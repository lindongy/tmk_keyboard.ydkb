#include "ws2812_spi.h"

// Define the spi your LEDs are plugged to here
// SPI1 MOSI PB15
#define RGBLED_NUM    101
#define PORT_WS2812   GPIOB
#define PIN_WS2812    15
#define WS2812_SPI SPID2

//
#define BYTES_FOR_LED_BYTE 4
#define NB_COLORS 3
#define BYTES_FOR_LED  (BYTES_FOR_LED_BYTE * NB_COLORS)
#define DATA_SIZE      (BYTES_FOR_LED * RGBLED_NUM)
#define RESET_SIZE     200
#define PREAMBLE_SIZE  4


static uint8_t txbuf[PREAMBLE_SIZE + DATA_SIZE + RESET_SIZE];
static uint8_t get_protocol_eq(uint8_t data, int pos);

 /*
 * This lib is meant to be used asynchronously, thus the colors contained in
 * the txbuf will be sent in loop, so that the colors are always the ones you
 * put in the table (the user thus have less to worry about)
 *
 * Since the data are sent via DMA, and the call to spiSend is a blocking one,
 * the processor ressources are not used to much, if you see your program being
 * too slow, simply add a:
 * chThdSleepMilliseconds(x);
 * after the spiSend, where you increment x untill you are satisfied with your
 * program speed, another trick may be to lower this thread priority : your call
 */
 #if 0
static THD_WORKING_AREA(LEDS_THREAD_WA, 128);
static THD_FUNCTION(ledsThread, arg) {
  (void) arg;
  while(1){
    spiSend(&WS2812_SPI, PREAMBLE_SIZE + DATA_SIZE + RESET_SIZE, txbuf);
  }
}
#endif

 static const SPIConfig spicfg = {
  NULL,
  PORT_WS2812,
  PIN_WS2812,
  SPI_CR1_BR_1|SPI_CR1_BR_0 // baudrate : fpclk / 8 => 1tick is 0.32us (2.25 MHz)
};

 /*
 * Function used to initialize the driver.
 *
 * Starts by shutting off all the LEDs.
 * Then gets access on the LED_SPI driver.
 * May eventually launch an animation on the LEDs (e.g. a thread setting the
 * txbuff values)
 */
void ws2812_spi_init(void){
  /* MOSI pin*/
  palSetPadMode(PORT_WS2812, PIN_WS2812, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
  for(int i = 0; i < RESET_SIZE; i++)
    txbuf[DATA_SIZE+i] = 0x00;
  for (int i=0; i<PREAMBLE_SIZE; i++)
    txbuf[i] = 0x00;
  spiAcquireBus(&WS2812_SPI);              /* Acquire ownership of the bus.    */
  spiStart(&WS2812_SPI, &spicfg);          /* Setup transfer parameters.       */
  spiSelect(&WS2812_SPI);                  /* Slave Select assertion.          */
  //spiSend(&WS2812_SPI, PREAMBLE_SIZE + DATA_SIZE + RESET_SIZE, txbuf);
  spiStartSend(&WS2812_SPI, PREAMBLE_SIZE + DATA_SIZE + RESET_SIZE, txbuf);
  //chThdCreateStatic(LEDS_THREAD_WA, sizeof(LEDS_THREAD_WA),NORMALPRIO, ledsThread, NULL);
}

 /*
 * As the trick here is to use the SPI to send a huge pattern of 0 and 1 to
 * the ws2812b protocol, we use this helper function to translate bytes into
 * 0s and 1s for the LED (with the appropriate timing).
 */
static uint8_t get_protocol_eq(uint8_t data, int pos){
  uint8_t eq = 0;
  if (data & (1 << (2*(3-pos))))
    eq = 0b1110;
  else
    eq = 0b1000;
  if (data & (2 << (2*(3-pos))))
    eq += 0b11100000;
  else
    eq += 0b10000000;
  return eq;
}

void ws2812_setleds(struct cRGB *ledarray, uint16_t number_of_leds) {
    /* do ws2812_spi_init here to prevent when RGB is turned off, 
    after the keyboard is restarted, RGB cannot be turned on*/
    static uint8_t ws2812_init_done = 0;
    if (!ws2812_init_done) {
        ws2812_spi_init();
        ws2812_init_done = 1;
    }
  uint8_t i = 0;
  while (i < number_of_leds) {
    set_led_color_rgb(ledarray[i], i);
    i++;
  }
  //spiSend(&WS2812_SPI, PREAMBLE_SIZE + DATA_SIZE + RESET_SIZE, txbuf);
  spiStartSend(&WS2812_SPI, PREAMBLE_SIZE + DATA_SIZE + RESET_SIZE, txbuf);
}

 /*
 * If you want to set a LED's color in the RGB color space, simply call this
 * function with a hsv_color containing the desired color and the index of the
 * led on the LED strip (starting from 0, the first one being the closest the
 * first plugged to the board)
 *
 * Only set the color of the LEDs through the functions given by this API
 * (unless you really know what you are doing)
 */
void set_led_color_rgb(struct cRGB color, int pos){
  for(int j = 0; j < 4; j++)
    txbuf[PREAMBLE_SIZE + BYTES_FOR_LED*pos + j] = get_protocol_eq(color.g, j);
  for(int j = 0; j < 4; j++)
    txbuf[PREAMBLE_SIZE + BYTES_FOR_LED*pos + BYTES_FOR_LED_BYTE+j] = get_protocol_eq(color.r, j);
  for(int j = 0; j < 4; j++)
    txbuf[PREAMBLE_SIZE + BYTES_FOR_LED*pos + BYTES_FOR_LED_BYTE*2+j] = get_protocol_eq(color.b, j);
}

void set_leds_color_rgb(struct cRGB color){
  for(int i = 0; i < RGBLED_NUM; i++)
    set_led_color_rgb(color, i);
}


void ws2812_setleds_rgbw(struct cRGB *ledarray, uint16_t number_of_leds) {

}
