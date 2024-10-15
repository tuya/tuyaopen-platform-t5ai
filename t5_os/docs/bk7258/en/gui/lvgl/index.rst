LVGL
========================

:link_to_translation:`zh_CN:[中文]`


1. Function overview
---------------------

	LVGL(Light and Versatile Graphics Library)is a free, open source and low-cost embedded graphics library with rich and powerful modular graphics components. It can be used to create beautiful interface for any MCU, MPU and display type and supports multiple input devices. It has become a popular graphics user interface library. LVGL official website address is https://lvgl.io.

2. Reference project
----------------------
	Currently avdk provides LVGL demo reference projects of various types and purposes. The code path of the reference project is ``\projects\lvgl\...`` and the compilation command is ``make bk7258 PROJECT=lvgl/xxx``. For specific project introduction, see `Reference projects <../../projects_work/lvgl/index.html>`_ 

3. Development steps
-------------------------------

Since BK7258 multimedia is executed on CPU1, the execution of lvgl needs to start CPU1 on CPU0 first, then send a mailbox message to CPU1 to execute the relevant code, and finally return to the execution state after the execution is completed on CPU1. This part of the execution code has been encapsulated into an independent interface. Please follow the following steps:

 - Set the value of the macro ``CONFIG_LV_COLOR_DEPTH`` according to the data type of the LCD, and open the corresponding macro configuration of the functions of the functions provided by LVGL as required.
 - Call the interface of ``media_app_lvgl_open()`` on CPU0 to start CPU1 and send a message to execute LVGL event.
 - Define the inferface of ``void lvgl_event_handle(media_mailbox_msg_t *msg)`` on CPU1 and execute the corresponding open or close code according to the ``msg->event`` parameter.
 - Obtain the LCD structure information according to ``msg->param`` and select the memory type of draw_buffer and configure the corresponding parameters of the ``lv_vnd_config_t`` sturcture.
 - Call the interface of ``lv_vendor_init(lv_vnd_config_t *config)`` to initialize the relevant configuration of LVGL.
 - Call the interface of ``lcd_display_open(lcd_open_t *config)`` to initialize and open the LCD display.
 - If the touch function exists, call the interface ``drv_tp_open(int hor_size, int ver_size, tp_mirror_type_t tp_mirror)`` to initialize the tp function.
 - Call the component interface provided by LVGL to draw the corresponding UI.
 - Call the interface of ``lv_vendor_start()`` to create an LVGL task and start scheduling execution.
 - Call the interface of ``msg_send_rsp_to_media_major_mailbox(media_mailbox_msg_t msg, uint32_t result, uint32_t dest)`` to return the execution status. The parameter of dest is APP_MODULE.


.. note::

	When draw_buffer of LVGL chooses to use sram memory, its size is 1/10 screen size according to official recommendations, which can save memory without losing frame rate.

4. Development description
---------------------------

 - Set draw_buffer to use psram memory or sram memory through ``CONFIG_LVGL_USE_PSRAM``.
 - Use the ``lv_vendor_disp_lock()`` and ``lv_vendor_disp_unlock()`` interfaces for code protection when calling the component interface provided by LVGL to draw the corresponding UI.
 - LVGL source code itself provides many additional third-party libraries, including file system interface, JPG decoder, BMP decoder, PNG decoder and GIF decoder etc. Due to the limitations of the system SRAM memory, these decoders can only decode small resolution pictures for display. For images with large resolution, PSRAM memory can be used for decoding.
 - SDK currently supports both fatfs and littlefs, which is based on bk_vfs's posix interface (refer to cli_vfs.c). When using it, open ``CONFIG_VFS`` and select littlefs or fatfs according to needs(``CONFIG_FATFS / CONFIG_LITTLEFS``). It can alse use another way that set the macro ``LV_USE_FS_FATFS`` in the lv_conf.h file to 1 to use the fatfs file system. 
 - When LVGL use PNG, JPG and GIF decoders to decode, you need to open the corresponding macros in the lv_conf.h file, which are ``LV_USE_PNG`` ``LV_USE_SJPG`` and ``LV_USE_GIF``.
 - There is no need to open the ``CONFIG_LVGL_USE_PSRAM`` macro when selecting PSRAM memory for decoding using PNG, JPG and GIF decoders.
 - Regarding the usage of LVGL rotation function, LVGL itself has its own software rotation function, which can be executed through the function ``lv_disp_set_rotation()``. The function pass the parameters of ``LV_DISP_ROT_90`` , ``LV_DISP_ROT_180`` or ``LV_DISP_ROT_270`` to achieve image rotation at 90, 180 and 270 degrees. However, due to the limitations of this feature, it is only applicable to screens with the same width and height. For the screen with different width and height, rotating 90 degrees and 270 degrees will cause abnormal image display. To solve this issue, and in the case where the display hardware cannot change the display direction, the SDK provides an additional rotation function, which can achieve image rotation display at 90 degrees, 180 degrees and 270 degrees. The specific implementation is as follows: When initializing LVGL by call the function ``lv_vendor_init(lv_vnd_config_t *config)`` in main function, the ``rotation`` parameter in the structure ``lv_vnd_config_t`` can be assigned by passing ``ROTATE_NONE``, ``ROTATE_90``, ``ROTATE_180``and ``ROTATE_270`` to indicate no rotation, rotation 90, rotation 180 and rotation 270 respectively. 
 - Regarding how to package resource files into bin file, select the corresponding packaging tool according to the selected file system type. For tools, please consult FAE.
 - LVGL’s project has been added to the automated partition list。If you want to reconfigure the partition size, just set the ``bk7258_partition.csv`` file in project directly and pay attention to some alignment requirements. 

