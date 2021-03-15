/*****************************************************************************
Filename: othread.cpp
Date    : 2020-02-10 10:59:01
Description: Execution trajectory obfuscation which make the program execution trajectory 
			 with function as the unit converted from the call execution in a single thread 
			 to the jump execution in the mode of repeated switching between multiple threads
			 1. Extract static information of program
			 2. Calculate maximum execution complexity and call depth of function
			 3. Assign functions to threads
			 4. Retrive structure for parameter passing
			 5. Replace the original function call with SubCall(Inter-thread communication function)
*****************************************************************************/
#include "stdafx.h"
#include "common.h"
#include "ComBineModuleAndSource.h"
#include "RetriveBigStruct.h"
#include "BuildUniqueID.h"
#include "CalcFuncNestNum.h"
#include "ExtractStaticInfo.h"


void Statistics(Module *mod)
{
	int FuncNum = 0;
	int InstNum = 0;
	for (Function &F : *mod)
	{
		if (!F.isIntrinsic() && !F.hasAvailableExternallyLinkage() && F.getNumUses() != 0 && F.getName().find("ForOthread") == string::npos && F.getName().find("llvm.") == string::npos)
		{
			FuncNum++;
			for (BasicBlock &B : F)
			{
				for (Instruction &I : B)
				{
					InstNum++;
				}
			}
		}
	}
	errs() << "File contains " << FuncNum << " Functions \n";
	errs() << "File contains " << InstNum << " lines of intermediate code\n";
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		errs() << "Usage::othread.exe [input].bc [output].bc";
		return 0;
	}
	string InputBCFile = argv[1];
	string OutputEXEBCFile = argv[2];
	Module* mod = MyParseIRFile(InputBCFile);
	if (mod == NULL)
	{
		errs() << "Failed to read input file\n";
	}
	Statistics(mod);
	llvm::IRBuilder<> builder(mod->getContext());

	/// \Extract static information of program, finish packaging, pruning and cloning
	errs() << "start to extract static information of program...\n";
	ExtractStaticInfo doExtractStaticInfo;
	if (!doExtractStaticInfo.GenerateGraph(mod))
	{
		errs() << "error in GenerateGraph\n";
		return 0;
	}
	/// \Calculate maximum execution complexity of function, guide function allocation
	errs() << "Start to calculate maximum execution complexity of function...\n";
	CalcFuncNestNum doCalcFuncNestNum;
	doCalcFuncNestNum.FuncList = doExtractStaticInfo.FuncList;
	doCalcFuncNestNum.GetNestInfo(mod);
	/// \Use graph coloring algorithm to assign functions to threads
	errs() << "Start to assign functions to threads...\n";
	BuildUniqueID doBuildUniqueID;
	doBuildUniqueID.FuncList = doCalcFuncNestNum.FuncList;
	doBuildUniqueID.FuncNestList = doCalcFuncNestNum.FuncNestList;
	mod = doBuildUniqueID.AllocThread(mod);
	/// \Retrive structure for parameter passing
	errs() << "Start to retrive structure for parameter passing...\n";
	RetriveBigStruct doRetriveBigStruct;
	doRetriveBigStruct.FuncList = doBuildUniqueID.FuncList;
	doRetriveBigStruct.FuncIDpair = doBuildUniqueID.FuncIDpair;
	if (!doRetriveBigStruct.ConstructFunctionArgs(mod))
	{
		errs() << "error in RetriveBigStruct\n";
		return 0;
	}
	/// \Replace the original function call with the preset startup function
	errs() << "Start to replace the original function call...\n";
	ComBineModuleAndSource doComBineModuleAndSource;
	doComBineModuleAndSource.FuncIDpair = doBuildUniqueID.FuncIDpair;
	doComBineModuleAndSource.FuncList = doRetriveBigStruct.FuncList;
	doComBineModuleAndSource.FuncNestList = doCalcFuncNestNum.FuncNestList;
	if (!doComBineModuleAndSource.GenThreadObject(mod))
	{
		errs() << "error in ComBineModuleAndSource\n";
		return 0;
	}
	errs() << "Finish obfuscation\n";
	errs() << "Start to output...\n";
	doWriteBackLL(mod, OutputEXEBCFile);
	errs() << "Success!\n";
	errs() << doComBineModuleAndSource.FuncList.size() << " Functions are obfuscated\n";
	return 0;
}