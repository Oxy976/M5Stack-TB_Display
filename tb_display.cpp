/******************************************************************************
 * tb_display.cpp
 * Library for a simple text buffer scrolling display on the M5StickC.
 * Hague Nusseck @ electricidea
 * v1.3 04.Feb.2020
 * https://github.com/electricidea/M5StickC-TB_Display
 *
 * This library makes it easy to display texts on the M5StickC.
 * The display behaves like a terminal: New text is added at the bottom.
 * The text scrolls up with every new line. The lines are automatically wrapped.
 * The display can be used in any orientation.
 *
 * Changelog:
 * v1.0 = - initial version
 * v1.1 = - Added delay parameter to tb_display_print_String function
 *        - Added text demo in Example (Button B on M5StickC)
 * v1.2 = - Supress of space characters as first character on a new row
 *          after a new line
 *        - Add a word wrapping fuction inside the print_char function
 * v1.3 = - Bugfix if the character that causes a word wrap is a space character
 *
 * Distributed as-is; no warranty is given.
 ******************************************************************************/

/*********
* 1. changed M5StickC to M5Stack (lib, size)
* 2. in drawChar(int16_t uniCode, int32_t x, uint16_t y, uint8_t font); last argument is font, not size. For set size declared M5.Lcd.setTextSize
* 3. for get symbols height declared M5.Lcd.fontHeight()
* 
* *for the future* I think if add a choice of screen size by checking which device, then 
* this can make the library more universal.
*/

#include <Arduino.h>
#include <M5Stack.h>
#include "tb_display.h"

// good readable - size 2 font 1. 
#define TEXT_SIZE 2
#define TEXT_FONT 1
//#define TEXT_HEIGHT 16
//TEXT_HEIGHT = M5.Lcd.fontHeight();  // Height of text in pixel
// Display size of M5Stack = 320x240 pix
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// screen buffer, for max characters. (for min font - #1 size 1 ,  probably)
#define TEXT_BUFFER_HEIGHT_MAX 16
#define TEXT_BUFFER_LINE_LENGTH_MAX 40
char text_buffer[TEXT_BUFFER_HEIGHT_MAX][TEXT_BUFFER_LINE_LENGTH_MAX];

int font_height;

int text_buffer_height;
int text_buffer_line_length;
int text_buffer_write_pointer_x;
int text_buffer_write_pointer_y;
int text_buffer_read_pointer;
// with M5.Lcd.setRotation(1)
// the position 0,0 is the upper left corner
// starting a bit more right...
#define SCREEN_XSTARTPOS 4
int screen_xpos = SCREEN_XSTARTPOS;
// start writing at the last line
int screen_ypos;
// maximum width of the screen
int screen_max;

// Enable or disable Waord Wrap
boolean tb_display_word_wrap = true;

// =============================================================
// Initialization of the Text Buffer and Screen
// ScreenRotation values:
// 1 = Button right
// 2 = Button above
// 3 = Button left
// 4 = Button below

// =============================================================
void tb_display_init(int ScreenRotation){
  M5.Lcd.setTextSize(TEXT_SIZE);
  font_height =  M5.Lcd.fontHeight();  //height for chosed font
  M5.Lcd.setRotation(ScreenRotation);
  switch (ScreenRotation) {
    case 1: case 3: {
      //  rows of text in landscape mode
   //   text_buffer_height = 15;
      text_buffer_height = floor (SCREEN_HEIGHT / font_height);  //  size screen / size symbol = number of lines
      text_buffer_line_length = 40;  // alas, the length of the string is not so easy to get
      // A small margin on the right side prevent false print results
      screen_max = SCREEN_WIDTH-2;
      break;
    }
    case 2: case 4: {
      //  rows of text in portrait mode
 //     text_buffer_height = 26;
      text_buffer_height = floor (SCREEN_WIDTH / font_height);
      text_buffer_line_length = 15;
      // A small margin on the right side prevent false print results
      screen_max = SCREEN_HEIGHT-2;
      break;
    }
    default: {
      break;
    }
  }
  tb_display_clear();
  tb_display_show();
}

// =============================================================
// clear the text buffer
// without refreshing the screen
// call tb_display_show(); to clear the screen
// =============================================================
void tb_display_clear(){
  for(int line=0; line<TEXT_BUFFER_HEIGHT_MAX; line++){
    for(int charpos=0; charpos<TEXT_BUFFER_LINE_LENGTH_MAX; charpos++){
      text_buffer[line][charpos]='\0';
    }
  }
  text_buffer_read_pointer = 0;
  text_buffer_write_pointer_x = 0;
  text_buffer_write_pointer_y = text_buffer_height-1;
  screen_xpos = SCREEN_XSTARTPOS;
//  screen_ypos = TEXT_HEIGHT*(text_buffer_height-1);
  screen_ypos = font_height*(text_buffer_height-1);
}

