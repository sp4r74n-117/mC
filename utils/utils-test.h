#pragma once
#include <list>
#include "utils/utils-printable.h"

namespace utils {
namespace test {
	class Test;
	class TestSuite {
	public:
		static TestSuite& get();
		void runTests();
		void registerTest(Test* test);
		void failure(Test* test);
		void silence();
	private:
		TestSuite();
		int fails;
		bool noOutput;
		std::list<Test*> tests;
	};

	class Test {
	public:
		Test(const std::string& name, bool enabled = true);
		virtual void run() = 0;
		virtual const std::string& getName() const;
	protected:
		std::string name;
	};

	#define MAKE_TEST_CLASS_NAME(category, name) Test##category##name
	#define MAKE_TEST_INSTANCE_NAME(category, name) __Test##category##name
	#define MAKE_STRING(str) #str

	#define TEST(category, name, ...) \
		class MAKE_TEST_CLASS_NAME(category, name) : public Test { \
		public: \
			using Test::Test; \
			void run() override; \
		}; \
		static MAKE_TEST_CLASS_NAME(category, name) MAKE_TEST_INSTANCE_NAME(category, name) \
			(MAKE_STRING(category##name), ##__VA_ARGS__); \
		void MAKE_TEST_CLASS_NAME(category, name)::run()

	bool expect_true(Test* test, bool actual, const std::string& expected, unsigned line);
	#define EXPECT(condition) do { if(!expect_true(this, !!(condition), MAKE_STRING(condition), __LINE__)) return; } while(0)

	bool expect_printable(Test* test, const std::string& actual, const std::string& expected, unsigned line);
	#define EXPECT_PRINTABLE(printable, expected) do { if(!expect_printable(this, toString(printable), (expected), __LINE__)) return; } while(0)
}
}
