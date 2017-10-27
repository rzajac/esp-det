#ifndef ESP_SDO_H
#define ESP_SDO_H

#include <c_types.h>

#define UART0   0
#define UART1   1

typedef enum {
  FIVE_BITS = 0x0,
  SIX_BITS = 0x1,
  SEVEN_BITS = 0x2,
  EIGHT_BITS = 0x3
} uart_bits_num_4char;

typedef enum {
  ONE_STOP_BIT = 0x1,
  ONE_HALF_STOP_BIT = 0x2,
  TWO_STOP_BIT = 0x3
} uart_stop_bits_num;

typedef enum {
  STICK_PARITY_DIS = 0,
  STICK_PARITY_EN = 1
} uart_exist_parity;

typedef enum {
  BIT_RATE_300 = 300,
  BIT_RATE_600 = 600,
  BIT_RATE_1200 = 1200,
  BIT_RATE_2400 = 2400,
  BIT_RATE_4800 = 4800,
  BIT_RATE_9600 = 9600,
  BIT_RATE_19200 = 19200,
  BIT_RATE_38400 = 38400,
  BIT_RATE_57600 = 57600,
  BIT_RATE_74880 = 74880,
  BIT_RATE_115200 = 115200,
  BIT_RATE_230400 = 230400,
  BIT_RATE_460800 = 460800,
  BIT_RATE_921600 = 921600,
  BIT_RATE_1843200 = 1843200,
  BIT_RATE_3686400 = 3686400,
} uart_baud_rate;

#define REG_UART_BASE(i)    (0x60000000 + (i) * 0xf00)
#define UART_FIFO(i)        (REG_UART_BASE(i) + 0x0)

#define UART_INT_CLR(i)     (REG_UART_BASE(i) + 0x10)
#define UART_STATUS(i)      (REG_UART_BASE(i) + 0x1C)
#define UART_TXFIFO_CNT     0x000000FF
#define UART_TXFIFO_CNT_S   16

#define UART_CONF0(i)       (REG_UART_BASE(i) + 0x20)
#define UART_TXFIFO_RST     (BIT(18))
#define UART_RXFIFO_RST     (BIT(17))
#define UART_STOP_BIT_NUM_S 4
#define UART_BIT_NUM_S      2

/**
 * Initialize stdout.
 *
 * @param br The baud rate.
 */
void ICACHE_FLASH_ATTR
stdout_init(uart_baud_rate br);

#endif //ESP_SDO_H