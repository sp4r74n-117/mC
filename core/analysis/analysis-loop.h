#pragma once
#include "core/core.h"
#include "core/analysis/analysis.h"
#include "core/arithmetic/arithmetic.h"

namespace core {
namespace analysis {
namespace loop {
  class Index;
  typedef Ptr<Index> IndexPtr;
  typedef PtrList<Index> IndexList;
  class Subscript;
  typedef Ptr<Subscript> SubscriptPtr;
  typedef PtrList<Subscript> SubscriptList;
  class Statement;
  typedef Ptr<Statement> StatementPtr;
  typedef PtrList<Statement> StatementList;
  class Loop;
  typedef Ptr<Loop> LoopPtr;
  typedef PtrList<Loop> LoopList;

  class Index : public Printable {
  public:
    enum Type { ZIV, SIV, MIV, UNKNOWN };
    Index() : type(UNKNOWN) { }
    void setType(Type type) { this->type = type; }
    Type getType() const { return type; }
    void setTerm(const arithmetic::formula::TermPtr& term) { this->term = term; }
    const arithmetic::formula::TermPtr& getTerm() const { return term; }
    bool hasTerm() const { return term != nullptr; }
    std::ostream& printTo(std::ostream& stream) const override;
  private:
    Type type;
    arithmetic::formula::TermPtr term;
  };

  class Subscript : public Printable {
  public:
    enum Type { READ, WRITE, UNKNOWN };
    Subscript(const InsnPtr& insn, const VariablePtr& var, const IndexList& indices);
    Type getType() const { return type; }
    const VariablePtr& getVariable() const { return var; }
    const IndexList& getIndices() const { return indices; }
    std::ostream& printTo(std::ostream& stream) const override;
  private:
    InsnPtr insn;
    VariablePtr var;
    IndexList indices;
    Type type;
  };

  class Statement : public Printable {
  public:
    Statement(const LocationPtr& location) : location(location) {}
    const LocationPtr& getLocation() const { return location; }
    SubscriptList& getSubscripts() { return subscripts; }
    const SubscriptList& getSubscripts() const { return subscripts; }
    std::ostream& printTo(std::ostream& stream) const override;
  private:
    SubscriptList subscripts;
    LocationPtr location;
  };

  class Loop : public Printable {
    LoopPtr parent;
    LoopList children;
    BasicBlockList bbs;
    StatementList statements;
    VariableSet inductionVars;
  public:
    Loop(const LoopPtr& parent, const BasicBlockList& bbs) :
      parent(parent), bbs(bbs) {
      assert(bbs.size() && "loop must not be empty");
    }
    const LoopPtr& getParent() const { return parent; }
    void setParent(const LoopPtr& parent) { this->parent = parent; }
    LoopList& getChildren() { return children; }
    const LoopList& getChildren() const { return children; }
    const BasicBlockList& getBasicBlocks() const { return bbs; }
    StatementList& getStatements() { return statements; }
    const StatementList& getStatements() const { return statements; }
    VariableSet& getInductionVariables() { return inductionVars; }
    const VariableSet& getInductionVariables() const { return inductionVars; }
    std::ostream& printTo(std::ostream& stream) const override;
  };

  LoopList findLoops(NodeManager& manager, const FunctionPtr& fun);
  LoopList findLoops(NodeManager& manager, const FunctionPtr& fun, const BasicBlockList& bbs);

  bool hasNoDependency(NodeManager& manager, const SubscriptPtr& write, const SubscriptPtr& other);
}
}
}
