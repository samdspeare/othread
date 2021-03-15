/*****************************************************************************
Filename: BuildUniqueID.cpp
Date    : 2020-02-19 16:21:15
Description: Assign functions to threads:
			 1. Extend the access matrix to generate the adjacency matrix of undirected graph
			 2. Distribution functions based on coloring principle
*****************************************************************************/
#include "stdafx.h"
#include "BuildUniqueID.h"
#include <math.h>

/// \Modify the constants in the template file
bool BuildUniqueID::ModifyModuleVar(Module * mod, int threadNum, int threshold)
{
	llvm::IRBuilder<> builder(mod->getContext());
	GlobalVariable *threadNumForOthread = NULL;
	GlobalVariable *thresholdForOthread = NULL;
	for (Module::global_iterator gi = mod->global_begin(); gi != mod->global_end(); gi++)
	{
		GlobalVariable* g_value = (GlobalVariable*)gi;
		if (g_value->getName().find("threadNumForOthread") != string::npos)
		{
			threadNumForOthread = g_value;
		}
		if (g_value->getName().find("thresholdForOthread") != string::npos)
		{
			thresholdForOthread = g_value;
		}
	}
	if (thresholdForOthread == NULL || threadNumForOthread == NULL)
	{
		return false;
	}
	else
	{
		thresholdForOthread->setInitializer(builder.getInt32(threshold));
		threadNumForOthread->setInitializer(builder.getInt32(threadNum));
	}

}

/// \Generate Function Call Graph
BOOL BuildUniqueID::BuildCallMatrix()
{
	CallMatrix = MallocGraph(AllFuncList.size());
	if (CallMatrix == NULL)
	{
		errs() << "Memory allocation failed\n";
		return false;
	}
	for (int i = 0; i < AllFuncList.size(); i++)
	{
		for (Value::user_iterator useri = AllFuncList[i]->user_begin(), usere = AllFuncList[i]->user_end(); useri != usere; useri++)
		{
			Value* tmpuser = dyn_cast<Value>(*useri);
			if (Instruction* inst = dyn_cast<Instruction>(tmpuser))
			{
				CallMatrix[IfFuncInlist(inst->getParent()->getParent(), AllFuncList)][i] = 1;

			}
		}
	}
	return true;
}

/// \Generate Function Access Graph		Access = CallMatrix^1 กล CallMatrix^2 กล CallMatrix^3 กล กญ กล CallMatrix^n
void BuildUniqueID::BuildAccessMatrix()
{
	int MatrixLength = AllFuncList.size();

	int** PowerMatrix = CopyGraph(MatrixLength, CallMatrix);
	AccessMatrix = CopyGraph(MatrixLength, CallMatrix);

	for (int i = 2; i <= MatrixLength; i++)
	{
		PowerMatrix = MatrixMulti(PowerMatrix, CallMatrix, MatrixLength);
		AccessMatrix = MatrixPlus(PowerMatrix, AccessMatrix, MatrixLength);
	}
	for (int i = 0; i < AllFuncList.size(); i++)
	{
		for (int j = 0; j < AllFuncList.size(); j++)
		{
			if (AccessMatrix[i][j] != 0)
			{
				AccessMatrix[i][j] = 1;
			}
		}
	}
}

void BuildUniqueID::GetUserInst(Value* g_value)
{
	for (Value::user_iterator useri = g_value->user_begin(), usere = g_value->user_end(); useri != usere; useri++)
	{
		if (StoreInst* inst = dyn_cast<StoreInst>(*useri))
		{
			DepList.push_back(inst);	
		}
		if (CallInst* inst = dyn_cast<CallInst>(*useri))
		{
			DepList.push_back(inst);
		}
		GetUserInst(*useri);
	}
}

/// \Handling dynamic pointer analysis
void BuildUniqueID::DynamicPtrAnalysis(Module* mod)
{
	vector<GlobalVariable*> GVList;
	for (Module::global_iterator gi = mod->global_begin(); gi != mod->global_end(); gi++)
	{
		GlobalVariable* g_value = (GlobalVariable*)gi;
		GVList.push_back(g_value);
	}

	for (int i = 0; i < GVList.size(); i++)
	{
		DepList.clear();
		GetUserInst(GVList[i]);

		vector<Function*> srcList;
		vector<Function*> tarList;

		for (int j = 0; j < DepList.size(); j++)
		{
			if (StoreInst* inst = dyn_cast<StoreInst>(DepList[j]))
			{
				for (Use &U : inst->operands())
				{
					Value* tmpuser = &*(U.get());
					if (Function* Func = dyn_cast<Function>(tmpuser))
					{
						tarList.push_back(Func);
					}
				}
			}
			if (CallInst* inst = dyn_cast<CallInst>(DepList[j]))
			{
				if (inst->getCalledFunction() == NULL)		//Is called by dynamic ptr not function
				{
					srcList.push_back(inst->getParent()->getParent());
				}

			}
		}
		for (int m = 0; m < tarList.size(); m++)
		{
			for (int n = 0; n < srcList.size(); n++)
			{
				int parent = IfFuncInlist(srcList[n], AllFuncList);
				int son = IfFuncInlist(tarList[m], AllFuncList);
				if (son != -1 && parent != -1)
				{
					CallMatrix[parent][son] = 1;
				}
			}
		}
	}
}

