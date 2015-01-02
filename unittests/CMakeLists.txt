set(LLVM_LINK_COMPONENTS
  Support
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  )

add_extra_unittest(ClangClosureTests
  SymbolsListingTest.cpp
  )

target_link_libraries(ClangClosureTests
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  clangClosure
  )
