/*****************************************************************************
Filename: rwBcFiles.cpp
Date    : 2020-02-10 11:25:35
Description: Read and Write Intermedia Representation file
*****************************************************************************/
#include"stdafx.h"
#include"rwBcFiles.h" 
/// \Read Intermedia Representation file
Module* MyParseIRFile(StringRef filename)
{
	SMDiagnostic Err;
	Module* M = NULL;
	std::unique_ptr<Module> mod = nullptr;
	LLVMContext& globalContext = llvm::getGlobalContext();
	mod = llvm::parseIRFile(filename, Err, globalContext);
	if (!mod)
	{
		Err.print("Open Module file error", errs());
		return NULL;
	}
	M = mod.get();
	if (!M)
	{
		errs() << ": error loading file '" << filename << "'\n";
		return NULL;
	}
	mod.release();
	return M;
}

/// \Write Intermedia Representation file
bool doWriteBackLL(Module* M, StringRef filename)
{
	std::error_code ErrorInfo;
	std::unique_ptr<tool_output_file> out(new tool_output_file(filename, ErrorInfo, llvm::sys::fs::F_None));
	if (ErrorInfo)
	{
		errs() << ErrorInfo.message() << "\n";
		return false;
	}
	M->print(out->os(), NULL);
	out->keep();
	return true;
}