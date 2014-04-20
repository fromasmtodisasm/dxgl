// DXGL
// Copyright (C) 2011-2014 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "stdafx.h"
#include "crc32.h"
#include "cfgmgr.h"
#include "../ddraw/resource.h"
#include <tchar.h>
using namespace std;
#ifdef _UNICODE
typedef wstring tstring;
#else
typedef string tstring;
#endif


TCHAR regkeyglobal[] = _T("Software\\DXGL\\Global");
TCHAR regkeybase[] = _T("Software\\DXGL\\");

DXGLCFG defaultmask;

INT_PTR CALLBACK CompatDialogCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		SetTimer(hWnd, NULL, 3000, NULL);
		return TRUE;
	case WM_TIMER:
		EndDialog(hWnd, 0);
		break;
	default:
		return FALSE;
	}
	return FALSE;
}

void ShowRestartDialog()
{
	BOOL(WINAPI *_GetModuleHandleEx)(DWORD dwFlags, LPCTSTR lpModuleName, HMODULE* phModule) = FALSE;
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	HMODULE hddraw;
	if (hKernel32)
	{
#ifdef _UNICODE
		_GetModuleHandleEx = (BOOL(WINAPI*)(DWORD, LPCTSTR, HMODULE*))GetProcAddress(hKernel32, "GetModuleHandleExW");
#else
		_GetModuleHandleEx = (BOOL(WINAPI*)(DWORD, LPCTSTR, HMODULE*))GetProcAddress(hKernel32, "GetModuleHandleExA");
#endif
	}
	if (_GetModuleHandleEx) _GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)ShowRestartDialog, &hddraw);
	else hddraw = GetModuleHandle(_T("ddraw.dll"));  //For old versions of Windows, may fail but they shouldn't have AppCompat anyways.
	if (hKernel32) FreeLibrary(hKernel32);
	DialogBox(hddraw, MAKEINTRESOURCE(IDD_COMPAT), NULL, CompatDialogCallback);
	LPTSTR cmdline = GetCommandLine();
	LPTSTR cmdline2 = (LPTSTR)malloc((_tcslen(cmdline) + 1)*sizeof(TCHAR));
	_tcscpy(cmdline2, cmdline);
	STARTUPINFO startupinfo;
	PROCESS_INFORMATION procinfo;
	ZeroMemory(&startupinfo, sizeof(STARTUPINFO));
	startupinfo.cb = sizeof(STARTUPINFO);
	CreateProcess(NULL, cmdline2, NULL, NULL, FALSE, 0, NULL, NULL, &startupinfo, &procinfo);
	CloseHandle(procinfo.hProcess);
	CloseHandle(procinfo.hThread);
	free(cmdline2);
	ExitProcess(0);
}

bool AddCompatFlag(LPTSTR flag)
{
	HKEY hKey;
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	BOOL(WINAPI *iswow64)(HANDLE, PBOOL) = NULL;
	if (hKernel32) iswow64 = (BOOL(WINAPI*)(HANDLE, PBOOL))GetProcAddress(hKernel32, "IsWow64Process");
	BOOL is64 = FALSE;
	if (iswow64) iswow64(GetCurrentProcess(), &is64);
	if (hKernel32) FreeLibrary(hKernel32);
	LRESULT error,error2;
	TCHAR filename[MAX_PATH + 1];
	TCHAR buffer[1024];
	error = RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"),
		0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (error == ERROR_SUCCESS)
	{
		GetModuleFileName(NULL, filename, MAX_PATH);
		ZeroMemory(buffer, 1024 * sizeof(TCHAR));
		DWORD sizeout = 1024 * sizeof(TCHAR);
		error2 = RegQueryValueEx(hKey, filename, NULL, NULL, (LPBYTE)buffer, &sizeout);
		if (error2 == ERROR_SUCCESS)
		{
			if (_tcsstr(buffer,flag))
			{ 
				RegCloseKey(hKey);
				return true;
			}
			else
			{
				_tcscat(buffer, _T(" "));
				_tcscat(buffer, flag);
				error2 = RegSetValueEx(hKey, filename, 0, REG_SZ, (BYTE*)buffer, (_tcslen(buffer) + 1)*sizeof(TCHAR));
				if (error2 == ERROR_SUCCESS) ShowRestartDialog();
				else
				{
					RegCloseKey(hKey);
					return false;
				}
			}
		}
		else if (error2 == ERROR_FILE_NOT_FOUND)
		{
			error2 = RegSetValueEx(hKey, filename, 0, REG_SZ, (BYTE*)flag, (_tcslen(flag) + 1)*sizeof(TCHAR));
			if (error2 == ERROR_SUCCESS) ShowRestartDialog();
			else
			{
				RegCloseKey(hKey);
				return false;
			}
		}
		else
		{
			RegCloseKey(hKey);
			return false;
		}

	}
	return false;
}

