
#define TAG "lcd_rgb"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
	unsigned int image_addr;  /**< normally flash jpeg  addr */
	unsigned int img_length;
	unsigned short x_start; 
	unsigned short y_start;
	unsigned short x_end;
	unsigned short y_end;
	unsigned char  display_type;
} lcd_display_t;

typedef enum{
	LCD_RGB_DISABLE,
	LCD_RGB_ENABLE,
}lcd_status_t;

typedef struct {
	unsigned int device_ppi;			/**< lcd open by ppi */
	char * device_name; 			/**< lcd open by lcd Driver IC name */
} lcd_rgb_project_t;


void lcd_rgb_display_rand_color(void);


