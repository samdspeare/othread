/*****************************************************************************
Filename: RetriveBigStruct.cpp
Date    : 2020-03-01 20:43:08
Description: Retrive structure for parameter passing:
			 1. Utilize structure to store function parameters and return values
			 2. Replace the parameter passing method and return value method of the function
*****************************************************************************/
#include "stdafx.h"
#include "RetriveBigStruct.h"

/// \The storage and use of global parameters, 
/// \Find the function call location, and store all parameters in the specified location of the structure in advance. 
/// \Find the return position of the function, and store the return value in the specified position of the structure in advance
bool RetriveBigStruct::GetStructValue(Function* Func)
{
	llvm::IRBuilder<> builder(Func->getParent()->getContext());
	vector<Value*> vect;
	int UseNum = Func->getNumUses();
	for (Value::user_iterator useri = Func->user_begin(), usere = Func->user_end(); useri != usere; useri++)
	{
		Value* FuncUser = dyn_cast<Value>(*useri);
		if (Instruction* inst = dyn_cast<Instruction>(FuncUser))
		{
			if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(inst))
			{
				int index = -1;
				for (int i = 0; i < CI->getNumArgOperands(); i++)
				{
					Value* tempArg = CI->getArgOperand(i);
					index = IfTypeInlist(index + 1, tempArg->getType(), BigStructFiled);
					vect.clear();
					vect.push_back(builder.getInt32(0));
					vect.push_back(builder.getInt32(index));
					GetElementPtrInst *gep1 = GetElementPtrInst::Create(BigStructType, BigGV, vect, BigStructType->getName() + "_" + to_string(index), inst);
					StoreInst* temp = new StoreInst(tempArg, gep1, inst);
				}

				if (!Func->getReturnType()->isVoidTy())
				{
					int RetIndex = IfTypeInlist(ReturnIndex, Func->getReturnType(), BigStructFiled);
					Instruction* temp = inst->getNextNode();
					vect.clear();
					vect.push_back(builder.getInt32(0));
					vect.push_back(builder.getInt32(RetIndex));
					GetElementPtrInst *gepRet = GetElementPtrInst::Create(BigStructType, BigGV, vect, BigStructType->getName() + "_" + to_string(RetIndex), temp);
					LoadInst* loadRetValue = new LoadInst(gepRet, "load_" + gepRet->getName(), false, temp);
					inst->replaceAllUsesWith(loadRetValue);
				}

			}
			else if (llvm::InvokeInst* II = llvm::dyn_cast<llvm::InvokeInst>(inst))
			{
				int index = -1;
				for (int i = 0; i < CI->getNumArgOperands(); i++)
				{
					Value* tempArg = CI->getArgOperand(i);
					index = IfTypeInlist(index + 1, tempArg->getType(), BigStructFiled);
					vect.clear();
					vect.push_back(builder.getInt32(0));
					vect.push_back(builder.getInt32(index));
					GetElementPtrInst *gep1 = GetElementPtrInst::Create(BigStructType, BigGV, vect, BigStructType->getName() + "_" + to_string(index), inst);
					StoreInst* temp = new StoreInst(tempArg, gep1, inst);
				}

				if (!Func->getReturnType()->isVoidTy())
				{
					int RetIndex = IfTypeInlist(ReturnIndex, Func->getReturnType(), BigStructFiled);
					Instruction* temp = inst->getNextNode();
					vect.clear();
					vect.push_back(builder.getInt32(0));
					vect.push_back(builder.getInt32(RetIndex));
					GetElementPtrInst *gepRet = GetElementPtrInst::Create(BigStructType, BigGV, vect, BigStructType->getName() + "_" + to_string(RetIndex), temp);
					LoadInst* loadRetValue = new LoadInst(gepRet, "load_" + gepRet->getName(), false, temp);
					inst->replaceAllUsesWith(loadRetValue);
				}

			}
		}
		else
		{
			errs() << Func->getName() << " is used not by a call or invoke inst" << "\n";
			return false;
		}
	}
	for (BasicBlock &B : *Func)
	{
		for (Instruction &I : B)
		{
			if (ReturnInst* inst = dyn_cast<ReturnInst>(&I))
			{
				if (!Func->getReturnType()->isVoidTy())
				{
					int RetIndex = IfTypeInlist(ReturnIndex, Func->getReturnType(), BigStructFiled);
					vect.clear();
					vect.push_back(builder.getInt32(0));
					vect.push_back(builder.getInt32(RetIndex));
					GetElementPtrInst *gepRet = GetElementPtrInst::Create(BigStructType, BigGV, vect, BigStructType->getName() + "_" + to_string(RetIndex), inst);
					StoreInst* temp = new StoreInst(inst->getReturnValue(), gepRet, inst);
				}
			}
		}
	}
	return true;
}


