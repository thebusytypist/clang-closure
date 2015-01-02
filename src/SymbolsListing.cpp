#include "clang/AST/AST.h"
#include "clang/AST/Mangle.h"
#include "SymbolsListing.h"
#include <string>
#include <vector>

namespace clang {
namespace closure {

// first: type(record or function)
// second: signature(mangled name)
typedef std::pair<std::string, std::string> SymbolsListNode;

#define SYMSLIST static_cast<std::vector<SymbolsListNode>*>(mSymbolsListImpl)

SymbolsList::SymbolsList() {
  mSymbolsListImpl = new std::vector<SymbolsListNode>;
}

SymbolsList::~SymbolsList() {
  delete SYMSLIST;
}

size_t SymbolsList::getCount() const {
  return SYMSLIST->size();
}

StringRef SymbolsList::getType(size_t index) const {
  return StringRef((*SYMSLIST)[index].first);
}

StringRef SymbolsList::getMangledName(size_t index) const {
  return StringRef((*SYMSLIST)[index].second);
}

static inline void AppendSymbol(
  void *mSymbolsListImpl,
  const char* type, const std::string& signature) {
  SYMSLIST->push_back(std::make_pair(type, signature));
}

#undef SYMSLIST

bool SymbolsListingVisitor::VisitFunctionDecl(FunctionDecl *fd) {
  if (mContext->getSourceManager().isInMainFile(fd->getLocation())) {
    std::string signature;
    std::unique_ptr<MangleContext> mangleContext
      = std::unique_ptr<MangleContext>(mContext->createMangleContext());
    mangleContext->mangleName(fd, llvm::raw_string_ostream(signature));
    AppendSymbol(mSymbols.mSymbolsListImpl, "function", signature);
  }
  return true;
}

bool SymbolsListingVisitor::VisitRecordDecl(RecordDecl *rd) {
  if (mContext->getSourceManager().isInMainFile(rd->getLocation())) {
    std::string signature;
    std::unique_ptr<MangleContext> mangleContext
      = std::unique_ptr<MangleContext>(mContext->createMangleContext());
    QualType type = rd->getTypeForDecl()->getCanonicalTypeInternal();
    mangleContext->mangleTypeName(type, llvm::raw_string_ostream(signature));
    AppendSymbol(mSymbols.mSymbolsListImpl, "record", signature);
  }
  return true;
}

bool SymbolsListingConsumer::HandleTopLevelDecl(DeclGroupRef DR) {
  for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
    mVisitor.TraverseDecl(*b);
  return true;
}

void SymbolsListingConsumer::Initialize(ASTContext &Context) {
  mVisitor.SetASTContext(&Context);
}

} // namespace closure
} // namespace clang
