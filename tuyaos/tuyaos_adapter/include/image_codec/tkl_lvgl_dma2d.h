
#ifndef TKL_LVGL_ADAPTER_H
#define TKL_LVGL_ADAPTER_H

#include "tuya_cloud_types.h"
#include "tkl_display.h"
#include "tuya_app_gui_core_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
/** DMA2D_Output_Color_Mode , used for fillfmt / memcpy dst fmt/ blend output fmt*/
typedef enum {
	COLOR_OUTPUT_RGB565 = 0,
	COLOR_OUTPUT_RGB888,
	COLOR_OUTPUT_ARGB888,
} ty_out_color_mode_t;

/** DMA2D_Input_Color_Mode, the members in order */
typedef enum {
	COLOR_INPUT_RGB565 = 0,
	COLOR_INPUT_RGB888,
	COLOR_INPUT_ARGB888,
} ty_input_color_mode_t;

typedef enum {
	PIXEL_TWO_BYTES   = 2,
	PIXEL_THREE_BYTES = 3,
	PIXEL_FOUR_BYTES  = 4,
}ty_color_bytes_t;

typedef enum {
	RB_REGULAR = 0x0,    /**< Select regular mode (RGB or ARGB) */
	RB_SWAP,             /**< Select swap mode (BGR or ABGR) * = 0*/
}ty_red_blue_swap_t;

typedef enum {
	FMT_UNKNOW,         /**< unknow image format */
	FMT_RGB565,
	FMT_RGB666,
	FMT_RGB888,
	FMT_ARGB888,
} ty_pixel_format_t;

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
	//ty_dma2d_mode_t    mode; 
	void * input_addr;               /**< The image memcpy or pixel convert src addr */
	void * output_addr;              /**< The mage memcpy or pixel convert dst addr */
	uint16_t src_frame_width;        /**< memcpy or pfc src image width */
	uint16_t src_frame_height;       /**< imemcpy or pfc src image height  */
	uint16_t src_frame_xpos;         /**< src img start copy/pfc x pos*/
	uint16_t src_frame_ypos;         /**< src img start copy/pfc y pos*/

	uint16_t dst_frame_width;         /**< memcpy to dst image, the dst image width */
	uint16_t dst_frame_height;        /**< memcpy to dst image, the dst image height   */
	uint16_t dst_frame_xpos;          /**< dma2d fill x pos based on frame_xsize */
	uint16_t dst_frame_ypos;          /**< dma2d fill y pos based on frame_ysize */
	uint16_t dma2d_width;              /**< dma2d memcpy or pfc width */
	uint16_t dma2d_height;               /**< dma2d memcpy or pfc height */

	ty_input_color_mode_t input_color_mode;  /**< The pixel convert src color mode */
	ty_out_color_mode_t output_color_mode;   /**< The pixel convert dst color mode */
	ty_color_bytes_t src_pixel_byte;
	ty_color_bytes_t dst_pixel_byte;
	uint8_t input_alpha;                /**< src data alpha, depend on alpha_mode */
	uint8_t output_alpha;                /**< dst data alpha,depend on alpha_mode */
	ty_red_blue_swap_t input_red_blue_swap;        /**< src img red blue swap, select DMA2D_RB_SWAP or  DMA2D_RB_REGULAR */
	ty_red_blue_swap_t output_red_blue_swap;        /**< src img red blue swap, select DMA2D_RB_SWAP or  DMA2D_RB_REGULAR */
}tkl_dma2d_memcpy_pfc_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
VOID tkl_lvgl_dma2d_init(VOID);

VOID tkl_lvgl_dma2d_deinit(VOID);

VOID tkl_lvgl_dma2d_memcpy(tkl_dma2d_memcpy_pfc_t *pixel_info);

BOOL_T tkl_lvgl_dma2d_is_busy(void);

VOID tkl_lvgl_dma2d_wait_transfer_finish(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*TKL_LVGL_ADAPTER_H*/
