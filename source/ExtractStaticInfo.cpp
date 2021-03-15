/*****************************************************************************
Filename: ExtractStaticInfo.cpp
Date    : 2020-02-15 12:35:44
Description: Extract static information of program:
			 1. Construct function call matrix and reachability matrix
			 2. Exclude recurse functions\initial functions\template functions\... from call matrix
			 3. Clone overlap functions, overlap function means one function called by different execution trajectory
*****************************************************************************/
#include "stdafx.h"
#include "ExtractStaticInfo.h"

/// \Replace ConstantExpr with Instructions Based on Fixed Point Theory
void ExtractStaticInfo::TransConstantExpr2Inst(Function *Func)			
{
	while (1)
	{
		int flag = 0;
		for (BasicBlock &Block : *Func)
		{
			for (Instruction &I : Block)
			{
				for (int k = 0; k < I.getNumOperands(); k++)
				{
					if (ConstantExpr* CExpr = dyn_cast<ConstantExpr>(I.getOperand(k)))		
					{
						flag = 1;
						Instruction* CE2Inst = CExpr->getAsInstruction();
						I.replaceUsesOfWith(CExpr, CE2Inst);
						CE2Inst->insertBefore(&I);
					}
				}
			}
		}
		if (flag == 0)
			break;
	}
}

/// \Generate Function Call Graph
BOOL ExtractStaticInfo::BuildCallMatrix()
{
	CallMatrix = MallocGraph(FuncList.size());
	if (CallMatrix == NULL)
	{
		errs() << "Memory allocation failed\n";
		return false;
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		for (Value::user_iterator useri = FuncList[i]->user_begin(), usere = FuncList[i]->user_end(); useri != usere; useri++)
		{
			Value* tmpuser = dyn_cast<Value>(*useri);
			if (Instruction* inst = dyn_cast<Instruction>(tmpuser))
			{
				if (IfFuncInlist(inst->getParent()->getParent(), FuncList) != -1)
				{
					CallMatrix[IfFuncInlist(inst->getParent()->getParent(), FuncList)][i] = 1;
				}

			}
		}
	}
	return true;
}

/// \Generate Function Access Graph		Access = CallMatrix^1 กล CallMatrix^2 กล CallMatrix^3 กล กญ กล CallMatrix^n
void ExtractStaticInfo::BuildAccessMatrix()
{
	int MatrixLength = FuncList.size();

	int** PowerMatrix = CopyGraph(MatrixLength,CallMatrix);
	AccessMatrix = CopyGraph(MatrixLength,CallMatrix);

	for (int i = 2; i <= MatrixLength; i++)
	{
		PowerMatrix = MatrixMulti(PowerMatrix, CallMatrix, MatrixLength);
		AccessMatrix = MatrixPlus(PowerMatrix, AccessMatrix, MatrixLength);
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		for (int j = 0; j < FuncList.size(); j++)
		{
			if (AccessMatrix[i][j] != 0)
			{
				AccessMatrix[i][j] = 1;
			}
		}
	}
}

