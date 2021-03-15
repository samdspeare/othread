#pragma once
#include "common.h"
class RetriveBigStruct
{
public:
	vector<Type*> TypeList;
	vector<Function*> FuncList;
	vector<pair<int, int>> FuncIDpair;

	BOOL ConstructFunctionArgs(Module * mod);
protected:
	int ReturnIndex;
	StructType* BigStructType;
	vector<Type*> BigStructFiled;
	GlobalVariable* BigGV;

	Function* ClearAllArgs(Function * Func);
	bool GetStructValue(Function * Func);
	bool SubstitueOldfnWithNewfn(Function * OldFun, Function * NewFunc);
	void ConstructArgStruct(Module* mod);
};
