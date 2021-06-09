#include "../framework/pch-sdk.h"
#include "logger.h"
#include <sstream>
#include <iostream>
#include "utility.h"

Logger Log;

void Logger::Create()
{
	this->logPipe = CreateFile(TEXT("\\\\.\\pipe\\SotMenuLogPipe"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
}

void Logger::Write(std::string verbosity, std::string source, std::string message)
{
	std::stringstream ss;
	ss << "[" << verbosity << " - " << source << "] " << message << std::endl;
	std::cout << ss.str();

	if (this->logPipe != INVALID_HANDLE_VALUE)
	{
		WriteFile(this->logPipe,
			ss.str().c_str(),
			ss.str().length() + 1,   // = length of string + terminating '\0' !!!
			&dwWritten,
			NULL);
	}
}

void Logger::Debug(std::string source, std::string message)
{
	Write("DEBUG", source, message);
}

void Logger::Error(std::string source, std::string message)
{
	Write("ERROR", source, message);
}

void Logger::Info(std::string source, std::string message)
{
	Write("INFO", source, message);
}

void Logger::Debug(std::string message)
{
	Debug("SOTM", message);
}

void Logger::Error(std::string message)
{
	Error("SOTM", message);
}

void Logger::Info(std::string message)
{
	Info("SOTM", message);
}

void Logger::Close() {
	CloseHandle(this->logPipe);
}