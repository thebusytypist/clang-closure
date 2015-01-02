include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_clang_executable(clang-closure
    ClangClosureMain.cpp)

target_link_libraries(clang-closure
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  clangClosure
  )
