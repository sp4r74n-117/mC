#include "core/analysis/analysis-loop.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-insn.h"
#include <stack>

namespace core {
namespace analysis {
namespace loop {
  namespace detail {
    Index::Type extractIndexType(const LoopPtr& loop, const arithmetic::formula::TermPtr& term) {
      VariableSet seen;
      unsigned count = 0;
      arithmetic::formula::visitValues(term, [&](const ValuePtr& value) {
        // fast-paths
        if (analysis::isConstant(value)) return;
        // traverse until we reach no parent
        LoopPtr curr = loop;
        auto var = cast<Variable>(value);
        while (curr) {
          auto it = curr->getInductionVariables().find(var);
          // we found it thus hop out if not seen already
          if (it != curr->getInductionVariables().end()) {
            // only increment if not already seen
            if (seen.insert(var).second) ++count;
            break;
          }
          curr = curr->getParent();
        }
      });

      switch (count) {
      case 0:  return Index::ZIV;
      case 1:  return Index::SIV;
      default: return Index::MIV;
      }
    }

    void collectIndices(const LoopPtr& loop, const arithmetic::formula::TermPtr& term, IndexList& indices) {
      auto res = std::make_shared<Index>();
      res->setType(extractIndexType(loop, term));
      res->setTerm(term);
      indices.push_back(res);
    }

    arithmetic::formula::TermPtr extractTerm(InsnList::const_reverse_iterator it, const InsnList& insns, const VariablePtr& tmp) {
      arithmetic::formula::TermPtr result;
      // try to find the assignment where tmp is defined
      for (; it != insns.rend(); ++it) {
        if (!insn::isAssignInsn(*it)) continue;

        auto assign = cast<AssignInsn>(*it);
        if (*assign->getLhs() != *tmp) continue;

        // obtain the static components -- in this term static is everything that
        // is not a temporary or offset!
        auto rhs1 = assign->getRhs1();
        auto rhs2 = assign->getRhs2();
        arithmetic::formula::TermPtr term1;
        arithmetic::formula::TermPtr term2;
        if (analysis::isTemporary(rhs1))
          term1 = extractTerm(it, insns, cast<Variable>(rhs1));
        else if (rhs1)
          term1 = arithmetic::formula::makeTerm(rhs1);

        if (rhs2 && AssignInsn::isBinaryOp(assign->getOp())) {
          if (analysis::isTemporary(rhs2))
            term2 = extractTerm(it, insns, cast<Variable>(rhs2));
          else
            term2 = arithmetic::formula::makeTerm(rhs2);
          // do we have everything required to build it?
          if (!term1 || !term2) return {};
          // well done!
          return arithmetic::formula::makeTerm(assign->getOp(), term1, term2);
        } else if (AssignInsn::isUnaryOp(assign->getOp())) {
          // do check if we have what we require
          if (!term1) return {};
          // construct the final term
          return arithmetic::formula::makeTerm(assign->getOp(), term1);
        }
      }
      return result;
    }

    SubscriptPtr extractSubscript(NodeManager& manager, const LoopPtr& loop, InsnList::const_reverse_iterator it,
      const InsnList& insns, const VariablePtr& off) {
      auto begin = it;
      VariablePtr var;
      IndexList indices;
      arithmetic::formula::TermPtr term;
      // first of all, try to find the offset calculation
      for (; it != insns.rend(); ++it) {
        if (!insn::isAssignInsn(*it)) continue;

        auto assign = cast<AssignInsn>(*it);
        if (*assign->getLhs() != *off) continue;

        // ok, we found our startpoint
        var = cast<Variable>(assign->getRhs1());
        // in case the variable points to a > 1D array we skip
        auto type = dyn_cast<ArrayType>(var->getType());
        assert(type && "offset computation requires an array");
        if (type->getNumOfDimensions() > 1) break;
        // fast-path for 0 .. N
        if (analysis::isIntConstant(assign->getRhs2())) {
          term = arithmetic::formula::makeTerm(assign->getRhs2());
          break;
        }
        // we assume that it is a linear 1D offset -- further decomposition
        // as pruning of sub-trees to obtain the dimension part is not part of this
        term = extractTerm(it, insns, cast<Variable>(assign->getRhs2()));
        // prune *4 on the rhs due to offset calculation -- if generateSub changes fix here too!
        if (term && term->getRhs()) {
          auto rhs = term->getRhs();
          if (term->getOp() == arithmetic::formula::Term::OpType::MUL &&
              rhs->isValue() && analysis::isIntConstant(rhs->getValue()))
              term = term->getLhs();
        }
        if (term) term = arithmetic::formula::simplify(manager, term);
        break;
      }
      if (term) collectIndices(loop, term, indices);
      // will result into invalid iff !var or indices.size() == 0
      return std::make_shared<Subscript>(*begin, var, indices);
    }

