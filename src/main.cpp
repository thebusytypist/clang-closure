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
#include <string>
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

//===----------------------------------------------------------------------===//
// Get symbols list
//===----------------------------------------------------------------------===//

class ListSymbolsVisitor : public RecursiveASTVisitor<ListSymbolsVisitor> {
public:
  ListSymbolsVisitor(std::vector<std::string> &symbols) :
    mContext(nullptr), mSymbols(symbols) {}

  bool VisitFunctionDecl(FunctionDecl *fd) {
    if (mContext->getSourceManager().isInMainFile(fd->getLocation())) {
      DeclarationName declName = fd->getNameInfo().getName();
      std::string funcName = declName.getAsString();
      mSymbols.push_back(funcName);
    }
    return true;
  }

  bool VisitRecordDecl(RecordDecl *rd) {
    if (mContext->getSourceManager().isInMainFile(rd->getLocation())) {
      mSymbols.push_back(rd->getName());
    }
    return true;
  }

  void SetASTContext(ASTContext *context) {
    mContext = context;
  }

private:
  ASTContext *mContext;
  std::vector<std::string> &mSymbols;
};

class ListSymbolsConsumer : public ASTConsumer {
public:
  explicit ListSymbolsConsumer(std::vector<std::string> &symbols) :
    mVisitor(symbols) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      mVisitor.TraverseDecl(*b);
    return true;
  }

  void Initialize(ASTContext &Context) override {
    mVisitor.SetASTContext(&Context);
  }

private:
  ListSymbolsVisitor mVisitor;
};

class ListSymbolsAction : public ASTFrontendAction {
public:
  void EndSourceFileAction() override {
    for (size_t i = 0, end = mSymbols.size(); i != end; ++i) {
      llvm::outs() << i << " " << mSymbols[i] << "\n";
    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    return llvm::make_unique<ListSymbolsConsumer>(mSymbols);
  }

private:
  std::vector<std::string> mSymbols;
};

//===----------------------------------------------------------------------===//
// Get signature of selected symbol
//===----------------------------------------------------------------------===//

class GetSignatureVisitor : public RecursiveASTVisitor<GetSignatureVisitor> {
public:
  GetSignatureVisitor(std::string &signature, int index) :
    mContext(nullptr), mSignature(signature), mIndex(index) {}

  bool VisitFunctionDecl(FunctionDecl *fd) {
    if (mIndex > 0
      && mContext->getSourceManager().isInMainFile(fd->getLocation())) {
      --mIndex;
      if (mIndex == 0) {
        std::unique_ptr<MangleContext> mangleContext =
          std::unique_ptr<MangleContext>(mContext->createMangleContext());
        mangleContext->mangleName(fd, llvm::raw_string_ostream(mSignature));
      }
    }
    return true;
  }

  bool VisitRecordDecl(RecordDecl *rd) {
    if (mIndex > 0
      && mContext->getSourceManager().isInMainFile(rd->getLocation())) {
      --mIndex;
      if (mIndex == 0) {
        std::unique_ptr<MangleContext> mangleContext =
          std::unique_ptr<MangleContext>(mContext->createMangleContext());
        mangleContext->mangleName(rd, llvm::raw_string_ostream(mSignature));
      }
    }
    return true;
  }

  void SetASTContext(ASTContext *context) {
    mContext = context;
  }

private:
  ASTContext *mContext;
  std::string &mSignature;
  int mIndex;
};

class GetSignatureConsumer : public ASTConsumer {
public:
  GetSignatureConsumer(std::string &signature, int index) :
    mVisitor(signature, index) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      mVisitor.TraverseDecl(*b);
    return true;
  }

  void Initialize(ASTContext &Context) override {
    mVisitor.SetASTContext(&Context);
  }

private:
  GetSignatureVisitor mVisitor;
};

class GetSignatureAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI,
    StringRef InFile) override {
    return llvm::make_unique<GetSignatureConsumer>(
      gSelectedSymbolSignature, SelectedSymbolIndex);
  }
};

//===----------------------------------------------------------------------===//
// Main
//===----------------------------------------------------------------------===//

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ClangClosureCategory);

  if (ListSymbols) {
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());
    return Tool.run(newFrontendActionFactory<ListSymbolsAction>().get());
  }
  else {
    ClangTool GetSignature(op.getCompilations(),
      llvm::ArrayRef<std::string>(FileOfSymbol));
    std::unique_ptr<FrontendActionFactory> factory(
      newFrontendActionFactory<GetSignatureAction>());
    GetSignature.run(factory.get());
    llvm::outs() << "Selected symbol signature: "
      << gSelectedSymbolSignature << "\n";
    return 0;
  }
}