/// \Convert directed graph to undirected graph
void BuildUniqueID::GenUndirectGraph(Module* mod)
{
	for (Function &F : *mod)
	{
		AllFuncList.push_back(&F);
	}
	BuildCallMatrix();
	DynamicPtrAnalysis(mod);
	BuildAccessMatrix();
	
	for (int i = 0; i < AllFuncList.size(); i++)
	{
		for (int j = 0; j < AllFuncList.size(); j++)
		{
			if (AccessMatrix[i][j] == 1)
			{
				int parent = IfFuncInlist(AllFuncList[i], FuncList);
				int son = IfFuncInlist(AllFuncList[j], FuncList);
				if (son != -1 && parent != -1)
				{
					Graph[parent + 1][son + 1] = 1;
					Graph[son + 1][parent + 1] = 1;
				}
			}
		}
	}
}

/// \Backtracking method, generate the corresponding relationship between functions and threads
/// \complex functions are allocated to threads with small ID
bool BuildUniqueID::ok(int c)
{
	for (int k = 1; k <= VecNum; k++)
	{
		if (Graph[c][k]==true && color[c] == color[k])
		{
			return false;
		}
	}
	return true;
}
void BuildUniqueID::backtrack(int cur)
{
	if (buildIdSuccess)
	{
		return;
	}
	if (cur>VecNum)
	{
		buildIdSuccess = true;
		for (int i = 1; i <= VecNum; i++)
		{		
			Func2ThreadList.push_back(make_pair(FuncList[i - 1], color[i] - 1));	
		}
		return;
		
	}
	else
	{
		if (FuncNestList[cur - 1] < 3)			
		{
			for (int i = colorNum; i >= 1; i--)
			{
				color[cur] = i;
				if (ok(cur))
				{
					backtrack(cur + 1);
					if (buildIdSuccess)
					{
						return;
					}
				}
				color[cur] = 0;
			}
		}
		else										//Complex functions are allocated from front to back
		{
			for (int i = 1; i <= colorNum; i++)
			{
				color[cur] = i;
				if (ok(cur))
				{
					backtrack(cur + 1);
					if (buildIdSuccess)
					{
						return;
					}
				}
				color[cur] = 0;
			}
		}
		
	}
}


/// \Allocate functions in threads
Module* BuildUniqueID::AllocThread(Module * mod)
{
	Graph = MallocGraph(FuncList.size() + 1);	
	GenUndirectGraph(mod);
	VecNum = FuncList.size();

	errs() << "Start to allocate the function. \n"
		   << "If this stage is running for a long time, please enter a larger \"threadnum\" in the input parameter..." << "\n";
	backtrack(1);

	errs() << "Finish to allocate the function. \n";
	int MaxID = 0;
	int MinID = 1000;
	for (int i = 0; i < Func2ThreadList.size(); i++)
	{
		if (FuncNestList[i] >= 3 && Func2ThreadList[i].second + 1 > MaxID)
		{
			MaxID = Func2ThreadList[i].second;
		}
		if (FuncNestList[i] < 3 && Func2ThreadList[i].second + 1 < MinID)
		{
			MinID = Func2ThreadList[i].second;
		}
	}
	int maxColNum = 0;
	for (int i = 0; i < Func2ThreadList.size(); i++)
	{
		if (FuncNestList[i] < 3)
		{
			Func2ThreadList[i].second = Func2ThreadList[i].second + (MaxID - MinID + 2);
		}
		if (Func2ThreadList[i].second > maxColNum)
		{
			maxColNum = Func2ThreadList[i].second;
		}
		FuncIDpair.push_back(make_pair(Func2ThreadList[i].second, i));
	}
	errs() << "Threads that id <= "<< MaxID << " will execute as efficiency-first" << "\n";
	ModifyModuleVar(mod, maxColNum + 1, MaxID + 1);
	return mod;
}