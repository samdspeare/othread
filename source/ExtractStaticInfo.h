#pragma once
#include "common.h"
class ExtractStaticInfo
{
public:
	int **CallMatrix;

	BOOL GenerateGraph(Module* mod);
	vector<Function*> FuncList;
protected:
	int** AccessMatrix;

	void CheckFuncList();
	void FuncListCloning();
	void BuildAccessMatrix();
	void TransConstantExpr2Inst(Function * Func);
	void SubDllFuncWithPackage(vector<Function*> IllegalFuncList, vector<Function*> RecordIllegalPath);
	BOOL UpdateGraph();
	BOOL BuildCallMatrix();
	BOOL FuncListPruning();
	BOOL InitialFuncList(Module * mod);
	BOOL IsRecurseFunction(Function * Func);
	int* InitialOverlapFunc();
	Function * PackageDllFunc(Function * OldFun);
	Function * CloneFunction(Function * OldFun, string CloneID);
};
