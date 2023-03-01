#pragma once
#include <stdint.h>
#include <stddef.h>

class SessionCallback{
	public:
	virtual void printChar(char c)=0;
	virtual size_t printf(const char *fmt, ...)=0;

	SessionCallback(){}
	virtual ~SessionCallback(){}
};

class SessionHandler{
	public:
	virtual int beginSession(SessionCallback* cb)=0;
	virtual int handleChars(const char* chars, SessionCallback* cb )=0;
	virtual int handleChar(char c, SessionCallback* cb )=0;
	virtual int endSession(SessionCallback* cb)=0;
};
	
