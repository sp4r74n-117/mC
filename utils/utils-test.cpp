#include "utils/utils-test.h"
#include "utils/utils-timex.h"
#include <iomanip>

namespace utils {
namespace test {
	TestSuite::TestSuite() :
		fails(0), noOutput(false) {}

	TestSuite& TestSuite::get() {
		static TestSuite instance;
		return instance;
	}

	void TestSuite::runTests() {
		for (const auto& test : tests) {
			double elapsed;
			int prev = fails;

			if (!noOutput)
				std::cout << "execute: " << test->getName();

			TIMEX_BGN(elapsed);
			test->run();
			TIMEX_END(elapsed);

			if (!noOutput) {
				std::cout << " took " << std::setprecision(3) << elapsed << "ms ";
				if (prev == fails) std::cout << "\033[0;32mPASSED!\033[0m" << std::endl;
				else							 std::cout << "\033[0;31mFAILED!\033[0m" << std::endl;
			}
		}
	}

	void TestSuite::registerTest(Test* test) {
		tests.push_back(test);
	}

	void TestSuite::failure(Test *test) {
		++fails;
	}

	Test::Test(const::std::string& name, bool enabled) :
		name(name) {
		if (enabled) TestSuite::get().registerTest(this);
	}

	const std::string& Test::getName() const {
		return name;
	}

	bool expect_true(Test* test, bool actual, const std::string& expected, unsigned line) {
		if (!actual) {
			std::cout << "TEST FAILED at line " << line << std::endl;
			std::cout << "     EXPRESSION: " << expected << std::endl;

			TestSuite::get().failure(test);
			return false;
		}
		return true;
	}

	bool expect_printable(Test* test, const std::string& actual, const std::string& expected, unsigned line) {
		if (actual != expected) {
			std::cout << "TEST FAILED at line " << line << std::endl;
			std::cout << "     EXPECTED: " << std::endl << expected << std::endl;
			std::cout << "     ACTUAL: " << std::endl << actual << std::endl;

			TestSuite::get().failure(test);
			return false;
		}
		return true;
	}
}
}
