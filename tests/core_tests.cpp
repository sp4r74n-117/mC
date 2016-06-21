#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>

#include "core/core.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis-live-variable.h"
#include "core/analysis/analysis-interference-graph.h"
#include "core/analysis/analysis-loop.h"
#include "core/arithmetic/arithmetic.h"
#include "core/passes/passes.h"
#include "frontend/parser.h"
#include "frontend/converter.h"
#include "backend/backend-memory.h"
#include "backend/backend-insn.h"
#include "stream_utils.h"
#include "utils/utils-graph-color.h"
#include "utils/utils-test.h"

using namespace core;
using namespace test;

namespace {

	TEST(Converter, Expression)
	{
		string str_compound{R"({ int a = 1 + 2 + 3; 4 + 5;})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(fun->getBasicBlocks().size() == 1);
		EXPECT(fun->getEdges().size() == 0);

		{
		std::string expected = "" \
			"L0 {\n" \
			"alloca a.0,4 int\n" \
			"$0 = 2+3\n" \
			"$1 = 1+$0\n" \
			"a.0 = $1\n" \
			"$2 = 4+5\n" \
			"}";
		EXPECT_PRINTABLE(fun->getBasicBlocks().front(), expected);
		}
	}

	TEST(Converter, Scope)
	{
		string str_compound{R"({ int a = 1; { int a = 2; } a = 3 + 4; })"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(fun->getBasicBlocks().size() == 1);
		EXPECT(fun->getEdges().size() == 0);

		{
		std::string expected = "" \
			"L0 {\n" \
			"alloca a.0,4 int\n" \
			"a.0 = 1\n" \
			"alloca a.1,4 int\n" \
			"a.1 = 2\n" \
			"$0 = 3+4\n" \
			"a.0 = $0\n" \
			"}";
		EXPECT_PRINTABLE(fun->getBasicBlocks().front(), expected);
		}
	}

	TEST(Converter, IfElse)
	{
		string str_compound{R"({ if (1) { int a = 0; } else { int b = 1; } })"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(fun->getBasicBlocks().size() == 4);
		EXPECT(fun->getEdges().size() == 4);

		auto bbs = fun->getBasicBlocks();
		EXPECT(analysis::controlflow::getEdges(fun, bbs[0], Direction::OUT).size() == 2);
		EXPECT(analysis::controlflow::getEdges(fun, bbs[1], Direction::IN).size()  == 1);
		EXPECT(analysis::controlflow::getEdges(fun, bbs[1], Direction::OUT).size() == 1);
		EXPECT(analysis::controlflow::getEdges(fun, bbs[2], Direction::IN).size()  == 1);
		EXPECT(analysis::controlflow::getEdges(fun, bbs[2], Direction::OUT).size() == 1);
		EXPECT(analysis::controlflow::getEdges(fun, bbs[3], Direction::IN).size()  == 2);

		{
		std::string expected = "" \
			"L0 {\n" \
			"fjmp 1 L1\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}

		{
		std::string expected = "" \
			"L3 {\n" \
			"alloca a.0,4 int\n" \
			"a.0 = 0\n" \
			"goto L2\n" \
			"}";
		EXPECT_PRINTABLE(bbs[1], expected);
		}

		{
		std::string expected = "" \
			"L1 {\n" \
			"alloca b.1,4 int\n" \
			"b.1 = 1\n" \
			"}";
		EXPECT_PRINTABLE(bbs[2], expected);
		}

		{
		std::string expected = "" \
			"L2 {\n" \
			"}";
		EXPECT_PRINTABLE(bbs[3], expected);
		}
	}

	TEST(Core, NodeEquals)
	{
		NodeManager manager;

		auto litInt0 = manager.buildIntConstant(0);
		auto litInt1 = manager.buildIntConstant(1);
		EXPECT(*litInt0 == *litInt0);
		EXPECT(*litInt0 != *litInt1);
		EXPECT(*litInt0 == *manager.buildIntConstant(0));

		auto litFlt0 = manager.buildFloatConstant(0.0);
		auto litFlt1 = manager.buildFloatConstant(1.0);
		EXPECT(*litFlt0 == *litFlt0);
		EXPECT(*litFlt0 != *litFlt1);
		EXPECT(*litFlt0 == *manager.buildFloatConstant(0.0));

		EXPECT(*litInt0 != *litFlt0);
		EXPECT(*litInt1 != *litFlt1);

		auto label0 = manager.buildLabel();
		auto label1 = manager.buildLabel();
		EXPECT(*label0 == *label0);
		EXPECT(*label0 != *label1);

		auto goto0 = manager.buildGoto(label0);
		auto goto1 = manager.buildGoto(label1);
		EXPECT(*goto0 == *goto0);
		EXPECT(*goto0 != *goto1);
	}

	TEST(Core, SharedVariables)
	{
		string str_compound{R"({ int a = 0; a = 1; })"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(fun->getBasicBlocks().size() == 1);
		auto bb = fun->getBasicBlocks().front();

		EXPECT(bb->getInsns().size() == 3);
		auto fst = dyn_cast<AssignInsn>(bb->getInsns()[1]);
		auto snd = dyn_cast<AssignInsn>(bb->getInsns()[2]);
		EXPECT(fst && snd);
		EXPECT(*(fst->getLhs()) == *(snd->getLhs()));
		EXPECT(fst->getLhs() == snd->getLhs());
	}

	TEST(Pass, Integrity)
	{
		using namespace core::passes;
		string str_compound{R"({ int a = 0; })"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		PassSequence seq(manager, makePass<IntegrityPass>(manager));
		seq.apply();
	}

	TEST(Core, VariableListEquals)
	{
		NodeManager manager;

		auto intType = manager.buildBasicType(Type::TI_Int);

		VariableList list1;
		list1.push_back(manager.buildVariable(intType, "a"));
		list1.push_back(manager.buildVariable(intType, "b"));

		VariableList list2;
		list2.push_back(manager.buildVariable(intType, "a"));
		EXPECT(list1 != list2);

		list2.push_back(manager.buildVariable(intType, "b"));
		EXPECT(list1 == list2);
	}

	TEST(Analysis, ModifiedVars)
	{
		string str_compound{R"({ int a = 0; a = 1;})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(fun->getBasicBlocks().size() == 1);
		auto bb = fun->getBasicBlocks().front();

		auto vars = analysis::controlflow::getModifiedVars(bb);
		EXPECT(vars.size() == 1);
		EXPECT_PRINTABLE(*vars.begin(), "a.0");
	}

	TEST(Analysis, IncomingVars)
	{
		string str_compound{R"({ int a = 0; if (1) { int b = a + 1; } else { int c = 0; int d = c;} })"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(fun->getBasicBlocks().size() == 4);
		auto bbs = fun->getBasicBlocks();

		EXPECT(analysis::controlflow::getIncomingVars(bbs[0]).size() == 0);
		EXPECT(analysis::controlflow::getIncomingVars(bbs[2]).size() == 0);
		EXPECT(analysis::controlflow::getIncomingVars(bbs[3]).size() == 0);

		auto vars = analysis::controlflow::getIncomingVars(bbs[1]);
		EXPECT(vars.size() == 1);

		auto varA = cast<AssignInsn>(bbs[0]->getInsns()[1])->getLhs();
		EXPECT(*varA == **vars.begin());
	}

	TEST(Analysis, DominatorMap)
	{
		string str_compound{R"({ int a = 0; if (1) { int b = a + 1; } else { int c = 0; int d = c;} })"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		auto domMap = analysis::controlflow::getDominatorMap(fun);
		EXPECT(domMap.size() == 4);

		auto bbs = fun->getBasicBlocks();
		EXPECT(domMap[bbs[0]].size() == 1);
		EXPECT(domMap[bbs[1]].size() == 2);
		EXPECT(domMap[bbs[2]].size() == 2);
		EXPECT(domMap[bbs[3]].size() == 2);

		EXPECT(domMap[bbs[1]] == (analysis::controlflow::DominatorSet{{bbs[0], bbs[1]}}));
		EXPECT(domMap[bbs[2]] == (analysis::controlflow::DominatorSet{{bbs[0], bbs[2]}}));
		EXPECT(domMap[bbs[3]] == (analysis::controlflow::DominatorSet{{bbs[0], bbs[3]}}));

		EXPECT(!analysis::controlflow::getImmediateDominator(domMap, bbs[0]));
		EXPECT(*analysis::controlflow::getImmediateDominator(domMap, bbs[1]) == bbs[0]);
		EXPECT(*analysis::controlflow::getImmediateDominator(domMap, bbs[2]) == bbs[0]);
		EXPECT(*analysis::controlflow::getImmediateDominator(domMap, bbs[3]) == bbs[0]);
	}

	TEST(Core, SSAIndex)
	{
		NodeManager manager;

		auto var = manager.buildVariable(manager.buildBasicType(Type::TI_Int), "a.0");
		EXPECT_PRINTABLE(var, "a.0");
		EXPECT(!var->hasSSAIndex());

		var->setSSAIndex(0);
		EXPECT_PRINTABLE(var, "a.0.0");
		EXPECT(var->hasSSAIndex());
		EXPECT(var->getSSAIndex() == 0);

		var->setSSAIndex(UINT_MAX);
		EXPECT_PRINTABLE(var, "a.0");
		EXPECT(!var->hasSSAIndex());
	}

	TEST(Pass, LocalValueNumbering)
	{
		using namespace core::passes;
		string str_compound{R"(
		{
			int a = 0;
			int b = 1;
			a = 2;
			int c = a;
			int x = 3;
			int y = 0;
			if (a < b) {
				a = x + y;
				b = y + x;
				a = 42 * 2;
				c = x + y;
			} else {
				c = x + y;
			}
		})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		PassSequence seq(manager, makePass<LocalValueNumberingPass>(manager));
		seq.apply();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		auto bbs = fun->getBasicBlocks();
		EXPECT(bbs.size() == 4);

		{
		std::string expected = "" \
			"L3 {\n" \
			"$1 = x.3+y.4\n" \
			"a.0 = $1\n" \
			"$2 = $1\n" \
			"b.1 = $1\n" \
			"$3 = 84\n" \
			"a.0 = 84\n" \
			"$4 = $1\n" \
			"c.2 = $1\n" \
			"goto L2\n" \
			"}";
		EXPECT_PRINTABLE(bbs[1], expected);
		}

		{
		std::string expected = "" \
			"L1 {\n" \
			"$5 = x.3+y.4\n" \
			"c.2 = $5\n" \
			"}";
		EXPECT_PRINTABLE(bbs[2], expected);
		}

		{
		std::string expected = "" \
			"L2 {\n" \
			"}";
		EXPECT_PRINTABLE(bbs[3], expected);
		}
	}

	TEST(Analysis, ExtendedBasicBlocks)
	{
		using namespace core::passes;
		string str_compound{R"(
		{
			int a = 0;
			int b = 1;
			int c = 0;
			int d = 4;
			int e = 2;
			int f = 3;

			int m = a + b;
			int n = a + b;
			if (1)
			{
				int p = c + d;
				int r = c + d;
			}
			else
			{
				int q = a + b;
				int r = c + d;
				if (1)
				{
					int e = b + 5;
					int s = a + b;
					int u = e + f;
				}
				else
				{
					int e = b + 8;
					int t = c + d;
					int u = e + f;
				}
				int v = a + b;
				int w = c + d;
				int x = e + f;
			}
			int y = a + b;
			int z = c + d;
		})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		PassSequence seq(manager, makePass<SuperLocalValueNumberingPass>(manager));
		seq.apply();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		auto bbs = fun->getBasicBlocks();
		EXPECT(bbs.size() == 7);

		auto ebbs = analysis::controlflow::getExtendedBasicBlocks(fun);
		EXPECT(ebbs[0] == (BasicBlockList{bbs[0], bbs[2], bbs[4], bbs[3], bbs[1]}));
		EXPECT(ebbs[1] == (BasicBlockList{bbs[5]}));
		EXPECT(ebbs[2] == (BasicBlockList{bbs[6]}));

		{
		std::string expected = "" \
			"L0 {\n" \
			"alloca a.0,4 int\n" \
			"a.0 = 0\n" \
			"alloca b.1,4 int\n" \
			"b.1 = 1\n" \
			"alloca c.2,4 int\n" \
			"c.2 = 0\n" \
			"alloca d.3,4 int\n" \
			"d.3 = 4\n" \
			"alloca e.4,4 int\n" \
			"e.4 = 2\n" \
			"alloca f.5,4 int\n" \
			"f.5 = 3\n" \
			"alloca m.6,4 int\n" \
			"$0 = a.0+b.1\n" \
			"m.6 = $0\n" \
			"alloca n.7,4 int\n" \
			"$1 = $0\n" \
			"n.7 = $0\n" \
			"fjmp 1 L1\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}

		{
		std::string expected = "" \
			"L5 {\n" \
			"alloca p.8,4 int\n" \
			"$2 = c.2+d.3\n" \
			"p.8 = $2\n" \
			"alloca r.9,4 int\n" \
			"$3 = $2\n" \
			"r.9 = $2\n" \
			"goto L2\n" \
			"}";
		EXPECT_PRINTABLE(bbs[1], expected);
		}

		{
		std::string expected = "" \
			"L1 {\n" \
			"alloca q.10,4 int\n" \
			"$4 = $0\n" \
			"q.10 = $0\n" \
			"alloca r.11,4 int\n" \
			"$5 = c.2+d.3\n" \
			"r.11 = $5\n" \
			"fjmp 1 L3\n" \
			"}";
		EXPECT_PRINTABLE(bbs[2], expected);
		}

		{
		std::string expected = "" \
			"L6 {\n" \
			"alloca e.12,4 int\n" \
			"$6 = b.1+5\n" \
			"e.12 = $6\n" \
			"alloca s.13,4 int\n" \
			"$7 = $0\n" \
			"s.13 = $0\n" \
			"alloca u.14,4 int\n" \
			"$8 = e.12+f.5\n" \
			"u.14 = $8\n" \
			"goto L4\n" \
			"}";
		EXPECT_PRINTABLE(bbs[3], expected);
		}

		{
		std::string expected = "" \
			"L3 {\n" \
			"alloca e.15,4 int\n" \
			"$9 = b.1+8\n" \
			"e.15 = $9\n" \
			"alloca t.16,4 int\n" \
			"$10 = $5\n" \
			"t.16 = $5\n" \
			"alloca u.17,4 int\n" \
			"$11 = e.15+f.5\n" \
			"u.17 = $11\n" \
			"}";
		EXPECT_PRINTABLE(bbs[4], expected);
		}

		{
		std::string expected = "" \
			"L4 {\n" \
			"alloca v.18,4 int\n" \
			"$12 = a.0+b.1\n" \
			"v.18 = $12\n" \
			"alloca w.19,4 int\n" \
			"$13 = c.2+d.3\n" \
			"w.19 = $13\n" \
			"alloca x.20,4 int\n" \
			"$14 = e.4+f.5\n" \
			"x.20 = $14\n" \
			"}";
		EXPECT_PRINTABLE(bbs[5], expected);
		}

		{
		std::string expected = "" \
			"L2 {\n" \
			"alloca y.21,4 int\n" \
			"$15 = a.0+b.1\n" \
			"y.21 = $15\n" \
			"alloca z.22,4 int\n" \
			"$16 = c.2+d.3\n" \
			"z.22 = $16\n" \
			"}";
		EXPECT_PRINTABLE(bbs[6], expected);
		}


		auto domMap = analysis::controlflow::getDominatorMap(fun);
		std::for_each(domMap.begin(), domMap.end(), [&](const std::pair<BasicBlockPtr, analysis::controlflow::DominatorSet>& entry) {
			auto bbName = entry.first->getLabel()->getName();
			const auto& domSet = entry.second;

			if 		(bbName == "L0") EXPECT_PRINTABLE(domSet, "{L0}");
			else if (bbName == "L1") EXPECT_PRINTABLE(domSet, "{L0,L1}");
			else if (bbName == "L2") EXPECT_PRINTABLE(domSet, "{L0,L2}");
			else if (bbName == "L3") EXPECT_PRINTABLE(domSet, "{L0,L1,L3}");
			else if (bbName == "L4") EXPECT_PRINTABLE(domSet, "{L0,L1,L4}");
			else if (bbName == "L5") EXPECT_PRINTABLE(domSet, "{L0,L5}");
			else if (bbName == "L6") EXPECT_PRINTABLE(domSet, "{L0,L1,L6}");
			else					 EXPECT(false && "invalid label name");
		});
	}

	TEST(Analysis, Liveness)
	{
		using namespace core::passes;
		using namespace core::analysis::worklist;
		NodeManager manager;

		string str_compound{R"(
		{
			int a = 3;
			int b = 5;
			int d = 4;
			int x = 100;
			if(a > b)
			{
				int c = a + b;
				b = 2;
			}
			x = 8;
			int c = 4;
			int u = b+c;
		})"};

		frontend::Converter converter(manager, str_compound);
		converter.convert();

		PassSequence seq(manager, makePass<SuperLocalValueNumberingPass>(manager));
		seq.apply();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		BasicBlockLiveness wl(fun);
		wl.apply(fun->getBasicBlocks().begin(), fun->getBasicBlocks().end());

		auto& nodeData = wl.getNodeData();
		auto bbs = fun->getBasicBlocks();

		{
		std::string expected = "" \
			"LIVEIN: {}\n" \
			"LIVEOUT: {a.0, b.1}";
		EXPECT_PRINTABLE(nodeData[bbs[0]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1}\n" \
			"LIVEOUT: {b.1}";
		EXPECT_PRINTABLE(nodeData[bbs[1]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {b.1}\n" \
			"LIVEOUT: {}";
		EXPECT_PRINTABLE(nodeData[bbs[2]], expected);
		}
	}

	TEST(Converter, While)
	{
		using namespace core::passes;
		using namespace core::analysis::worklist;
		NodeManager manager;

		string str_compound{R"(
		{
			int a = 3;
			int b = 5;
			int c = 4;
			a = c + b;

			if(a < b) {
				int d = 4;
				d = c + b;
				while(a > 1) {
					c = c + b;
					a = a - 1;

					while(c < 20) {
						d = d + 1;
					}
					int m = d;
				}
				b = 3;
			} else {
				int e = 5;
			}
			int u = a + b;
		})"};

		frontend::Converter converter(manager, str_compound);
		converter.convert();

		PassSequence seq(manager, makePass<SuperLocalValueNumberingPass>(manager));
		seq.apply();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		/* TEST THREE ADDRESS CODE + SLVN */

		auto bbs = fun->getBasicBlocks();
		EXPECT(bbs.size() == 10);

		{
		std::string expected = "" \
			"L0 {\n" \
			"alloca a.0,4 int\n" \
			"a.0 = 3\n" \
			"alloca b.1,4 int\n" \
			"b.1 = 5\n" \
			"alloca c.2,4 int\n" \
			"c.2 = 4\n" \
			"$0 = c.2+b.1\n" \
			"a.0 = $0\n" \
			"$1 = a.0<b.1\n" \
			"fjmp $1 L1\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}

		{
		std::string expected = "" \
			"L7 {\n" \
			"alloca d.3,4 int\n" \
			"d.3 = 4\n" \
			"$2 = $0\n" \
			"d.3 = $0\n" \
			"}";
		EXPECT_PRINTABLE(bbs[1], expected);
		}

		{
		std::string expected = "" \
			"L2 {\n" \
			"$3 = a.0>1\n" \
			"fjmp $3 L3\n" \
			"}";
		EXPECT_PRINTABLE(bbs[2], expected);
		}

		{
		std::string expected = "" \
			"L8 {\n" \
			"$4 = c.2+b.1\n" \
			"c.2 = $4\n" \
			"$5 = a.0-1\n" \
			"a.0 = $5\n" \
			"}";
		EXPECT_PRINTABLE(bbs[3], expected);
		}

		{
		std::string expected = "" \
			"L4 {\n" \
			"$6 = c.2<20\n" \
			"fjmp $6 L5\n" \
			"}";
		EXPECT_PRINTABLE(bbs[4], expected);
		}

		{
		std::string expected = "" \
			"L9 {\n" \
			"$7 = d.3+1\n" \
			"d.3 = $7\n" \
			"goto L4\n" \
			"}";
		EXPECT_PRINTABLE(bbs[5], expected);
		}

		{
		std::string expected = "" \
			"L5 {\n" \
			"alloca m.4,4 int\n" \
			"m.4 = d.3\n" \
			"goto L2\n" \
			"}";
		EXPECT_PRINTABLE(bbs[6], expected);
		}

		{
		std::string expected = "" \
			"L3 {\n" \
			"b.1 = 3\n" \
			"goto L6\n" \
			"}";
		EXPECT_PRINTABLE(bbs[7], expected);
		}

		{
		std::string expected = "" \
			"L1 {\n" \
			"alloca e.5,4 int\n" \
			"e.5 = 5\n" \
			"}";
		EXPECT_PRINTABLE(bbs[8], expected);
		}

		{
		std::string expected = "" \
			"L6 {\n" \
			"alloca u.6,4 int\n" \
			"$8 = a.0+b.1\n" \
			"u.6 = $8\n" \
			"}";
		EXPECT_PRINTABLE(bbs[9], expected);
		}


		/* TEST EXTENDED BASIC BLOCKS */

		auto ebbs = analysis::controlflow::getExtendedBasicBlocks(fun);
		EXPECT(ebbs.size() == 4);

		EXPECT(ebbs[0] == (BasicBlockList{bbs[0], bbs[8], bbs[1]}));
		EXPECT(ebbs[1] == (BasicBlockList{bbs[2], bbs[7], bbs[3]}));
		EXPECT(ebbs[2] == (BasicBlockList{bbs[4], bbs[6], bbs[5]}));
		EXPECT(ebbs[3] == (BasicBlockList{bbs[9]}));


		/* TEST DOMINATOR SET AND IMMEDIATE DOMINATOR */

		auto domMap = analysis::controlflow::getDominatorMap(fun);
		std::for_each(domMap.begin(), domMap.end(), [&](const std::pair<BasicBlockPtr, analysis::controlflow::DominatorSet>& entry) {
			auto bbName = entry.first->getLabel()->getName();
			auto domSet = entry.second;

			if 		(bbName == "L0") {EXPECT_PRINTABLE(domSet, "{L0}");
									  EXPECT(!analysis::controlflow::getImmediateDominator(domMap, entry.first));}
			else if (bbName == "L1") {EXPECT_PRINTABLE(domSet, "{L0,L1}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[0]);}
			else if (bbName == "L2") {EXPECT_PRINTABLE(domSet, "{L0,L2,L7}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[1]);}
			else if (bbName == "L3") {EXPECT_PRINTABLE(domSet, "{L0,L2,L3,L7}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[2]);}
			else if (bbName == "L4") {EXPECT_PRINTABLE(domSet, "{L0,L2,L4,L7,L8}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[3]);}
			else if (bbName == "L5") {EXPECT_PRINTABLE(domSet, "{L0,L2,L4,L5,L7,L8}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[4]);}
			else if (bbName == "L6") {EXPECT_PRINTABLE(domSet, "{L0,L6}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[0]);}
			else if (bbName == "L7") {EXPECT_PRINTABLE(domSet, "{L0,L7}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[0]);}
			else if (bbName == "L8") {EXPECT_PRINTABLE(domSet, "{L0,L2,L7,L8}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[2]);}
			else if (bbName == "L9") {EXPECT_PRINTABLE(domSet, "{L0,L2,L4,L7,L8,L9}");
									  EXPECT(*analysis::controlflow::getImmediateDominator(domMap, entry.first) == bbs[4]);}
			else					 EXPECT(false && "invalid label name");
		});


		/* TEST LIVENESS ANALYSIS */

		BasicBlockLiveness wl(fun);
		wl.apply(fun->getBasicBlocks().begin(), fun->getBasicBlocks().end());
		auto& nodeData = wl.getNodeData();

		{
		std::string expected = "" \
			"LIVEIN: {}\n" \
			"LIVEOUT: {$0, a.0, b.1, c.2}";
		EXPECT_PRINTABLE(nodeData[bbs[0]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {$0, a.0, b.1, c.2}\n" \
			"LIVEOUT: {a.0, b.1, c.2, d.3}";
		EXPECT_PRINTABLE(nodeData[bbs[1]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1, c.2, d.3}\n" \
			"LIVEOUT: {a.0, b.1, c.2, d.3}";
		EXPECT_PRINTABLE(nodeData[bbs[2]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1, c.2, d.3}\n" \
			"LIVEOUT: {a.0, b.1, c.2, d.3}";
		EXPECT_PRINTABLE(nodeData[bbs[3]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1, c.2, d.3}\n" \
			"LIVEOUT: {a.0, b.1, c.2, d.3}";
		EXPECT_PRINTABLE(nodeData[bbs[4]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1, c.2, d.3}\n" \
			"LIVEOUT: {a.0, b.1, c.2, d.3}";
		EXPECT_PRINTABLE(nodeData[bbs[5]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1, c.2, d.3}\n" \
			"LIVEOUT: {a.0, b.1, c.2, d.3}";
		EXPECT_PRINTABLE(nodeData[bbs[6]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0}\n" \
			"LIVEOUT: {a.0, b.1}";
		EXPECT_PRINTABLE(nodeData[bbs[7]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1}\n" \
			"LIVEOUT: {a.0, b.1}";
		EXPECT_PRINTABLE(nodeData[bbs[8]], expected);
		}

		{
		std::string expected = "" \
			"LIVEIN: {a.0, b.1}\n" \
			"LIVEOUT: {}";
		EXPECT_PRINTABLE(nodeData[bbs[9]], expected);
		}
	}

	TEST(Converter, For)
	{
		string str_compound{R"({
			int a = 0;
			for (int b = a, c = 10; b < c; a = a + 1, b = a) {
				c = 10;
			}
		})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		auto bbs = fun->getBasicBlocks();
		EXPECT(bbs.size() == 4);

		{
		std::string expected = "" \
			"L0 {\n" \
			"alloca a.0,4 int\n" \
			"a.0 = 0\n" \
			"alloca b.1,4 int\n" \
			"b.1 = a.0\n" \
			"alloca c.2,4 int\n" \
			"c.2 = 10\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}

		{
		std::string expected = "" \
			"L1 {\n" \
			"$0 = b.1<c.2\n" \
			"fjmp $0 L2\n" \
			"}";
		EXPECT_PRINTABLE(bbs[1], expected);
		}

		{
		std::string expected = "" \
			"L3 {\n" \
			"c.2 = 10\n" \
			"$1 = a.0+1\n" \
			"a.0 = $1\n" \
			"b.1 = a.0\n" \
			"goto L1\n" \
			"}";
		EXPECT_PRINTABLE(bbs[2], expected);
		}

		{
		std::string expected = "" \
			"L2 {\n" \
			"}";
		EXPECT_PRINTABLE(bbs[3], expected);
		}
	}

	TEST(Converter, Functions_Basic_Main)
	{
		string str_program{R"(
			void main()
			{
				int a = 0;
				return;
			})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_program);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(manager.getProgram()->getFunctions().size() == 1);
		EXPECT(fun->getName() == "_main");

		auto bbs = fun->getBasicBlocks();
		EXPECT(bbs.size() == 1);

		{
		std::string expected = "" \
			"_main {\n" \
			"alloca a.0,4 int\n" \
			"a.0 = 0\n"
			"ret\n"
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}
	}

	TEST(Converter, Functions_Basic_Main_With_Return)
	{
		string str_program{R"(
			int main()
			{
				return 0;
			})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_program);
		converter.convert();

		auto fun = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(manager.getProgram()->getFunctions().size() == 1);
		EXPECT(fun->getName() == "_main");

		auto bbs = fun->getBasicBlocks();
		EXPECT(bbs.size() == 1);

		{
		std::string expected = "" \
			"_main {\n" \
			"ret 0\n"
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}
	}

	TEST(Converter, Functions_Basic_Calls)
	{
		string str_program{R"(
			int add(int a, int b)
			{
				return a + b;
			}

			int main()
			{
				return add(add(1, 2), 3);
			})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_program);
		converter.convert();

		EXPECT(manager.getProgram()->getFunctions().size() == 2);

		auto main = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(main);
		EXPECT(main->getName() == "_main");

		auto add = analysis::callgraph::findFunction(manager.getProgram(), "_add");
		EXPECT(add);
		EXPECT((*add)->getName() == "_add");

		auto bbs = main->getBasicBlocks();
		EXPECT(bbs.size() == 1);

		{
		std::string expected = "" \
			"_main {\n" \
			"push 3\n" \
			"push 2\n" \
			"push 1\n" \
			"call _add,$1\n" \
			"pop 8\n" \
			"push $1\n" \
			"call _add,$2\n" \
			"pop 8\n" \
			"ret $2\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}

		bbs = (*add)->getBasicBlocks();
		EXPECT(bbs.size() == 1);

		{
		std::string expected = "" \
			"_add {\n" \
			"$0 = a.0+b.1\n" \
			"ret $0\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}
	}

	TEST(Converter, Functions_Recursion)
	{
		string str_program{R"(
			int fib(int n)
			{
				if (n == 0)
				{
					return n;
				}
				if (n == 1)
				{
					return n;
				}

        return fib(n - 1) + fib(n - 2);
			}

			int main()
			{
				return fib(2);
			})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_program);
		converter.convert();

		EXPECT(manager.getProgram()->getFunctions().size() == 2);

		auto main = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(main);
		EXPECT(main->getName() == "_main");

		auto fib = analysis::callgraph::findFunction(manager.getProgram(), "_fib");
		EXPECT(fib);
		EXPECT((*fib)->getName() == "_fib");

		auto bbs = main->getBasicBlocks();
		EXPECT(bbs.size() == 1);

		{
		std::string expected = "" \
			"_main {\n" \
			"push 2\n" \
			"call _fib,$7\n" \
			"pop 4\n" \
			"ret $7\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}

		bbs = (*fib)->getBasicBlocks();
		EXPECT(bbs.size() == 5);

		{
		std::string expected = "" \
			"_fib {\n" \
			"$0 = n.0==0\n" \
			"fjmp $0 L0\n" \
			"}";
		EXPECT_PRINTABLE(bbs[0], expected);
		}

		{
		std::string expected = "" \
			"L2 {\n" \
			"ret n.0\n" \
			"}";
		EXPECT_PRINTABLE(bbs[1], expected);
		}

		{
		std::string expected = "" \
			"L0 {\n" \
			"$1 = n.0==1\n" \
			"fjmp $1 L1\n" \
			"}";
		EXPECT_PRINTABLE(bbs[2], expected);
		}

		{
		std::string expected = "" \
			"L3 {\n" \
			"ret n.0\n" \
			"}";
		EXPECT_PRINTABLE(bbs[3], expected);
		}

		{
		std::string expected = "" \
			"L1 {\n" \
			"$2 = n.0-1\n" \
			"push $2\n" \
			"call _fib,$3\n" \
			"pop 4\n" \
			"$4 = n.0-2\n" \
			"push $4\n" \
			"call _fib,$5\n" \
			"pop 4\n" \
			"$6 = $3+$5\n" \
			"ret $6\n" \
			"}";
		EXPECT_PRINTABLE(bbs[4], expected);
		}
	}

	TEST(Converter, Functions_Mutal_Recursion)
	{
		string str_program{R"(
			int is_odd(int);
			int is_even(int n)
			{
				if (n == 0)
				{
					return 1;
				}
				else
				{
					return is_odd(n - 1);
				}
			}

			int is_odd(int n)
			{
				if (n == 0)
				{
					return 0;
				}
				else
				{
					return is_even(n - 1);
				}
			}

			int main()
			{
				return is_odd(7);
			})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_program);
		converter.convert();

		EXPECT(manager.getProgram()->getFunctions().size() == 3);

		auto main = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(main);
		EXPECT(main->getName() == "_main");

		auto is_odd = analysis::callgraph::findFunction(manager.getProgram(), "_is_odd");
		EXPECT(is_odd);
		EXPECT((*is_odd)->getName() == "_is_odd");

		auto is_even = analysis::callgraph::findFunction(manager.getProgram(), "_is_even");
		EXPECT(is_even);
		EXPECT((*is_even)->getName() == "_is_even");

		analysis::callgraph::CallGraph expected;
		expected.addEdge(main, *is_odd);
		expected.addEdge(*is_even, *is_odd);
		expected.addEdge(*is_odd, *is_even);


		auto actual = analysis::callgraph::getCallGraph(manager.getProgram());
		EXPECT(actual == expected);
	}

  TEST(Converter, Functions_No_Args)
  {
    string str_program{R"(
			int bar()
			{
				return 2;
			}

      void foo(int n)
      {
        int a = n;
      }

      void main()
      {
        foo(bar());
      })"};

    NodeManager manager;
    frontend::Converter converter(manager, str_program);
    converter.convert();

    EXPECT(manager.getProgram()->getFunctions().size() ==3);

    auto main = analysis::callgraph::getMainFunction(manager.getProgram());
    EXPECT(main);
    EXPECT(main->getName() == "_main");

    auto foo = analysis::callgraph::findFunction(manager.getProgram(), "_foo");
    EXPECT(foo);
    EXPECT((*foo)->getName() == "_foo");

		auto bar = analysis::callgraph::findFunction(manager.getProgram(), "_bar");
		EXPECT(bar);
		EXPECT((*bar)->getName() == "_bar");

    auto bbs = main->getBasicBlocks();
    EXPECT(bbs.size() == 1);

    {
    std::string expected = "" \
      "_main {\n" \
      "call _bar,$0\n" \
			"push $0\n" \
      "call _foo\n" \
      "pop 4\n" \
			"ret\n" \
      "}";
    EXPECT_PRINTABLE(bbs[0], expected);
    }

    bbs = (*foo)->getBasicBlocks();
    EXPECT(bbs.size() == 1);

    {
    std::string expected = "" \
      "_foo {\n" \
			"alloca a.1,4 int\n" \
      "a.1 = n.0\n" \
			"ret\n" \
      "}";
    EXPECT_PRINTABLE(bbs[0], expected);
    }

		bbs = (*bar)->getBasicBlocks();
    EXPECT(bbs.size() == 1);

    {
    std::string expected = "" \
      "_bar {\n" \
			"ret 2\n" \
      "}";
    EXPECT_PRINTABLE(bbs[0], expected);
    }
  }

	TEST(MachineOperand, Builders)
	{
		using namespace backend::insn;
		auto op = buildMemOperand(MachineOperand::OR_Ebp, MachineOperand::OS_32Bit, -8);
		EXPECT_PRINTABLE(op, "-8(%ebp)");
	}

	TEST(MachineInsn, Builders)
	{
		using namespace backend::insn;
		auto rhs1 = buildMemOperand(MachineOperand::OR_Ebp, MachineOperand::OS_32Bit, -8);
		auto rhs2 = buildRegOperand(MachineOperand::OR_Ecx, MachineOperand::OS_32Bit);
		auto insn = buildMovInsn(rhs1, rhs2);
		EXPECT_PRINTABLE(insn, "movl -8(%ebp),%ecx");
	}

	TEST(MachineInsn, PushXmm0)
	{
		using namespace backend::insn;
		auto rhs1 = buildRegOperand(MachineOperand::OR_Xmm0, MachineOperand::OS_32Bit);
		auto insn = buildPushTemplate(rhs1);
		EXPECT_PRINTABLE(insn, "subl $0x4,%esp\nmovss %xmm0,0(%esp)");
	}

	TEST(Backend, StackFrame)
	{
		string str_program{R"(
		void foo(int n, int m)
		{
			int a = n;
			int b = 2;
			int c = 3;
		}

		int main()
		{
			int a = 1;
			foo(a, 2);
			return 0;
		})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_program);
		converter.convert();

		EXPECT(manager.getProgram()->getFunctions().size() == 2);

		auto main = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(main);
		EXPECT(main->getName() == "_main");

		auto foo = analysis::callgraph::findFunction(manager.getProgram(), "_foo");
		EXPECT(foo);
		EXPECT((*foo)->getName() == "_foo");


		/**
		* Stack of function main
		*
		* high address:
		* ...
		* ret addr
		* old ebp <-- ebp points here
		* a.5
		* ...
		* low address:
		*/
		auto mainFrame = backend::memory::getStackFrame(main);
		auto locals = core::analysis::controlflow::getAllVars(main, true);
		int offset = -4;

		for(auto var : locals) {
			EXPECT(offset == mainFrame->getRelativeOffset(var));
			offset -= 4;
		}

		/**
		* Stack of function foo
		*
		* high address:
		* m.0
		* n.0
		* ret addr
		* old ebp <-- ebp points here
		* a.2
		* b.3
		* c.4
		* ...
		* low address:
		*/
		auto fooFrame = backend::memory::getStackFrame(*foo);

		auto params = (*foo)->getParameters();
		locals = core::analysis::controlflow::getAllVars(*foo, true);

		// remove all params from the locals set
		for (auto param : params)
			locals.erase(param);

		offset = -4;
		for(auto var : locals) {
			EXPECT(offset == fooFrame->getRelativeOffset(var));
			offset -= 4;
		}

		offset = 8;
		for(auto var : params) {
			EXPECT(offset == fooFrame->getRelativeOffset(var));
			offset += 4;
		}
	}

	TEST(Utils, ColorGraph_ThreeColorable)
	{
		using namespace graph::color;
		ColorGraph<int> graph;

		std::vector<ColorGraph<int>::vertex_type> vertices(5);
		for (unsigned i = 0 ; i < vertices.size(); ++i) vertices[i] = makeVertex<int>(i);

		graph.addEdge(vertices[0], vertices[1]);
		graph.addEdge(vertices[0], vertices[2]);
		graph.addEdge(vertices[2], vertices[3]);
		graph.addEdge(vertices[1], vertices[3]);
		graph.addEdge(vertices[1], vertices[4]);
		graph.addEdge(vertices[2], vertices[4]);
		graph.addEdge(vertices[3], vertices[4]);

		auto mapping = getColorMappings(graph, 3);
		EXPECT(*mapping[4].vertex == *vertices[4] && mapping[4].color == 0);
		EXPECT(*mapping[3].vertex == *vertices[3] && mapping[3].color == 1);
		EXPECT(*mapping[2].vertex == *vertices[2] && mapping[2].color == 2);
		EXPECT(*mapping[1].vertex == *vertices[1] && mapping[1].color == 2);
		EXPECT(*mapping[0].vertex == *vertices[0] && mapping[0].color == 0);
	}

	TEST(Utils, ColorGraph_NotTwoColorable)
	{
		using namespace graph::color;
		ColorGraph<int> graph;

		std::vector<ColorGraph<int>::vertex_type> vertices(3);
		for (unsigned i = 0 ; i < vertices.size(); ++i) vertices[i] = makeVertex<int>(i);

		graph.addEdge(vertices[0], vertices[1]);
		graph.addEdge(vertices[1], vertices[2]);
		graph.addEdge(vertices[2], vertices[0]);

		auto mapping = getColorMappings(graph, 2);
		EXPECT(*mapping[0].vertex == *vertices[0] && mapping[0].color == -1);
		EXPECT(*mapping[1].vertex == *vertices[1] && mapping[1].color == 1);
		EXPECT(*mapping[2].vertex == *vertices[2] && mapping[2].color == 0);
	}

	TEST(Utils, ColorGraph_TwoColorable)
	{
		using namespace graph::color;
		ColorGraph<int> graph;

		std::vector<ColorGraph<int>::vertex_type> vertices(4);
		for (unsigned i = 0 ; i < vertices.size(); ++i) vertices[i] = makeVertex<int>(i);

		graph.addEdge(vertices[0], vertices[1]);
		graph.addEdge(vertices[1], vertices[2]);
		graph.addEdge(vertices[2], vertices[3]);
		graph.addEdge(vertices[3], vertices[0]);

		auto mapping = getColorMappings(graph, 2);
		EXPECT(*mapping[0].vertex == *vertices[0] && mapping[0].color == 1);
		EXPECT(*mapping[1].vertex == *vertices[1] && mapping[1].color == 0);
		EXPECT(*mapping[2].vertex == *vertices[2] && mapping[2].color == 1);
		EXPECT(*mapping[3].vertex == *vertices[3] && mapping[3].color == 0);
	}

	TEST(Utils, PtrSet)
	{
		NodeManager manager;
		auto type = manager.buildBasicType(Type::TI_Int);

		VariableSet setA;
		setA.insert(manager.buildVariable(type, "a"));
		setA.insert(manager.buildVariable(type, "a"));
		EXPECT(setA.size() == 1);

		VariableSet setB;
		setB.insert(manager.buildVariable(type, "a"));
		EXPECT(setB.size() == 1);

		auto setC = algorithm::set_difference(setA, setB);
		EXPECT(setC.size() == 0);

		auto setD = algorithm::set_difference(setC, setB);
		EXPECT(setD.size() == 0);

		auto setE = algorithm::set_union(setA, setB);
		EXPECT(setE.size() == 1);
	}

	TEST(Arithmetic, Diophantine)
	{
		NodeManager manager;
		using namespace core::arithmetic::formula;

		auto type = manager.buildBasicType(Type::TI_Int);
		auto var = manager.buildVariable(type, "i");
		// X[2*i+3] = X[2*i] + 50;
		// a=2, b=3, c=2, d=0 and GCD(a,c)=2 and (d-b) is -3
		auto term = makeTerm(Term::OpType::ADD,
			makeTerm(Term::OpType::MUL,
				makeTerm(manager.buildIntConstant(2)),
				makeTerm(var)),
			makeTerm(Term::OpType::MUL,
				makeTerm(manager.buildIntConstant(2)),
				makeTerm(var)));

		auto res = trySolveDiophantine(manager, term, manager.buildIntConstant(-3));
		EXPECT(!res);
		// however with c == 4, it must produce a "conflict"
		res = trySolveDiophantine(manager, term, manager.buildIntConstant(-4));
		EXPECT(res);
		EXPECT(*res == 2);
	}

	TEST(Arithmetic, Simplify)
	{
		NodeManager manager;
		using namespace core::arithmetic::formula;
		// (2*6) + (3*4) = 24
		auto term = makeTerm(Term::OpType::ADD,
			makeTerm(Term::OpType::MUL,
				makeTerm(manager.buildIntConstant(2)),
				makeTerm(manager.buildIntConstant(6))),
			makeTerm(Term::OpType::MUL,
				makeTerm(manager.buildIntConstant(3)),
				makeTerm(manager.buildIntConstant(4))));
		auto result = simplify(manager, term);
		EXPECT(result->isValue());
		EXPECT(core::arithmetic::getValue<int>(result->getValue()) == 24);
	}

	TEST(Analysis, Loop)
	{
		string str_compound{R"(
		int main()
		{
			int a[20];
			for (int i = 1; i < 5; i = i + 1)
			{
				a[(2*i)] = a[2] + a[(2*i)+3];
			}
			return 0;
		})"};

		NodeManager manager;
		frontend::Converter converter(manager, str_compound);
		converter.convert();

		passes::PassSequence(manager, passes::makePass<passes::InlineAssignmentsPass>(manager)).apply();

		auto main = analysis::callgraph::getMainFunction(manager.getProgram());
		EXPECT(main);

		auto loops = analysis::loop::findLoops(manager, main);
		EXPECT(loops.size() == 1);

		const auto& vars = loops[0]->getInductionVariables();
		EXPECT(vars.size() == 1);
		EXPECT_PRINTABLE(*vars.begin(), "i.1");

		const auto& stmts = loops[0]->getStatements();
		EXPECT(stmts.size() == 1);

		const auto& subs = stmts[0]->getSubscripts();
		EXPECT(subs.size() == 3);

		EXPECT(analysis::loop::hasNoDependency(manager, subs[0], subs[2]));
		EXPECT(!analysis::loop::hasNoDependency(manager, subs[0], subs[1]));
	}
}

void test_core() {
	TestSuite::get().runTests();
}
