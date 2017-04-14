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

volatile uint16_t z;
volatile uint16_t ones;
volatile uint8_t count;
volatile uint8_t second;
volatile bool parity_error;
volatile bool found_nine_ones;

struct {
	int8_t data[400];
	volatile unsigned int index[4];
	volatile bool ready;
	volatile bool done;
	int8_t buff[10];
	
}RFID;


void interr_init(void) {
	DDRD &= ~(1 << PIND2);		//Receiver input
	PORTD |= 1 << PIND2;		// pull up resistor
	EIMSK = 1 << INT0;			//enable interrupt 0
	EIFR = 1 << INTF0;			//clear flag
	MCUCR = 1 << ISC00;			//trigger on any edge
	
}

ISR(INT0_vect) {
	
	_delay_us(483);
	
	//483
	
	RFID.data[z] = ((PIND & 0x04)>>2);
	
	if (RFID.data[z] == 1) {
		
		count++;

		if(count == 9 && RFID.ready == false) {		//wait until 9 consecutive 1s
			
			RFID.index[ones] = z+1;		//store the index of each 9 1s

			if (ones > 3){
				RFID.ready = true;
				found_nine_ones = true;
				ones = 0;
				count = 0;
			}
			
			else ones++;
		}
		
	}
	
	else count = 0;	
	
	if (z < 398) z++;		
	else { z = 0; RFID.done = true;}
	
	
						
	EIFR = 1 << INTF0;		//clear flag
	
}


bool manchester_done(void) {
	
	if (RFID.done == true && found_nine_ones == true){	
		cli();
		
		unsigned int index;
		index = RFID.index[ones];
		int8_t col_parity[4] = {0}; 
		volatile int8_t row_parity[10] = {0}; 
			 
		 for (int8_t i = 0; i < 10; i++) {					//10 parity bits = 50 total iterations 
			 
			volatile int8_t rfid_char = 0;
			
			 for (int8_t j = 3; j >= 0; j--) {		
				 int8_t decoded_bit = RFID.data[index];		//save each bit
				 rfid_char += decoded_bit << j;				//shift 4 times to create 8 bit int
				 index++;									//increment the index 4 times
			 }
			 
			 RFID.buff[i] = rfid_char;				//save each character
			 index++;								//increment the index to the parity bit (5x)
			 row_parity[i] = RFID.data[index];//save the row parity bit
			 
			 //row_P_check[i] = (((!(RFID.data[index-1]) != !(RFID.data[index-2]) != !(RFID.data[index-3])) != !(RFID.data[index-2])
			 
			if(row_parity[i] != (rfid_char & 1)) parity_error = true;

		 }
		 
		 for (int8_t i = 3; i >= 0; i--) {			//final 4 parity bits
			 col_parity[i] += RFID.data[index];
			 index++;
		 }
		 
		  
		 int8_t stop_bit = RFID.data[index];
		 
		 if (stop_bit != 0 && col_parity[1] !=0){ 
			index = RFID.index[ones++];
			 return false;
			 }
			 
			RFID.done = false;
			RFID.ready = false;
			found_nine_ones = false;
			index = 0;
			return true;		
	 }
	 
	 return false;
}

char toChar(int8_t i) {
	if ( 0 <= i && i <= 9){
		return i + '0';
		} else {
		return (i - 10) + 'A';
	}
}

int main( void )
{
 
	lcd_init();
	USART_init();
	frequency_init();
	interr_init();
	SPI_init();
	
	lcd_instruction(clear);
	lcd_string((uint8_t *)"Ready to Scan");

	sei();
	
	while (1) {	
		
		if(!manchester_done()) continue;

		lcd_instruction(clear);
		
		for (int i = 0; i < 10; i++) {
			lcd_char(toChar(RFID.buff[i]));
			_delay_us(50);
		}
		
		USART_send(0x0A);
		for (int i = 0; i < 10; i++) {
			USART_send(toChar(RFID.buff[i]));
		}
		USART_send(0x0D);
		beep();
		_delay_ms(1000);	
		ones = 0;
		z = 0;
		sei();
		
		/* lcd_instruction(clear);
		lcd_string((uint8_t *)"Ready to Scan");
		lcd_instruction(setCursor | lineTwo);
		
		RFID_done();
		
		for(uint8_t i = 1; i < 11; i++) {
		lcd_char(RF.ID[i]);
		_delay_us(50);
		USART_send('\n');
		beep();
		RFID_ready();
		_delay_ms(2000);
		*/
		
	}
	
	return 0;


}