#include "LiquidCrystal_I2C.h"

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows, uint8_t charsize)
{
	_addr = lcd_addr;
	_cols = lcd_cols;
	_rows = lcd_rows;
	_charsize = charsize;
	_backlightval = LCD_BACKLIGHT;
}

#if defined(ESP8266)
void LiquidCrystal_I2C::begin(uint8_t sda, uint8_t scl){
	Wire.begin(sda, scl);
	init();
}
#else
void LiquidCrystal_I2C::begin(){
	Wire.begin();
	init();
}
#endif

void LiquidCrystal_I2C::init(){

	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	if (_rows > 1) {
		_displayfunction |= LCD_2LINE;
	}

	// for some 1 line displays you can select a 10 pixel high font
	if ((_charsize != 0) && (_rows == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	delay(50); // wait for LCD to power up

	expanderWrite(_backlightval); // reset expander and turn backlight off
	delay(1000);

	write4bits(0x03 << 4);
	delayMicroseconds(4500); 

	write4bits(0x03 << 4);
	delayMicroseconds(4500); 

	write4bits(0x03 << 4); 
	delayMicroseconds(150);

	write4bits(0x02 << 4); 

	command(LCD_FUNCTIONSET | _displayfunction);  
	
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	
	clear();
	
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	command(LCD_ENTRYMODESET | _displaymode);
	
	home();
}

void LiquidCrystal_I2C::clear(){
	command(LCD_CLEARDISPLAY);
	delayMicroseconds(2000);
}

void LiquidCrystal_I2C::home(){
	command(LCD_RETURNHOME);
	delayMicroseconds(2000);
}

void LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if (row > _rows) {
		row = _rows - 1;
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void LiquidCrystal_I2C::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LiquidCrystal_I2C::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LiquidCrystal_I2C::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LiquidCrystal_I2C::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7;
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i = 0; i < 8; i++) {
		write(charmap[i]);
	}
}

void LiquidCrystal_I2C::noBacklight(void) {
	_backlightval = LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void LiquidCrystal_I2C::backlight(void) {
	_backlightval = LCD_BACKLIGHT;
	expanderWrite(0);
}

bool LiquidCrystal_I2C::getBacklight() {
	return _backlightval == LCD_BACKLIGHT;
}

inline void LiquidCrystal_I2C::command(uint8_t value) {
	send(value, 0);
}

inline size_t LiquidCrystal_I2C::write(uint8_t value) {
	send(value, Rs);
	return 1;  // Added this line to return a size_t value
}

void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
	uint8_t highnib = value & 0xf0;
	uint8_t lownib = (value << 4) & 0xf0;
	write4bits(highnib | mode);
	write4bits(lownib | mode);
}

void LiquidCrystal_I2C::write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}

void LiquidCrystal_I2C::expanderWrite(uint8_t _data) {
	Wire.beginTransmission(_addr);
	Wire.write((int)(_data) | _backlightval);
	Wire.endTransmission();
}

void LiquidCrystal_I2C::pulseEnable(uint8_t _data) {
	expanderWrite(_data | En);
	delayMicroseconds(1);

	expanderWrite(_data & ~En);
	delayMicroseconds(50);
}

void LiquidCrystal_I2C::load_custom_character(uint8_t char_num, uint8_t *rows) {
	createChar(char_num, rows);
}

void LiquidCrystal_I2C::setBacklight(uint8_t new_val) {
	if (new_val) {
		backlight();
	} else {
		noBacklight();
	}
}

void LiquidCrystal_I2C::printstr(const char c[]) {
	print(c);
}
