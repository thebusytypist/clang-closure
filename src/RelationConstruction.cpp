#include "RelationConstruction.h"

namespace clang {
namespace closure {

static FilesMapType::iterator FindOrInsert(FilesMapType &m,
  const FileEntry *file) {
  FilesMapType::iterator iter = m.find(file->getUniqueID());
  if (iter == m.end()) {
    iter = m.insert(std::make_pair(
      file->getUniqueID(),
      FileNode(file->getName())
      )).first;
  }
  return iter;
}

void InclusionPPCallbacks::InclusionDirective(
  SourceLocation HashLoc,
  const Token &IncludeTok,
  StringRef FileName,
  bool IsAngled,
  CharSourceRange FilenameRange,
  const FileEntry *File,
  StringRef SearchPath,
  StringRef RelativePath,
  const Module *Imported) {
  const FileEntry *h = mSourceManager.getFileEntryForID(
    mSourceManager.getFileID(HashLoc));

  if (mSourceManager.isInSystemHeader(HashLoc)) {
    mSystemHeadersInMainFiles.insert(h->getUniqueID());
    return;
  }

  FilesMapType::iterator parentIter = FindOrInsert(mFiles, h);
  FindOrInsert(mFiles, File);
  parentIter->second.appendInclusion(File->getUniqueID());
}

class DeclRefVisitor : public RecursiveASTVisitor<DeclRefVisitor> {
public:
  void VisitStmt(Stmt *stmt) {
    if (llvm::isa<DeclRefExpr>(stmt)) {
      DeclRefExpr *drExpr = llvm::cast<DeclRefExpr>(stmt);
      llvm::outs() << "DeclRefExpr: " << drExpr->getDecl()->getName() << "\n";
    }
  }

  void VisitDeclRefExpr(DeclRefExpr *drExpr) {
    llvm::outs() << "DeclRefExpr: " << drExpr->getDecl()->getName() << "\n";
  }
};

bool RelationConstructionVisitor::VisitFunctionDecl(FunctionDecl *fd) {
  if (fd->hasBody()) {
    Stmt *body = fd->getBody();
    DeclRefVisitor declRefVisitor;
    // declRefVisitor.TraverseStmt(body);
#if 1
    for (Stmt::child_iterator iter = body->child_begin(),
      end = body->child_end();
      iter != end; ++iter) {
      llvm::outs() << iter->getStmtClassName() << "\n";
      declRefVisitor.VisitStmt(*iter);
    }
#endif
  }
  return true;
}

bool RelationConstructionConsumer::HandleTopLevelDecl(DeclGroupRef DR) {
  for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
    mVisitor.TraverseDecl(*b);
  return true;
}

void RelationConstructionConsumer::Initialize(ASTContext &Context) {
  mVisitor.SetASTContext(&Context);
}

} // namespace closure
} // namespace clang