    void collectStatements(NodeManager& manager, const LoopPtr& loop, const BasicBlockPtr& bb) {
      const auto& insns = bb->getInsns();
      auto& statements = loop->getStatements();
      for (auto it = insns.rbegin(); it != insns.rend(); ++it) {
        auto insn = *it;
        // from a top-level perspective, we are only interested into load & store
        LocationPtr location;
        SubscriptPtr subscript;
        if (insn::isLoadInsn(insn)) {
          auto load = cast<LoadInsn>(insn);
          location = load->getSource()->getLocation();
          subscript = extractSubscript(manager, loop, it, insns, load->getSource());
        } else if (insn::isStoreInsn(insn)) {
          auto store = cast<StoreInsn>(insn);
          location = store->getTarget()->getLocation();
          subscript = extractSubscript(manager, loop, it, insns, store->getTarget());
        }
        else continue;
        assert(subscript && "failed to collect subscript");
        assert(location && "failed to obtain location information");
        // check where we are supposed to insert it
        auto st = std::find_if(statements.begin(), statements.end(), [&](const StatementPtr& stmt) {
          return *stmt->getLocation() == *location;
        });
        if (st == statements.end()) {
          // build a new one
          auto statement = std::make_shared<Statement>(location);
          statement->getSubscripts().push_back(subscript);
          statements.insert(statements.begin(), statement);
        } else {
          (*st)->getSubscripts().push_back(subscript);
        }
      }
    }

    void collectStatements(NodeManager& manager, const LoopPtr& loop) {
      BasicBlockPtr tail;
      // only look for statements which are not bound to child loops
      for (const auto& bb : loop->getBasicBlocks()) {
        // does this bb belong to a child?
        for (const auto& child : loop->getChildren()) {
          if (*(*child->getBasicBlocks().begin()) == *bb) {
            // yep it so, so set tail and skip until that point
            tail = *child->getBasicBlocks().rbegin();
            break;
          }
        }

        if (tail) {
          // in this case this is the last one of the child
          if (*bb == *tail) tail = nullptr;
          continue;
        }
        collectStatements(manager, loop, bb);
      }
    }

    void collectInductionVariables(const LoopPtr& loop, const BasicBlockPtr& bb) {
      // we assume that assignments have been inlined!
      for (const auto& insn : bb->getInsns()) {
        if (!insn::isAssignInsn(insn)) continue;

        auto assign = cast<AssignInsn>(insn);
        if (!assign->isBinary()) continue;
        if ((assign->getOp() != AssignInsn::ADD) && (assign->getOp() != AssignInsn::SUB)) continue;

        // it must be i = i {+,-} 1
        if (!analysis::isIntConstant(assign->getRhs2())) continue;
        // check the step
        unsigned step = arithmetic::getValue<unsigned>(assign->getRhs2());
        if (step != 1) continue;
        // check the variable itself
        if (*assign->getLhs() != *assign->getRhs1()) continue;
        // excellent we found one
        loop->getInductionVariables().insert(assign->getLhs());
      }
    }
  }

  std::ostream& Index::printTo(std::ostream& stream) const {
    switch (type) {
    case Type::ZIV: stream << "ZIV "; break;
    case Type::SIV: stream << "SIV "; break;
    case Type::MIV: stream << "MIV "; break;
    default:        stream << "?? "; break;
    }
    if (!hasTerm()) stream << "??";
    else stream << toString(*term);
    return stream;
  }

