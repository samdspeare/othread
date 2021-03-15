/*****************************************************************************
Filename: common.cpp
Date    : 2020-02-12 09:13:16
Description: Commonly used functions
*****************************************************************************/
#include "stdafx.h"
#include "common.h"

/// \Check if number in list
/// \Return -1 means there is no target number in list, Return others means the order of target number in list
int IfIntInlist(int num, vector<int> SearchList)
{
	int flag = -1;
	for (int i = 0; i < SearchList.size(); i++)
	{
		if (num == SearchList[i])
		{
			flag = i;
			break;
		}
	}
	return flag;
}

/// \Check if function in list
/// \Return -1 means there is no target function in list, Return others means the order of target function in list
int IfFuncInlist(Function* Func, vector<Function*> SearchList)
{
	int flag = -1;
	for (int i = 0; i < SearchList.size(); i++)
	{
		if (Func == SearchList[i])
		{
			flag = i;
			break;
		}
	}
	return flag;
}

/// \Check if basic block in list
/// \Return -1 means there is no target block in list, Return others means the order of target block in list
int IfBlockInlist(BasicBlock* ins, vector<BasicBlock*> SearchIns)
{
	BOOL flag = FALSE;
	for (int i = 0; i < SearchIns.size(); i++)
	{
		if (ins == SearchIns[i])
		{
			flag = TRUE;
			break;
		}
	}
	return flag;
}

/// \Check if type in list
/// \Return -1 means there is no target type in list, Return others means the order of type function in list
/// parameter Start means starting point for searching in the list
int IfTypeInlist(int Start, Type* Ty, vector<Type*> SearchList)
{
	int flag = -1;
	for (int i = Start; i < SearchList.size(); i++)
	{
		if (Ty == SearchList[i])
		{
			flag = i;
			break;
		}
	}
	return flag;
}

/// \Delete specified function from list
vector<Function*> EraseFuncFromList(Function* Func, vector<Function*> FuncList)
{
	vector<Function*> tempFuncList;
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (FuncList[i] == Func)
		{
			continue;
		}
		tempFuncList.push_back(FuncList[i]);
	}
	return tempFuncList;
}




/// \Return a matrix of size * size, and assign the initial value 0
int** MallocGraph(int size)
{
	int** Graph = (int **)malloc(sizeof(int) * size);
	for (int j = 0; j < size; j++)
	{
		Graph[j] = (int *)malloc(sizeof(int) * size);
	}
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			Graph[i][j] = 0;
		}
	}
	return Graph;
}

/// \Copy the value of matrix A to matrix B and return the pointer of matrix B
int** CopyGraph(int size, int** sourceGraph)
{
	int** Graph = (int **)malloc(sizeof(int) * size);
	for (int j = 0; j < size; j++)
	{
		Graph[j] = (int *)malloc(sizeof(int) * size);
	}
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			Graph[i][j] = sourceGraph[i][j];
		}
	}
	return Graph;
}

/// \Matrix multiplication
int** MatrixMulti(int** tempMatrixA, int** tempMatrixB, int MatrixLength)
{
	int** pathMatrix = MallocGraph(MatrixLength);
	for (int i = 0; i < MatrixLength; i++)
	{
		for (int k = 0; k < MatrixLength; k++)
		{
			int r = tempMatrixA[i][k];
			for (int j = 0; j < MatrixLength; j++)
			{
				pathMatrix[i][j] += r*tempMatrixB[k][j];
				if (pathMatrix[i][j] != 0)
				{
					pathMatrix[i][j] = 1;
				}
			}
		}
	}
	return pathMatrix;
}

/// \Matrix addition
int** MatrixPlus(int** MatrixA, int** MatrixB, int MatrixLength)
{
	int** AccessMatrix = MallocGraph(MatrixLength);
	for (int i = 0; i < MatrixLength; i++)
	{
		for (int j = 0; j < MatrixLength; j++)
		{
			AccessMatrix[i][j] = MatrixA[i][j] + MatrixB[i][j];
		}
	}
	return AccessMatrix;
}

/// \Determine whether the target function is a callback function
bool isCallBackFnType(Function* f)
{
	for (llvm::Value::user_iterator useri = f->user_begin(), usere = f->user_end(); useri != usere; useri++)
	{
		if (Instruction* inst = dyn_cast<Instruction>(*useri))
		{
			if (inst->getOpcode() == Instruction::Call || inst->getOpcode() == Instruction::Invoke)
			{
				CallSite CS(inst);
				if (CS && CS.getCalledValue() != f)
					return true;
			}
			else if (inst->getOpcode() != Instruction::Call && inst->getOpcode() != Instruction::Invoke)
				return true;
		}
		else
			return true;
	}
	return false;
}


