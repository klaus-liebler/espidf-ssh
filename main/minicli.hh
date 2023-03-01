#pragma once
#include <shell_handler.hh>
#include <array>
#include <vector>
#include <argtable3/argtable3.h>
namespace CLI
{
    constexpr size_t MAX_LINE_LENGTH{160};
/*
    constexpr const char *COLOR_RESET = "\x1B[0m";
    constexpr const char *COLOR_BLACK = "\x1B[0;30m";
    constexpr const char *COLOR_RED = "\x1B[0;31m";
    constexpr const char *COLOR_GREEN = "\x1B[0;32m";
    constexpr const char *COLOR_YELLOW = "\x1B[0;33m";
    constexpr const char *COLOR_BLUE = "\x1B[0;34m";
    constexpr const char *COLOR_MAGENTA = "\x1B[0;35m";
    constexpr const char *COLOR_CYAN = "\x1B[0;36m";
    constexpr const char *COLOR_WHITE = "\x1B[0;37m";
    constexpr const char *COLOR_BOLD_ON = "\x1B[1m";
    constexpr const char *COLOR_BOLD_OFF = "\x1B[22m";
    constexpr const char *COLOR_BRIGHT_BLACK = "\x1B[0;90m";
    constexpr const char *COLOR_BRIGHT_RED = "\x1B[0;91m";
    constexpr const char *COLOR_BRIGHT_GREEN = "\x1B[0;92m";
    constexpr const char *COLOR_BRIGHT_YELLOW = "\x1B[0;99m";
    constexpr const char *COLOR_BRIGHT_BLUE = "\x1B[0;94m";
    constexpr const char *COLOR_BRIGHT_MAGENTA = "\x1B[0;95m";
    constexpr const char *COLOR_BRIGHT_CYAN = "\x1B[0;96m";
    constexpr const char *COLOR_BRIGHT_WHITE = "\x1B[0;97m";
    constexpr const char *COLOR_UNDERLINE = "\x1B[4m";
    constexpr const char *COLOR_BRIGHT_RED_BACKGROUND = "\x1B[41;1m";
    */
   #define COLOR_RESET "\x1B[0m"
#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_YELLOW "\x1B[0;33m"
#define COLOR_BLUE "\x1B[0;34m"
#define COLOR_MAGENTA "\x1B[0;35m"
#define COLOR_CYAN "\x1B[0;36m"
#define COLOR_WHITE "\x1B[0;37m"
#define COLOR_BOLD_ON "\x1B[1m"
#define COLOR_BOLD_OFF "\x1B[22m"
#define COLOR_BRIGHT_BLACK "\x1B[0;90m"
#define COLOR_BRIGHT_RED "\x1B[0;91m"
#define COLOR_BRIGHT_GREEN "\x1B[0;92m"
#define COLOR_BRIGHT_YELLOW "\x1B[0;99m"
#define COLOR_BRIGHT_BLUE "\x1B[0;94m"
#define COLOR_BRIGHT_MAGENTA "\x1B[0;95m"
#define COLOR_BRIGHT_CYAN "\x1B[0;96m"
#define COLOR_BRIGHT_WHITE "\x1B[0;97m"
#define COLOR_UNDERLINE "\x1B[4m"
#define COLOR_BRIGHT_RED_BACKGROUND "\x1B[41;1m"
#define CURSOR_BACKWARD(N) "\033[ND"
#define CURSOR_FORWARD(N) "\033[NC"
    class AbstractCommand
    {
    public:
        virtual int Execute(ShellCallback *cb, int argc, char *argv[]) = 0;
        virtual const char *GetName() = 0;
    };

    class MiniCli : public ShellHandler
    {
        public:
        ShellId beginShell(ShellCallback *cb) override;
        int handleChars(const char *chars, size_t len, ShellId shellId, ShellCallback *cb) override;
        void endShell(ShellId shellId, ShellCallback *cb) override;
        MiniCli();
        

        private:
        int executeLine(ShellId shellId, ShellCallback *cb);
        int printPrompt(ShellId shellId, ShellCallback *cb);
        std::vector<AbstractCommand*> commands;
        char line[MAX_LINE_LENGTH+1];
        size_t pos{0};
    };
}
