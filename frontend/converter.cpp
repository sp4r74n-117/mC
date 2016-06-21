#include "frontend/converter.h"
#include "core/checks/checks.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis-types.h"
#include "core/arithmetic/arithmetic.h"

namespace frontend {
	void Converter::convert() {
		if (auto program = dyn_cast<ast::program>(root))
			convertProgram(program);
		else
			convertFragment(root);
		assert(checks::fullCheck(manager.getProgram()) && "checks failed!");
	}

	void Converter::convertFragment(sptr<ast::node> node) {
		// the task is to generate an anonymous function which holds the fragment
		auto type = manager.buildFunctionType(manager.buildBasicType(Type::TI_Void), {});
		auto fun = manager.buildFunction("", type, {});
		// allocate a default label L0
		instructions.push_back(manager.buildLabel());
		convert(node);
		buildBasicBlocks(fun);
		// and add it to the program
		manager.getProgram()->addFunction(fun);
	}

	void Converter::convertProgram(sptr<ast::program> program) {
		auto prog = manager.getProgram();
		// first of all, only register them as calls depend on this
		for (const auto& fun : program->funs) {
			auto type = dyn_cast<FunctionType>(generateType(fun->decl->type));
			assert(type && "function is associated with a non function type");

			VariableList parameters;
			for (const auto& var : fun->decl->params)
				parameters.push_back(generateVariable(var));

			prog->addFunction(manager.buildFunction(fun->decl->name, type, parameters));
		}
		// bind the member to the program
		this->program = prog;
		// now convert each on in arbitray order
		for (const auto& fun : program->funs) {
			auto target = analysis::callgraph::findFunction(prog, fun->decl->name);
			assert(target && "failed to find previously registered function");

			// if it is an external one, there is nothing to generate
			if (!fun->body) continue;

			// prepare the instructions list with a new label
			instructions.clear();
			instructions.push_back((*target)->getLabel());
			// hop into the conversion process
			convert(fun->body);
			// if we are a void function check if we need to generate an implicit ret
			if (!analysis::types::hasReturn((*target)->getType()) &&
				  (instructions.size() == 0 || !analysis::insn::isReturnInsn(*instructions.rbegin())))
			  instructions.push_back(manager.buildReturn());
			// and finally populate the internal graph of BBs
			buildBasicBlocks(*target);
		}
	}

	std::string Converter::getUniqueVariableName(const std::string& name) {
		return name + "." + std::to_string(varNr++);
	}

	AssignInsn::OpType Converter::generateOp(const sptr<ast::unary_operand>& op) {
		switch (*op) {
		case ast::unary_operand::MINUS: return AssignInsn::SUB;
		case ast::unary_operand::NOT: 	return AssignInsn::NOT;
		default:
			assert(false && "unsupported unary op");
			break;
		}
		return AssignInsn::NONE;
	}

	ValuePtr Converter::generateLoadIfNeeded(const ValuePtr& value) {
		if (!core::analysis::isOffset(value)) return value;

		// in this case it is either a memory location or an offsets
		auto lhs = manager.buildTemporary(value->getType());
		instructions.push_back(manager.buildLoad(cast<Variable>(value), lhs));
		return lhs;
	}

	AssignInsn::OpType Converter::generateOp(const sptr<ast::binary_operand>& op) {
		switch (*op) {
		case ast::binary_operand::ADD:		return AssignInsn::ADD;
		case ast::binary_operand::SUB:		return AssignInsn::SUB;
		case ast::binary_operand::MUL:		return AssignInsn::MUL;
		case ast::binary_operand::DIV:		return AssignInsn::DIV;
		case ast::binary_operand::EQ:		return AssignInsn::EQ;
		case ast::binary_operand::NE:		return AssignInsn::NE;
		case ast::binary_operand::LE:		return AssignInsn::LE;
		case ast::binary_operand::GE:		return AssignInsn::GE;
		case ast::binary_operand::LT:		return AssignInsn::LT;
		case ast::binary_operand::GT:		return AssignInsn::GT;
		case ast::binary_operand::ASSIGN:	return AssignInsn::NONE;
		default:
			assert(false && "unsupported binary op");
			break;
		}
		return AssignInsn::NONE;
	}

