set(LLVM_LINK_COMPONENTS
  Support
  )

add_extra_unittest(ClangClosureTests
  SymbolsListingTest.cpp
  )

target_link_libraries(ClangClosureTests
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )
