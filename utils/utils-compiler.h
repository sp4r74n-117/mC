#pragma once
#include "utils/utils-printable.h"
#include <iostream>
#include <vector>

namespace utils {
namespace compiler {
  class Compiler;
  typedef Ptr<Compiler> CompilerPtr;

  class Compiler {
    std::string executable;
    std::vector<std::string> compilerFlags;
    std::vector<std::string> linkerFlags;
    std::vector<std::string> dependencies;
    std::string libraryPath;
  public:
    Compiler(const std::string& executable) :
      executable(executable), libraryPath("./lib/")
    { }
    void addCompilerFlag(const std::string& flag);
    void addLinkerFlag(const std::string& flag);
    void addDependency(const std::string& dependency);
    void setLibraryPath(const std::string& path);
    bool compile(const std::vector<std::string> inputFiles, const std::string& outputFile);
  };

  CompilerPtr makeStandardCompiler();
  CompilerPtr makeBackendCompiler();
}
}
