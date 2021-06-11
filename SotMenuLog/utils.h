#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

class utils
{
public:
	utils() = delete;
	utils(const utils&) = delete;
	utils& operator=(const utils& o) = delete;
	~utils() = delete;

	static DWORD GetPIDFromName(const wchar_t* name);
};