#include "stdafx.h"
#include "symbol.h"

#include <string>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")


typedef std::map<std::string, HMODULE> LoadedModuleMap;
static LoadedModuleMap g_loadedModuleMap;

bool initSymbol()
{
	// 初始化符号处理器
	DWORD symOptions = SymGetOptions();
	symOptions |= SYMOPT_DEBUG | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS;
	symOptions &= ~SYMOPT_DEFERRED_LOADS;
	SymSetOptions(symOptions);

	std::string symPath;

	// Local
	{
		char localSym[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, localSym);
		strcat(localSym, "\\symbols");
		symPath += localSym;
		symPath += ";";
	}
	// _NT_SYMBOL_PATH
	DWORD len = GetEnvironmentVariableA("_NT_SYMBOL_PATH", NULL, 0);
	if (len != 0)
	{
		LPSTR buffer = (LPSTR)malloc(len + 1);
		GetEnvironmentVariableA("_NT_SYMBOL_PATH", buffer, len + 1);
		symPath += buffer;
		free(buffer);
	}

	return SymInitialize(GetCurrentProcess(), symPath.c_str(), FALSE) ? true : false;
}

void termSymbol()
{
	SymCleanup(GetCurrentProcess());
}

bool symLoadModule(const char *dllName)
{
	LoadedModuleMap::iterator iter = g_loadedModuleMap.find(dllName);
	if (iter != g_loadedModuleMap.end())
		return true;

	HMODULE hDLL = LoadLibraryExA(dllName, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!hDLL)
		return false;

	SymLoadModule64(GetCurrentProcess(), NULL, dllName, NULL, (DWORD64)hDLL, 0);

	g_loadedModuleMap[dllName] = hDLL;

	return true;
}

#define MAX_SYMBOL_NAME_LENGTH   256 // Maximum symbol name length that we will allow. Longer names will be truncated.
#define MAX_SYMBOL_INFO_BUF_SIZE (sizeof(SYMBOL_INFO) + (MAX_SYMBOL_NAME_LENGTH - 1) * sizeof(char))

unsigned int getFuncInfo(const char *moduleName, unsigned int offset, char info[512])
{
	const char *pszModuleName = "?";
	const char *pszFuncName = "?";

	DWORD moduleNameSize = (DWORD)strlen(moduleName);
	if (moduleNameSize) {
		// 找到最后一个目录分隔符的位置
		DWORD lastDirSepPos = moduleNameSize - 1;
		while (lastDirSepPos) {
			if (moduleName[lastDirSepPos] == '\\' || moduleName[lastDirSepPos] == '/') {
				lastDirSepPos++;
				break;
			}
			lastDirSepPos--;
		}

		pszModuleName = moduleName + lastDirSepPos;
	}

	// 根据文件名找到HMODULE
	LoadedModuleMap::iterator iter = g_loadedModuleMap.find(moduleName);
	if (iter == g_loadedModuleMap.end()){
		return sprintf(info, "%s!0x%x", pszModuleName, offset);
	}
	HMODULE hModule = iter->second;

	// 重新计算出函数在本进程空间中的地址
	void *func = (void*)((unsigned char*)hModule + offset);

	// 取得函数名称
	char symbolInfoBuf[MAX_SYMBOL_INFO_BUF_SIZE];
	SYMBOL_INFO *functionInfo = (SYMBOL_INFO*)symbolInfoBuf;
	functionInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	functionInfo->MaxNameLen = MAX_SYMBOL_NAME_LENGTH;
	DWORD64 symDisplacement = 0;
	if (SymFromAddr(GetCurrentProcess(), (DWORD64)func, &symDisplacement, functionInfo)) {
		pszFuncName = functionInfo->Name;
	}

	// 取得函数对应的源代码信息
	BOOL foundLine = FALSE;
	IMAGEHLP_LINE64 sourceInfo = { 0 };
	sourceInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	DWORD lineDisplacement = 0;
	foundLine = SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)func, &lineDisplacement, &sourceInfo);

	// 格式化输出
	unsigned int retCode = 0;
	if (foundLine) {
		retCode = sprintf(info, "%s!%s+0x%I64x, %s(%d)",
			pszModuleName, pszFuncName, symDisplacement, sourceInfo.FileName, sourceInfo.LineNumber);
	} else {
		retCode = sprintf(info, "%s!%s+0x%I64x, ?(?)",
			pszModuleName, pszFuncName, symDisplacement);
	}

	return retCode;
}
