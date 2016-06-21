#include "utils/utils-compiler.h"
#include <sstream>
#include <cstdlib>

extern "C" {
#include <unistd.h>
}

namespace utils {
namespace compiler {

  void Compiler::addCompilerFlag(const std::string &flag) {
    compilerFlags.push_back(flag);
  }

  void Compiler::addLinkerFlag(const std::string& flag) {
    linkerFlags.push_back(flag);
  }

  void Compiler::addDependency(const std::string &dependency) {
    dependencies.push_back(dependency);
  }

  void Compiler::setLibraryPath(const std::string &path) {
    libraryPath = path;
  }

  bool Compiler::compile(const std::vector<std::string> inputFiles, const std::string &outputFile) {
    std::stringstream ss;
    ss << executable << " ";
    // pass in compiler flags used during compilation
    for (const auto& flag : compilerFlags)
      ss << flag << " ";
    // pass in the output file
    ss << "-o \"" << outputFile << "\" ";
    // add the optional dependencies, e.g. lib.c
    for (const auto& dependency : dependencies)
      ss << "\"" << libraryPath << "/" << dependency << "\" ";
    // add the user provided input files
    for (const auto& inputFile : inputFiles)
      ss << "\"" << inputFile << "\" ";
    // pass in linker flags used during compilation
    for (const auto& flag : linkerFlags)
      ss << flag << " ";
    return std::system(ss.str().data()) == 0;
  }

  CompilerPtr makeStandardCompiler() {
    auto compiler = std::make_shared<Compiler>("gcc");
    compiler->addCompilerFlag("-std=gnu99");
    compiler->addCompilerFlag("-O0");
    compiler->addCompilerFlag("-Wall");
    return compiler;
  }

  CompilerPtr makeBackendCompiler() {
    auto compiler = makeStandardCompiler();
    compiler->addCompilerFlag("-m32");
    compiler->addCompilerFlag("-mfpmath=sse");
    compiler->addCompilerFlag("-march=pentium4");
    compiler->addCompilerFlag("-mno-fp-ret-in-387");
    compiler->addCompilerFlag("-g");
    compiler->addCompilerFlag("-gstabs");
    compiler->addDependency("lib.c");
    return compiler;
  }
}
}
