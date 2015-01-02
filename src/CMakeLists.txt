set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangClosure
  SymbolsListing.cpp

  LINK_LIBS
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )

add_subdirectory(tool)
