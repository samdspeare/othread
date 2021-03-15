/*****************************************************************************
Filename: ExtractStaticInfo.cpp
Date    : 2020-02-17 8:42:51
Description: Calculate maximum execution complexity and call depth of function:
			 1. Calculate the loop complexity of basic blocks
			 2. Generate a weighted directed graph of function calls, the weight is the loop complexity of the basic block of the call site
			 3. Generate a weighted directed graph of function depth, The weight is a boolean variable, which identifies the calling relationship
			 4. Dijkstra longest path algorithm
*****************************************************************************/
#include "stdafx.h"
#include "CalcFuncNestNum.h"

/// \Generate Function Call Graph
void CalcFuncNestNum::BuildCallMatrix()
{
	int size = AllFuncList.size();
	CallMatrix = (int **)malloc(sizeof(int) * size);
	for (int j = 0; j < size; j++)
	{
		CallMatrix[j] = (int *)malloc(sizeof(int) * size);
	}
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			CallMatrix[i][j] = -999999;
		}
	}
	for (int i = 0; i < AllFuncList.size(); i++)
	{
		for (Value::user_iterator useri = AllFuncList[i]->user_begin(), usere = AllFuncList[i]->user_end(); useri != usere; useri++)
		{
			Value* tmpuser = dyn_cast<Value>(*useri);
			if (Instruction* inst = dyn_cast<Instruction>(tmpuser))
			{
				int temp = 0;
				for (int j = 0; j < BlockInfoList.size(); j++)
				{
					if (BlockInfoList[j].BB == inst->getParent())
					{
						temp = BlockInfoList[j].LoopNum;
						break;
					}
				}
				if (CallMatrix[IfFuncInlist(inst->getParent()->getParent(), AllFuncList)][i] < temp)
				{
					CallMatrix[IfFuncInlist(inst->getParent()->getParent(), AllFuncList)][i] = temp;
				}
			}
		}
	}
}

/// \Calculate the nesting complexity of each basic block 
/// \Based on LLVM IR block name
/// \Approximately accurate
void CalcFuncNestNum::BlockLoopNest(Module * mod)
{
	for (Function &F : *mod)
	{
		int i = 0;
		for (BasicBlock &B : F)
		{
			if (B.getName().find("while.cond") != string::npos || B.getName().find("for.cond") != string::npos || B.getName().find("do.body") != string::npos)
			{
				i++;
			}
			BlockInfo tempBlock;
			tempBlock.BB = &B;
			tempBlock.LoopNum = i;
			BlockInfoList.push_back(tempBlock);
			if ((B.getName().find("while.end") != string::npos || B.getName().find("for.end") != string::npos || B.getName().find("do.end") != string::npos) && i != 0)
			{
				i--;
			}
		}
	}
}

/// \Dijikstra Alogrithm
int* CalcFuncNestNum::Dijikstra(int **CallMatrix, int n, int startnode)
{
	int count, maxdistance, nextnode, i, j;
	int* distance = (int *)malloc(sizeof(int) * n);
	int* pred = (int *)malloc(sizeof(int) * n);
	int* visited = (int *)malloc(sizeof(int) * n);

	for (i = 0; i < n; i++)
	{
		distance[i] = CallMatrix[startnode][i];
		pred[i] = startnode;
		visited[i] = 0;
	}
	visited[startnode] = 1;
	count = 1;
	while (count < n - 1)
	{
		maxdistance = -999999;
		for (i = 0; i < n; i++)
		{
			if (distance[i] > maxdistance && !visited[i])
			{
				maxdistance = distance[i];
				nextnode = i;
			}
		}
		visited[nextnode] = 1;
		for (i = 0; i < n; i++)
		{
			if (maxdistance + CallMatrix[nextnode][i] > distance[i])
			{
				distance[i] = maxdistance + CallMatrix[nextnode][i];
				pred[i] = nextnode;
			}
		}
		count++;
	}
	return distance;
}


