#include "SymbolsListing.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

using namespace clang;
using namespace clang::tooling;

class SymbolsListingTestAction : public ASTFrontendAction {
public:
  SymbolsListingTestAction(closure::SymbolsList &symbols)
    : mSymbols(symbols) {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    return llvm::make_unique<closure::SymbolsListingConsumer>(mSymbols);
  }

  closure::SymbolsList &mSymbols;
};

const char *simple_c = R"(
int inc(int x) {
  return x + 1;
}

struct MyStruct {
  int value;
};
)";

TEST(SymbolListingTest, CheckCount) {
  closure::SymbolsList symbols;
  SymbolsListingTestAction *action = new SymbolsListingTestAction(symbols);
  bool r = runToolOnCode(action, simple_c, "simple.c");
  EXPECT_TRUE(r);
  EXPECT_EQ(2, symbols.getCount());
}
