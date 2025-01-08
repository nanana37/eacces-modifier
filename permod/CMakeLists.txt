include_directories(./include)

add_llvm_pass_plugin(PermodPass
    # List your source files here.
    Permod.cpp
    Condition.cpp
    ConditionAnalysis.cpp
    Instrumentation.cpp
    Utils.cpp
)
