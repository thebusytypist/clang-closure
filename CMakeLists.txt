set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_executable(clang-closure
  src/main.cpp
  )

target_link_libraries(clang-closure
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )
