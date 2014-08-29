/* Use Yellow LED connected to Port C-5 */

#define LED_DDR     DDRC
#define LED_PORT    PORTC
#define LED_PIN     PINC
#define LED         5

/* The following UART config will only be used with SoftwareSerial */

#define UART_DDR	DDRD
#define UART_PORT	PORTD
#define UART_PIN	PIND
#define UART_RX_BIT	0
#define UART_TX_BIT	1
