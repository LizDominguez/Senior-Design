#define F_CPU 8000000UL
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>

#define BAUD 9600
#define BAUDRATE (((F_CPU / (BAUD * 16UL))) - 1)
#define RXD0 PIND0
#define SIZE 16
#define ICP PIND6
#define REGISTERED_TAGS  2      



/*************** LCD Configuration ****************/

#define lcdDdr		DDRA		//Data direction
#define lcdPort		PORTA		//PortD
#define lcdD7Bit	PORTA0		//LCD D7 (pin 14) -> PORTD7
#define lcdD6Bit	PORTA1		//LCD D6 (pin 13) -> PORTD6
#define lcdD5Bit	PORTA2		//LCD D5 (pin 12) -> PORTD5
#define lcdD4Bit	PORTA3		//LCD D4 (pin 11) -> PORTD4
#define lcdEBit		PORTA4		//LCD E (pin 6)   -> PORTD3
#define lcdRSBit	PORTA5		//LCD RS (pin 4)  -> PORTD2

#define lineOne			0x00                // Line 1
#define lineTwo			0x40                // Line 2
#define clear           0b00000001          // clears all characters
#define home            0b00000010          // returns cursor to home
#define entryMode       0b00000110          // moves cursor from left to right
#define off      		0b00001000          // LCD off
#define on       		0b00001100          // LCD on
#define reset   		0b00110000          // reset the LCD
#define bit4Mode 		0b00101000          // we are using 4 bits of data
#define setCursor       0b10000000          //sets the position of cursor


void lcd_write(uint8_t);
void lcd_instruction(uint8_t);
void lcd_char(uint8_t);
void lcd_string(uint8_t string[]);
void lcd_init(void);


void lcd_init(void)
{
	lcdDdr |= (1 << lcdD7Bit) | (1 << lcdD6Bit) | (1 << lcdD5Bit) | (1 << lcdD4Bit) | (1 << lcdEBit) | (1 << lcdRSBit);
	_delay_ms(100);

	lcdPort &= ~(1 << lcdRSBit);                 // RS low
	lcdPort &= ~(1 << lcdEBit);                 // E low

	// LCD resets
	lcd_write(reset);
	_delay_ms(8);                           // 5 ms delay min

	lcd_write(reset);
	_delay_us(200);                       // 100 us delay min

	lcd_write(reset);
	_delay_us(200);
	
	lcd_write(bit4Mode);               	//set to 4 bit mode
	_delay_us(50);                     // 40us delay min

	lcd_instruction(bit4Mode);   	 // set 4 bit mode
	_delay_us(50);                  // 40 us delay min

	// display off
	lcd_instruction(off);        	// turn off display
	_delay_us(50);

	// Clear display
	lcd_instruction(clear);              // clear display
	_delay_ms(3);                       // 1.64 ms delay min

	// entry mode
	lcd_instruction(entryMode);          // this instruction shifts the cursor
	_delay_us(40);                      // 40 us delay min

	// Display on
	lcd_instruction(on);          // turn on the display
	_delay_us(50);               // same delay as off
}


void lcd_string(uint8_t string[])
{
	int i = 0;                             //while the string is not empty
	while (string[i] != 0)
	{
		lcd_char(string[i]);
		i++;
		_delay_us(50);                              //40 us delay min
	}
}


void lcd_char(uint8_t data)
{
	lcdPort |= (1 << lcdRSBit);                 // RS high
	lcdPort &= ~(1 << lcdEBit);                // E low
	lcd_write(data);                          // write the upper four bits of data
	lcd_write(data << 4);                    // write the lower 4 bits of data
}


void lcd_instruction(uint8_t instruction)
{
	lcdPort &= ~(1 << lcdRSBit);                // RS low
	lcdPort &= ~(1 << lcdEBit);                // E low
	lcd_write(instruction);                   // write the upper 4 bits of data
	lcd_write(instruction << 4);             // write the lower 4 bits of data
}


void lcd_write(uint8_t byte)
{
	lcdPort &= ~(1 << lcdD7Bit);                        // assume that data is '0'
	if (byte & 1 << 7) lcdPort |= (1 << lcdD7Bit);     // make data = '1' if necessary

	lcdPort &= ~(1 << lcdD6Bit);                        // repeat for each data bit
	if (byte & 1 << 6) lcdPort |= (1 << lcdD6Bit);

	lcdPort &= ~(1 << lcdD5Bit);
	if (byte & 1 << 5) lcdPort |= (1 << lcdD5Bit);

	lcdPort &= ~(1 << lcdD4Bit);
	if (byte & 1 << 4) lcdPort |= (1 << lcdD4Bit);

	// write the data
	
	lcdPort |= (1 << lcdEBit);                   // E high
	_delay_us(1);                               // data setup
	lcdPort &= ~(1 << lcdEBit);                // E low
	_delay_us(1);                             // hold data
}

