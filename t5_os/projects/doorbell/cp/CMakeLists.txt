set(incs .
         customer_msg
         doorbell_service
    )
set(srcs
    cp_main.c
    customer_msg/customer_msg.c
    doorbell_service/doorbell_core.c
    )

set(prev_req controller_if lwip_intf_v2_1 bt)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${prev_req}")