/// \Modify the way of passing parameters
BOOL RetriveBigStruct::ConstructFunctionArgs(Module * mod)
{
	ConstructArgStruct(mod);
	llvm::IRBuilder<> builder(mod->getContext());
	vector<Value*> vect;

	for (int i = 0; i < FuncList.size(); i++)	
	{
		Instruction* Insert = &FuncList[i]->front().front();
		int index = -1;
		for (Function::arg_iterator argi = FuncList[i]->arg_begin(), arge = FuncList[i]->arg_end(); argi != arge; argi++)
		{
			Argument* arg = &*argi;
			index = IfTypeInlist(index + 1, arg->getType(), BigStructFiled);
			vect.clear();
			vect.push_back(builder.getInt32(0));
			vect.push_back(builder.getInt32(index));
			ArrayRef<Value*> tmpArrayRef = makeArrayRef(vect);
			GetElementPtrInst *gep1 = GetElementPtrInst::Create(BigStructType, BigGV, tmpArrayRef, BigStructType->getName() + "_" + to_string(index), Insert);
			LoadInst* loadArgValue = new LoadInst(gep1, "load_" + gep1->getName(), false, Insert);
			
			arg->replaceAllUsesWith(loadArgValue);
		}
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (!GetStructValue(FuncList[i]))
		{
			return false;
		}
	}
	for (int i = 0; i < FuncList.size(); i++)
	{
		Function* NewFunc = ClearAllArgs(FuncList[i]);
		SubstitueOldfnWithNewfn(FuncList[i], NewFunc);
		FuncList[i] = NewFunc;
	}
	return true;

}

/// \Clear all parameters of function, function will get parameters from global variable
Function* RetriveBigStruct::ClearAllArgs(Function* OldFun)
{
	int k = 1;
	Module* Mod = OldFun->getParent();
	std::vector<Type*>FuncTy_args;
	FunctionType* FuncTy = FunctionType::get(
		Type::getVoidTy(Mod->getContext()),
		FuncTy_args,
		false);
	Function* NewFunc = Function::Create(FuncTy, GlobalValue::LinkageTypes::ExternalLinkage, OldFun->getName() + "_Clear", Mod);  
	NewFunc->setCallingConv(CallingConv::C);
	if (OldFun->isDeclaration()) return NULL;
	ValueToValueMapTy VMap;
	AllocaInst *pa = NULL;
	if (!OldFun->isDeclaration()) {

		for (Function::iterator b = OldFun->begin(), be = OldFun->end(); b != be; ++b) {
			BasicBlock* label1_entry = BasicBlock::Create(Mod->getContext(), b->getName(), NewFunc);
			pa = new AllocaInst(Type::getInt32Ty(Mod->getContext()), 0, "indexLoc", label1_entry);
			for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
				Instruction *new_inst = i->clone();
				new_inst->setName(i->getName());	
				VMap[&*i] = new_inst;
				new_inst->insertBefore(pa);
				llvm::RemapInstruction(new_inst, VMap, RF_NoModuleLevelChanges | static_cast<RemapFlags>(2));

			}
			pa->eraseFromParent();
		}

	}
	vector<Instruction*> DelList;
	for (BasicBlock &B : *NewFunc)
	{
		for (Instruction &I : B)
		{
			if (ReturnInst* inst = dyn_cast<ReturnInst>(&I))
			{
				ReturnInst *Insert = ReturnInst::Create(NewFunc->getParent()->getContext(), nullptr, inst);
				Insert->setName(inst->getName());
				DelList.push_back(inst);
			}
		}
	}

	for (int i = 0; i < DelList.size(); i++)
	{
		DelList[i]->eraseFromParent();
	}
	return NewFunc;
}

