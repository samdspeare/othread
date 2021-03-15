#pragma once
#include "common.h"
Module* MyParseIRFile(StringRef filename);		
bool doWriteBackLL(Module* M, StringRef filename);