if(MSVC)
  # suppress D9025
  string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
endif()
