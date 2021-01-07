function (load_git_properties prefix output_dir)

    # Load the current working branch
    execute_process(
        COMMAND git symbolic-ref -q --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (GIT_BRANCH STREQUAL "")
        execute_process(
            COMMAND git describe --tags --exact-match
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif ()

    # Get the latest abbreviated commit hash of the working branch
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if ("${GIT_BRANCH}" STREQUAL "")
        set(GIT_BRANCH "N/A")
    endif ()

    if ("${GIT_HASH}" STREQUAL "")
        set(GIT_HASH "N/A")
    endif ()

    string(TOUPPER "${prefix}" var_prefix)
    string(CONCAT VERSION_DATA "const char* ${var_prefix}_GIT_HASH = \"${GIT_HASH}\";\n"
        "const char* ${var_prefix}_GIT_BRANCH = \"${GIT_BRANCH}\";\n"
        )

    if (EXISTS ${output_dir}/${prefix}_version.cpp)
        file(READ ${output_dir}/${prefix}_version.cpp VERSION_DATA_)
    else ()
        set(VERSION_DATA_ "")
    endif ()

    if (NOT "${VERSION_DATA}" STREQUAL "${VERSION_DATA_}")
        file(WRITE ${output_dir}/${prefix}_version.cpp "${VERSION_DATA}")
    endif ()

endfunction ()
