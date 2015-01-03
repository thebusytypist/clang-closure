set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangClosure
  SymbolsListing.cpp
  SymbolLocating.cpp
  RelationConstruction.cpp

  LINK_LIBS
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )

add_subdirectory(tool)
