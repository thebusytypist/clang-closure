#include "RelationConstruction.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang::ast_matchers;

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

class DeclRefExprHandler : public MatchFinder::MatchCallback {
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const DeclRefExpr *drExpr
      = Result.Nodes.getNodeAs<DeclRefExpr>("declRefExpr")) {
      const FunctionDecl *fd
        = Result.Nodes.getNodeAs<FunctionDecl>("functionDecl");
      if (fd == mFunctionDecl)
        llvm::outs() << "DeclRefExpr: " << drExpr->getDecl()->getName()
          << " in " << fd->getName() << "\n";
    }
  }

private:
  const FunctionDecl *mFunctionDecl;

public:
  void setCurrentFunctionDecl(const FunctionDecl *fd) {
    mFunctionDecl = fd;
  }
};

static DeclRefExprHandler gDeclRefExprHandler;

RelationConstructionVisitor::RelationConstructionVisitor(
  SymbolsMapType &symbols) : mSymbols(symbols) {
  Matcher.addMatcher(
    functionDecl(
      forEachDescendant(
        declRefExpr().bind("declRefExpr"))).bind("functionDecl"),
    &gDeclRefExprHandler);
}

bool RelationConstructionVisitor::VisitFunctionDecl(FunctionDecl *fd) {
  if (fd->hasBody()) {
    Stmt *body = fd->getBody();
    llvm::outs() << "Function: " << fd->getName() << "\n";
    gDeclRefExprHandler.setCurrentFunctionDecl(fd);
    Matcher.matchAST(*mContext);
    llvm::outs() << "Function END.\n";
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