	TypePtr Converter::generateType(const sptr<ast::type>& type) {
		if (auto intType = dyn_cast<ast::int_type>(type)) {
			return manager.buildBasicType(Type::TI_Int);
		} else if (auto floatType = dyn_cast<ast::float_type>(type)) {
			return manager.buildBasicType(Type::TI_Float);
		} else if (auto voidType = dyn_cast<ast::void_type>(type)) {
			return manager.buildBasicType(Type::TI_Void);
		} else if (auto funType = dyn_cast<ast::function_type>(type)) {
			auto returnType = generateType(funType->return_type);

			TypeList parameterTypes;
			for (const auto& paramType : funType->parameter_types) {
				parameterTypes.push_back(generateType(paramType));
			}
			return manager.buildFunctionType(returnType, parameterTypes);
		} else if (auto arrType = dyn_cast<ast::array_type>(type)) {
			auto elementType = generateType(arrType->element_type);
			return manager.buildArrayType(elementType, arrType->dimensions);
		}
		assert(false && "unsupported type");
		return nullptr;
	}

	VariablePtr Converter::generateVariable(const sptr<ast::variable>& var) {
		auto it = vars.find(var);
		if (it == vars.end()) {
			auto tmp = manager.buildVariable(generateType(var->var_type),
				getUniqueVariableName(var->name));

			vars.insert(std::make_pair(var, tmp));
			return tmp;
		}
		return manager.buildVariable(it->second->getType(), it->second->getName());
	}

	ValuePtr Converter::generateLiteral(const sptr<ast::literal>& var) {
		if (auto intLit = dyn_cast<ast::int_literal>(var)) {
			return manager.buildIntConstant(intLit->value);
		} else if (auto floatLit = dyn_cast<ast::float_literal>(var)) {
			return manager.buildFloatConstant(floatLit->value);
		}
		assert(false && "unsupported literal");
		return nullptr;
	}

	ValuePtr Converter::generateUnary(const sptr<ast::unary_operation>& unary) {
		auto rhs1 = generateLoadIfNeeded(generateExpr(unary->sub));
		auto insn = manager.buildAssign(generateOp(unary->op), manager.buildTemporary(rhs1->getType()), rhs1);
		instructions.push_back(insn);
		return insn->getLhs();
	}

	ValuePtr Converter::generateBinary(const sptr<ast::binary_operation>& binary) {
		auto rhs1 = generateExpr(binary->lhs);
		auto rhs2 = generateLoadIfNeeded(generateExpr(binary->rhs));

		auto op = generateOp(binary->op);
		if (op == AssignInsn::NONE) {
			// this means we face a plain assignment
			assert(core::analysis::isLValue(rhs1) && "assignment to non lvalue");

			core::InsnPtr insn;
			if (core::analysis::isOffset(rhs1)) {
				insn = manager.buildStore(rhs2, cast<Variable>(rhs1));
			} else
				insn = manager.buildAssign(cast<Variable>(rhs1), rhs2);
			instructions.push_back(insn);
			return rhs1;
		}
		// may load rhs1
		rhs1 = generateLoadIfNeeded(rhs1);
		// depending on the op we use the type of rhs or implicitly convert to int
		VariablePtr lhs;
		if (core::AssignInsn::isLogicalBinaryOp(op))
			lhs = manager.buildTemporary(manager.buildBasicType(Type::TI_Int));
		else
			lhs = manager.buildTemporary(rhs1->getType());

		auto insn = manager.buildAssign(op, lhs, rhs1, rhs2);
		instructions.push_back(insn);
		return insn->getLhs();
	}

	ValuePtr Converter::generateSub(const sptr<ast::subscript_operation>& sub) {
		auto var = generateVariable(sub->var);
		assert(core::analysis::types::isArray(var->getType()) &&
			"subscript can only be used on arrays");
		auto off = generateOffset(sub);

		auto type = core::analysis::types::getElementType(var->getType());
		auto lhs = manager.buildOffset(type);
		// attach the line information to the offset variable -- loop analysis requires this
		if (sub->row >= 0) lhs->setLocation(std::make_shared<core::Location>(sub->row));
		instructions.push_back(manager.buildAssign(core::AssignInsn::ADD, lhs, var, off));
		return lhs;
	}

