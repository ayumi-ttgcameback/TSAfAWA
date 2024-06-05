#pragma once
class LogBase
{
public:
	virtual void error(const wchar_t* format, ...) = 0;
	virtual void info(const wchar_t* format, ...) = 0;
	virtual void warn(const wchar_t* format, ...) = 0;

};

