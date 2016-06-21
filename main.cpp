#include <iostream>
#include <fstream>
#include <sstream>

#include "stream_utils.h"
#include "frontend/frontend.h"
#include "core/passes/passes.h"
#include "tests/parser_tests.h"
#include "tests/core_tests.h"
#include "backend/backend.h"
#include "utils/utils-profile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>

extern "C" {
#include <unistd.h>
#include <getopt.h>
}

namespace {
	struct arguments {
		enum backend { simple, regalloc, standard };

		arguments() :
			optimize(true), unitTests(true), compile(true), instrument(false),
			loopAnalysis(false), instrumentMaxPoints(3000), instrumentMaxRecursion(50),
			outputFile("a.out"), backendType(standard) {}
		bool optimize;
		bool unitTests;
		bool compile;
		bool instrument;
		bool loopAnalysis;
		unsigned instrumentMaxPoints;
		unsigned instrumentMaxRecursion;
		std::string dumpIR;
		std::string dumpAS;
		std::string libPath;
		std::string profileFile;
		std::string inputFile;
		std::string outputFile;
		backend backendType;
	};

	bool parse_args(int argc, char** argv, arguments& args) {
			static struct option long_options[] = {
				{"dump-ir", required_argument, 0, 0},
				{"dump-as", required_argument, 0, 1},
				{"output", required_argument, 0, 2},
				{"no-optimizations", no_argument, 0, 3},
				{"no-unit-tests", no_argument, 0, 4},
				{"no-compile", no_argument, 0, 5},
				{"libs", required_argument, 0, 6},
				{"backend-simple", no_argument, 0, 7},
				{"backend-regalloc", no_argument, 0, 8},
				{"instrument", no_argument, 0, 9},
				{"instrument-max-points", required_argument, 0, 10},
				{"instrument-max-recursion", no_argument, 0, 11},
				{"profile", required_argument, 0, 12},
				{"loop-analysis", no_argument, 0, 13},
				{0, 0, 0, 0}
			};
			if (argc < 2) return false;

			int option = 0;
			int option_index;
			while (option >= 0) {
				option = getopt_long(argc, argv, "", long_options, &option_index);
				switch (option) {
				case 0:		args.dumpIR = std::string(argv[optind-1]); break;
				case 1:		args.dumpAS = std::string(argv[optind-1]); break;
				case 2:		args.outputFile = std::string(argv[optind-1]); break;
				case 3:		args.optimize = false; break;
				case 4:		args.unitTests = false; break;
				case 5:		args.compile = false; break;
				case 6:   args.libPath = std::string(argv[optind-1]); break;
				case 7:   args.backendType = arguments::backend::simple; break;
				case 8:   args.backendType = arguments::backend::regalloc; break;
				case 9:		args.instrument = true; break;
				case 10:  args.instrumentMaxPoints = std::atoi(argv[optind-1]); break;
				case 11:	args.instrumentMaxRecursion = std::atoi(argv[optind-1]); break;
				case 12:  args.profileFile = std::string(argv[optind-1]); break;
				case 13:  args.loopAnalysis = true; break;
				default:	break;
				}
			}
			// the last argument is the file
			args.inputFile = std::string(argv[argc-1]);
			// last but not least setup the random generator
			std::srand(std::time(nullptr));
			return true;
		}

		void print_usage(int argc, char** argv) {
			std::cout << "usage " << std::string(argv[0]) << ":" << std::endl;
			std::cout << " [--dump-ir          output directory]" << std::endl;
			std::cout << " [--dump-as          output directory]" << std::endl;
			std::cout << " [--output           file name       ]" << std::endl;
			std::cout << " [--no-optimizations                 ]" << std::endl;
			std::cout << " [--no-unit-tests                    ]" << std::endl;
			std::cout << " [--no-compile                       ]" << std::endl;
			std::cout << " [--libs             library path    ]" << std::endl;
			std::cout << " [--backend-simple                   ]" << std::endl;
			std::cout << " [--backend-regalloc                 ]" << std::endl;
			std::cout << " [--instrument                       ]" << std::endl;
			std::cout << " [--instrument-max-points            ]" << std::endl;
			std::cout << " [--instrument-max-recursion         ]" << std::endl;
			std::cout << " [--profile          mprof.out       ]" << std::endl;
			std::cout << " [--loop-analysis                    ]" << std::endl;
			std::cout << " file name" << std::endl;
		}

