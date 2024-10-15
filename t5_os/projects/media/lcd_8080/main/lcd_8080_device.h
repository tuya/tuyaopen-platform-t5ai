

#define TAG "lcd_8080"

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

extern void lcd_8080_display_rand_color(void);


