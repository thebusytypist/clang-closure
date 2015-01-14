#include "RelationConstruction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

using namespace clang;
using namespace clang::tooling;

class RelationConstructionTestAction : public ASTFrontendAction {
public:
  RelationConstructionTestAction(
    closure::FilesSetType &s,
    closure::FilesMapType &m,
    closure::SymbolsMapType &symbols)
      : mFilesSet(s), mFilesMap(m), mSymbols(symbols) {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    Preprocessor &pp = CI.getPreprocessor();
    pp.addPPCallbacks(llvm::make_unique<closure::InclusionPPCallbacks>(
      CI.getSourceManager(),
      mFilesSet,
      mFilesMap));
    return llvm::make_unique<closure::RelationConstructionConsumer>(mSymbols);
  }

  closure::FilesSetType &mFilesSet;
  closure::FilesMapType &mFilesMap;
  closure::SymbolsMapType &mSymbols;
};

static const char *simplemain_c = R"(
#include "simpleheader.h"
#include <stdio.h>
)";

static const char *simpleheader_h = R"(
#include "simpleheader2.h"
)";

TEST(RelationConstructionTest, Simple) {
  closure::FilesSetType filesSet;
  closure::FilesMapType filesMap;
  closure::SymbolsMapType symbols;
  
  FileContentMappings contents;
  contents.push_back(std::make_pair("simpleheader.h", simpleheader_h));
  contents.push_back(std::make_pair("simpleheader2.h", ""));

  RelationConstructionTestAction *action = new RelationConstructionTestAction(
    filesSet, filesMap, symbols);

  EXPECT_TRUE(runToolOnCodeWithArgs(action,
    simplemain_c,
    std::vector<std::string>(),
    "simplemain.c",
    contents));
}

class InclusionTestAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    return llvm::make_unique<ASTConsumer>();
  }
};

TEST(InclusionTest, Debug) {
  InclusionTestAction *action = new InclusionTestAction;
  FileContentMappings contents;
  contents.push_back(std::make_pair("simpleheader.h", simpleheader_h));
  contents.push_back(std::make_pair("simpleheader2.h", ""));
  runToolOnCodeWithArgs(action,
    simplemain_c,
    std::vector<std::string>(),
    "simplemain.c",
    contents);
}
