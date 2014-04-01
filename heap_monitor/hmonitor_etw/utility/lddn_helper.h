/*
 *  File   : lddn_helper.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/3/29 15:09:09
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __LDDN_HELPER_H__
#define __LDDN_HELPER_H__

#include <string>

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

// Logical Device Dos Name
class CLDDNHelper
{
public:
	CLDDNHelper()
	{
		WCHAR tmp[MAX_PATH];
		GetWindowsDirectoryW(tmp, MAX_PATH);
		m_windowsDir = tmp;

		buildLDDNMap();
	}
	~CLDDNHelper()
	{
	}

	void DeviceForm2DosForm(LPCWSTR org, WCHAR nor[MAX_PATH])
	{
		if (org[1] == L':')
		{
			wcscpy_s(nor, MAX_PATH, org);
			return;
		}

		// \SystemRoot
		{
			LPCWSTR systemRoot = L"\\SystemRoot";
			size_t cch = wcslen(systemRoot);
			if (_wcsnicmp(org, systemRoot, cch) == 0)
			{
				wcscpy_s(nor, MAX_PATH, m_windowsDir.c_str());
				wcscat_s(nor, MAX_PATH, org + cch);
				return;
			}
		}

		for (size_t i = 0; i < m_lddnVec.size(); ++i)
		{
			const LDDN &item = m_lddnVec[i];
			size_t cch = item.second.length();
			ASSERT(cch > 2);
			if (_wcsnicmp(org, item.second.c_str(), cch) == 0)
			{
				nor[0] = item.first[0];
				nor[1] = L':';
				wcscpy_s(nor + 2, MAX_PATH - 2, org + cch);
				return;
			}
		}

		// Try system drive
		{
			nor[0] = m_windowsDir[0];
			nor[1] = L':';
			wcscpy_s(nor + 2, MAX_PATH - 2, org);
			if (::PathFileExistsW(nor))
				return;
		}

		// Try another drive
		for (size_t i = 0; i < m_lddnVec.size(); ++i)
		{
			const LDDN &item = m_lddnVec[i];
			if (_wcsnicmp(item.first.c_str(), m_windowsDir.c_str(), 1) == 0)
				continue;

			nor[0] = item.first[0];
			nor[1] = L':';
			wcscpy_s(nor + 2, MAX_PATH - 2, org);
			if (::PathFileExistsW(nor))
				return;
		}
	}

protected:
	bool buildLDDNMap()
	{
		WCHAR szTemp[MAX_PATH];
		szTemp[MAX_PATH - 1] = L'\0';
		if (!GetLogicalDriveStringsW(MAX_PATH - 1, szTemp))
			return false;

		WCHAR szName[MAX_PATH];
		DWORD cchName;
		WCHAR szDrive[3] = L" :";
		WCHAR *p = szTemp;
		do {
			// Copy the drive letter to the template string
			*szDrive = *p;

			cchName = QueryDosDeviceW(szDrive, szName, MAX_PATH);
			if (cchName) {
				LDDN item;
				item.first = szDrive;
				item.second = szName;
				m_lddnVec.push_back(item);
			}

			// Go to the next NULL character.
			while (*p++);

		} while (*p); // end of string

		return true;
	}

protected:
	typedef std::pair<std::wstring, std::wstring> LDDN;
	typedef std::vector<LDDN> LDDNVec;
	LDDNVec m_lddnVec;

	std::wstring m_windowsDir;
};

extern CLDDNHelper g_lddnHelper;


#endif /* __LDDN_HELPER_H__ */