bool DelCompatFlag(LPTSTR flag, bool initial)
{
	HKEY hKey;
	HKEY hKeyWrite = NULL;
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	BOOL(WINAPI *iswow64)(HANDLE, PBOOL) = NULL;
	if (hKernel32) iswow64 = (BOOL(WINAPI*)(HANDLE, PBOOL))GetProcAddress(hKernel32, "IsWow64Process");
	BOOL is64 = FALSE;
	if (iswow64) iswow64(GetCurrentProcess(), &is64);
	if (hKernel32) FreeLibrary(hKernel32);
	LRESULT error;
	TCHAR filename[MAX_PATH + 1];
	TCHAR buffer[1024];
	TCHAR *bufferpos;
	tstring writekey;
	// Check system first.
	writekey.assign(_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"));
	if (is64) error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	else error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey.c_str(), 0, KEY_READ, &hKey);
	if (error == ERROR_SUCCESS)
	{
		GetModuleFileName(NULL, filename, MAX_PATH);
		ZeroMemory(buffer, 1024 * sizeof(TCHAR));
		DWORD sizeout = 1024 * sizeof(TCHAR);
		if (RegQueryValueEx(hKey, filename, NULL, NULL, (LPBYTE)buffer, &sizeout) == ERROR_SUCCESS)
		{
			bufferpos = _tcsstr(buffer, flag);
			if (bufferpos)
			{
				memmove(bufferpos, bufferpos + _tcslen(flag), (_tcslen(bufferpos + _tcslen(flag))+1)*sizeof(TCHAR));
				if(is64) error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey.c_str(), 0, KEY_WRITE | KEY_WOW64_64KEY, &hKeyWrite);
				else error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey.c_str(), 0, KEY_WRITE, &hKeyWrite);
				if (error == ERROR_SUCCESS)
				{
					error = RegSetValueEx(hKeyWrite, filename, 0, REG_SZ, (BYTE*)buffer, (_tcslen(bufferpos + _tcslen(flag)))*sizeof(TCHAR)+sizeof(TCHAR));
					RegCloseKey(hKeyWrite);
					hKeyWrite = NULL;
				}
				if (error == ERROR_ACCESS_DENIED)
				{
					if (!initial)
					{
						if (MessageBox(NULL, _T("DXGL has detected an incompatible AppCompat flag for the program you are currently running and requires administrative rights to remove it.\nWould you like to continue?"),
							_T("AppCompat error"), MB_YESNO | MB_ICONWARNING) == IDYES)
						{
							tstring command;
							if (is64) command.assign(_T("ADD \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\" /reg:64 /f /v \""));
							else command.assign(_T("ADD \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\" /f /v \""));
							command.append(filename);
							command.append(_T("\" /t REG_SZ /d \""));
							command.append(buffer);
							command.append(_T("\""));
							SHELLEXECUTEINFO info;
							ZeroMemory(&info, sizeof(SHELLEXECUTEINFO));
							info.cbSize = sizeof(SHELLEXECUTEINFO);
							info.lpVerb = _T("runas");
							info.lpFile = _T("reg.exe");
							info.lpParameters = command.c_str();
							info.nShow = SW_SHOWNORMAL;
							info.fMask = SEE_MASK_NOCLOSEPROCESS;
							ShellExecuteEx(&info);
							WaitForSingleObject(info.hProcess, INFINITE);
							GetExitCodeProcess(info.hProcess, (LPDWORD)&error);
						}
						if (!error) ShowRestartDialog();
						else
						{
							MessageBox(NULL, _T("Registry value could not be updated.  Your program may crash as a result."), _T("Error"), MB_OK | MB_ICONWARNING);
							if(hKeyWrite) RegCloseKey(hKeyWrite);
							RegCloseKey(hKey);
						}
						return false;
					}
					else
					{
						if (hKeyWrite) RegCloseKey(hKeyWrite);
						RegCloseKey(hKey);
						return false;
					}
				}
				else if (error == ERROR_SUCCESS) ShowRestartDialog();
			}
		}
		RegCloseKey(hKey);
	}
	// Next check user.
	writekey.assign(_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"));
	error = RegOpenKeyEx(HKEY_CURRENT_USER, writekey.c_str(), 0, KEY_READ, &hKey);
	if (error == ERROR_SUCCESS)
	{
		GetModuleFileName(NULL, filename, MAX_PATH);
		ZeroMemory(buffer, 1024 * sizeof(TCHAR));
		DWORD sizeout = 1024 * sizeof(TCHAR);
		if (RegQueryValueEx(hKey, filename, NULL, NULL, (LPBYTE)buffer, &sizeout) == ERROR_SUCCESS)
		{
			bufferpos = _tcsstr(buffer, flag);
			if (bufferpos)
			{
				memmove(bufferpos, bufferpos + _tcslen(flag), (_tcslen(bufferpos + _tcslen(flag))+1)*sizeof(TCHAR));
				error = RegOpenKeyEx(HKEY_CURRENT_USER, writekey.c_str(), 0, KEY_WRITE, &hKeyWrite);
				if (error == ERROR_SUCCESS)
				{
					error = RegSetValueEx(hKeyWrite, filename, 0, REG_SZ, (BYTE*)buffer, (_tcslen(bufferpos + _tcslen(flag)))*sizeof(TCHAR)+sizeof(TCHAR));
					RegCloseKey(hKeyWrite);
					hKeyWrite = NULL;
				}
				if (error == ERROR_ACCESS_DENIED)
				{
					if (hKeyWrite)
					{
						RegCloseKey(hKeyWrite);
						hKeyWrite = NULL;
					}
					if (!initial)
					{
						if (MessageBox(NULL, _T("DXGL has detected an incompatible AppCompat flag for the program you are currently running and requires administrative rights to remove it.\nWould you like to continue?"),
							_T("AppCompat error"), MB_YESNO | MB_ICONWARNING) == IDYES)
						{
							tstring command;
							command.assign(_T("ADD \"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\" /f /v \""));
							command.append(filename);
							command.append(_T("\" /t REG_SZ /d \""));
							command.append(buffer);
							command.append(_T("\""));
							SHELLEXECUTEINFO info;
							ZeroMemory(&info, sizeof(SHELLEXECUTEINFO));
							info.cbSize = sizeof(SHELLEXECUTEINFO);
							info.lpVerb = _T("runas");
							info.lpFile = _T("reg.exe");
							info.lpParameters = command.c_str();
							info.nShow = SW_SHOWNORMAL;
							info.fMask = SEE_MASK_NOCLOSEPROCESS;
							ShellExecuteEx(&info);
							WaitForSingleObject(info.hProcess, INFINITE);
							GetExitCodeProcess(info.hProcess, (LPDWORD)&error);
						}
						if (!error)
						{
							ShowRestartDialog();
						}
						else MessageBox(NULL, _T("Registry value could not be updated.  Your program may crash as a result."), _T("Error"), MB_OK | MB_ICONWARNING);
						if (hKeyWrite) RegCloseKey(hKeyWrite);
						return false;
					}
					else
					{
						if (hKeyWrite) RegCloseKey(hKeyWrite);
						return false;
					}
				}
				else if (error == ERROR_SUCCESS)
				{
					ShowRestartDialog();
				}
			}
		}
		RegCloseKey(hKey);
	}
	return true;
}