/// \Package Function With Fixed Parameters Passing Forms such as DllImportFunction
Function* ExtractStaticInfo::PackageDllFunc(Function * OldFun)
{
	FunctionType* Fnty = (FunctionType*)OldFun->getValueType();
	Type *Result = Fnty->getReturnType();
	int ParamNum = Fnty->getNumParams();
	vector< Type* > params;
	params.clear();
	for (int i = 0; i < ParamNum; i++)
	{
		params.push_back(Fnty->getParamType(i));
	}
	FunctionType* NewFnTy = FunctionType::get(Result, params, OldFun->isVarArg());
	Function* NewFunc = Function::Create(NewFnTy, OldFun->getLinkage(), OldFun->getName() + "_Package");  //Create New Function
	NewFunc->setCallingConv(OldFun->getCallingConv());  

	OldFun->getParent()->getFunctionList().push_back(NewFunc);
	OldFun->replaceAllUsesWith(NewFunc);
	BasicBlock* entryblock = BasicBlock::Create(OldFun->getParent()->getContext(), "entry", NewFunc, 0);

	vector<Value*> args;
	int i = 0;
	for (Function::arg_iterator argi = NewFunc->arg_begin(), arge = NewFunc->arg_end(); argi != arge; argi++)
	{
		Argument* arg = &*argi;
		arg->setName("PackArg" + to_string(i));
		args.push_back(arg);
		i = i + 1;
	}
	if (NewFunc->getReturnType() == Type::getVoidTy(OldFun->getParent()->getContext()))
	{
		CallInst* CallAtomFunc = CallInst::Create(OldFun, args, "", entryblock);
		CallAtomFunc->setCallingConv(OldFun->getCallingConv());
		ReturnInst::Create(OldFun->getParent()->getContext(), nullptr, entryblock);
	}
	else
	{
		CallInst* CallAtomFunc = CallInst::Create(OldFun, args, "call", entryblock);
		CallAtomFunc->setCallingConv(OldFun->getCallingConv());
		ReturnInst::Create(OldFun->getParent()->getContext(), CallAtomFunc, entryblock);

	}
	return NewFunc;
}

/// \Substitute Specified Function With Packaged Function
void ExtractStaticInfo::SubDllFuncWithPackage(vector<Function*> IllegalFuncList, vector<Function*> RecordIllegalPath)
{
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (IfFuncInlist(FuncList[i], IllegalFuncList) != -1 || IfFuncInlist(FuncList[i], RecordIllegalPath) != -1)
		{
			continue;
		}
		if (FuncList[i]->isDeclaration() || FuncList[i]->hasDLLImportStorageClass() || FuncList[i]->hasLinkOnceLinkage())
		{
			FuncList[i] = PackageDllFunc(FuncList[i]);
		}
		
	}
}

/// \Initial Function List (Package DllImportFunction, Exclude Illegal Function and Function Called By Illegal Function)
BOOL ExtractStaticInfo::InitialFuncList(Module * mod)
{
	vector<Function*> IllegalFuncList;
	for (Function &F : *mod)
	{
		TransConstantExpr2Inst(&F);
		FuncList.push_back(&F);
		if (F.isIntrinsic() || F.hasAvailableExternallyLinkage() || F.isVarArg() || F.getName().find("ForOthread") != string::npos || F.getName().find(".cpp") != string::npos)
		{
			IllegalFuncList.push_back(&F);
		}
	}
	if (!UpdateGraph())
	{
		errs() << "Failed to Update Function Call Matrix When InitialFuncList\n";
		return false;
	}
	vector<Function*> RecordIllegalPath;
	for (int i = 0; i < IllegalFuncList.size(); i++)
	{
		int temp = IfFuncInlist(IllegalFuncList[i], FuncList);
		for (int j = 0; j < FuncList.size(); j++)
		{
			if (AccessMatrix[temp][j] == 1 || AccessMatrix[j][j])
			{
				RecordIllegalPath.push_back(FuncList[j]);
			}
		}
	}
	SubDllFuncWithPackage(IllegalFuncList, RecordIllegalPath);

	vector<Function*> tempFuncList;
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (IfFuncInlist(FuncList[i], RecordIllegalPath) != -1 || IfFuncInlist(FuncList[i], IllegalFuncList) != -1)
		{
			continue;
		}
		tempFuncList.push_back(FuncList[i]);
	}
	FuncList = tempFuncList;
}

/// \When Function List Changed, Update CallMatrix and AccessMatrix
BOOL ExtractStaticInfo::UpdateGraph()
{
	if (!BuildCallMatrix())
	{
		errs() << "Failed to Generate Function Call Matrix\n";
		return false;
	}
	BuildAccessMatrix();
}

