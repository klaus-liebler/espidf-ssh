#pragma once
#include <session_handler.hh>

class MiniCli:public SessionHandler{
    int beginSession(SessionCallback* cb ) override;
    int handleChars(const char* command, SessionCallback* cb ) override;
    int handleChar(char c, SessionCallback* cb ) override;
    int endSession(SessionCallback* cb ) override;    
};