	ValuePtr Converter::generateOffset(const sptr<ast::subscript_operation>& sub) {
		auto arr = cast<ast::array>(sub->var);
		auto& dimensions = arr->dimensions;
		auto& exprs = sub->exprs;

		auto combine = [&](core::AssignInsn::OpType op, const core::ValuePtr& lhs, const core::ValuePtr& rhs) -> core::ValuePtr {
			// do not automatically constant fold as it complicates loop analysis
			// if (core::arithmetic::isEvaluable(lhs, rhs)) return core::arithmetic::evaluate(manager, op, lhs, rhs);

			auto tmp = manager.buildTemporary(lhs->getType());
			instructions.push_back(manager.buildAssign(op, tmp, lhs, rhs));
			return tmp;
		};

		core::ValuePtr result;
		for (unsigned i = 0; i < exprs.size(); ++i) {
		  core::ValuePtr rhs;
		  // for a given subcript & array
		  // a[2][3] int a[5][10];
		  // computes:
		  // (2*10) + 3
		  for (unsigned j = i + 1; j < exprs.size(); ++j) {
		    if (!rhs) rhs = generateLoadIfNeeded(generateExpr(dimensions[j]));
		    else			rhs = combine(core::AssignInsn::MUL, rhs, generateLoadIfNeeded(generateExpr(dimensions[j])));
		  }
		  // multiply all rhs dimensions sizes with the index of the current sub.
		  if (!rhs) rhs = generateLoadIfNeeded(generateExpr(exprs[i]));
		  else 		  rhs = combine(core::AssignInsn::MUL, rhs, generateLoadIfNeeded(generateExpr(exprs[i])));
		  // sum all with the rest
		  if (!result) result = rhs;
		  else 		  	 result = combine(core::AssignInsn::ADD, result, rhs);
		}
		// convert index to plain offset
		return combine(core::AssignInsn::MUL, result, manager.buildIntConstant(4));
	}

	ValuePtr Converter::generateExpr(const sptr<ast::expression>& expr) {
		if (auto paren = std::dynamic_pointer_cast<ast::paren_expr>(expr))
			return generateExpr(paren->sub);
		else if (auto unary = std::dynamic_pointer_cast<ast::unary_operation>(expr))
			return generateUnary(unary);
		else if (auto binary = std::dynamic_pointer_cast<ast::binary_operation>(expr))
			return generateBinary(binary);
		else if (auto var = std::dynamic_pointer_cast<ast::variable>(expr))
			return generateVariable(var);
		else if (auto lit = std::dynamic_pointer_cast<ast::literal>(expr))
			return generateLiteral(lit);
		else if (auto call = std::dynamic_pointer_cast<ast::call_expr>(expr))
			return generateCall(call);
		else if (auto sub = std::dynamic_pointer_cast<ast::subscript_operation>(expr))
			return generateSub(sub);

		assert(false && "unsupported expression");
		return nullptr;
	}

	void Converter::generateDecl(const sptr<ast::var_decl_stmt>& decl) {
		auto lhs = generateVariable(decl->var);
		ValuePtr size;
		ValueList dims;
		// in case we do not face a primitive type, we need to consider the dimensions
		if (core::analysis::types::isArray(lhs->getType())) {
			auto arr = dyn_cast<ast::array>(decl->var);
			auto cur = manager.buildIntConstant(4);
			for (unsigned i = 0; i < arr->dimensions.size(); ++i) {
				// construct the new size
				auto rhs = generateExpr(arr->dimensions[i]);
				// if it is not a constant preserve the dimension size in an additional var!
				// is cannot be a temporary as we need an ast::var for it! (care: subscript lookup)
				if (!core::analysis::isConstant(rhs)) {
					auto var = std::make_shared<ast::variable>(std::make_shared<ast::int_type>(), "dim");
					arr->dimensions[i] = var;

					// now build the additional IR constructs
					auto dim = generateVariable(var);
					instructions.push_back(manager.buildAlloca(dim, manager.buildIntConstant(4)));
					instructions.push_back(manager.buildAssign(dim, rhs));
					// the rest of the algo shall not take notice of it
					rhs = dim;
				}
				// remember the dimension vars
				dims.push_back(rhs);
				// check if we can constant fold it
				if (core::arithmetic::isEvaluable(cur, rhs)) {
					cur = core::arithmetic::evaluate(manager, core::AssignInsn::MUL, cur, rhs);
					continue;
				}
				auto tmp = manager.buildTemporary(cur->getType());
				instructions.push_back(manager.buildAssign(core::AssignInsn::MUL, tmp, cur, rhs));
				cur = tmp;
			}
			size = cur;
		} else {
			size = manager.buildIntConstant(4);
		}
		// allocate stack for it
		instructions.push_back(manager.buildAlloca(lhs, size, dims));

		// do not generate an assignment if we have no init expr
		if (!decl->init_expr) return;
		// ist must not be an array
		assert(!core::analysis::types::isArray(lhs->getType()) &&
			"arrays may not be initialized at point of definition");

		auto rhs1 = generateExpr(decl->init_expr);
		auto insn = manager.buildAssign(lhs, rhs1);
		instructions.push_back(insn);
	}