// =============================================================
// clear the screen and display the text buffer
// =============================================================
void tb_display_show(){
  M5.Lcd.fillScreen(TFT_BLACK);
  int yPos = 0;
  for(int n=0; n<text_buffer_height; n++){
    // modulo operation for line position
    int line = (text_buffer_read_pointer+n) % text_buffer_height;
    int xPos = SCREEN_XSTARTPOS;
    int charpos=0;
    while(xPos < screen_max && text_buffer[line][charpos] != '\0'){
      xPos += M5.Lcd.drawChar(text_buffer[line][charpos],xPos,yPos,TEXT_FONT);
      charpos++;
    }
//    yPos = yPos + TEXT_HEIGHT;
    yPos = yPos + font_height;
  }
  screen_xpos = SCREEN_XSTARTPOS;
}

// =============================================================
// creates a new line and scroll the display upwards
// =============================================================
void tb_display_new_line(){
  text_buffer_write_pointer_x = 0;
  text_buffer_write_pointer_y++;
  text_buffer_read_pointer++;
  // circular buffer...
  if(text_buffer_write_pointer_y >= text_buffer_height)
    text_buffer_write_pointer_y = 0;
  if(text_buffer_read_pointer >= text_buffer_height)
    text_buffer_read_pointer = 0;
  // clear the actual new line for writing (first character a null terminator)
  text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
  tb_display_show();
}

// =============================================================
// print a single character
// the character is added to the text buffer and
// directly printed on the screen.
// The text is automatically wrapped if longer than the display
// example:
//    tb_display_print_char('X');
// =============================================================
void tb_display_print_char(byte data){
  // check for LF for new line
  if (data == '\n') {
    // last character in the text_buffer line  should be always a null terminator
    text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
    tb_display_new_line();
  }
  // only 'printable' characters
  if (data > 31 && data < 128) {
    // print the character and get the new xpos
    screen_xpos += M5.Lcd.drawChar(data,screen_xpos,screen_ypos,TEXT_FONT);
    // if maximum number of characters reached
    if(text_buffer_write_pointer_x >= text_buffer_line_length-1){
      tb_display_new_line();
      // draw the character again because it was out of the screen last time
      screen_xpos += M5.Lcd.drawChar(data,screen_xpos,screen_ypos,TEXT_FONT);
    }
    // or if line wrap is reached
    if(screen_xpos >= screen_max) {
      // prepare for Word-Wrap stuff...
      // the buffer for storing the last word content
      char Char_buffer[TEXT_BUFFER_LINE_LENGTH_MAX];
      int n = 1;
      Char_buffer[0] = data;
      Char_buffer[n] = '\0';
      // if Word-Wrap, go backwards and get the last "word" by finding the
      // last space character:
      if(tb_display_word_wrap){
        int test_pos = text_buffer_write_pointer_x-1;
        // get backwards and search a space character
        while(test_pos > 0 && text_buffer[text_buffer_write_pointer_y][test_pos] != ' '){
          // store all the characters on the way back to the last space character
          Char_buffer[n] = text_buffer[text_buffer_write_pointer_y][test_pos];
          test_pos--;
          n++;
          Char_buffer[n] = '\0';
        }
        // if there was no space character in the row, Word-Wrap is not possible
        if(test_pos == 0) {
          // don't use the buffer but draw the character passed to the function
          n = 1;
        } else {
          // otherwise use the buffer to print the last found characters of the word
          // but only, if the charachter that causes a word wrap is not a space character
          if(data != ' '){
            // place a \0 at the position of the found space so that the drawing fuction ends here
            text_buffer[text_buffer_write_pointer_y][test_pos] = '\0';
          }
        }
      }
      tb_display_new_line();
      // icharacter passed to the function is a space character, then don't display
      // it as the first character of the new line
      if(data == ' ')
        // don't use the buffer at all
        n = 0;
      n--;
      while(n >= 0){
        // draw the characters from the buffer back on the screen
        screen_xpos += M5.Lcd.drawChar(Char_buffer[n],screen_xpos,screen_ypos,TEXT_FONT);
        // write the characters into the screen buffer of the new line
        text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = Char_buffer[n];
        text_buffer_write_pointer_x++;
        n--;
      }
      text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
    } else {
      // write the character into the screen buffer
      text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = data;
      text_buffer_write_pointer_x++;
      // following character a null terminator to clear the old characters of the line
      text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
    }
  }
}

// =============================================================
// print a string
// The string is added to the text buffer and directly printed
// on the screen.
// The otional parameter "chr_delay" allows a "character by character"
// processing of the String. Then, it looks like Teletype or Typewriter
// The delay is in milliseconds.
// The text is automatically wrapped if longer than the display
// example:
//    tb_display_print_String("a new line\n");
//    tb_display_print_String("one\nand two lines\n");
//
//    char String_buffer[128];
//    snprintf(String_buffer, sizeof(String_buffer), "\nthe value: %i",value);
//    tb_display_print_String(String_buffer);
//
//    std::string msg;
//    msg = ss.str();
//    const char * c_msg = msg.c_str();
//    tb_display_print_String(c_msg);
// =============================================================
void tb_display_print_String(const char *s, int chr_delay){
  while(*s != 0){
    tb_display_print_char(*s++);
    if(chr_delay > 0)
      delay(chr_delay);
  }
}
