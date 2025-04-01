add_llvm_pass_plugin(PermodPass
    # List your source files here.
    main.cpp
    Condition.cpp
    ConditionAnalysis.cpp
    Instrumentation.cpp
    LogManager.cpp
    LogParser.cpp
)

target_include_directories(PermodPass
    PRIVATE
    ${LLVM_INCLUDE_DIRS}
)

if(DEFINED KERNEL_MODE AND KERNEL_MODE)
    target_compile_definitions(PermodPass PRIVATE KERNEL_MODE=1)
endif()