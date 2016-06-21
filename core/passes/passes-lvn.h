#pragma once
#include "core/passes/passes.h"

namespace core {
namespace passes {

	class HashTable : public Printable {
		struct Entry {
			size_t number;
			ValuePtr value;
		};
		std::list<Entry> entries;

		size_t hash(const ValuePtr& val);
		size_t hash(AssignInsn::OpType op);
		size_t hash(AssignInsn::OpType op, const ValuePtr& rhs1, const ValuePtr& rhs2);
	public:
		HashTable() {}
		HashTable(const HashTable& other) = default;
		size_t hash(const AssignInsnPtr& insn);

		optional<ValuePtr> find(size_t number);
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class LocalValueNumberingPass : public Pass {
	public:
		LocalValueNumberingPass(NodeManager& manager) :
			Pass(manager) {}
		void apply() override;
	private:
		PtrMap<Variable, Value> replacements;
		void apply(const FunctionPtr& fun);
	protected:
		void apply(HashTable& table, const InsnPtr& insn);
		void clear() { replacements.clear(); }
	};

	class SuperLocalValueNumberingPass : public LocalValueNumberingPass {
	public:
		SuperLocalValueNumberingPass(NodeManager& manager) :
			LocalValueNumberingPass(manager) {}
		void apply() override;
	private:
		void apply(const FunctionPtr& fun);
		void apply(const FunctionPtr& fun, HashTable& table, const BasicBlockList& ebb, const BasicBlockPtr& bb, BasicBlockList& seen);
	};
}
}
