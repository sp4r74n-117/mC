#include "core/passes/passes.h"
#include "core/analysis/analysis.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/arithmetic/arithmetic.h"
#include <algorithm>

namespace core {
namespace passes {

	size_t HashTable::hash(const ValuePtr& val) {
		if (!val) return 42;

		// check if we know about this value already
		for (const auto& entry : entries) {
			if (*entry.value == *val) return entry.number;
		}

		size_t base = 0;
		size_t type = std::hash<unsigned>()(static_cast<unsigned>(val->getType()->getTypeId()));
		if (val->getValueCategory() == Value::VC_Constant) {
			if (analysis::types::isInt(val->getType()))
				base = std::hash<int>()(arithmetic::getValue<int>(val));
			else if (analysis::types::isFloat(val->getType()))
				base = std::hash<float>()(arithmetic::getValue<float>(val));
			else
				assert(false && "unsupported value for hash");
		} else {
			// in this case we face a variable
			base = std::hash<std::string>()(cast<Variable>(val)->getName());
		}
		return combine_hash(base, type);
	}

	size_t HashTable::hash(AssignInsn::OpType op) {
		return static_cast<AssignInsn::OpType>(op);
	}

	size_t HashTable::hash(AssignInsn::OpType op, const ValuePtr& rhs1, const ValuePtr& rhs2) {
		auto valOne = rhs1;
		auto valTwo = rhs2;
		switch (op) {
		case AssignInsn::ADD:
		case AssignInsn::MUL:
		case AssignInsn::EQ:
		case AssignInsn::NE:
				if (valOne && valTwo && *valTwo < *valOne)
					std::swap(valOne, valTwo);
				break;
		default:
				break;
		}
		if (op == AssignInsn::NONE)
			return combine_hash(valOne ? hash(valOne) : hash(valTwo));
		else
			return combine_hash(hash(op), hash(valOne), hash(valTwo));
	}

	size_t HashTable::hash(const AssignInsnPtr& insn) {
		size_t result = hash(insn->getOp(), insn->getRhs1(), insn->getRhs2());
		// check if we need to overwrite an entry
		for (auto& entry : entries) {
			if (*entry.value != *insn->getLhs()) continue;
			// ok replace the number
			entry.number = result;
			return result;
		}
		entries.push_back(Entry{result, insn->getLhs()});
		return result;
	}

	optional<ValuePtr> HashTable::find(size_t number) {
		for (auto& entry : entries) {
			if (entry.number == number) return entry.value;
		}
		return {};
	}

	std::ostream& HashTable::printTo(std::ostream& stream) const {
		for (auto& entry : entries) {
			entry.value->printTo(stream);
			stream << " -> " << entry.number << std::endl;
		}
		return stream;
	}

	void LocalValueNumberingPass::apply() {
		for (const auto& fun : manager.getProgram()->getFunctions())
			apply(fun);
	}

	void LocalValueNumberingPass::apply(const FunctionPtr& fun) {
		for (auto bb : fun->getBasicBlocks()) {
			// reset constants
			clear();

			HashTable table;
			for (const auto& insn : bb->getInsns())
				// apply the transformation to each instruction one-per-one
				apply(table, insn);
		}
	}

	void LocalValueNumberingPass::apply(HashTable& table, const InsnPtr& insn) {
		// replace all constants & vars which we might have computed
		for (const auto& replacement : replacements)
			insn->replaceNode(replacement.first, replacement.second);

		if (insn->getInsnType() != Insn::IT_Assign) return;

		auto assign = cast<AssignInsn>(insn);
		if (assign->isAssign() && assign->getRhs1()->getValueCategory() == Value::VC_Constant) return;
		if (assign->isBinary() && arithmetic::isEvaluable(assign->getRhs1(), assign->getRhs2())) {
			assign->setRhs1(arithmetic::evaluate(manager, assign->getOp(),
				assign->getRhs1(), assign->getRhs2()));
			assign->setRhs2(nullptr);
			assign->setOp(AssignInsn::NONE);

			// save the evaluated constant for later on
			replacements.insert(std::make_pair(assign->getLhs(), assign->getRhs1()));
		}

		auto result = table.find(table.hash(assign));
		if (!result) return;

		auto val = *result;
		if (val->getValueCategory() == Value::VC_Constant) return;

		auto var = cast<Variable>(val);
		if (*var == *assign->getLhs()) return;

		// ok, replace rhs
		assign->setRhs1(var);
		assign->setRhs2(nullptr);
		assign->setOp(AssignInsn::NONE);

		// in case we alias temporaries, replace that usage and safe insns!
		if (analysis::isTemporary(assign->getLhs()) && analysis::isTemporary(assign->getRhs1()))
			// save the rhs1 of the orphan assign for later on
			replacements.insert(std::make_pair(assign->getLhs(), assign->getRhs1()));
	}

	void SuperLocalValueNumberingPass::apply() {
		for (const auto& fun : manager.getProgram()->getFunctions())
			apply(fun);
	}

	void SuperLocalValueNumberingPass::apply(const FunctionPtr& fun) {
		auto ebbs = analysis::controlflow::getExtendedBasicBlocks(fun);
		// reset constants
		clear();

		BasicBlockList seen;
		for (const auto& ebb : ebbs) {
			HashTable table;
			apply(fun, table, ebb, ebb[0], seen);
		}
	}

	void SuperLocalValueNumberingPass::apply(const FunctionPtr& fun, HashTable& table, const BasicBlockList& ebb,
																					 const BasicBlockPtr& bb, BasicBlockList& seen) {
		// check if we have already seen this bb
		auto seenIt = std::find(seen.begin(), seen.end(), bb);
		if(seenIt != seen.end()) return;
		// check if we have reached the end of the ebb
		auto it = std::find(ebb.begin(), ebb.end(), bb);
		if (it == ebb.end()) return;
		// obtain a fresh copy from the parent -- thus modifications do not propagate
		// into other branches of the same apply!
		HashTable derived(table);
		for (const auto& insn : bb->getInsns())
			// invoke local variable numbering
			LocalValueNumberingPass::apply(derived, insn);
		seen.push_back(bb);
		// now recurse into our successors
		for (const auto& succ : analysis::controlflow::getSuccessors(fun, bb))
			apply(fun, derived, ebb, succ, seen);
	}
}
}