/// \Replace the original function call with a function with empty parameters
bool RetriveBigStruct::SubstitueOldfnWithNewfn(Function * OldFun, Function* NewFunc)
{
	time_t t;
	srand((unsigned)time(&t));
	llvm::IRBuilder<> builder(OldFun->getParent()->getContext());
	int i = 0, j = 0;
	int UseNum = OldFun->getNumUses();

	for (Value::user_iterator useri = OldFun->user_begin(), usere = OldFun->user_end(); useri != usere; useri++)
	{
		Value* tmpuser = dyn_cast<Value>(*useri);
		if (Instruction* inst = dyn_cast<Instruction>(tmpuser))
		{
			if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(inst))
			{
				std::vector< Value* > ptr_fn_params;
				ptr_fn_params.clear();
				CallInst* call_NewFunc;
				if (NewFunc->getReturnType()->isVoidTy())
					call_NewFunc = CallInst::Create(NewFunc, ptr_fn_params, "", CI);
				else
					call_NewFunc = CallInst::Create(NewFunc, ptr_fn_params, "call_" + NewFunc->getName(), CI);
				call_NewFunc->setCallingConv(CallingConv::C);
				call_NewFunc->setTailCall(CI->isTailCall());
				CI->dropAllReferences();
				CI->removeFromParent();
			}
			else if (llvm::InvokeInst* II = llvm::dyn_cast<llvm::InvokeInst>(inst))
			{
				std::vector< Value* > ptr_fn_params;
				ptr_fn_params.clear();
				InvokeInst* Invoke_NewFunc = NULL;
				if (NewFunc->getReturnType()->isVoidTy())
					Invoke_NewFunc = InvokeInst::Create(NewFunc, II->getNormalDest(), II->getUnwindDest(), ptr_fn_params, "", II);
				else
					Invoke_NewFunc = InvokeInst::Create(NewFunc, II->getNormalDest(), II->getUnwindDest(), ptr_fn_params, "call_" + NewFunc->getName(), II);
				Invoke_NewFunc->setCallingConv(CallingConv::C);
				II->dropAllReferences();
				II->removeFromParent();
			}
		}
		else
		{
			errs() << OldFun->getName() << " is used not by a call or invoke inst" << "\n";
		}
	}
	return true;
}

/// \Build a large structure to transfer function parameters and return values.
/// \The large structure corresponds to the parameters of each function from front to back, 
/// \and the return value of each function from back to front.
void RetriveBigStruct::ConstructArgStruct(Module * mod)
{
	
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (i == 0)
		{
			for (Function::arg_iterator argi = FuncList[i]->arg_begin(), arge = FuncList[i]->arg_end(); argi != arge; argi++)
			{
				Argument* arg = &*argi;
				BigStructFiled.push_back(arg->getType());
			}
		}
		else
		{
			int index = 0;
			for (Function::arg_iterator argi = FuncList[i]->arg_begin(), arge = FuncList[i]->arg_end(); argi != arge; argi++)
			{
				Argument* arg = &*argi;
				if (index >= BigStructFiled.size())
				{
					if (!arg->getType()->isVoidTy())
					{
						BigStructFiled.push_back(arg->getType());
						index = index + 1;
					}
					continue;
				}
				int flag = 0;
				for (int j = index; j < BigStructFiled.size(); j++)
				{
					if (BigStructFiled[j] == arg->getType())
					{
						flag = 1;
						index = j + 1;
						break;
					}
					index = j + 1;
				}
				if (flag == 1)
				{
					continue;
				}
				else
				{
					if (!arg->getType()->isVoidTy())
					{
						BigStructFiled.push_back(arg->getType());
						index = index + 1;
					}
					
				}
			}
		}
	}
	ReturnIndex = BigStructFiled.size();
	for (int i = 0; i < FuncList.size(); i++)
	{
		if (i == 0)
		{
			if (!FuncList[i]->getReturnType()->isVoidTy())
			{
				BigStructFiled.push_back(FuncList[i]->getReturnType());
			}
		}
		else
		{
			int flag = 0;
			for (int j = ReturnIndex; j < BigStructFiled.size(); j++)
			{
				if (BigStructFiled[j] == FuncList[i]->getReturnType())
				{
					flag = 1;
					break;
				}
			}
			if (flag == 1)
			{
				continue;
			}
			else
			{
				if (!FuncList[i]->getReturnType()->isVoidTy())
				{
					BigStructFiled.push_back(FuncList[i]->getReturnType());
				}
				
			}
		}
		
	}
	BigStructType = StructType::create(mod->getContext(), "struct.BigStruct");
	BigStructType->setBody(BigStructFiled, true);
	BigGV = new GlobalVariable(*mod, BigStructType, false, GlobalValue::InternalLinkage, Constant::getNullValue(BigStructType), "struct.BigStruct");
	BigGV->setAlignment(4);
}