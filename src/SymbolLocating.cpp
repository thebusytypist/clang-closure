#include "SymbolLocating.h"
#include "clang/AST/AST.h"
#include "clang/AST/Mangle.h"

namespace clang {
namespace closure {

bool SymbolLocatingVisitor::VisitFunctionDecl(FunctionDecl *fd) {
  if (mIndex < 0)
    return false;

  if (mContext->getSourceManager().isInMainFile(fd->getLocation())) {
    if (mIndex == 0) {
      std::unique_ptr<MangleContext> mangleContext =
        std::unique_ptr<MangleContext>(mContext->createMangleContext());
      mangleContext->mangleName(fd, llvm::raw_string_ostream(mSignature));
    }
    --mIndex;
  }
  return true;
}

bool SymbolLocatingVisitor::VisitRecordDecl(RecordDecl *rd) {
  if (mIndex < 0)
    return false;

  if (mContext->getSourceManager().isInMainFile(rd->getLocation())) {
    if (mIndex == 0) {
      std::unique_ptr<MangleContext> mangleContext
        = std::unique_ptr<MangleContext>(mContext->createMangleContext());
      QualType type = rd->getTypeForDecl()->getCanonicalTypeInternal();
      mangleContext->mangleTypeName(type,
        llvm::raw_string_ostream(mSignature));
    }
    --mIndex;
  }
  return true;
}

bool SymbolLocatingConsumer::HandleTopLevelDecl(DeclGroupRef DR) {
  for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
    mVisitor.TraverseDecl(*b);
  return true;
}

void SymbolLocatingConsumer::Initialize(ASTContext &Context) {
  mVisitor.SetASTContext(&Context);
}

} // namespace closure
} // namespace clang
