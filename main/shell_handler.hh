#pragma once
#include <stdint.h>
#include <stddef.h>

class ShellCallback{
	public:
	virtual void printChar(char c)=0;
	virtual size_t printf(const char *fmt, ...)=0;
	virtual bool IsPrivilegedUser()=0;

	ShellCallback(){}
	virtual ~ShellCallback(){}
};

typedef uint32_t ShellId;

class ShellHandler{
	public:
	virtual ShellId beginShell(ShellCallback* cb)=0;
	virtual int handleChars(const char* chars, size_t len, ShellId shellId, ShellCallback* cb )=0;
	virtual void endShell(ShellId shellId, ShellCallback* cb)=0;
};


	