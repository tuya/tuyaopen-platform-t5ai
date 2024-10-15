/**
 * @file lv_port_disp_templ.c
 *
 */

 /*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp.h"
#include "lv_vendor.h"
#include <modules/image_scale.h>


#define TAG "LVGL_DISP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void *rotate_buffer = NULL;
extern lv_vnd_config_t vendor_config;
extern media_debug_t *media_debug;

extern void lv_memcpy_one_line(void *dest_buf, const void *src_buf, uint32_t point_num);
extern void lcd_driver_display_frame_with_gui(void *buffer, int width, int height);
extern void dma2d_memcpy_psram_wait_last_transform_is_finish(void);
extern void dma2d_memcpy_psram_wait_last_transform(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline);


int lv_port_disp_init(void)
{
    int ret = BK_OK;

    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
    //static lv_disp_draw_buf_t draw_buf_dsc_1;
    //static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
    //lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 2) */
    static lv_disp_draw_buf_t draw_buf_dsc_2;

    LOGI("LVGL addr1:%x, addr2:%x, pixel size:%d, fb1:%x, fb2:%x\r\n", vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2,
                                        vendor_config.draw_pixel_size, vendor_config.frame_buf_1, vendor_config.frame_buf_2);

    lv_disp_draw_buf_init(&draw_buf_dsc_2, vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2, vendor_config.draw_pixel_size);   /*Initialize the display buffer*/

    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
    //static lv_disp_draw_buf_t draw_buf_dsc_3;
    //static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
    //static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*An other screen sized buffer*/
    //lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2, MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    if ((vendor_config.rotation == ROTATE_90) || (vendor_config.rotation == ROTATE_270)) {
        disp_drv.hor_res = vendor_config.lcd_ver_res;
        disp_drv.ver_res = vendor_config.lcd_hor_res;
    } else {
        disp_drv.hor_res = vendor_config.lcd_hor_res;
        disp_drv.ver_res = vendor_config.lcd_ver_res;
    }

#if LVGL_USE_PSRAM
#if LVGL_USE_DIRECT_MODE
    disp_drv.full_refresh = 0;
    disp_drv.direct_mode = 1;
#else
    disp_drv.full_refresh = 1;
    disp_drv.direct_mode = 0;
#endif
#endif

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_2;

    if (vendor_config.rotation == ROTATE_180) {
        disp_drv.sw_rotate = 1;
        disp_drv.rotated = LV_DISP_ROT_180;
    }

    if ((vendor_config.rotation == ROTATE_90) || (vendor_config.rotation == ROTATE_270)) {
        rotate_buffer = os_malloc(vendor_config.draw_pixel_size * sizeof(lv_color_t));
        if (rotate_buffer == NULL) {
            LOGE("lvgl rotate buffer malloc fail!\n");
            return BK_FAIL;
        }
    }

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);

    return ret;
}

