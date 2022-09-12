/*
	ChromeDriverUpdater - checks installed Chrom version and current chromedriver.
	(c) 2022 Artem Moroz <artem.moroz@gmail.com>

*/

#include "pch.h"
#include <iostream>

#include "miniz.h"

#include "ChromeDriverUpdater.h"

const TCHAR g_szChromePath1[] = _T("c:\\Program Files (x86)\\Google\\Chrome\\");
const TCHAR g_szChromePath2[] = _T("c:\\Program Files\\Google\\Chrome\\");

TCHAR g_szWorkingPath[MAX_PATH];

TCHAR g_szChromeDriverPath[MAX_PATH];

CHAR chProcessBuf[512];

void GetWorkingPath()
{
	GetModuleFileName(NULL, g_szWorkingPath, MAX_PATH);
	PathRemoveFileSpec(g_szWorkingPath);

	_tcscpy(g_szChromeDriverPath, g_szWorkingPath);
	PathAppend(g_szChromeDriverPath, _T("chromedriver.exe"));
}

int GetInstalledChormeVersion()
{
	TCHAR* szExePath = new TCHAR[MAX_PATH];
	_tcscpy(szExePath, g_szChromePath2);
	PathAppend(szExePath, _T("Application\\chrome.exe"));

	int nVer = -1;
   
	VS_FIXEDFILEINFO* lpFfi = NULL;
	UINT uVerLen = 0;

    DWORD dwSize = GetFileVersionInfoSize(szExePath, NULL);
    if (dwSize > 0)
    {
        BYTE* pbInfo = new BYTE[dwSize];
        if (pbInfo != NULL)
        {
            if (GetFileVersionInfo(szExePath, NULL, dwSize, pbInfo))
            {
				if (VerQueryValue(pbInfo, _T("\\"), (LPVOID*)&lpFfi, &uVerLen))
				{
					if (lpFfi != NULL)
					{
						DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
						DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

						DWORD dwLeftMost = HIWORD(dwFileVersionMS);
						nVer = (int)dwLeftMost;
						
						//DWORD dwSecondLeft = LOWORD(dwFileVersionMS);
						//DWORD dwSecondRight = HIWORD(dwFileVersionLS);
						//DWORD dwRightMost = LOWORD(dwFileVersionLS);

						
					}
				}
				
            }
            
            delete[] pbInfo;
        }
    }

	

	delete[] szExePath;
	
	return nVer;
}

int CheckInstalledChromeDriverVersion()
{
	TCHAR* pszChormeDriverPath = new TCHAR[MAX_PATH + 128];
	if (!pszChormeDriverPath)
	{
		return -1;
	}

	_tcscpy(pszChormeDriverPath, g_szChromeDriverPath);
	_tcscat(pszChormeDriverPath, _T(" --version"));

	if (!MyRunProcess(pszChormeDriverPath))
	{
		delete[] pszChormeDriverPath;
		return -1;
	}
	
	delete[] pszChormeDriverPath;
	
	int nVersion = -1;

	const int nLen = strlen(chProcessBuf);

	if (nLen > 0)
	{
		CHAR chNumber[64] = { 0 };

		for (int i = 0, j = 0; i < nLen; i++)
		{
			char c = chProcessBuf[i];
			if (c == '.')
				break;

			if (c >= '0' && c <= '9')
			{
				if (j < sizeof(chNumber) - 2)
					chNumber[j++] = c;
			}
		}

		nVersion = atoi(chNumber);
	}

	return nVersion;
}



BOOL MyRunProcess(TCHAR* pszCommandLine)
{
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	SECURITY_ATTRIBUTES saAttr = { 0 };
	HANDLE hChildStd_OUT_Rd = NULL;
	HANDLE hChildStd_OUT_Wr = NULL;
	DWORD dwRead;
	
	chProcessBuf[0] = 0;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 

	if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
	{
		return FALSE;
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
	{
		CloseHandle(hChildStd_OUT_Rd);
		CloseHandle(hChildStd_OUT_Wr);
		return FALSE;
	}

	si.cb = sizeof(si);
	si.hStdError = hChildStd_OUT_Wr;
	si.hStdOutput = hChildStd_OUT_Wr;
	si.dwFlags |= STARTF_USESTDHANDLES;

	// Start the child process. 
	if (CreateProcess(NULL,           // No module name (use command line)
		pszCommandLine,    // Command line
		NULL,                           // Process handle not inheritable
		NULL,                           // Thread handle not inheritable
		TRUE,                           // Set handle inheritance
		0,                              // No creation flags
		NULL,                           // Use parent's environment block
		NULL,                           // Use parent's starting directory 
		&si,                            // Pointer to STARTUPINFO structure
		&pi)                            // Pointer to PROCESS_INFORMATION structure
		)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);

		dwRead = 0;

		if (PeekNamedPipe(hChildStd_OUT_Rd,
			chProcessBuf, sizeof(chProcessBuf) - 1, &dwRead, NULL, NULL))
		{
			chProcessBuf[dwRead] = 0;
		}

		/*
		if (ReadFile(hChildStd_OUT_Rd, chProcessBuf, sizeof(chProcessBuf) - 1, &dwRead, NULL) && dwRead < sizeof(chProcessBuf) - 1)
		{
			chProcessBuf[dwRead] = 0;
		}
		*/

		printf("%s", chProcessBuf);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	CloseHandle(hChildStd_OUT_Rd);
	CloseHandle(hChildStd_OUT_Wr);

	return TRUE;
}

