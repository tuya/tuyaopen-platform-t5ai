set(incs .)
set(srcs)

if (CONFIG_LCD_DMA2D_BLEND OR CONFIG_LCD_FONT_BLEND)
list(APPEND srcs
		draw_blend.c
		string.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common  multimedia bk_peripheral)




