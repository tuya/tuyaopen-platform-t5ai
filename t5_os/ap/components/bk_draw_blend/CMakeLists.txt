set(incs .)
set(srcs)

if (CONFIG_BLEND)
list(APPEND srcs
		draw_blend_driver.c
		bk_draw_blend.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common  multimedia bk_peripheral)




