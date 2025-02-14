function(saucer_check_cxx_compiles CODE OUTPUT)
  set(FILE_PATH "${CMAKE_BINARY_DIR}/saucer_polyfill/${OUTPUT}.cpp")
  file(WRITE "${FILE_PATH}" "${CODE}")

  try_compile(
     ${OUTPUT}
     "${CMAKE_BINARY_DIR}"
     "${FILE_PATH}"
     CXX_STANDARD 23 
     CXX_STANDARD_REQUIRED ON
  )

  return(PROPAGATE ${OUTPUT})
endfunction()

function(saucer_determine_polyfills RESULTS)
  saucer_check_cxx_compiles(
    "#include <thread>

    int main()
    {
      std::jthread{[]{}};
      return 0;
    }"
    has_jthread
  )

  saucer_check_cxx_compiles(
    "#include <functional>

    int main()
    {
      std::move_only_function<void()>{[]{}};
      return 0;
    }"
    has_move_only_function
  )

  saucer_check_cxx_compiles(
    "#include <expected>

    int main()
    {
      std::expected<int, int>{};
      return 0;
    }"
    has_expected
  )

  set(${RESULTS} "")

  if (NOT has_jthread)
    list(APPEND ${RESULTS} "jthread")
  endif()

  if (NOT has_move_only_function)
    list(APPEND ${RESULTS} "functional")
  endif()

  if (NOT has_expected)
    list(APPEND ${RESULTS} "expected")
  endif()

  return(PROPAGATE ${RESULTS})
endfunction()

