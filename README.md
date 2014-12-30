# Introduction
This is a clang tooling project for practice.

Given a symbol, clang-closure extracts a minimum set of files from project
which define this symbol and all its dependencies.

# Update
* Make clang-closure a part of clang-tools-extra project.
  Currently manual intervention is requred.

  Add:
  ```
  add_subdirectory(path/to/clang-closure
    ${CMAKE_CURRENT_BINARY_DIR}/clang-closure
    )
  ```
  to `path/to/clang-tools-extra/CMakeLists.txt`,
  and
  ```
  add_subdirectory(path/to/clang-closure/unittests
    ${CMAKE_CURRENT_BINARY_DIR}/clang-closure
    )
  ```
  to `path/to/clang-tools-extra/unittests/CMakeLists.txt`.
