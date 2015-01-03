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
} // namespace closure
} // namespace clang
