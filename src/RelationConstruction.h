#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_RELATION_CONSTRUCTION_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_CLOSURE_RELATION_CONSTRUCTION_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
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

class RelationConstructionConsumer : public ASTConsumer {
};

} // namespace closure
} // namespace clang

#endif