/// \Excule Recursing Function
BOOL ExtractStaticInfo::FuncListPruning()
{
	vector<Function*> tempFuncList;
	BOOL flag = false;
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (CallMatrix[i][i] != 1)
		{
			tempFuncList.push_back(FuncList[i]);
		}
		else
		{
			flag = true;
		}
	}
	FuncList = tempFuncList;
	return flag;
}

/// \Get OverlapFunction, Overlap Function Means a Function Called By Multiple Threads
/// \Because we can't accurately find the thread execution path, we use the callback function as the starting point for thread execution
/// \It may cause code redundancy, but ensure the correctness of the program
int* ExtractStaticInfo::InitialOverlapFunc()
{
	vector<Function*> CabFuncList;
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (isCallBackFnType(FuncList[i]))
		{
			CabFuncList.push_back(FuncList[i]);
		}
	}
	int* Graph = (int *)malloc(sizeof(int) * FuncList.size());
	for (int i = 0; i < FuncList.size(); i++)
	{
		Graph[i] = 0;
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		for (int j = 0; j < CabFuncList.size(); j++)
		{
			if (IfFuncInlist(CabFuncList[j], FuncList) != -1)
			{
				Graph[i] = Graph[i] + AccessMatrix[IfFuncInlist(CabFuncList[j], FuncList)][i];
			}
		}
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		for (int j = 0; j < FuncList.size(); j++)
		{
			if ((AccessMatrix[i][j] == 1 && Graph[i] == 1 && Graph[j] == 1))
			{
				//errs() << FuncList[j]->getName() << "\n";
				Graph[j] = -1;
			}
		}
	}

	return Graph;
}

/// \Clone function
Function* ExtractStaticInfo::CloneFunction(Function * OldFun, string CloneID)
{
	int i;
	FunctionType* Fnty = (FunctionType*)OldFun->getValueType();
	Type *Result = Fnty->getReturnType();
	int ParamNum = Fnty->getNumParams();
	vector< Type* > params;
	params.clear();
	for (i = 0; i < ParamNum; i++)
	{
		params.push_back(Fnty->getParamType(i));
	}

	FunctionType* NewFnTy = FunctionType::get(Result, params, OldFun->isVarArg());
	Function* NewFunc = Function::Create(NewFnTy, OldFun->getLinkage(), OldFun->getName() + CloneID); 
	NewFunc->setCallingConv(OldFun->getCallingConv());  
	// NewFunc->getArgumentList().back().setName(AddedPointer);
	if (OldFun->isDeclaration()) return NULL;
	ValueToValueMapTy VMap;
	if (!OldFun->isDeclaration()) {
		Function::arg_iterator DestI = NewFunc->arg_begin();
		for (Function::const_arg_iterator J = OldFun->arg_begin(); J != OldFun->arg_end(); ++J)
		{
			DestI->setName(J->getName());
			VMap[&*J] = &*DestI++;
		}
		SmallVector< ReturnInst*, 8 > Returns;  // Ignore returns cloned.
		CloneFunctionInto(NewFunc, OldFun, VMap, /*ModuleLevelChanges=*/true, Returns);
	}

	if (OldFun->hasPersonalityFn())
		NewFunc->setPersonalityFn(MapValue(OldFun->getPersonalityFn(), VMap));
	OldFun->getParent()->getFunctionList().push_back(NewFunc);
	return NewFunc;
}



