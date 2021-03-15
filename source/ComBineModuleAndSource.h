#pragma once
#include "common.h"
class ComBineModuleAndSource
{
public:
	vector<int> FuncNestList;
	vector<Function*> FuncList;
	vector<pair<int, int>> FuncIDpair;

	BOOL GenThreadObject(Module *mod);
protected:
	Function* SubOldCall;
	Function* MultiThread;
	Function* ResumeForOthread;
	Function* SuspendForOthread;
	Function* printNumForOthread;
	GlobalVariable* FunctionGrgph;
	
	void GetModuleVar(Module * mod);
	void SumSubCallNum(Function * main);
	BOOL AddSubOldCall(Function * OldFun, Function * NewFunc, int tempSignal, int Index);
};
