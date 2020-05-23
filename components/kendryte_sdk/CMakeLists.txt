
################# Add include #################
list(APPEND ADD_INCLUDE "include"
                        "kendryte-freertos-sdk/lib/arch/include"
                        "kendryte-freertos-sdk/lib/bsp/include"
                        "kendryte-freertos-sdk/lib/bsp/syscalls"
                        "kendryte-freertos-sdk/lib/drivers/include"
                        "kendryte-freertos-sdk/lib/drivers/include/misc/ws2812b"
                        "kendryte-freertos-sdk/lib/drivers/include/network"
                        "kendryte-freertos-sdk/lib/drivers/include/storage"
                        "kendryte-freertos-sdk/lib/freertos/include"
                        "kendryte-freertos-sdk/lib/freertos/include/kernel"
                        "kendryte-freertos-sdk/lib/freertos/kernel"
                        "kendryte-freertos-sdk/lib/freertos/portable"
                        "kendryte-freertos-sdk/lib/hal/include"
                        "kendryte-freertos-sdk/lib/posix"
                        "kendryte-freertos-sdk/lib/posix/include"
                        "kendryte-freertos-sdk/lib/posix/include/sys"
                        "kendryte-freertos-sdk/lib/utils/include"
                        "kendryte-freertos-sdk/third_party"
                        "kendryte-freertos-sdk/third_party/fatfs"
                        "kendryte-freertos-sdk/third_party/fatfs/source"
                        "kendryte-freertos-sdk/third_party/gsl"
                        "kendryte-freertos-sdk/third_party/lwip"
                        "kendryte-freertos-sdk/third_party/lwip/src/include"
    )
# list(APPEND ADD_PRIVATE_INCLUDE "include_private")
###############################################

############## Add source files ###############
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/bsp")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/bsp/config")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/bsp/device")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/bsp/syscalls")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/drivers/src/misc/ws2812b")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/drivers/src/network")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/drivers/src/storage")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/freertos")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/freertos/kernel")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/freertos/kernel/network")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/freertos/kernel/storage")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/freertos/portable")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/hal")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/lib/posix")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/third_party/fatfs/source")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/third_party/lwip/source/api")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/third_party/lwip/source/arch")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/third_party/lwip/source/core")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/third_party/lwip/source/core/ipv4")
append_srcs_dir(ADD_SRCS "kendryte-freertos-sdk/third_party/lwip/source/core/ipv6")

# append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase")
# FILE(GLOB_RECURSE NNCASE_SRC
#     "kendryte-standalone-sdk/lib/nncase/*.c"
#     "kendryte-standalone-sdk/lib/nncase/*.cpp"
#     )

append_srcs_dir(ADD_SRCS "src")
set(ADD_ASM_SRCS "kendryte-freertos-sdk/lib/bsp/crt.S")

# list(APPEND ADD_SRCS  ${ADD_ASM_SRCS} ${NNCASE_SRC})
list(APPEND ADD_SRCS  ${ADD_ASM_SRCS} )

# list(REMOVE_ITEM COMPONENT_SRCS "src/test.c")

SET_PROPERTY(SOURCE ${ADD_ASM_SRCS} PROPERTY LANGUAGE C)
SET_SOURCE_FILES_PROPERTIES(${ADD_ASM_SRCS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp -D __riscv64")
###############################################

###### Add required/dependent components ######
# list(APPEND ADD_REQUIREMENTS stdc++ m)
list(APPEND ADD_REQUIREMENTS gcc m c atomic)
###############################################

############ Add static libs ##################
# list(APPEND ADD_STATIC_LIB "libs/lib_mic.a"
#                            )
###############################################

list(APPEND ADD_DEFINITIONS -DCONFIG_LOG_LEVEL=${CONFIG_SDK_LOG_LEVEL}
                            -DNNCASE_TARGET=k210
                            -DTCB_SPAN_NO_EXCEPTIONS
                            -DTCB_SPAN_NO_CONTRACT_CHECKING
                            -DCONFIG_LOG_ENABLE=1
                            -DCONFIG_LOG_COLORS=1
                            -DLOG_KERNEL=1
                            -D__riscv64
                            )

register_component()