void GetDirFromPath(LPTSTR path)
{
	int len = _tcslen(path);
	for(int i = len; i > 0; i--)
	{
		if((path[i] == '\\') || (path[i] == '/'))
		{
			path[i] = 0;
			break;
		}
	}
}

int FindStringInMultiSz(LPTSTR multisz, LPCTSTR comp)
{
	LPTSTR str = multisz;
	while(str[0] != 0)
	{
		if(!_tcscmp(str,comp)) return 1;
		str += (_tcslen(str) + 1);
	}
	return 0;
}

void AddStringToMultiSz(LPTSTR multisz, LPCTSTR string)
{
	LPTSTR str = multisz;
	while(str[0] != 0)
	{
		str += (_tcslen(str) + 1);
	}
	_tcscpy(str,string);
}


bool ReadBool(HKEY hKey, bool original, bool &mask, LPCTSTR value)
{
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		mask = true;
		if(dwOut) return true;
		else return false;
	}
	else
	{
		mask = false;
		return original;
	}
}

DWORD ReadDWORD(HKEY hKey, DWORD original, DWORD &mask, LPCTSTR value)
{
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		mask = 1;
		return dwOut;
	}
	else
	{
		mask = 0;
		return original;
	}
}

float ReadFloat(HKEY hKey, float original, float &mask, LPCTSTR value)
{
	float dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		mask = 1.0f;
		return dwOut;
	}
	else
	{
		mask = 0.0f;
		return original;
	}
}

