set(incs
	.
	utils)

if (CONFIG_WPA3 AND NOT CONFIG_USE_MBEDTLS)
list(APPEND srcs
	"wolfcrypt/src/ecc.c"
	"wolfcrypt/src/hmac.c"
	"wolfcrypt/src/md5.c"
	"wolfcrypt/src/memory.c"
	"wolfcrypt/src/random.c"
	"wolfcrypt/src/sha.c"
	"wolfcrypt/src/sha256.c"
	"wolfcrypt/src/sha512.c"
	"wolfcrypt/src/tfm.c"
	"wolfcrypt/src/wolfmath.c"
	)
endif()
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_wifi wpa_supplicant-2.10)