void lv_port_disp_deinit(void)
{
    if ((vendor_config.rotation == ROTATE_90) || (vendor_config.rotation == ROTATE_270)) {
        if (rotate_buffer) {
            os_free(rotate_buffer);
        }
    }

    lv_disp_remove(lv_disp_get_default());
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static lv_color_t *update_dual_buffer_with_direct_mode(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *colour_p)
{
    lv_disp_t* disp = _lv_refr_get_disp_refreshing();
    lv_coord_t y, hres = lv_disp_get_hor_res(disp);
    uint16_t i;
    lv_color_t *buf_cpy;
    lv_coord_t w;
    lv_area_t *inv_area = NULL;
    int offset = 0;

    if (colour_p == disp_drv->draw_buf->buf1) {
        buf_cpy = disp_drv->draw_buf->buf2;
    } else {
        buf_cpy = disp_drv->draw_buf->buf1;
    }

    LOGD("inv_p:%d, x1:%d, y1:%d, x2:%d, y2:%d, buf_cpy:%x\r\n", disp->inv_p, disp->inv_areas[0].x1, disp->inv_areas[0].y1, disp->inv_areas[0].x2, disp->inv_areas[0].y2, buf_cpy);
    for(i = 0; i < disp->inv_p; i++) {
        if(disp->inv_area_joined[i])
            continue;  /* Only copy areas which aren't part of another area */

        inv_area = &disp->inv_areas[i];
        w = lv_area_get_width(inv_area);

        offset = inv_area->y1 * hres + inv_area->x1;

        for(y = inv_area->y1; y <= inv_area->y2 && y < disp_drv->ver_res; y++) {
            lv_memcpy_one_line(buf_cpy + offset, colour_p + offset, w);
            offset += hres;
        }
    }

    return buf_cpy;
}

static void lv_image_rotate90_anticlockwise(void *dst, void *src, lv_coord_t width, lv_coord_t height)
{
#if (LV_COLOR_DEPTH == 16)
    rgb565_rotate_degree270((unsigned char *)src, (unsigned char *)dst, width, height);
#elif (LV_COLOR_DEPTH == 32)
    argb8888_rotate_degree270((unsigned char *)src, (unsigned char *)dst, width, height);
#endif
}

static void lv_image_rotate90_clockwise(void *dst, void *src, lv_coord_t width, lv_coord_t height)
{
#if (LV_COLOR_DEPTH == 16)
    rgb565_rotate_degree90((unsigned char *)src, (unsigned char *)dst, width, height);
#elif (LV_COLOR_DEPTH == 32)
    argb8888_rotate_degree90((unsigned char *)src, (unsigned char *)dst, width, height);
#endif
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
#if (!LVGL_USE_PSRAM)
    if(disp_flush_enabled) {
        lv_color_t *color_ptr = color_p;
        static lv_color_t *disp_buf = NULL;
        static u8 disp_buff_index = DISPLAY_BUFFER_DEF;
        lv_color_t *disp_buf1 = vendor_config.frame_buf_1;
        lv_color_t *disp_buf2 = vendor_config.frame_buf_2;

        lv_coord_t lv_hor = LV_HOR_RES;
        lv_coord_t lv_ver = LV_VER_RES;

        int y = 0;
        int offset = 0;
        int width = lv_area_get_width(area);

        if (disp_buf2 != NULL) {
            if(DISPLAY_BUFFER_1 == disp_buff_index) {
                dma2d_memcpy_psram_wait_last_transform_is_finish();
                disp_buf = disp_buf2;
            } else if (DISPLAY_BUFFER_2 == disp_buff_index) {
                dma2d_memcpy_psram_wait_last_transform_is_finish();
                disp_buf = disp_buf1;
            }
            else //first display
            {
                dma2d_memcpy_psram_wait_last_transform_is_finish();
                disp_buf = disp_buf1;
            }
        } else {
            disp_buf = disp_buf1;
        }

        if ((ROTATE_NONE == vendor_config.rotation) || (ROTATE_180 == vendor_config.rotation)) {
			offset = area->y1 * lv_hor + area->x1;
			for (y = area->y1; y <= area->y2 && y < disp_drv->ver_res; y++) {
				lv_memcpy_one_line(disp_buf + offset, color_ptr, width);
				offset += lv_hor;
				color_ptr += width;
			}
        } else if (ROTATE_270 == vendor_config.rotation) {
            lv_coord_t height = lv_area_get_height(area);
            lv_color_t *dst_ptr = rotate_buffer;

            lv_image_rotate90_clockwise(rotate_buffer, color_p, width, height);

            offset = area->x1 * lv_ver + (lv_ver - area->y2 - 1);
            for (y = area->x1; y <= area->x2 && y < disp_drv->hor_res; y++) {
                lv_memcpy_one_line(disp_buf + offset, dst_ptr, height);
                offset += lv_ver;
                dst_ptr += height;
            }
        } else if (ROTATE_90 == vendor_config.rotation) {
            lv_coord_t width = lv_area_get_width(area);
            lv_coord_t height = lv_area_get_height(area);
            lv_color_t *dst_ptr = rotate_buffer;

            lv_image_rotate90_anticlockwise(rotate_buffer, color_p, width, height);

            offset = (lv_hor - (area->x2 + 1)) * lv_ver + area->y1;
            for (y = lv_hor - (area->x2 + 1); y <= lv_hor - (area->x1 + 1) && y < disp_drv->hor_res; y++) {
                lv_memcpy_one_line(disp_buf + offset, dst_ptr, height);
                offset += lv_ver;
                dst_ptr += height;
            }
        }

        if (lv_disp_flush_is_last(disp_drv)) {
            media_debug->lvgl_draw++;
            if (ROTATE_90 == vendor_config.rotation || ROTATE_270 == vendor_config.rotation) {
                lcd_driver_display_frame_with_gui(disp_buf, lv_ver, lv_hor);
            } else {
                lcd_driver_display_frame_with_gui(disp_buf, lv_hor, lv_ver);
            }

            if(disp_buf2) {
                if (DISPLAY_BUFFER_1 == disp_buff_index) {
                    dma2d_memcpy_psram_wait_last_transform(disp_buf, disp_buf1, lv_hor, lv_ver, 0, 0);
                    disp_buff_index = 2;
                } else if (DISPLAY_BUFFER_2 == disp_buff_index) {
                    dma2d_memcpy_psram_wait_last_transform(disp_buf, disp_buf2, lv_hor, lv_ver, 0, 0);
                    disp_buff_index = 1;
                }
                else //first display
                {
                    dma2d_memcpy_psram_wait_last_transform(disp_buf, disp_buf2, lv_hor, lv_ver, 0, 0);
                    disp_buff_index = 2;
                }
            }
        }
    }

    lv_disp_flush_ready(disp_drv);
#else
    if (disp_flush_enabled) {
        #if LVGL_USE_DIRECT_MODE
        static int first_call = 1;
        if (first_call) {
            first_call = 0;
            lcd_driver_display_frame_with_gui(color_p, lv_area_get_width(area), lv_area_get_height(area));
        }

        if(lv_disp_flush_is_last(disp_drv)) {
            lcd_driver_display_frame_with_gui(color_p, lv_area_get_width(area), lv_area_get_height(area));
            update_dual_buffer_with_direct_mode(disp_drv, area, (lv_color_t *)color_p);
        }
        #else
        lcd_driver_display_frame_with_gui(color_p, lv_area_get_width(area), lv_area_get_height(area));
        #endif
    }

    lv_disp_flush_ready(disp_drv);
#endif
}


/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}


#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
