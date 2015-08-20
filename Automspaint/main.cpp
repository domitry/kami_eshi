#include <Windows.h>
#include <stdio.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")

const char* dll_name = "C:\\dll\\mspaint.dll";

DWORD find_mspaint(){
	HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;
    PWTS_PROCESS_INFO ProcessInfo;
    DWORD dwCount = 0;

    WTSEnumerateProcesses(hServer,
        0,
        1,
        &ProcessInfo,
        &dwCount);

    for (DWORD i=0; i < dwCount; i++) {
		if(!wcscmp(ProcessInfo[i].pProcessName, L"mspaint.exe")){
			return ProcessInfo[i].ProcessId;
		}
    }
	return NULL;
}

int main(int argc, TCHAR* argv[]){
	auto pid = find_mspaint();
	if(!pid){
		OutputDebugString(L"No process named mspaint was found.");
		return -1;
	}

	HMODULE kernel32 = ::GetModuleHandle(L"Kernel32");
	if(!kernel32){
		OutputDebugString(L"kernel32.dll not found.");
		return -1;
	}
	
	HANDLE handle = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, pid);
	if(!handle){
		OutputDebugString(L"cannot open handle.");
		return -1;
	}

	void* memory = VirtualAllocEx(handle, NULL, strlen(dll_name), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(!memory){
		OutputDebugString(L"cannot alloc memory.");
		CloseHandle(handle);
		return -1;
	}

	::WriteProcessMemory(handle, memory, (void*)dll_name,strlen(dll_name), NULL);

	HANDLE thread = ::CreateRemoteThread(handle, NULL, 0, (LPTHREAD_START_ROUTINE) ::GetProcAddress(kernel32, "LoadLibraryA"), memory, 0, NULL );


	if(!thread){
		OutputDebugString(L"cannot create new thread.");
		return -1;
	}

	::WaitForSingleObject(thread, INFINITE);

	DWORD hLibModule;
	::GetExitCodeThread(thread, &hLibModule);


	if(!hLibModule){
		OutputDebugString(L"something went wrong...");
	}else{
		wchar_t str[100];
		OutputDebugString(L"Injection succeeded!\n");
		swprintf(str, 100, L"%08x", hLibModule);
		OutputDebugString(str);
	}


	::VirtualFreeEx(handle, memory, strlen(dll_name), MEM_RELEASE);
	::CloseHandle(handle);

	return 0;
}
