#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_SYMBOLS_LISTING_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_SYMBOLS_LISTING_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/ADT/StringRef.h"

namespace clang {
namespace closure {

class SymbolsList final {
  friend class SymbolsListingVisitor;

public:
  SymbolsList();
  ~SymbolsList();

  size_t getCount() const;
  StringRef getType(size_t index) const;
  StringRef getSignature(size_t index) const;

private:
  void *mSymbolsListImpl;
};

class SymbolsListingVisitor
  : public RecursiveASTVisitor<SymbolsListingVisitor> {
public:
  SymbolsListingVisitor(SymbolsList &symbols) :
    mContext(nullptr), mSymbols(symbols) {}

  bool VisitFunctionDecl(FunctionDecl *fd);

  bool VisitRecordDecl(RecordDecl *rd);

  void SetASTContext(ASTContext *context) {
    mContext = context;
  }

private:
  ASTContext *mContext;
  SymbolsList &mSymbols;
};

class SymbolsListingConsumer : public clang::ASTConsumer {
public:
  explicit SymbolsListingConsumer(SymbolsList &symbols)
    : mVisitor(symbols) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override;

  void Initialize(ASTContext &Context) override;

private:
  SymbolsListingVisitor mVisitor;
};

} // namespace closure
} // namespace clang

#endif
