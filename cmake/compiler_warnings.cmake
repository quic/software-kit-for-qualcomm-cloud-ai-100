
cmake_minimum_required (VERSION 3.24)

set(WARNINGS_ENABLED
    -Werror
    -Wall
    -Wextra
    -Wmissing-field-initializers
    -Wunused
    -Wunused-variable
    -Wunused-parameter
    -Wdeprecated-declarations
    -Wno-array-bounds
    -Wsign-compare
    -Wunused-but-set-variable
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(WARNINGS_ENABLED ${WARNINGS_ENABLED} "-Wunused-but-set-variable -Wno-old-style-definition")
else()
    #set(WARNINGS_ENABLED ${WARNINGS_ENABLED} "-Wno-infinite-recursion")
endif()


function(set_target_compiler_warnings project_name)
    target_compile_options(${project_name} PRIVATE ${WARNINGS_ENABLED})
endfunction()

function(set_compiler_warnings)
    add_compile_options(${WARNINGS_ENABLED})
endfunction()
