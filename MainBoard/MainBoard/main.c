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
#define REGISTERED_TAGS  3  

#define ASSERT(condition)       if(!(condition)) {asm("break");}

       

/********************************************************************** LCD Configuration ********************************************************************/

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

/******************************************************************* RFID Configuration ********************************************************************/
#define CREADER_INDEX -1

typedef enum {adopted, surrendered, ALARMED} card_status_t;

struct {
	char tag[SIZE + 1]; // the RFID tag of the card attached to the medicine
	card_status_t status;           // is this medicine currently checked out, checked it, or overdue?
	bool armed;                     // flag used by check_alarm to upload the alarm triggered event only once
	} 
	
	cards[REGISTERED_TAGS] = {             // default initializations mainly used for debugging
	[0].tag = {0x00, 0x33, 0x31, 0x30, 0x30, 0x33, 0x37, 0x44, 0x39, 0x33, 0x44, 0x00, 0x00},
	[0].status = surrendered, [0].armed = true,
	[1].tag = {0x00, 0x33, 0x31, 0x30, 0x30, 0x33, 0x37, 0x44, 0x39, 0x33, 0x44, 0x00, 0x00},
	[1].status = surrendered, [0].armed = true,
	[2].tag = {0x00, 0x36, 0x36, 0x30, 0x30, 0x36, 0x43, 0x34, 0x42, 0x37, 0x46, 0x00, 0x00},
	[2].status = surrendered, [1].armed = true
};

struct {
	/* RFID buffer */
	volatile char ID[SIZE + 1];
	volatile uint8_t index;
	volatile bool done;
}RF;


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


inline bool RFID_done(void) {
	return RF.done;
}

inline void RFID_ready(void) {
	/* RFID buffer is ready to be refilled*/
	RF.done = false;
}

char * get_card_id(int8_t index) {
	char * rfid = (index == CREADER_INDEX)? (char *) RF.ID : cards[index].tag;
	return  (rfid + 1); // actually return a pointer to index 1 as index 0 is always 0x00
}

int find_card(void) {
	for (int i = 0; i < REGISTERED_TAGS; i++) {
		if (strcmp(cards[i].tag + 1, (char *)(RF.ID + 1)) == 0) {
			return i;
		}
	}
	return -1;
}

ISR(USART0_RX_vect) {
	char num = USART_RF_receive();
	USART_RF_send(num); // debug:: echo
	if (RF.done) return;
	int8_t index = RF.index;
	ASSERT(0 <= index && index < SIZE);
	if ((index == 0 && num != 0x0A) || (index == SIZE - 1 && num != 0x0D)) {
		RF.index = 0; // reset buffer since data is not valid
		return;
	}
	RF.ID[RF.index++] = num;
	if (RF.index >= SIZE) { // we successfully scanned a card.
		RF.index = 0;
		RF.ID[SIZE - 1] = RF.ID[0] = 0; // insert null at the beginning and at the end
		RF.done = true; // lock the buffer so it won't be modified until consumed by user
	}
}


/******************************************************************* Wifi Configuration *****************************************************************/

int main( void )
{
	
	sei();
	lcd_init();
	USART_RF_init();
	lcd_instruction(clear);
	lcd_string((uint8_t *)"My Shelter");
	_delay_ms(2000);


	while (1) {
		
	}
	
	return 0;


}