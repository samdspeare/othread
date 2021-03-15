#pragma once
#include "common.h"
#define MAX 100
class CalcFuncNestNum
{
public:
	
	vector<int> FuncNestList;
	vector<Function*> FuncList;
	
	void GetNestInfo(Module * mod);
protected:
	struct BlockInfo
	{
		BasicBlock* BB;
		int LoopNum;
	};

	int **CallMatrix;
	int FuncNestNum = 1;
	vector<Function*> AllFuncList;
	vector<BlockInfo> BlockInfoList;
	void OptiFuncListMin();
	void OptiFuncListMax();
	void CheckFuncDepth(int * distance);
	void BlockLoopNest(Module * mod);
	void BuildCallMatrix();
	int GetFuncComplexity(Module * mod);
	int* Dijikstra(int ** G, int n, int startnode);
};