void ReadPath(HKEY hKey, TCHAR *path, TCHAR *mask, LPCTSTR value)
{
	DWORD sizeout = MAX_PATH*sizeof(TCHAR);
	DWORD regsz = REG_SZ;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regsz,(LPBYTE)path,&sizeout);
	if(error == ERROR_SUCCESS) mask[0] = 0xFF;
	else
	{
		mask[0] = 0;
		path[0] = 0;
	}
}

void ReadSettings(HKEY hKey, DXGLCFG *cfg, DXGLCFG *mask, bool global, bool dll, LPTSTR dir)
{
	DXGLCFG *cfgmask;
	if(mask) cfgmask = mask;
	else cfgmask = &defaultmask;
	TCHAR path[MAX_PATH+1];
	LONG error;
	DWORD regmultisz = REG_MULTI_SZ;
	DWORD sizeout=4;
	cfg->scaler = ReadDWORD(hKey,cfg->scaler,cfgmask->scaler,_T("ScalingMode"));
	cfg->colormode = ReadBool(hKey,cfg->colormode,cfgmask->colormode,_T("ChangeColorDepth"));
	cfg->scalingfilter = ReadDWORD(hKey,cfg->scalingfilter,cfgmask->scalingfilter,_T("ScalingFilter"));
	cfg->texfilter = ReadDWORD(hKey,cfg->texfilter,cfgmask->texfilter,_T("TextureFilter"));
	cfg->anisotropic = ReadDWORD(hKey,cfg->anisotropic,cfgmask->anisotropic,_T("AnisotropicFiltering"));
	cfg->msaa = ReadDWORD(hKey,cfg->msaa,cfgmask->msaa,_T("Antialiasing"));
	cfg->aspect3d = ReadDWORD(hKey,cfg->aspect3d,cfgmask->aspect3d,_T("AdjustAspectRatio"));
	cfg->highres = ReadBool(hKey,cfg->highres,cfgmask->highres,_T("AdjustPrimaryResolution"));
	ReadPath(hKey,cfg->shaderfile,cfgmask->shaderfile,_T("ShaderFile"));
	cfg->SortModes = ReadDWORD(hKey,cfg->SortModes,cfgmask->SortModes,_T("SortModes"));
	cfg->AllColorDepths = ReadBool(hKey,cfg->AllColorDepths,cfgmask->AllColorDepths,_T("AllColorDepths"));
	cfg->ExtraModes = ReadBool(hKey,cfg->ExtraModes,cfgmask->ExtraModes,_T("ExtraModes"));
	cfg->vsync = ReadDWORD(hKey,cfg->vsync,cfgmask->vsync,_T("VSync"));
	cfg->TextureFormat = ReadDWORD(hKey,cfg->TextureFormat,cfgmask->TextureFormat,_T("TextureFormat"));
	cfg->TexUpload = ReadDWORD(hKey,cfg->TexUpload,cfgmask->TexUpload,_T("TexUpload"));
	cfg->Windows8Detected = ReadBool(hKey,cfg->Windows8Detected,cfgmask->Windows8Detected,_T("Windows8Detected"));
	cfg->DPIScale = ReadDWORD(hKey,cfg->DPIScale,cfgmask->DPIScale,_T("DPIScale"));
	cfg->aspect = ReadFloat(hKey, cfg->aspect, cfgmask->aspect, _T("ScreenAspect"));
	if(!global && dll)
	{
		LPTSTR paths;
		sizeout = 0;
		if(!dir) GetModuleFileName(NULL,path,MAX_PATH);
		else _tcsncpy(path,dir,MAX_PATH+1);
		GetDirFromPath(path);
		error = RegQueryValueEx(hKey,_T("InstallPaths"),NULL,&regmultisz,NULL,&sizeout);
		if(error == ERROR_FILE_NOT_FOUND)
		{
			sizeout = (_tcslen(path)*2)+4;
			paths = (LPTSTR)malloc(sizeout*sizeof(TCHAR));
			if(!paths) return;
			ZeroMemory(paths,sizeout*sizeof(TCHAR));
			_tcscpy(paths,path);
			error = RegSetValueEx(hKey,_T("InstallPaths"),0,REG_MULTI_SZ,(LPBYTE)paths,sizeout);
			free(paths);
		}
		else
		{
			paths = (LPTSTR)malloc(sizeout+(MAX_PATH*2)+4);
			if(!paths) return;
			ZeroMemory(paths,sizeout+(MAX_PATH*2)+4);
			error = RegQueryValueEx(hKey,_T("InstallPaths"),NULL,&regmultisz,(LPBYTE)paths,&sizeout);
			if(!FindStringInMultiSz(paths,path))
			{
				AddStringToMultiSz(paths,path);
				sizeout += (MAX_PATH*2)+4;
				error = RegSetValueEx(hKey,_T("InstallPaths"),0,REG_MULTI_SZ,(LPBYTE)paths,sizeout);
			}
			free(paths);
		}
	}
}

