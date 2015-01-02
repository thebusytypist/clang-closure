#include "SymbolsListing.h"
#include "SymbolLocating.h"
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

class FileNode;
typedef std::map<llvm::sys::fs::UniqueID, FileNode> FileInclusionTreeType;
FileInclusionTreeType gFileInclusionTree;

std::set<llvm::sys::fs::UniqueID> gSystemHeadersInMainFile;

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
// File inclusion tree builder
//===----------------------------------------------------------------------===//

class FileNode {
public:
  FileNode(StringRef fileName) : mFileName(fileName) {}

  StringRef getFileName() const {
    return mFileName;
  }
  
  const std::vector<llvm::sys::fs::UniqueID>& getInclusion() const {
    return mInclusion;
  }

  void appendInclusion(const llvm::sys::fs::UniqueID &id) {
    mInclusion.push_back(id);
  }

private:
  std::string mFileName;
  std::vector<llvm::sys::fs::UniqueID> mInclusion;
};

class InclusionPPCallbacks : public PPCallbacks {
public:
  InclusionPPCallbacks(SourceManager &srcMgr) : mSourceManager(srcMgr) {}

  virtual void InclusionDirective(
    SourceLocation HashLoc,
    const Token &IncludeTok,
    StringRef FileName,
    bool IsAngled,
    CharSourceRange FilenameRange,
    const FileEntry *File,
    StringRef SearchPath,
    StringRef RelativePath,
    const Module *Imported) override {
    const FileEntry *h = mSourceManager.getFileEntryForID(
      mSourceManager.getFileID(HashLoc));

    if (mSourceManager.isInSystemHeader(HashLoc)) {
      gSystemHeadersInMainFile.insert(h->getUniqueID());
      return;
    }

    FileInclusionTreeType::iterator parentIter
      = findOrInsert(h);
    findOrInsert(File);
    parentIter->second.appendInclusion(File->getUniqueID());
  }

private:
  SourceManager &mSourceManager;
  typedef std::set<llvm::sys::fs::UniqueID> mSystemHeaderInMainFile;
  int mCount;

  FileInclusionTreeType::iterator findOrInsert(const FileEntry *file) {
    FileInclusionTreeType::iterator iter
      = gFileInclusionTree.find(file->getUniqueID());
    if (iter == gFileInclusionTree.end()) {
      iter = gFileInclusionTree.insert(std::make_pair(
        file->getUniqueID(),
        FileNode(file->getName())
        )).first;
    }
    return iter;
  }
};

//===----------------------------------------------------------------------===//
// Relation graph construction
//===----------------------------------------------------------------------===//

class RelationGraphConstructionConsumer : public ASTConsumer {
};

class RelationGraphConstructionAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    Preprocessor &pp = CI.getPreprocessor();
    pp.addPPCallbacks(llvm::make_unique<InclusionPPCallbacks>(
      CI.getSourceManager()));
    return llvm::make_unique<RelationGraphConstructionConsumer>();
  }
};

static bool IsSystemHeaderInMainFile(llvm::sys::fs::UniqueID f) {
  return gSystemHeadersInMainFile.find(f)
    != gSystemHeadersInMainFile.end();
}

static size_t CountUserHeader(const std::vector<llvm::sys::fs::UniqueID> &v) {
  size_t count = 0;
  for (auto iter = v.begin(); iter != v.end(); ++iter) {
    if (!IsSystemHeaderInMainFile(*iter))
      ++count;
  }
  return count;
}

static void PrintInclusionTree() {
  for (auto iter = gFileInclusionTree.begin();
    iter != gFileInclusionTree.end(); ++iter) {
    if (IsSystemHeaderInMainFile(iter->first))
      continue;

    llvm::outs() << iter->first.getDevice() << "-"
      << iter->first.getFile() << " "
      << iter->second.getFileName() << "\n";

    auto inclusion = iter->second.getInclusion();
    if (CountUserHeader(inclusion) != 0)
      llvm::outs() << "includes:\n";
    for (auto j = inclusion.begin(); j != inclusion.end(); ++j) {
      if (IsSystemHeaderInMainFile(*j))
        continue;
      llvm::outs() << j->getDevice() << "-" << j->getFile();

      FileInclusionTreeType::const_iterator r
        = gFileInclusionTree.find(*j);
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
      newFrontendActionFactory<RelationGraphConstructionAction>());
    RelationGraphConstructionTool.run(RGFactory.get());
    PrintInclusionTree();
    return 0;
  }
}