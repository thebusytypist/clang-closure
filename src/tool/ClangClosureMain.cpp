#include "SymbolsListing.h"
#include "SymbolLocating.h"
#include "RelationConstruction.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

//===----------------------------------------------------------------------===//
// Command line options
//===----------------------------------------------------------------------===//

static llvm::cl::OptionCategory ClangClosureCategory("clang-closure options");

llvm::cl::opt<bool> ListSymbols("list-symbols",
  llvm::cl::desc("List symbols in main file"),
  llvm::cl::cat(ClangClosureCategory));

llvm::cl::opt<int> SelectedSymbolIndex("symbol",
  llvm::cl::desc("Select symbol by its ID"),
  llvm::cl::cat(ClangClosureCategory));

llvm::cl::opt<std::string> FileOfSymbol("file",
  llvm::cl::desc("The file where the selected symbol resides"),
  llvm::cl::cat(ClangClosureCategory));

//===----------------------------------------------------------------------===//
// Global variables
//===----------------------------------------------------------------------===//
std::string gSelectedSymbolSignature;

closure::FilesMapType gFileInclusionTree;
closure::FilesSetType gSystemHeadersInMainFiles;

closure::SymbolsMapType gSymbols;

//===----------------------------------------------------------------------===//
// Symbols listing
//===----------------------------------------------------------------------===//

class SymbolsListingAction : public ASTFrontendAction {
public:
  void EndSourceFileAction() override {
    for (size_t i = 0, count = mSymbols.getCount(); i != count; ++i) {
      llvm::outs() << i << " "
        << mSymbols.getType(i) << " "
        << mSymbols.getSignature(i) << "\n";
    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    return llvm::make_unique<closure::SymbolsListingConsumer>(mSymbols);
  }

private:
  closure::SymbolsList mSymbols;
};

//===----------------------------------------------------------------------===//
// Symbol locating of selected symbol
//===----------------------------------------------------------------------===//

class SymbolLocatingAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    return llvm::make_unique<closure::SymbolLocatingConsumer>(
      gSelectedSymbolSignature, SelectedSymbolIndex);
  }
};

//===----------------------------------------------------------------------===//
// Relation construction
//===----------------------------------------------------------------------===//

class RelationConstructionAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    Preprocessor &pp = CI.getPreprocessor();
    pp.addPPCallbacks(llvm::make_unique<closure::InclusionPPCallbacks>(
      CI.getSourceManager(),
      gSystemHeadersInMainFiles,
      gFileInclusionTree));
    return llvm::make_unique<closure::RelationConstructionConsumer>(gSymbols);
  }
};

static bool IsSystemHeaderInMainFile(closure::FileKeyType f) {
  return gSystemHeadersInMainFiles.find(f)
    != gSystemHeadersInMainFiles.end();
}

static size_t CountUserHeader(const closure::FileNode &node) {
  size_t r = 0;
  for (size_t i = 0, count = node.getInclusionsCount(); i != count; ++i) {
    if (!IsSystemHeaderInMainFile(node.getInclusion(i)))
      ++r;
  }
  return r;
}

static void PrintInclusionTree() {
  for (auto iter = gFileInclusionTree.begin();
    iter != gFileInclusionTree.end(); ++iter) {
    if (IsSystemHeaderInMainFile(iter->first))
      continue;

    llvm::outs() << iter->first.getDevice() << "-"
      << iter->first.getFile() << " "
      << iter->second.getFileName() << "\n";

    if (CountUserHeader(iter->second) != 0)
      llvm::outs() << "includes:\n";
    for (size_t j = 0, count = iter->second.getInclusionsCount();
      j != count;
      ++j) {
      const closure::FileKeyType &k = iter->second.getInclusion(j);
      if (IsSystemHeaderInMainFile(k))
        continue;
      llvm::outs() << k.getDevice() << "-" << k.getFile();

      auto r = gFileInclusionTree.find(k);
      if (r != gFileInclusionTree.end()) {
        llvm::outs() << " " << r->second.getFileName();
      }
      llvm::outs() << "\n";
    }

    llvm::outs() << "\n";
  }
}

//===----------------------------------------------------------------------===//
// Main
//===----------------------------------------------------------------------===//

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ClangClosureCategory);

  if (ListSymbols) {
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());
    return Tool.run(newFrontendActionFactory<SymbolsListingAction>().get());
  }
  else {
    ClangTool SymbolLocatingTool(op.getCompilations(),
      llvm::ArrayRef<std::string>(FileOfSymbol));
    std::unique_ptr<FrontendActionFactory> factory(
      newFrontendActionFactory<SymbolLocatingAction>());
    SymbolLocatingTool.run(factory.get());
    llvm::outs() << "Selected symbol signature: "
      << gSelectedSymbolSignature << "\n";

    ClangTool RelationGraphConstructionTool(op.getCompilations(),
      op.getSourcePathList());
    std::unique_ptr<FrontendActionFactory> RGFactory(
      newFrontendActionFactory<RelationConstructionAction>());
    RelationGraphConstructionTool.run(RGFactory.get());
    PrintInclusionTree();
    return 0;
  }
}