/// \Check the depth of function
/// \Avoid generating too many "while" threads to affect the CPU
void CalcFuncNestNum::CheckFuncDepth(int* distance)
{
	for (int i = 0; i < AllFuncList.size(); i++)
	{
		for (int j = 0; j < AllFuncList.size(); j++)
		{
			if (CallMatrix[i][j] != -999999)
			{
				CallMatrix[i][j] = 1;
			}
		}
	}
	int flag = 0;
	vector<Function*> depthList;
	vector<Function*> tempFuncList;
	while (1)
	{
		flag = 0;
		int* depth = Dijikstra(CallMatrix, AllFuncList.size(), 0);
		for (int i = 0; i < FuncList.size(); i++)
		{
			if (distance[IfFuncInlist(FuncList[i], AllFuncList)] > 3 && depth[IfFuncInlist(FuncList[i], AllFuncList)] > 4 && IfFuncInlist(FuncList[i], depthList) == -1)
			{
				for (int j = 0; j < AllFuncList.size(); j++)
				{
					CallMatrix[j][i] == -999999;
				}
				depthList.push_back(FuncList[i]);
				flag = 1;
				break;
			}
		}
		if (flag == 0)
		{
			break;
		}
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (IfFuncInlist(FuncList[i], depthList) != -1)
		{
			continue;
		}
		tempFuncList.push_back(FuncList[i]);
	}
	FuncList = tempFuncList;
}

/// \Calculate the number of nested calling complexity of the function, 
/// \Based on Dijikstra
int CalcFuncNestNum::GetFuncComplexity(Module* mod)
{
	Function* main = mod->getFunction("main");
	if (main == nullptr)
	{
		main = mod->getFunction("\x01_WinMain@16");
	}
	AllFuncList.push_back(main);				// take main function as start point
	for (Function &F : *mod)
	{
		if (&F != main)
		{
			AllFuncList.push_back(&F);
		}
	}
	BuildCallMatrix();
	int* distance = Dijikstra(CallMatrix, AllFuncList.size(), 0);
	CheckFuncDepth(distance);
	for (int i = 0; i < FuncList.size(); i++)
	{
		FuncNestList.push_back(distance[IfFuncInlist(FuncList[i], AllFuncList)]);
	}
	return 0;
}

/// \Sort functions according to call complexity from smallest to largest
void CalcFuncNestNum::OptiFuncListMin()
{
	for (int j = 0; j < FuncList.size() - 1; j++)
	{
		for (int i = 0; i < FuncList.size() - 1 - j; i++)
		{
			if (FuncNestList[i] > FuncNestList[i + 1])
			{
				int tempNum = FuncNestList[i];
				FuncNestList[i] = FuncNestList[i + 1];
				FuncNestList[i + 1] = tempNum;


				Function* tempFunc = FuncList[i];
				FuncList[i] = FuncList[i + 1];
				FuncList[i + 1] = tempFunc;
			}
		}
	}
}


/// \Sort functions according to call complexity from largest to smallest
void CalcFuncNestNum::OptiFuncListMax()
{
	for (int j = 0; j < FuncList.size() - 1; j++)
	{
		for (int i = 0; i < FuncList.size() - 1 - j; i++)
		{
			if (FuncNestList[i] < FuncNestList[i + 1])
			{
				int tempNum = FuncNestList[i];
				FuncNestList[i] = FuncNestList[i + 1];
				FuncNestList[i + 1] = tempNum;


				Function* tempFunc = FuncList[i];
				FuncList[i] = FuncList[i + 1];
				FuncList[i + 1] = tempFunc;
			}
		}
	}
}



/// \Get information of nested loop, and sort functions by complexity 
void CalcFuncNestNum::GetNestInfo(Module* mod)
{
	BlockLoopNest(mod);
	GetFuncComplexity(mod);
	OptiFuncListMax();

}
