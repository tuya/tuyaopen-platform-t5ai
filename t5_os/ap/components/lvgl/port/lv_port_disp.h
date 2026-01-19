/**
 * @file lv_port_disp_templ.h
 *
 */

 /*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define DISPLAY_BUFFER_1    1
#define DISPLAY_BUFFER_2    2
#if CONFIG_LVGL_USE_TRIPLE_BUFFERS
#define DISPLAY_BUFFER_3    3
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void);

int lv_port_disp_init();

void lv_port_disp_deinit(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*Disable/Enable content*/

