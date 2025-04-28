add_library(Permod_rt STATIC
    rtlib.c
)

if(DEFINED USER_MODE AND USER_MODE)
    target_compile_definitions(Permod_rt PRIVATE USER_MODE=1)
elseif(DEFINED KERNEL_MODE AND KERNEL_MODE)
    target_compile_definitions(Permod_rt PRIVATE KERNEL_MODE=1)
endif()