void WriteBool(HKEY hKey, bool value, bool mask, LPCTSTR name)
{
	const DWORD one = 1;
	const DWORD zero = 0;
	if(mask)
	{
		if(value) RegSetValueEx(hKey,name,0,REG_DWORD,(BYTE*)&one,4);
		else RegSetValueEx(hKey,name,0,REG_DWORD,(BYTE*)&zero,4);
	}
	else RegDeleteValue(hKey,name);
}

void WriteDWORD(HKEY hKey, DWORD value, DWORD mask, LPCTSTR name)
{
	if(mask) RegSetValueEx(hKey,name,0,REG_DWORD,(BYTE*)&value,4);
	else RegDeleteValue(hKey,name);
}
void WritePath(HKEY hKey, const TCHAR *path, const TCHAR *mask, LPCTSTR name)
{
	if(mask[0]) RegSetValueEx(hKey,name,0,REG_SZ,(BYTE*)path,(_tcslen(path)+1)*sizeof(TCHAR));
	else RegDeleteValue(hKey,name);
}
void WriteFloat(HKEY hKey, float value, float mask, LPCTSTR name)
{
	if (abs(mask) > 0.5f) RegSetValueEx(hKey, name, 0, REG_DWORD, (BYTE*)&value, 4);
	else RegDeleteValue(hKey, name);
}

