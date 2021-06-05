#pragma once
#include <Windows.h>
#include <vector>
#include <filesystem>

int randi(int lo, int hi);
std::filesystem::path getModulePath(HMODULE hModule);