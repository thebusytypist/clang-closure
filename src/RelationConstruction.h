#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_RELATION_CONSTRUCTION_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_RELATION_CONSTRUCTION_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/FileSystem.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace clang {
namespace closure {

typedef llvm::sys::fs::UniqueID FileKeyType;
typedef std::set<FileKeyType> FilesSetType;
class FileNode;
typedef std::map<FileKeyType, FileNode> FilesMapType;
typedef std::string SymbolKeyType;
class SymbolNode;
typedef std::map<SymbolKeyType, SymbolNode> SymbolsMapType;

class FileNode {
public:
  FileNode(StringRef fileName) : mFileName(fileName) {}

  StringRef getFileName() const {
    return mFileName;
  }

  const FileKeyType& getInclusion(size_t index) const {
    return mInclusions[index];
  }

  size_t getInclusionsCount() const {
    return mInclusions.size();
  }

  void appendInclusion(const FileKeyType &id) {
    mInclusions.push_back(id);
  }

private:
  typedef std::vector<FileKeyType> InclusionsType;
  std::string mFileName;
  InclusionsType mInclusions;
};

class InclusionPPCallbacks : public PPCallbacks {
public:
  InclusionPPCallbacks(SourceManager &srcMgr,
    FilesSetType &filesSet,
    FilesMapType &files)
    : mSourceManager(srcMgr),
    mSystemHeadersInMainFiles(filesSet),
    mFiles(files) {}

  virtual void InclusionDirective(
    SourceLocation HashLoc,
    const Token &IncludeTok,
    StringRef FileName,
    bool IsAngled,
    CharSourceRange FilenameRange,
    const FileEntry *File,
    StringRef SearchPath,
    StringRef RelativePath,
    const Module *Imported) override;

private:
  SourceManager &mSourceManager;
  FilesSetType &mSystemHeadersInMainFiles;
  FilesMapType &mFiles;
};

class SymbolNode {
public:
  SymbolNode(const FileKeyType &file) : mFile(file) {}

  size_t getDependencyCount() const {
    return mDependencies.size();
  }

  const SymbolKeyType& getDependency(size_t index) const {
    return mDependencies[index];
  }

  void appendDependency(const SymbolKeyType &dep) {
    mDependencies.push_back(dep);
  }

  const FileKeyType& getDefinitionFile() const {
    return mFile;
  }

private:
  std::vector<SymbolKeyType> mDependencies;
  FileKeyType mFile;
};

class RelationConstructionVisitor
  : public RecursiveASTVisitor<RelationConstructionVisitor> {
public:
  RelationConstructionVisitor(SymbolsMapType &symbols);

  bool VisitFunctionDecl(FunctionDecl *fd);

  void SetASTContext(ASTContext *context) {
    mContext = context;
  }

private:
  ASTContext *mContext;
  SymbolsMapType &mSymbols;
  ast_matchers::MatchFinder Matcher;
};

class RelationConstructionConsumer : public ASTConsumer {
public:
  RelationConstructionConsumer(SymbolsMapType &symbols) : mVisitor(symbols) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override;

  void Initialize(ASTContext &Context) override;

private:
  RelationConstructionVisitor mVisitor;
};

} // namespace closure
} // namespace clang

#endif
