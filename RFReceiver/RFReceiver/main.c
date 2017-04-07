#define F_CPU 8000000UL
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include <string.h>

#define BAUD 9600
#define BAUDRATE (((F_CPU / (BAUD * 16UL))) - 1)
#define RXD0 PIND0
#define SIZE 16
#define ICP PIND6

int16_t sine[50] = {576,639,700,758,812,862,906,943,974,998,1014,1022,1022,1014,998,974,943,906,862,812,758,700,639,576,512,447,384,323,265,211,161,117,80,49,25,9,1,1,9,25,49,80,117,161,211,265,323,384,447,511};


/************************************************************************* LCD Configuration *************************************************************/

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
		_delay_us(10);
		lcd_write(instruction << 4);             // write the lower 4 bits of data
		_delay_ms(5);
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

/*********************************************************** USART Configuration *****************************************************************/

void USART_init(void)
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

void USART_send(unsigned char data)
{
	/* Wait for data to be received */
	while (!( UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

/*
unsigned char USART_receive(void)
{
	// Wait for data to be received
	while(~(UCSR0A) & (1<<RXC0));
	//Get and return received data from buffer 
	return UDR0;
}
struct {
	//RFID buffer 
	volatile char ID[SIZE + 1];
	volatile uint8_t index;
	volatile bool done;
}RF;
inline void RFID_done(void) {
	while(!RF.done);
}
inline void RFID_ready(void) {
	//RFID buffer is ready to be refilled
	RF.done = false;
}
ISR(USART0_RX_vect){
	//load RFID into buffer 
	char num = USART_receive();
	if(!RF.done) {
		RF.ID[RF.index++] = num;
		if(RF.index == SIZE) {
			RF.index = 0;
			RF.done = true;
		}
	}
	USART_send(num);
}
*/

/*************************************************************** SPI to DAC **********************************************************/
volatile uint32_t adcVal = 0;
volatile uint32_t freq = 1;

void inline SPI_init()
{
	DDRB |= (1<<PORTB5 | 1<<PORTB7 | 1<<PORTB4); 	// initializing ss, sck, and mosi pins
	SPSR |= 1<<SPI2X;
	SPCR |= (1<<SPE | 1<<MSTR); 			//enabling master mode, frequency 2 mhz
	PORTB |= 1<<PORTB4; 					//ss high
}

void dac_write(uint16_t val)
{
	PORTB &= ~(1<<PORTB4); 		//turn off ss

	val = val << 2;
	val |= (0b1001 << 12);

	SPDR = val >> 8; 		//upper bytes

	while(!(SPSR & 1<<SPIF)); //wait til upper bytes done

	SPDR = (0xFF & val); 		//lower bytes

	while(!(SPSR & 1<<SPIF)); //wait til lower bytes done

	PORTB |= 1<<PORTB4; 	//turn on ss
}

void frequency(uint32_t frequency)
{
	
	if (frequency == 0) {
		for(volatile uint16_t i = 0; i < 1; i++);	//800 Hz
	}
	
	else if (frequency == 1) {
		for(volatile uint16_t i = 0; i < 2; i++);	//1 kHz
	}
		

}

void output_waveform(uint32_t value, uint16_t arr[])
{
	for (int i = 0; i < 50; i++)		//iterating through wave lookup table
	{
		dac_write(arr[i]);
		frequency(value);
	}
}

void beep(void) {
	
	for(uint8_t i = 0; i < 150; i++) {
		output_waveform(freq, (uint16_t *)sine);
	}
}

/***************************************************************** 125kHz wave **********************************************************/
void frequency_init(void) {
	DDRD |= (1 << PORTD7);
	TCCR2A |= (1<<WGM20 | 1<<WGM21 | 1<<COM2A0);
	TCCR2B |= (1<<WGM22 | 1<<CS20); //Fast PWM
	OCR2A = 31;		// 8000000/64 = 125k, but it's half with 50% duty cycle. 31 was value with least percent error at 126.6 kHz
}

/****************************************************** Manchester Decoding **********************************************************/

volatile int i = 0;
unsigned int count = 0;

struct {
	char buff[500];
	unsigned int tag;
	unsigned int index;
	bool flag;
}RFID;

char card1[] = {0x32, 0x43, 0x30, 0x30, 0x41, 0x43, 0x36, 0x39, 0x33, 0x45};
char card2[] = {0x33, 0x31, 0x30, 0x30, 0x33, 0x37, 0x44, 0x39, 0x33, 0x44};
char card3[] = {0x36, 0x46, 0x30, 0x30, 0x35, 0x43, 0x41, 0x44, 0x36, 0x30};


void interr_init(void) {
	DDRD &= ~(1 << PIND2);		//Receiver input
	PORTD |= 1 << PIND2;		// pull up resistor
	EIMSK = 1 << INT0;			//enable interrupt 0
	EIFR = 1 << INTF0;			//clear flag
	MCUCR = 1 << ISC00;			//trigger on any edge
	
}

ISR(INT0_vect) {
	
	_delay_us(350);
	
	RFID.buff[i] = ((PIND & 0x04)>>2);
	
	if (RFID.buff[i] == 1) {		
		count += RFID.buff[i];	
	}
		
	else count = 0;	
	
	if(count == 9 && RFID.flag == false) {
		RFID.index = i+1;
		RFID.flag = true;
		}
		
	if(i < 499) i++;
	
	else {
		i = 0;
		RFID.flag = false;
		cli();
	}
	
	EIFR = 1 << INTF0; //clear flag
	
}

bool found_tag(void){

	if(i == 499){
		RFID.tag = 0;	
		
		for(int j = 11; j <51; j++){
			RFID.tag += RFID.buff[RFID.index + j];
			
			if (j == 50){
				
				lcd_instruction(clear);
				
				switch(RFID.tag){
					case 31:
					lcd_string((uint8_t *) card1);
					break;
					
					case 32:
					lcd_string((uint8_t *) card2);
					break;
					
					case 33:
					lcd_string((uint8_t *) card3);
					break;
					
				}
				
				
				return true;

			}
		}
	
	}
	
	return false;
	
}

int main( void )
{

	 
	lcd_init();
	USART_init();
	frequency_init();
	interr_init();
	SPI_init();

	
	sei();
	
	while (1) {
		
		if(!found_tag()) continue;
		sei();
		beep();

		
	}
	
	return 0;


}