cmake_policy(SET CMP0067 NEW)
include(CheckCXXSourceCompiles)

function(check_features)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    check_cxx_source_compiles(
        "#include <thread>

        int main()
        {
            std::jthread t{[](){}};
            return 0;
        }"
        has_jthread
    )

    check_cxx_source_compiles(
        "#include <functional>

        int main()
        {
            std::move_only_function<void()> f{[](){}};
            return 0;
        }"
        has_move_only_function
    )

    if (NOT has_jthread)
        set(saucer_polyfill_thread ON PARENT_SCOPE)
    endif()

    if (NOT has_move_only_function)
        set(saucer_polyfill_functional ON PARENT_SCOPE)
    endif()
endfunction()