BOOL KillAllChromeDrivers()
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32 = { 0 };

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return FALSE;
	}

	do
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
		if (hProcess != NULL)
		{
			TCHAR szExePath[MAX_PATH];
			szExePath[0] = 0;

			if (GetModuleFileNameExW(hProcess, NULL, szExePath, MAX_PATH))
			{
				if (_tcsicmp(g_szChromeDriverPath, szExePath) == 0)
				{
					_tprintf(_T("Killing process %s, id %d\n"), szExePath, pe32.th32ProcessID);

					TerminateProcess(hProcess, 0);
				}
			}

			CloseHandle(hProcess);
		}

	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	
	return TRUE;
}


TCHAR* GetLatestChromeDriverUrl(int nVersion)
{
	TCHAR szCommand[255] = { 0 };
	_stprintf(szCommand, _T("curl -s https://chromedriver.storage.googleapis.com/LATEST_RELEASE_%d"), nVersion);

	if (MyRunProcess(szCommand))
	{
		if (strlen(chProcessBuf) > 0)
		{
			TCHAR szVersion[128] = { 0 };
			MultiByteToWideChar(CP_ACP, 0, chProcessBuf, -1, szVersion, sizeof(szVersion) / sizeof(szVersion[0]));

			TCHAR* pszZipUrl = new TCHAR[255];
			if (pszZipUrl)
			{
				_stprintf(pszZipUrl, _T("https://chromedriver.storage.googleapis.com/%s/chromedriver_win32.zip"), szVersion);
				return pszZipUrl;
			}
		}
	}
	
	return NULL;
}


TCHAR* DownloadChromiumZip(TCHAR* pszZipUrl)
{
	
	TCHAR* pszZipFile = new TCHAR[255];

	TCHAR szCommand[255] = { 0 };
	
	_tcscpy(pszZipFile, g_szWorkingPath);
	PathAppend(pszZipFile, _T("chromedriver_win32.zip"));

	_stprintf(szCommand, _T("curl -o %s %s -s"), pszZipFile, pszZipUrl);

	if (MyRunProcess(szCommand))
	{
		return pszZipFile;
	}
	return NULL;
}


BOOL UnzipChromiumZip(TCHAR* szZipPath)
{
	BOOL bMyStatus = FALSE;
	
	mz_zip_archive zip_archive = {0};
	mz_bool status = 0;
	
	char szAZipPath[MAX_PATH];
	szAZipPath[0] = 0;
	WideCharToMultiByte(CP_ACP, 0, szZipPath, -1, szAZipPath, sizeof(szAZipPath), NULL, NULL);

	char szAWorkingDir[MAX_PATH];
	szAWorkingDir[0] = 0;
	WideCharToMultiByte(CP_ACP, 0, g_szWorkingPath, -1, szAWorkingDir, sizeof(szAWorkingDir), NULL, NULL);
	
	status = mz_zip_reader_init_file(&zip_archive, szAZipPath, 0);
	if (status)
	{
		for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
		{
			mz_zip_archive_file_stat file_stat;
			if (mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
			{
				if (!mz_zip_reader_is_file_a_directory(&zip_archive, i))
				{
					char szAOutPath[MAX_PATH];
					szAOutPath[0] = 0;
					
					PathCombineA(szAOutPath, szAWorkingDir, file_stat.m_filename);
					
					status = mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename, szAOutPath, 0);

					if (status)
					{
						bMyStatus = TRUE;
					}
				}
			}
		}

		mz_zip_reader_end(&zip_archive);
	}

	return bMyStatus;
	
}

int main()
{
	GetWorkingPath();

	KillAllChromeDrivers();


	int nVerInstalledChrome = GetInstalledChormeVersion();
	if (nVerInstalledChrome > 0)
	{
		std::wcout << _T("Found installed Chrome version: ") << nVerInstalledChrome << std::endl;
	}

	int nVerChromeDriver = CheckInstalledChromeDriverVersion();
	if (nVerChromeDriver > 0)
	{
		std::wcout << _T("Found installed ChromeDriver version: ") << nVerChromeDriver << std::endl;
	}

	if (nVerInstalledChrome > 0 && nVerInstalledChrome != nVerChromeDriver)
	{		
		TCHAR* szUrl = GetLatestChromeDriverUrl(nVerInstalledChrome);
		if (szUrl)
		{
			std::wcout << _T("Latest ChromeDriverUrl: ") << szUrl << std::endl;

			TCHAR* pszZipFile = DownloadChromiumZip(szUrl);
			if (pszZipFile)
			{
				std::wcout << _T("Download zip to: ") << pszZipFile << std::endl;

				if (UnzipChromiumZip(pszZipFile))
				{
					std::wcout << _T("Unzip success!") << std::endl;
				}
				else
				{
					std::wcout << _T("Unzip failed") << std::endl;
				}
					
				delete[] pszZipFile;
			}
			delete[] szUrl;
		}
	}
}