void WriteSettings(HKEY hKey, const DXGLCFG *cfg, const DXGLCFG *mask, bool global)
{
	const DXGLCFG *cfgmask;
	if(mask) cfgmask = mask;
	else cfgmask = &defaultmask;
	memset(&defaultmask,1,sizeof(DXGLCFG));
	defaultmask.aspect = 1.0f;
	WriteDWORD(hKey,cfg->scaler,cfgmask->scaler,_T("ScalingMode"));
	WriteBool(hKey,cfg->colormode,cfgmask->colormode,_T("ChangeColorDepth"));
	WriteDWORD(hKey,cfg->scalingfilter,cfgmask->scalingfilter,_T("ScalingFilter"));
	WriteDWORD(hKey,cfg->texfilter,cfgmask->texfilter,_T("TextureFilter"));
	WriteDWORD(hKey,cfg->anisotropic,cfgmask->anisotropic,_T("AnisotropicFiltering"));
	WriteDWORD(hKey,cfg->msaa,cfgmask->msaa,_T("Antialiasing"));
	WriteDWORD(hKey,cfg->aspect3d,cfgmask->aspect3d,_T("AdjustAspectRatio"));
	WriteBool(hKey,cfg->highres,cfgmask->highres,_T("AdjustPrimaryResolution"));
	WritePath(hKey,cfg->shaderfile,cfgmask->shaderfile,_T("ShaderFile"));
	WriteDWORD(hKey,cfg->SortModes,cfgmask->SortModes,_T("SortModes"));
	WriteBool(hKey,cfg->AllColorDepths,cfgmask->AllColorDepths,_T("AllColorDepths"));
	WriteBool(hKey,cfg->ExtraModes,cfgmask->ExtraModes,_T("ExtraModes"));
	WriteDWORD(hKey,cfg->vsync,cfgmask->vsync,_T("VSync"));
	WriteDWORD(hKey,cfg->TextureFormat,cfgmask->TextureFormat,_T("TextureFormat"));
	WriteDWORD(hKey,cfg->TexUpload,cfgmask->TexUpload,_T("TexUpload"));
	WriteBool(hKey,cfg->Windows8Detected,cfgmask->Windows8Detected,_T("Windows8Detected"));
	WriteDWORD(hKey,cfg->DPIScale,cfgmask->DPIScale,_T("DPIScale"));
	WriteFloat(hKey, cfg->aspect, cfgmask->aspect, _T("ScreenAspect"));
}

tstring newregname;

LPTSTR MakeNewConfig(LPTSTR path)
{
	HKEY hKey;
	DXGLCFG tmp;
	TCHAR crcstr[10];
	unsigned long crc;
	FILE *file = _tfopen(path,_T("rb"));
	if(file != NULL) Crc32_ComputeFile(file,&crc);
	else crc = 0;
	_itot(crc,crcstr,16);
	if(file) fclose(file);
	tstring regkey = regkeybase;
	int i;
	TCHAR filename[MAX_PATH+1];
	_tcsncpy(filename,path,MAX_PATH);
	filename[MAX_PATH] = 0;
	for(i = _tcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
	i++;
	regkey.append(&filename[i]);
	regkey.append(_T("-"));
	regkey.append(crcstr);
	newregname = &filename[i];
	newregname.append(_T("-"));
	newregname.append(crcstr);
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,&tmp,NULL,false,true,path);
	RegCloseKey(hKey);
	return (LPTSTR)newregname.c_str();
}

bool IsInstalledDXGLTest(LPCTSTR path)
{
	LRESULT err;
	TCHAR dir[MAX_PATH+1];
	TCHAR cmp[MAX_PATH+1];
	_tcsncpy(dir,path,MAX_PATH);
	GetDirFromPath(dir);
	HKEY hKey;
	DWORD sizeout = MAX_PATH*sizeof(TCHAR);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\DXGL"),0,KEY_READ,&hKey) != ERROR_SUCCESS)
		return false;
	err = RegQueryValueEx(hKey,_T("InstallDir"),NULL,NULL,(LPBYTE)cmp,&sizeout);
	if(err != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);
	if(!_tcsnicmp(dir,cmp,MAX_PATH)) return true;
	else return false;
}

