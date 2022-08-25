#include "pch.h"
#include <iostream>

#include "miniz.h"

const TCHAR g_szChromePath[] = _T("c:\\Program Files (x86)\\Google\\Chrome\\");

TCHAR g_szWorkingPath[MAX_PATH];

void GetWorkingPath()
{
	GetModuleFileName(NULL, g_szWorkingPath, MAX_PATH);
	PathRemoveFileSpec(g_szWorkingPath);
}

int GetInstalledChormeVersion()
{
	TCHAR* szExePath = new TCHAR[MAX_PATH];
	_tcscpy(szExePath, g_szChromePath);
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

TCHAR* GetLatestChromeDriverUrl(int nVersion)
{
	TCHAR szLatestUrl[255];
	_stprintf(szLatestUrl, _T("https://chromedriver.storage.googleapis.com/LATEST_RELEASE_%d"), nVersion);
	
	IStream* stream;
	//Source URL

	// URLDownloadToFile returns S_OK on success 
	if (URLOpenBlockingStream(0, szLatestUrl, &stream, 0, 0) == S_OK)
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
    
	int nVer = GetInstalledChormeVersion();

	if (nVer > 0)
	{
		std::wcout << _T("Found installed chrome version: ") << nVer << std::endl;
		TCHAR* szUrl = GetLatestChromeDriverUrl(nVer);
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

