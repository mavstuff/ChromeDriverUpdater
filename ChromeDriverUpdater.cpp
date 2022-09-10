/*
	ChromeDriverUpdater - checks installed Chrom version and current chromedriver.
	(c) 2022 Artem Moroz <artem.moroz@gmail.com>

*/

#include "pch.h"
#include <iostream>

#include "miniz.h"

const TCHAR g_szChromePath1[] = _T("c:\\Program Files (x86)\\Google\\Chrome\\");
const TCHAR g_szChromePath2[] = _T("c:\\Program Files\\Google\\Chrome\\");

TCHAR g_szWorkingPath[MAX_PATH];
CHAR chProcessBuf[512];

void GetWorkingPath()
{
	GetModuleFileName(NULL, g_szWorkingPath, MAX_PATH);
	PathRemoveFileSpec(g_szWorkingPath);
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

	_tcscpy(pszChormeDriverPath, g_szWorkingPath);
	PathAppend(pszChormeDriverPath, _T("chromedriver.exe"));
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

		if (ReadFile(hChildStd_OUT_Rd, chProcessBuf, sizeof(chProcessBuf) - 1, &dwRead, NULL) && dwRead < sizeof(chProcessBuf) - 1)
		{
			chProcessBuf[dwRead] = 0;
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	CloseHandle(hChildStd_OUT_Rd);
	CloseHandle(hChildStd_OUT_Wr);

	return TRUE;
}

TCHAR* GetLatestChromeDriverUrl(int nVersion)
{

	TCHAR szCommand[255] = { 0 };
	_stprintf(szCommand, _T("curl https://chromedriver.storage.googleapis.com/LATEST_RELEASE_%d"), nVersion);
	
	IStream* stream;
	//Source URL
	
	// URLDownloadToFile returns S_OK on success 
	HRESULT hr = URLOpenBlockingStream(NULL, szLatestUrl, &stream, 0, NULL);
	if (hr == S_OK)
	{
		char buff[128] = { 0 };
		char* p = buff;

		ULONG bytesRead = 0;
		while ((p - buff) < sizeof(buff) - 1  &&
			   stream->Read(p, sizeof(buff) - 1 - (p-buff), &bytesRead) == S_OK)
		{	
			p += bytesRead;
		};
		stream->Release(); //Realease the interface.

		TCHAR szVersion[128] = { 0 };

		MultiByteToWideChar(CP_ACP, 0, buff, -1, szVersion, sizeof(szVersion) / sizeof(szVersion[0]));

		TCHAR* pszZipUrl = new TCHAR[255];
		if (pszZipUrl)
		{
			_stprintf(pszZipUrl, _T("https://chromedriver.storage.googleapis.com/%s/chromedriver_win32.zip"), szVersion);
			return pszZipUrl;
		}
	}
	else
	{
		_tprintf(_T("URLOpenBlockingStream hr = %X, url = %s"), hr, szLatestUrl);
	}
	return NULL;
}


TCHAR* DownloadChromiumZip(TCHAR* pszZipUrl)
{
	TCHAR* pszZipFile = new TCHAR[MAX_PATH];
	if (pszZipFile)
	{
		_tcscpy(pszZipFile, g_szWorkingPath);
		PathAppend(pszZipFile, _T("chromedriver_win32.zip"));
		
		if (URLDownloadToFile(NULL, pszZipUrl, pszZipFile, 0, NULL) == S_OK)
		{
			return pszZipFile;
		}
		
		delete[] pszZipFile;
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

	if (nVerInstalledChrome != nVerChromeDriver)
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