void GetCurrentConfig(DXGLCFG *cfg, bool initial)
{
	HKEY hKey;
	unsigned long crc;
	tstring regkey;
	FILE *file;
	TCHAR filename[MAX_PATH+1];
	TCHAR crcstr[10];
	GetModuleFileName(NULL,filename,MAX_PATH);
	if(IsInstalledDXGLTest(filename))
	{
		regkey = regkeybase;
		regkey.append(_T("DXGLTestApp"));
	}
	else
	{
		file = _tfopen(filename,_T("rb"));
		if(file != NULL) Crc32_ComputeFile(file,&crc);
		else crc = 0;
		_itot(crc,crcstr,16);
		if(file) fclose(file);
		regkey = regkeybase;
		int i;
		for(i = _tcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
		i++;
		regkey.append(&filename[i]);
		regkey.append(_T("-"));
		regkey.append(crcstr);
	}
	GetGlobalConfig(cfg, initial);
	if (initial) RegOpenKeyEx(HKEY_CURRENT_USER, regkey.c_str(), 0, KEY_READ, &hKey);
	else RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	if (hKey)
	{
		ReadSettings(hKey, cfg, NULL, false, true, NULL);
		RegCloseKey(hKey);
	}
	hKey = NULL;
	HMODULE hSHCore = NULL;
	HMODULE hUser32 = NULL;
	if (cfg->DPIScale == 2)	AddCompatFlag(_T("HIGHDPIAWARE"));
	else DelCompatFlag(_T("HIGHDPIAWARE"),initial);
	if (initial)
	{
		if (cfg->DPIScale == 1)
		{
			bool DPIAwarePM = false;
			hSHCore = LoadLibrary(_T("SHCore.dll"));
			if (hSHCore)
			{
				HRESULT(WINAPI *_SetProcessDpiAwareness)(DWORD value)
					= (HRESULT(WINAPI*)(DWORD))GetProcAddress(hSHCore, "SetProcessDpiAwareness");
				if (_SetProcessDpiAwareness)
				{
					DPIAwarePM = true;
					_SetProcessDpiAwareness(2);
				}
			}
			if (!DPIAwarePM)
			{
				hUser32 = LoadLibrary(_T("User32.dll"));
				if (hUser32)
				{
					BOOL(WINAPI *_SetProcessDPIAware)()
						= (BOOL(WINAPI*)())GetProcAddress(hUser32, "SetProcessDPIAware");
					if (_SetProcessDPIAware) _SetProcessDPIAware();
				}
			}
			if (hSHCore) FreeLibrary(hSHCore);
			if (hUser32) FreeLibrary(hUser32);
		}
	}
	DelCompatFlag(_T("DWM8And16BitMitigation"), initial);
}
void GetGlobalConfig(DXGLCFG *cfg, bool initial)
{
	HKEY hKey;
	ZeroMemory(cfg,sizeof(DXGLCFG));
	cfg->DPIScale = 1;
	if (initial) RegOpenKeyEx(HKEY_CURRENT_USER, regkeyglobal, 0, KEY_READ, &hKey);
	else RegCreateKeyEx(HKEY_CURRENT_USER, regkeyglobal, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (hKey)
	{
		ReadSettings(hKey, cfg, NULL, true, false, NULL);
		if (!cfg->Windows8Detected)
		{
			OSVERSIONINFO osver;
			osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osver);
			if (osver.dwMajorVersion > 6) cfg->Windows8Detected = true;
			if ((osver.dwMajorVersion == 6) && (osver.dwMinorVersion >= 2)) cfg->Windows8Detected = true;
			if (cfg->Windows8Detected) cfg->AllColorDepths = true;
		}
		RegCloseKey(hKey);
	}
}

void GetConfig(DXGLCFG *cfg, DXGLCFG *mask, LPCTSTR name)
{
	HKEY hKey;
	tstring regkey = regkeybase;
	regkey.append(name);
	ZeroMemory(cfg,sizeof(DXGLCFG));
	cfg->DPIScale = 1;
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,cfg,mask,false,false,NULL);
	RegCloseKey(hKey);
}
void SetGlobalConfig(const DXGLCFG *cfg)
{
	HKEY hKey;
	RegCreateKeyEx(HKEY_CURRENT_USER,regkeyglobal,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	WriteSettings(hKey,cfg,NULL,true);
	RegCloseKey(hKey);
}

void SetConfig(const DXGLCFG *cfg, const DXGLCFG *mask, LPCTSTR name)
{
	HKEY hKey;
	tstring regkey = regkeybase;
	regkey.append(name);
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	WriteSettings(hKey,cfg,mask,false);
	RegCloseKey(hKey);
}