  Subscript::Subscript(const InsnPtr& insn, const VariablePtr& var, const IndexList& indices) :
    insn(insn), var(var), indices(indices) {
    if (insn::isLoadInsn(insn)) { type = READ; }
    else if (insn::isStoreInsn(insn)) { type = WRITE; }
    else {
      assert(false && "Subscript requires store or load");
    }

    // algo was not able to extract the index
    if (indices.empty() || !var) type = UNKNOWN;
  }

  std::ostream& Subscript::printTo(std::ostream& stream) const {
    switch (type) {
    case Type::READ: stream << "READ"; break;
    case Type::WRITE: stream << "WRITE"; break;
    default: stream << "??"; break;
    }
    getVariable()->printTo(stream << " ");
    for (const auto& index : indices) {
      stream << "[";
      index->printTo(stream);
      stream << "]";
    }
    return stream;
  }

  std::ostream& Statement::printTo(std::ostream& stream) const {
    location->printTo(stream);
    for (const auto& sub : subscripts) sub->printTo(stream << " ");
    return stream;
  }

  std::ostream& Loop::printTo(std::ostream& stream) const {
    bool first = true;
    for (const auto& stmt : statements) {
      if (first) first = false;
      else       stream << std::endl;
      stmt->printTo(stream);
    }
    for (const auto& loop : children) {
      if (first) first = false;
      else       stream << std::endl;
      loop->printTo(stream);
    }
    return stream;
  }

  LoopList findLoops(NodeManager& manager, const FunctionPtr& fun) {
    return findLoops(manager, fun, controlflow::getLinearBasicBlockList(fun));
  }

  LoopList findLoops(NodeManager& manager, const FunctionPtr& fun, const BasicBlockList& bbs) {
    LoopList result;
    LoopList loops;
    std::stack<LoopPtr> nest;
    // try to find loops within the given list of BasciBlocks
    // thus a loop is defined as follows:
    // label L1
    // ...
    // label L2
    // ujmp L1
    //
    // however care: for init exprs are not entailed within!
    for (auto head = bbs.begin(); head != bbs.end(); ++head) {
      // in case we have fully processed a loop adjust the stack
      if (nest.size() && *nest.top()->getBasicBlocks().rbegin() == *head) {
        nest.pop();
        continue;
      }
      // try to find incoming edges which does not belong to our immediate pred
      auto ei = controlflow::getEdges(fun, *head, utils::Direction::IN);

      BasicBlockList candidates;
      for (const auto& edge : ei) candidates.push_back(edge->getSource());
      // find the 'jump' back to the loop head
      auto tail = std::find_if(head+1, bbs.end(), [&] (const BasicBlockPtr& bb) {
        return std::find(candidates.begin(), candidates.end(), bb) != candidates.end();
      });
      // were we able to lookup the bounds?
      if (tail == bbs.end()) continue;
      // construct a bb list which references the entire loop range
      candidates.clear();
      // may also take into account head-1 (iff head != bbs.begin())
      std::copy(head, tail+1, std::back_inserter(candidates));

      LoopPtr parent;
      if (nest.size()) parent = nest.top();

      auto loop = std::make_shared<Loop>(parent, candidates);
      // check tail for induction variables
      detail::collectInductionVariables(loop, *tail);
      // in case we are a new top-level loop add it to the result
      if (!parent) result.push_back(loop);
      // always add to general loop list to fixup children later on
      loops.push_back(loop);
      // push ourselfs onto the stack
      nest.push(loop);
    }

    // fixup parent -> child relationship
    for (const auto& loop : loops) {
      if (!loop->getParent()) continue;
      loop->getParent()->getChildren().push_back(loop);
    }
    // find all statements, prior to this point it is not valid due to
    // overlapping regions of parent / child
    for (const auto& loop : loops) detail::collectStatements(manager, loop);
    return result;
  }

