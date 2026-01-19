set(srcs)

if (CONFIG_ADK_G722_ENCODER)

list(APPEND srcs
        g722_encoder.c
    )

if (CONFIG_ADK_G722_ENCODER_TEST)
    list(APPEND srcs
        g722_encoder_test.c
    )
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)