	void Converter::generateIf(const sptr<ast::if_stmt>& stmt) {
		auto expr    = generateLoadIfNeeded(generateExpr(stmt->condition));
		auto target0 = manager.buildLabel();
		auto fjmp    = manager.buildFalseJump(expr, target0);

		instructions.push_back(fjmp);
		convert(stmt->then_stmt);

		// check if we have an empty else {} or not
		bool skip = false;
		if (auto compound = dyn_cast<ast::compound_stmt>(stmt->else_stmt)) {
			skip = compound->statements.empty();
		}

		if (!skip) {
			// now we need to generate the else branch, but there are two cases to consider
			if (analysis::insn::isReturnInsn(*instructions.rbegin())) {
				instructions.push_back(target0);
				convert(stmt->else_stmt);
			} else {
				auto target1 = manager.buildLabel();
				auto ujmp = manager.buildGoto(target1);
				instructions.push_back(ujmp);
				instructions.push_back(target0);
				convert(stmt->else_stmt);
				instructions.push_back(target1);
			}
		} else {
			instructions.push_back(target0);
		}
	}

	void Converter::generateWhile(const sptr<ast::while_stmt>& stmt) {
		auto target1 = manager.buildLabel();
		instructions.push_back(target1);
		auto expr    = generateLoadIfNeeded(generateExpr(stmt->condition));
		auto target0 = manager.buildLabel();
		auto fjmp    = manager.buildFalseJump(expr, target0);
		auto ujmp    = manager.buildGoto(target1);
		instructions.push_back(fjmp);
		convert(stmt->body);
		instructions.push_back(ujmp);
		instructions.push_back(target0);
	}

	void Converter::generateFor(const sptr<ast::for_stmt>& stmt) {
		for (const auto& init : stmt->init)
			convert(init);
		auto target1 = manager.buildLabel();
		instructions.push_back(target1);

		ValuePtr expr;
		if (stmt->condition) expr = generateLoadIfNeeded(generateExpr(stmt->condition));
		else				 expr = manager.buildIntConstant(1);

		auto target0 = manager.buildLabel();
		auto fjmp    = manager.buildFalseJump(expr, target0);
		auto ujmp    = manager.buildGoto(target1);
		instructions.push_back(fjmp);
		convert(stmt->body);
		for (const auto& iter : stmt->iteration)
			convert(iter);
		instructions.push_back(ujmp);
		instructions.push_back(target0);
	}

	void Converter::generateRet(const sptr<ast::return_stmt>& stmt) {
		if (stmt->expr) instructions.push_back(manager.buildReturn(generateLoadIfNeeded(generateExpr(stmt->expr))));
		else					  instructions.push_back(manager.buildReturn());
	}

	void Converter::generateComp(const sptr<ast::compound_stmt>& comp) {
		VariablePtr sp;
		if (comp->dyn_stack) {
			// build a local internal variable to hold the sp, temporary is not allowed by alloca!
			sp = manager.buildVariable(manager.buildBasicType(Type::TI_Int), getUniqueVariableName("sp"));
			instructions.push_back(manager.buildAlloca(sp, manager.buildIntConstant(4)));
			instructions.push_back(manager.buildPushSp(sp));
			// set the outermost compound pushsp
			if (!pushSp) pushSp = sp;
		}
		for(auto stmt : comp->statements) {
			convert(stmt);
		}
		if (pushSp) {
			bool swap = core::analysis::insn::isReturnInsn(instructions.back());
			if (!swap && !sp) return;
			instructions.push_back(manager.buildPopSp(swap ? pushSp : sp));
			if (swap) {
				std::iter_swap(instructions.rbegin(), instructions.rbegin()+1);
			}
			// are we the outermost popsp?
			if (sp && *pushSp == *sp) pushSp = nullptr;
		}
	}

