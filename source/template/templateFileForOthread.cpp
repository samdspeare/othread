#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

HANDLE hMutexList[30] = { NULL };
HANDLE MutexFuncList[30] = { NULL };
int WhileEndFlag[30] = { 0 };

int GlobalSignal = -1;				//Global function number
int GlobalThreadID = -1;			//Global thread number
int thresholdForOthread = 2;
int threadNumForOthread = 10;
int FunctionGrgphForOthread[30][1000] = { 0 };			//Fill in the function address by position after the main function starts
typedef void(*funcptr)();
void SubOldCallForOthread(int tempSignal, int ID);
void MultiThreadForOthread(int IDLength);
void StartThreadEventForOthread(int CurID);
void StartThreadWhileForOthread(int CurID);
void printNumForOthread();

//Resource priority thread
void StartThreadEventForOthread(int CurID)			
{
	while (1)
	{
		WaitForSingleObject(hMutexList[CurID], INFINITE);
		ResetEvent(hMutexList[CurID]);
		funcptr func = (funcptr)(FunctionGrgphForOthread[CurID][GlobalSignal]);
		func();
		SetEvent(MutexFuncList[CurID]);
	}
}

//Efficiency priority thread
void StartThreadWhileForOthread(int CurID)			
{
	while (1)
	{
		if (GlobalThreadID == CurID)
		{
			funcptr func = (funcptr)(FunctionGrgphForOthread[CurID][GlobalSignal]);
			func();
			GlobalThreadID = -1;
			WhileEndFlag[CurID] = -1;			
		}
	}
}

//Create thread
void MultiThreadForOthread(int IDLength)			
{
	char str[10] = { 0 }; int CurID = 0;
	for (CurID = 0; CurID < IDLength; CurID++)
	{
		if (CurID < thresholdForOthread)
		{
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartThreadWhileForOthread, (LPVOID)CurID, 0, NULL);	
		}
		else
		{
			itoa(CurID, str, 10);
			hMutexList[CurID] = CreateEventW(nullptr, true, false, (LPCWSTR)(str));
			itoa((CurID + threadNumForOthread + 1), str, 10);
			MutexFuncList[CurID] = CreateEventW(nullptr, true, false, (LPCWSTR)(str));
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartThreadEventForOthread, (LPVOID)CurID, 0, NULL);
		}
		
	}
}

//Thread start function, notify the corresponding thread to execute the target function
void SubOldCallForOthread(int tempSignal, int ID)			
{
	GlobalSignal = tempSignal;
	if (ID < thresholdForOthread)
	{
		GlobalThreadID = ID;
		while (1)
		{
			if (WhileEndFlag[ID] == -1)
			{
				WhileEndFlag[ID] = 0;
				break;
			}
		}
	}
	else
	{	
		SetEvent(hMutexList[ID]);
		WaitForSingleObject(MutexFuncList[ID], INFINITE);
		ResetEvent(MutexFuncList[ID]);
	}
}
