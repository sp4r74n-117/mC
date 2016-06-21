#pragma once
#include "core/core.h"

namespace core {
namespace checks {

	class Checker {
	protected:
		ProgramPtr program;
	public:
		Checker(const ProgramPtr& program) : program(program) {}
		virtual bool check() = 0;
		virtual std::string getName() const = 0;
	};
	typedef sptr<Checker> CheckerPtr;

	class FunctionChecker : public Checker {
	public:
		using Checker::Checker;
		bool check() override final;
	protected:
		virtual bool check(const FunctionPtr& fun) = 0;
	};

	class AssignChecker : public FunctionChecker {
		bool check(const BasicBlockPtr& bb);
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class LoadChecker : public FunctionChecker {
		bool check(const BasicBlockPtr& bb);
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class StoreChecker : public FunctionChecker {
		bool check(const BasicBlockPtr& bb);
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class GotoChecker : public FunctionChecker {
		bool check(const FunctionPtr& fun, const BasicBlockPtr& bb);
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class LabelChecker : public Checker {
	public:
		using Checker::Checker;
		std::string getName() const override;
		bool check() override;
	};

	class FalseJumpChecker : public FunctionChecker {
		bool check(const FunctionPtr& fun, const BasicBlockPtr& bb);
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class CallChecker : public FunctionChecker {
		bool check(const FunctionPtr& fun, const BasicBlockPtr& bb);
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class ReturnChecker : public FunctionChecker {
		bool check(const FunctionPtr& fun, const BasicBlockPtr& bb);
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class BasicBlockChecker : public FunctionChecker {
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	class EdgeChecker : public FunctionChecker {
	public:
		using FunctionChecker::FunctionChecker;
		std::string getName() const override;
	protected:
		bool check(const FunctionPtr& fun) override;
	};

	bool fullCheck(const ProgramPtr& program);
}
}
