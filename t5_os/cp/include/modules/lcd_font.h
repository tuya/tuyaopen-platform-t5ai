
#ifndef _LCD_FONT_H
#define _LCD_FONT_H


#define  FILTER_Y 0x20

#define LCD_FONT_WIDTH  480
#define LCD_FONT_HEIGHT 36

#define UI_COLOR_BLACK    0x0000
#define UI_COLOR_WHITE    0xFFFF
#define UI_COLOR_RED      (0x1F)
#define UI_COLOR_GREEN    (0x3F<<5)
#define UI_COLOR_BLUE     (0x1F<<11)
#define UI_DIS_Y_MAX        20

#define UI_COLOR_YUYV_BLACK    0x00800080  /* Y U V 00 80 80*/
#define UI_COLOR_YUYV_WHITE    0xFF80FF80  /* Y U V FF 80 80*/
#define UI_COLOR_VUYY_BLACK    0x80800000
#define UI_COLOR_VUYY_WHITE    0x8080FFFF

typedef enum {
	FONT_RGB565 = 0,       /**< RGB565 DMA2D color mode   */
	FONT_YUYV,
	FONT_VUYY,
	FONT_RGB565_LE,
} font_format_t;

typedef struct
{
    unsigned char *buf;
    unsigned short y0;
    unsigned short y1;
    unsigned short num;
    unsigned short y_buf[UI_DIS_Y_MAX];
}ui_display_info_struct;

typedef struct
{
    unsigned int offset;
    int x0;//unsigned short x0;
    int y0;//unsigned short y0;
    int x1;//unsigned short x1;
    int y1;//unsigned short y1;
}ui_part_info_struct;


#ifndef GUI_CONST_STORAGE
  #define GUI_CONST_STORAGE const
#endif

typedef struct
{
	const unsigned short value;
	const unsigned char bit_point; //every point user bit.
	const unsigned char x_size;
	const unsigned char y_size;
	const signed char x_pos;
	const signed char y_pos;
	const unsigned char width;
	const unsigned char *const data;
}gui_font_digit_struct;


typedef enum {
	FONT_WHITE = 0xFFFF,
	FONT_BLACK = 0,

}font_colot_t;

typedef struct
{
	ui_display_info_struct info;
	const gui_font_digit_struct *digit_info;
	const char * s;
	uint32_t font_color;
	font_format_t font_fmt;
	int width;
	int height;
	int x_pos;
	int y_pos;
}font_t;

void lcd_draw_font(font_t *font);

#endif //_FONT_H