/// \Get overlap function list and clone overlap function, substitute old call with new call
void ExtractStaticInfo::FuncListCloning()
{
	vector<Function*> OverlapFuncList;
	vector<pair<Function*, Function*>> SubForOverlapList;
	int *Graph = InitialOverlapFunc();
	BuildCallMatrix();

	for (int i = 0; i < FuncList.size(); i++)
	{
		int temp = Graph[i];
		vector<Function*> ClonedFuncList;
		if (temp > 1)
		{
			while (temp > 1)
			{
				temp--;
				ClonedFuncList.push_back(CloneFunction(FuncList[i], "cloned_" + to_string(temp)));
			}
		}
		int index = 0;
		for (int j = 0; j < FuncList.size() && index < ClonedFuncList.size(); j++)
		{
			if (CallMatrix[j][i] == 1)
			{
				OverlapFuncList.push_back(FuncList[i]);
				SubForOverlapList.push_back(make_pair(FuncList[i], ClonedFuncList[index]));
				index++;
			}
		}
	}
	for (int i = 0; i < OverlapFuncList.size(); i++)
	{
		for (Value::user_iterator useri = OverlapFuncList[i]->user_begin(), usere = OverlapFuncList[i]->user_end(); useri != usere; useri++)
		{
			Value* tmpuser = dyn_cast<Value>(*useri);
			if (Instruction* inst = dyn_cast<Instruction>(tmpuser))
			{
				inst->replaceUsesOfWith(SubForOverlapList[i].first, SubForOverlapList[i].second);
			}
		}
	}

	for (int i = 0; i < SubForOverlapList.size(); i++)
	{
		if (IfFuncInlist(SubForOverlapList[i].second, FuncList) == -1 && !isCallBackFnType(FuncList[i]))
		{
			FuncList.push_back(SubForOverlapList[i].second);
		}
	}

	vector<Function*> tempFuncList;
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (Graph[i] == -1)
		{
			continue;
		}
		tempFuncList.push_back(FuncList[i]);
	}
	FuncList = tempFuncList;
}

/// \Check the function is resurse function 
BOOL ExtractStaticInfo::IsRecurseFunction(Function* Func)
{
	for (llvm::Value::user_iterator Useri = Func->user_begin(), Usere = Func->user_end(); Useri != Usere; Useri++)
	{
		if (Instruction* inst = dyn_cast<Instruction>(*Useri))
		{
			if (inst->getParent()->getParent() == Func)
			{
				return true;
			}
		}
	}
	return false;
}


/// \Check function list after cloning, make sure there is no illegal function in function list
void ExtractStaticInfo::CheckFuncList()
{
	vector<Function*> ExclueFuncList;
	vector<Function*> tempFuncList;
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (FuncList[i]->isDeclaration() || FuncList[i]->hasDLLImportStorageClass() || FuncList[i]->hasLinkOnceLinkage())
		{
			ExclueFuncList.push_back(FuncList[i]);
		}
		if (FuncList[i]->isIntrinsic() || FuncList[i]->hasAvailableExternallyLinkage())
			ExclueFuncList.push_back(FuncList[i]);
		if (isCallBackFnType(FuncList[i]) || FuncList[i]->isVarArg() || FuncList[i]->getNumUses() == 0 || IsRecurseFunction(FuncList[i]))
			ExclueFuncList.push_back(FuncList[i]);
		if (FuncList[i]->getName().find("Thread") != string::npos)
			ExclueFuncList.push_back(FuncList[i]);
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (IfFuncInlist(FuncList[i], ExclueFuncList) != -1)
		{
			continue;
		}
		tempFuncList.push_back(FuncList[i]);
	}
	FuncList = tempFuncList;
}


/// \Generate function graph, finish packaging, pruning and cloning 
BOOL ExtractStaticInfo::GenerateGraph(Module * mod)
{
	if (!InitialFuncList(mod))
	{
		errs() << "Failed to Initial Function List\n";
		return false;
	}
	if (!BuildCallMatrix())
	{
		errs() << "Failed to Generate Function Call Matrix\n";
		return false;
	}
	if (FuncListPruning())
	{
		if (!UpdateGraph())
		{
			errs() << "Failed to Update Function Call Matrix After Pruning\n";
			return false;
		}
	}
	else
	{
		BuildAccessMatrix();
	}
	FuncListCloning();
	if (!BuildCallMatrix())
	{
		errs() << "Failed to Generate Function Call Matrix After Cloning\n";
		return false;
	}
	CheckFuncList();	
	return true;
}


