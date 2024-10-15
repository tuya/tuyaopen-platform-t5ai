armino_build_get_property(project_dir PROJECT_DIR)
set(incs
	.
	)

set(srcs
	pbkdf2_sha256.c
	)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
