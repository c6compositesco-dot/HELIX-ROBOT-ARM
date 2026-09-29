file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/probe.cpp" "#include \"helix/motion/cubic_segment.hpp\"\nint main() { return 0; }\n")
foreach(flag -ffast-math -ffinite-math-only)
  execute_process(COMMAND "${CXX}" -std=c++17 "${flag}" "-I${INCLUDES}"
    -c "${WORK}/probe.cpp" -o "${WORK}/probe.o"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(result EQUAL 0)
    message(FATAL_ERROR "Unsafe floating-point mode unexpectedly compiled: ${flag}")
  endif()
  if(NOT error MATCHES "Continuous interval validation requires strict floating-point semantics")
    message(FATAL_ERROR "Unexpected compile failure for ${flag}: ${output} ${error}")
  endif()
endforeach()
message(STATUS "Unsafe floating-point modes rejected with the intended diagnostic")
