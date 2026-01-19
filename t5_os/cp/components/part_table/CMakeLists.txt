set(incs
	$ENV{PARTITIONS_DIR}
)
set(srcs)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_cli bk_common bk_log)