	ValuePtr Converter::generateCall(const sptr<ast::call_expr>& expr) {
		VariablePtr result;
		/*
		goal is to transform into the following:
		bar(3, 4);

		push 4
		push 3
		call _bar, $t0
		pop 8
		*/
		// lookup the callee -- this must succeed as the converter processes "decls" first
		auto fun = analysis::callgraph::findFunction(program, expr->fun->decl->name);
		assert(fun && "failed to lookup callee");

		// generate the push sequence to pass arguments
		auto args = expr->args;
		std::for_each(args.rbegin(), args.rend(), [&](const sptr<ast::expression>& arg) {
			instructions.push_back(manager.buildPush(generateLoadIfNeeded(generateExpr(arg))));
		});

		// generate the call itself
		if (analysis::types::hasReturn((*fun)->getType())) {
			result = manager.buildTemporary(analysis::types::getReturnType((*fun)->getType()));
			// generate a new temporary as result storage, generateExpr will use that!
			instructions.push_back(manager.buildCall(*fun, result));
		} else {
			// build a call without result storage
			instructions.push_back(manager.buildCall(*fun));
		}

		// generate the final pop
		if (args.size()) instructions.push_back(manager.buildPop(args.size() * 4));
		return result;
	}

	void Converter::buildBasicBlocks(const FunctionPtr& fun) {

		auto& basicBlocks = fun->getBasicBlocks();

		basicBlocks.push_back(std::make_shared<BasicBlock>());

		for(auto insn : instructions) {
			if (auto lbl = dyn_cast<LabelInsn>(insn)) {
				if (!basicBlocks.back()->getLabel()) {
					basicBlocks.back()->setLabel(lbl);
					continue;
				}
				basicBlocks.push_back(std::make_shared<BasicBlock>());
				basicBlocks.back()->setLabel(lbl);
			} else if (dyn_cast<GotoInsn>(insn)) {
				BasicBlock::append(basicBlocks.back(), insn);
				basicBlocks.push_back(std::make_shared<BasicBlock>());
			} else if(dyn_cast<FalseJumpInsn>(insn)) {
				BasicBlock::append(basicBlocks.back(), insn);
				basicBlocks.push_back(std::make_shared<BasicBlock>());
				basicBlocks.back()->setLabel(manager.buildLabel());
			} else {
				BasicBlock::append(basicBlocks.back(), insn);
			}
		}

		for (auto it = basicBlocks.begin(); it != basicBlocks.end(); ++it) {
			auto bb = *it;
			bb->setParent(fun);
			if (!bb->getLabel()) {
				auto lbl = manager.buildLabel();
				bb->setLabel(lbl);
			}
		}

		for(auto it = basicBlocks.begin(); it != basicBlocks.end(); ++it) {
			InsnPtr last;

			if (!(*it)->getInsns().empty())
				last = (*it)->getInsns().back();

			if (auto label = analysis::insn::getJumpTarget(last)) {
				auto bb = *it;
				auto target = analysis::controlflow::findBasicBlock(fun, [&](const BasicBlockPtr& bb) {
					return *bb->getLabel() == **label;
				});
				assert(target && "failed to find bb");

				fun->getGraph().addEdge(bb, target);
				if(last->getInsnType() == Insn::IT_Goto)
					continue;
			}

			if (last && last->getInsnType() == Insn::IT_Return) continue;

			if(std::next(it) != basicBlocks.end()) {
					auto bb = *it;
					fun->getGraph().addEdge(bb, *std::next(it));
			}
		}
	}

	void Converter::convert(sptr<ast::node> tree) {
		if(auto comp_stmt = std::dynamic_pointer_cast<ast::compound_stmt>(tree)) {
			generateComp(comp_stmt);
		} else if (auto var_decl_stmt = std::dynamic_pointer_cast<ast::var_decl_stmt>(tree)) {
			generateDecl(var_decl_stmt);
		} else if (auto if_stmt = std::dynamic_pointer_cast<ast::if_stmt>(tree)) {
			generateIf(if_stmt);
		} else if (auto while_stmt = std::dynamic_pointer_cast<ast::while_stmt>(tree)) {
			generateWhile(while_stmt);
		} else if (auto for_stmt = std::dynamic_pointer_cast<ast::for_stmt>(tree)) {
			generateFor(for_stmt);
		} else if (auto expr_stmt = std::dynamic_pointer_cast<ast::expr_stmt>(tree)) {
			generateExpr(expr_stmt->sub);
		} else if (auto ret_stmt = std::dynamic_pointer_cast<ast::return_stmt>(tree)) {
			generateRet(ret_stmt);
		} else if (auto call_stmt = std::dynamic_pointer_cast<ast::call_stmt>(tree)) {
			generateCall(call_stmt->expr);
		} else {
			assert(false && "unsupported tree node");
		}
	}
}
