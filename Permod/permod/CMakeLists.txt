add_llvm_pass_plugin(PermodPass
    # List your source files here.
    main.cpp
    Condition.cpp
    ConditionAnalysis.cpp
    Instrumentation.cpp
    LogManager.cpp
)

target_include_directories(PermodPass
    PRIVATE
    ./include
    ${LLVM_INCLUDE_DIRS}
)

if(DEFINED USER_MODE AND USER_MODE)
    target_compile_definitions(PermodPass PRIVATE USER_MODE=1)
endif()