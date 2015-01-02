#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_SYMBOL_LOCATING_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_SYMBOL_LOCATING_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace clang {
namespace closure {

class SymbolLocatingVisitor
  : public RecursiveASTVisitor<SymbolLocatingVisitor> {
public:
  SymbolLocatingVisitor(std::string &signature, int index) :
    mContext(nullptr), mSignature(signature), mIndex(index) {}

  bool VisitFunctionDecl(FunctionDecl *fd);

  bool VisitRecordDecl(RecordDecl *rd);

  void SetASTContext(ASTContext *context) {
    mContext = context;
  }

private:
  ASTContext *mContext;
  std::string &mSignature;
  int mIndex;
};

class SymbolLocatingConsumer : public ASTConsumer {
public:
  SymbolLocatingConsumer(std::string &signature, int index) :
    mVisitor(signature, index) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override;

  void Initialize(ASTContext &Context) override;

private:
  SymbolLocatingVisitor mVisitor;
};

} // namespace closure
} // namespace clang

#endif
