/*****************************************************************************
Filename: ComBineModuleAndSource.cpp
Date    : 2020-03-10 15:21:42
Description: Replace the original function call with SubCall(Inter-thread communication function):
			 1. Get template function
			 2. Add a start function to start the specified thread after the program runs
			 3. Replace the original function call
*****************************************************************************/
#include "stdafx.h"
#include "ComBineModuleAndSource.h"

/// \Get the variables to be used in the template file
void ComBineModuleAndSource::GetModuleVar(Module * mod)
{
	for (Function &F : *mod)
	{
		if (F.getName().find("MultiThreadForOthread") != string::npos)
		{
			MultiThread = &F;
		}
		if (F.getName().find("SubOldCallForOthread") != string::npos)
		{
			SubOldCall = &F;
		}
		if (F.getName().find("printNumForOthread") != string::npos)
		{
			printNumForOthread = &F;
		}
	}

	for (Module::global_iterator gi = mod->global_begin(); gi != mod->global_end(); gi++)
	{
		GlobalVariable* g_value = (GlobalVariable*)gi;
		if (g_value->getName().find("FunctionGrgphForOthread") != string::npos)
		{
			FunctionGrgph = g_value;
		}
	}
}

/// \Experimental function, used to calculate the degree of discrete distribution of functions
void ComBineModuleAndSource::SumSubCallNum(Function* main)
{
	for (BasicBlock &B : *main)
	{
		for (Instruction &I : B)
		{
			if (ReturnInst* inst = dyn_cast<ReturnInst>(&I))
			{
				std::vector< Value* > ptr_fn_params;
				ptr_fn_params.clear();
				CallInst* call_NewFunc = CallInst::Create(printNumForOthread, ptr_fn_params, "", inst);
			}
		}
	}
}


/// \Replace the old function with the "SubCall" function
BOOL ComBineModuleAndSource::AddSubOldCall(Function* OldFun, Function* NewFunc, int tempSignal, int Index)
{
	llvm::IRBuilder<> builder(OldFun->getParent()->getContext());
	for (Value::user_iterator useri = OldFun->user_begin(), usere = OldFun->user_end(); useri != usere; useri++)
	{
		Value* tmpuser = dyn_cast<Value>(*useri);
		if (Instruction* inst = dyn_cast<Instruction>(tmpuser))
		{
			if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(inst))
			{
				std::vector< Value* > ptr_fn_params;
				vector<Value*> vect;
				ptr_fn_params.clear();
				
				ptr_fn_params.push_back(builder.getInt32(tempSignal));
				ptr_fn_params.push_back(builder.getInt32(Index));

				CallInst* call_NewFunc = CallInst::Create(NewFunc, ptr_fn_params, "", CI);
				
				call_NewFunc->setCallingConv(CI->getCallingConv());
				call_NewFunc->setTailCall(CI->isTailCall());
				CI->dropAllReferences();
				CI->removeFromParent();

			}
			else if (llvm::InvokeInst* II = llvm::dyn_cast<llvm::InvokeInst>(inst))
			{
				std::vector< Value* > ptr_fn_params;
				vector<Value*> vect;
				ptr_fn_params.clear();

				ptr_fn_params.push_back(builder.getInt32(tempSignal));
				ptr_fn_params.push_back(builder.getInt32(Index));

				InvokeInst* Invoke_NewFunc  = InvokeInst::Create(NewFunc, II->getNormalDest(), II->getUnwindDest(), ptr_fn_params, "", II);
			
				Invoke_NewFunc->setCallingConv(II->getCallingConv());
				II->dropAllReferences();
				II->removeFromParent();

			}
		}
		else
		{
			errs() << OldFun->getName() << " is used not by a call or invoke inst" << "\n";
			return false;
		}
	}
	return true;
}


/// \Generate thread object, complete function replacement
BOOL ComBineModuleAndSource::GenThreadObject(Module * mod)
{
	llvm::IRBuilder<> builder(mod->getContext());
	GetModuleVar(mod);	
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (!AddSubOldCall(FuncList[i], SubOldCall, FuncIDpair[i].second, FuncIDpair[i].first))
		{
			return false;
		}
	}

	int IDLength = 0;
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (FuncIDpair[i].first > IDLength)
		{
			IDLength = FuncIDpair[i].first;
		}
	}

	Function* main = mod->getFunction("main");
	if (main == nullptr)
	{
		main = mod->getFunction("\x01_WinMain@16");
	}

	//SumSubCallNum(main);	

	Instruction* Start = &main->front().front();
	vector<Value*> vect;
	vect.push_back(builder.getInt32(IDLength + 1));
	//At the beginning of the main function, call the thread start function
	CallInst* MultiThreadFunc = CallInst::Create(MultiThread, vect, "", Start);

	for (int i = 0; i < FuncList.size(); i++)
	{
		vector<Value*> vect;
		vect.push_back(builder.getInt32(0));
		vect.push_back(builder.getInt32(FuncIDpair[i].first));
		vect.push_back(builder.getInt32(FuncIDpair[i].second));
		GetElementPtrInst *GrgphGep = GetElementPtrInst::CreateInBounds(FunctionGrgph->getValueType(), FunctionGrgph, vect, FunctionGrgph->getName(), Start);
		PtrToIntInst* ptr2intcaller = new PtrToIntInst(FuncList[i], Type::getInt32Ty(mod->getContext()), "ptr2intCaller", Start);
		new StoreInst(ptr2intcaller, GrgphGep, Start);
	}
	return true;
}
