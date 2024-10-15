if (CONFIG_SOC_BK7258)
set(incs include)
set(srcs "")

if (CONFIG_MEDIA_DISPLAY_SERVICE)
	list(APPEND srcs
		src/lcd_display_service.c
	)
endif()

if (CONFIG_LCD_FONT_BLEND OR CONFIG_LCD_DMA2D_BLEND)
	list(APPEND srcs
		src/lcd_draw_blend.c
	)
endif()

if (CONFIG_LCD_QSPI)
	list(APPEND srcs
		src/lcd_qspi_display_service.c
	)
endif()

if (CONFIG_LCD_SPI_DISPLAY)
	list(APPEND srcs
		src/lcd_spi_display_service.c
	)
endif()

set(priv_req bk_common unity include )
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}" multimedia bk_draw_blend
)