		bool read_input(const std::string& fileName, std::stringstream& buffer) {
			if (fileName == "-") {
				buffer << std::cin.rdbuf();
				return true;
			}

			std::ifstream input{fileName};
			if (!input) {
				std::cout << "cannot read file: " << fileName << std::endl;
				return false;
			}

			buffer << input.rdbuf();
			return true;
		}

		bool compile(const backend::BackendPtr& backend, const arguments& args) {
			std::string tmpName = "/tmp/mc-gen-" + std::to_string(std::rand()) + ".s";
			// .. and open a stream to it for the backend to dump state
			std::ofstream of{tmpName};
			if (!of) return false;
			backend->printTo(of);
			if (!of) return false;
			of.close();

			auto compiler = compiler::makeBackendCompiler();
			// set the user specified path
			if (!args.libPath.empty()) compiler->setLibraryPath(args.libPath);
			// enable instrumentation support
			if (args.instrument) {
				compiler->addDependency("instrument.c");
				compiler->addLinkerFlag("-ldl");
				compiler->addCompilerFlag("-DMCC_INSTRUMENT_MAX_POINTS" + std::to_string(args.instrumentMaxPoints));
				compiler->addCompilerFlag("-DMCC_INSTRUMENT_MAX_RECURSION" + std::to_string(args.instrumentMaxRecursion));
			}
			// actually compile the file
			bool result = compiler->compile({tmpName}, args.outputFile);
			// remove the dummy one
			if (result) std::remove(tmpName.c_str());
			return result;
		}
}

int main(int argc, char** argv) {
	arguments args;
	bool result = parse_args(argc, argv, args);
	// let unit tests run if no file is specified
	if (args.unitTests) {
		test_parser();
		test_core();
	}

	if (!result) {
		print_usage(argc, argv);
		return EXIT_FAILURE;
	}

	if (args.profileFile.size()) {
		profile::Profiler profiler(args.inputFile, args.profileFile);
		if (!profiler.run()) return EXIT_FAILURE;
		// dump to std out and return
		profiler.printTo(std::cout);
		return EXIT_SUCCESS;
	}

	std::stringstream buffer;
	if (!read_input(args.inputFile, buffer))
		return EXIT_FAILURE;

	core::NodeManager manager;
	auto frontend = frontend::makeDefaultFrontend(manager, buffer.str());
	frontend->convert();

	if (args.optimize)
		// apply literally all passes we support
		core::passes::makePassSequence(manager, args.loopAnalysis)->apply();

	if (args.dumpIR.size())
		// dump all internal core structures to the given path
		core::dumpTo(manager.getProgram(), args.dumpIR);

	// generate assembler code out of the IR
	backend::BackendPtr backend;
	switch (args.backendType) {
	case arguments::backend::standard:
			backend = backend::makeDefaultBackend(manager.getProgram());
			break;
	case arguments::backend::simple:
			backend = backend::makeSimpleBackend(manager.getProgram());
			break;
	case arguments::backend::regalloc:
			backend = backend::makeRegAllocBackend(manager.getProgram());
			break;
	}
	assert(backend && "no backend selected for ir conversion");
	// enable instrumentation if required
	backend->setInstrument(args.instrument);
	backend->convert();

	if (args.dumpAS.size())
		// dump all generated assembler code to the given path
		backend::dumpTo(backend, args.dumpAS);

	// generate the final executable
	result = args.compile ? compile(backend, args) : true;
	return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