  namespace detail {
    class DiophantineParams : public std::pair<arithmetic::formula::TermPtr, ValuePtr> {
    public:
      using std::pair<arithmetic::formula::TermPtr, ValuePtr>::pair;
      const arithmetic::formula::TermPtr& getK() const { return this->first; }
      const ValuePtr& getD() const { return this->second; }
      operator bool() const { return this->first && this->second; }
    };
    // extracts (k,d) out of k*x + d where x is the induction variable
    DiophantineParams extractDiophantineParams(NodeManager& manager, const arithmetic::formula::TermPtr& term) {
      ValuePtr d;
      arithmetic::formula::TermPtr k;
      // due to op precedence in FE, the tree has as root (if present a plus node)
      arithmetic::formula::TermPtr curr = term;
      if (curr->getOp() == arithmetic::formula::Term::OpType::ADD) {
        if (analysis::isIntConstant(curr->getRhs()->getValue())) {
          d = curr->getRhs()->getValue();
          curr = curr->getLhs();
        } else if (analysis::isIntConstant(curr->getLhs()->getValue())) {
          d = curr->getLhs()->getValue();
          curr = curr->getRhs();
        } else return {};
      } else if (curr->isValue() && analysis::isIntConstant(curr->getValue())) {
        // construct from value itself
        d = curr->getValue();
        curr = nullptr;
      } else {
        // default construct a 'd'
        d = manager.buildIntConstant(0);
      }

      // if it is not a mul
      if (curr && curr->getOp() != arithmetic::formula::Term::OpType::MUL) {
          // is it a value?
          if (!curr->isValue()) return {};

          // in case of a true constant, use that
          if (analysis::isIntConstant(curr->getValue()))
            k = curr;
          else
            // construct 1*x where k = 1
            k = arithmetic::formula::makeTerm(manager.buildIntConstant(1));
      } else if (curr) {
        // check on which side the constant is
        if (analysis::isIntConstant(curr->getRhs()->getValue())) {
          k = curr->getRhs();
        } else if (analysis::isIntConstant(curr->getLhs()->getValue())) {
          k = curr->getLhs();
        } else return {};
      } else {
        k = arithmetic::formula::makeTerm(manager.buildIntConstant(0));
      }
      return {k, d};
    }

    bool comparableIndices(const IndexPtr& wi, const IndexPtr& oi) {
      if (wi->getType() != Index::ZIV && wi->getType() != Index::SIV) return false;
      if (oi->getType() != Index::ZIV && oi->getType() != Index::SIV) return false;

      ValueList vw;
      ValueList vi;
      // check if both use the same variable!
      auto pred = [&](const ValuePtr& value) { return analysis::isVariable(value); };
      arithmetic::formula::collectValues(wi->getTerm(), vw, pred);
      arithmetic::formula::collectValues(oi->getTerm(), vi, pred);
      if (vw.empty() || vi.empty()) return true;
      // now compare the lists
      return vw == vi;
    }
  }

  bool hasNoDependency(NodeManager& manager, const SubscriptPtr& write, const SubscriptPtr& other) {
    if (write->getType() == Subscript::UNKNOWN) return false;
    if (other->getType() == Subscript::UNKNOWN) return false;
    assert(write->getIndices().size() == other->getIndices().size() &&
      "write and other must have the same number of indices");

    unsigned count = 0;
    for (unsigned i = 0; i < write->getIndices().size(); ++i) {
      auto wi = write->getIndices()[i];
      auto oi = other->getIndices()[i];
      if (!detail::comparableIndices(wi, oi)) continue;
      auto wp = detail::extractDiophantineParams(manager, wi->getTerm());
      auto op = detail::extractDiophantineParams(manager, oi->getTerm());
      // of one of them failed, we fail too
      if (!wp || !op) continue;
      // construct the constant required for diophantine
      auto constant = arithmetic::evaluate(manager, AssignInsn::SUB, op.getD(), wp.getD());
      // construct a simple equation we want to solve
      auto formula = arithmetic::formula::makeTerm(arithmetic::formula::Term::OpType::ADD, wp.getK(), op.getK());
      // now try to solve it
      auto result = arithmetic::formula::trySolveDiophantine(manager, formula, constant);
      if (!result) ++count;
    }
    return count == write->getIndices().size();
  }
}
}
}
