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

static const char *simple_c = R"(
int inc(int x) {
  return x + 1;
}

struct MyStruct {
  int value;
};
)";

TEST(SymbolListingTest, CountAndType) {
  closure::SymbolsList symbols;
  SymbolsListingTestAction *action = new SymbolsListingTestAction(symbols);
  bool r = runToolOnCode(action, simple_c, "simple.c");
  EXPECT_TRUE(r);
  EXPECT_EQ(2, symbols.getCount());

  StringRef e[] = {"function", "record"};

  for (size_t i = 0, count = symbols.getCount(); i != count; ++i) {
    EXPECT_TRUE(symbols.getType(i) == e[i]);
  }
}

static const char *structtypedef_c = R"(
typedef struct MyStruct {
  int value;
} MyStructT;
)";

TEST(SymbolListingTest, StructTypedef) {
  closure::SymbolsList symbols;
  SymbolsListingTestAction *action = new SymbolsListingTestAction(symbols);
  bool r = runToolOnCode(action, structtypedef_c, "structtypedef.c");
  EXPECT_TRUE(r);
  EXPECT_EQ(1, symbols.getCount());
  EXPECT_TRUE(symbols.getType(0) == "record");
}
