#pragma once
#include "common.h"
class BuildUniqueID
{
public:
	int colorNum = 100;
	vector<int> FuncNestList;
	vector<Function*> FuncList;
	vector<pair<int, int>> FuncIDpair;
	vector<pair<Function*, int>> Func2ThreadList;

	Module* AllocThread(Module* mod);
protected:
	int** Graph;
	int** CallMatrix;
	int** AccessMatrix;
	int colorCur = 0;
	int color[1000] = { 0 };
	int VecNum = 0;
	bool buildIdSuccess = false;
	vector<Instruction*> DepList;

	void backtrack(int cur);
	void BuildAccessMatrix();
	void GetUserInst(Value * g_value);
	void DynamicPtrAnalysis(Module * mod);
	void GenUndirectGraph(Module * mod);
	bool ok(int c);
	bool ModifyModuleVar(Module * mod, int threadNum, int threshold);
	BOOL BuildCallMatrix();
	vector<Function*> AllFuncList;
};