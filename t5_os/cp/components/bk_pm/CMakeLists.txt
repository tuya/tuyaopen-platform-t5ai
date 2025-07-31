armino_build_get_property(target ARMINO_SOC)
set(incs include)

set(srcs
	pm.c
	pm_model.c
	low_vol.c
	pm_misc.c
	)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
