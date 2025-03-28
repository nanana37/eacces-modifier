add_library(Permod_rt STATIC
    rtlib.c
)

if(DEFINED KERNEL_MODE AND KERNEL_MODE)
    target_compile_definitions(PermodPass PRIVATE KERNEL_MODE=1)
endif()