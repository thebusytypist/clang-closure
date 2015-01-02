#include "SymbolLocating.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"
#include <string>

using namespace clang;
using namespace clang::tooling;

class SymbolLocatingTestAction : public ASTFrontendAction {
public:
  SymbolLocatingTestAction(int index, std::string &signature)
    : mIndex(index), mSignature(signature) {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    return llvm::make_unique<closure::SymbolLocatingConsumer>(
      mSignature, mIndex);
  }

  int mIndex;
  std::string &mSignature;
};

static const char *simple_c = R"(
int inc(int x) {
  return x + 1;
}

struct MyStruct {
  int value;
};
)";

TEST(SymbolLocatingTest, Simple) {
  std::string signature;
  SymbolLocatingTestAction *action = new SymbolLocatingTestAction(
    0, signature);
  bool r = runToolOnCode(action, simple_c, "simple.c");
  EXPECT_TRUE(r);
  EXPECT_TRUE(signature != "");
}