/****************** RFID Configuration **********************/

void USART_RF_init(void)
{
	/*Set baud rate */
	UBRR0H = (BAUDRATE>>8);
	UBRR0L = BAUDRATE;
	
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);

	/* Set frame format: 8data, 1stop bit, no parity */
	UCSR0C = (3<<UCSZ00);
	
	/* Enable interrupt */
	UCSR0B |= (1 << RXCIE0);

}

unsigned char USART_RF_receive(void)
{
	/* Wait for data to be received */
	while(~(UCSR0A) & (1<<RXC0));

	/* Get and return received data from buffer */
	return UDR0;
}

void USART_RF_send(unsigned char data)
{
	/* Wait for data to be received */
	while (!( UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

struct {
	char tag[SIZE + 1];
	bool adopted;                   	// dog is currently in the shelter
	} cards[REGISTERED_TAGS] = {
	[0].tag = {0x0a, 0x30, 0x46, 0x30, 0x32, 0x44, 0x37, 0x37, 0x37, 0x43, 0x36, 0x0d, 0x00},
	[0].adopted = false,
	[1].tag = {0x0a, 0x30, 0x46, 0x30, 0x32, 0x44, 0x37, 0x37, 0x37, 0x43, 0x46, 0x0d, 0x00},
	[1].adopted = false
	[2].tag = {0x0a, 0x30, 0x46, 0x30, 0x32, 0x44, 0x37, 0x37, 0x37, 0x43, 0x46, 0x0d, 0x00},
	[2].adopted = false
};

struct {
	/* RFID buffer */
	volatile char ID[SIZE + 1];
	volatile uint8_t index;
	volatile bool done;
}RF;

inline bool RFID_done(void) {
	return RF.done;
}

inline void RFID_ready(void) {
	/* RFID buffer is ready to be refilled*/
	RF.done = false;
}

ISR(USART0_RX_vect){
	/* load RFID into buffer */
	char num = USART_RF_receive();
	if (RF.done) return;
	if(!RF.done) {
		RF.ID[RF.index++] = num;
		if(RF.index == SIZE) {
			RF.index = 0;
			RF.done = true;
		}
	}
	USART_RF_send(num);


ISR(USART0_RX_vect) {
	char num = USART_RF_receive();
	if (RF.done) return;
	uint8_t index = RF.index;
	if ((index == 0 && num != 0x0A) || (index == SIZE - 1 && num != 0x0D)) {
		RF.index = 0;	// reset buffer 
		return;
	}
	RF.ID[RF.index++] = num;
	if (RF.index >= SIZE) {
		RF.index = 0;
		RF.ID[SIZE] = 0; // null terminator
		RF.done = true; // lock the buffer 
	}
}

int find_card(char * str) {
	for (int i = 0; i < REGISTERED_TAGS; i++) {
		if (strcmp(cards[i].tag, str) == 0) {
			return i;
		}
	}
	return -1;
}

void probe_card_reader(void) {
	if (!RFID_done()) { // no card is near the RFID scanner
		return;
	}
	lcd_instruction(clear);
	int card_index = find_card((char *)RF.ID);
	if (card_index >= 0) {                      // check if card is found
		cards[card_index].adopted ^= 1;     // toggle card status
		lcd_string("Card ");
		lcd_char(card_index + '1');
		lcd_string(" detected!");
		lcd_instruction(setCursor | lineTwo);
		lcd_instruction(cards[card_index].tag, 1, SIZE - 1);
	}
	else {
		lcd_string("This card is");
		lcd_instruction(setCursor | lineTwo);
		lcd_string("not registered.");
	}
	_delay_ms(3000);
	lcd_instruction(clear);
	RFID_ready();
}

/****************** Wifi Configuration **********************/


int main( void )
{

	lcd_init();
	USART_RF_init();
	sei();
	
	lcd_string((uint8_t *)"Scan a tag");

	while (1) {
		
		RFID_done();
		lcd_instruction(setCursor | lineTwo);
		
		for(uint8_t i = 1; i < 11; i++) {
			lcd_char(RF.ID[i]);
			_delay_us(50);
		}
		
		USART_RF_send('\n');
		_delay_ms(200);
		RFID_ready();
	}
	
	return 0;


}