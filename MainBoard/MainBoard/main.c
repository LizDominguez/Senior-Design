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
#define SIZE 12
#define ICP PIND6
#define REGISTERED_TAGS  3 
#define IP_ADDRESS       "52.24.121.235" 


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

/******************************************************************* RFID Configuration ********************************************************************/
#define CREADER_INDEX -1

typedef enum {adopted, surrendered} dog_status;

struct {
	char tag[SIZE + 1]; //RFID Tag
	dog_status status;           
	} 
	
	cards[REGISTERED_TAGS] = {            
	[0].tag = {0x00, 0x32, 0x43, 0x30, 0x30, 0x41, 0x43, 0x36, 0x39, 0x33, 0x45, 0x00, 0x00},
	[0].status = surrendered, 
	[1].tag = {0x00, 0x33, 0x31, 0x30, 0x30, 0x33, 0x37, 0x44, 0x39, 0x33, 0x44, 0x00, 0x00},
	[1].status = surrendered, 
	[2].tag = {0x00, 0x54, 0x46, 0x30, 0x30, 0x35, 0x43, 0x41, 0x44, 0x36, 0x30, 0x00, 0x00},
	[2].status = surrendered
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

#define ESP8266_ROW_SIZE 15
#define ESP8266_COL_SIZE 52

struct ESP8266_buff {
	volatile char buffer[ESP8266_ROW_SIZE][ESP8266_COL_SIZE];
	volatile uint8_t row_index;
	volatile uint8_t col_index;
} ESP8266;

void UART_ESP8266_send(unsigned char data) {
	while (!( UCSR1A & (1<<UDRE1)));
	UDR1 = data;
}

void UART_ESP8266_cmd(char string[]) {
	for (int i = 0; string[i] != 0; i++) {
		UART_ESP8266_send(string[i]);
	}
	UART_ESP8266_send(0x0D);
	UART_ESP8266_send(0x0A);
}

unsigned char UART_ESP8266_receive(void) {
	while(~(UCSR1A) & (1<<RXC1));
	return UDR1;
}

int ESP8266_search_for_str(char string[]) {
	for (int i = 0; i < ESP8266_ROW_SIZE - 1; i++) {
		if(strcmp((char *)ESP8266.buffer[i], string) == 0) {
			ESP8266.buffer[i][0] = 0; // clear the string
			return i;
		}
	}
	return -1;
}

bool ESP8266_find(char string[]) {
	for (int i = 0; i < ESP8266_ROW_SIZE - 1; i++) {
		if(strcmp((char *)ESP8266.buffer[i], string) == 0) {
			ESP8266.buffer[i][0] = 0; // clear the string
			return true;
		}
	}
	return false;
}

void ESP8266_clear_buffer(void) {
	for (int i = 0; i < ESP8266_ROW_SIZE - 1; i++) {
		ESP8266.buffer[i][0] = 0;
	}
	ESP8266.row_index = 0;
	ESP8266.col_index = 0;
}

bool isConnected(void) {
	lcd_instruction(clear);
	lcd_string((uint8_t *)"Wifi is...         ");
	
	for (;;) {
		lcd_instruction(setCursor | lineTwo);
		UART_ESP8266_cmd("AT+CIPSTATUS");
		_delay_ms(500);
		if (ESP8266_find("STATUS:2")) {
			lcd_string((uint8_t *)"Connected!       ");
			ESP8266_clear_buffer();
			return true;
			} 
		else if (ESP8266_find("STATUS:5")) {
			lcd_string((uint8_t *)"Not Connected.  ");
			ESP8266_clear_buffer();
			} 
		else {
			lcd_string((uint8_t *)"Not Responding.   ");
			return false;
		}

	}
}


void upload_to_server(char * rfid, char action) {
	char HTTP_request_buffer[] = "GET /add/##########/& HTTP/1.0";
	for (int i = 0 ; i < 10; i++) { // copy the RFID to the buffer (starting at first # which is index 9)
		HTTP_request_buffer[9 + i] = rfid[i];
	}
	HTTP_request_buffer[20] = action; // copy the action (index 20 which is &)
	UART_ESP8266_cmd("AT+CIPSTART=\"TCP\",\""IP_ADDRESS"\",80");
	_delay_ms(1000);
	UART_ESP8266_cmd("AT+CIPSEND=34");
	_delay_ms(1000);
	UART_ESP8266_cmd(HTTP_request_buffer);
	UART_ESP8266_cmd("");
	_delay_ms(1000);
}

void UART_ESP8266_init(void) {
	
	UBRR1H = (BAUDRATE>>8);
	UBRR1L = BAUDRATE;
	UCSR1B = (1<<TXEN1) | (1<<RXEN1);
	UCSR1C = (3<<UCSZ10);
	UCSR1B |= (1 << RXCIE1); 
	
	for (;;) {
		ESP8266_clear_buffer();
		UART_ESP8266_cmd("AT+RST");
		
		lcd_instruction(clear);
		lcd_string((uint8_t *)"Configuring Wifi...");
		_delay_ms(1000);
		
		if (!ESP8266_find("ready")) { // seems like the ESP8266 didn't respond...
			lcd_instruction(clear);
			lcd_string((uint8_t *)"timeout/UART err");
			lcd_instruction(setCursor | lineTwo);
			lcd_string((uint8_t *)"Restarting...");
			_delay_ms(1000);
			continue;
		}
		
		UART_ESP8266_cmd("ATE0"); // disable ESP8266 echo functionality
		_delay_ms(500);
		
		if (!isConnected()) continue;       // if wifi is not responding
		upload_to_server("----------",'b');  // restart system
		break;
	}
}


ISR(USART1_RX_vect) {
	char c = UART_ESP8266_receive();
	int row = ESP8266.row_index, col = ESP8266.col_index;
	ESP8266.buffer[row][col] = c;
	if ((col > 0 && ESP8266.buffer[row][col - 1] == 0x0D && ESP8266.buffer[row][col] == 0x0A)
	|| (col == ESP8266_COL_SIZE - 1)) {
		ESP8266.buffer[row][col - 1] = 0; // insert null terminator
		ESP8266.row_index = (row == ESP8266_ROW_SIZE - 1)? 0: row + 1;
		ESP8266.col_index = 0;  // return to the beginning of the line
		return;
	}
	ESP8266.col_index++;
}

void probe_card_reader(void) {
	
	if (!RFID_done()) return;
	
	lcd_instruction(clear);
	
	int card_index = find_card();
	
	if (card_index < 0) { // card not found
		lcd_string((uint8_t *)"This card is    ");
		lcd_instruction(setCursor | lineTwo);
		lcd_string((uint8_t *)"not registered.   ");
		_delay_ms(500);
		lcd_instruction(clear);
		RFID_ready();
		return;
	}
	
	lcd_string((uint8_t *)"Dog ");
	lcd_char(card_index + '1');
	
	RFID_ready();
	
	dog_status current_status = cards[card_index].status;
	char status_to_upload = '?';
	
	switch(current_status) {
		case adopted:
			cards[card_index].status = surrendered;
			lcd_string((uint8_t *)"surrendered    ");
			status_to_upload = 's';
			break;
		case surrendered:
			cards[card_index].status = adopted;
			lcd_string((uint8_t *)"adopted    ");
			status_to_upload = 'a';
			break;
	}

	lcd_instruction(setCursor | lineTwo);
	lcd_string((uint8_t *)"ID: ");
	lcd_string((uint8_t *)get_card_id(card_index));
	upload_to_server(get_card_id(card_index), status_to_upload);
	lcd_instruction(clear);
	RFID_ready();
	
}


int main( void )
{
	
	sei();
	lcd_init();
	USART_RF_init();
	UART_ESP8266_init();


	while (1) {
		
	probe_card_reader();
		
	}
	
	return 0;


}