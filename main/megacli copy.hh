#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <shell_handler.hh>
#include <array>
#include <vector>
#include <argtable3/argtable3.h>
#include <string>

namespace CLI
{
    constexpr size_t MAX_LINE_LENGTH{160};
    constexpr size_t LINENOISE_DEFAULT_HISTORY_MAX_LEN{16};
    constexpr const char *PROMT_ROOT{"ROOT> "};
    constexpr const char *PROMT_ROOT{"USER> "};
    constexpr size_t PLEN{6};
    constexpr int EVBIT_CURSOR_POS_RECEIVED{(1 << 0)};
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
    enum class KEY_ACTION
    {
        KEY_NULL = 0,   /* NULL */
        CTRL_A = 1,     /* Ctrl+a */
        CTRL_B = 2,     /* Ctrl-b */
        CTRL_C = 3,     /* Ctrl-c */
        CTRL_D = 4,     /* Ctrl-d */
        CTRL_E = 5,     /* Ctrl-e */
        CTRL_F = 6,     /* Ctrl-f */
        CTRL_H = 8,     /* Ctrl-h */
        TAB = 9,        /* Tab */
        CTRL_K = 11,    /* Ctrl+k */
        CTRL_L = 12,    /* Ctrl+l */
        ENTER = 13,     /* Enter */
        CTRL_N = 14,    /* Ctrl-n */
        CTRL_P = 16,    /* Ctrl-p */
        CTRL_T = 20,    /* Ctrl-t */
        CTRL_U = 21,    /* Ctrl+u */
        CTRL_W = 23,    /* Ctrl+w */
        ESC = 27,       /* Escape */
        BACKSPACE = 127 /* Backspace */
    };

    typedef struct linenoiseCompletions
    {
        size_t len;
        char **cvec;
    } linenoiseCompletions;

    typedef void(linenoiseCompletionCallback)(const char *, linenoiseCompletions *);
    typedef char *(linenoiseHintsCallback)(const char *, int *color, int *bold);
    typedef void(linenoiseFreeHintsCallback)(void *);
    void linenoiseSetCompletionCallback(linenoiseCompletionCallback *);
    void linenoiseSetHintsCallback(linenoiseHintsCallback *);
    void linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *);
    void linenoiseAddCompletion(linenoiseCompletions *, const char *);

    class AbstractCommand
    {
    public:
        virtual int Execute(ShellCallback *cb, int argc, char *argv[]) = 0;
        virtual const char *GetName() = 0;
    };

    class MegaCli : public ShellHandler
    {
    public:
        ShellId beginShell(ShellCallback *cb) override;
        int handleChars(const char *chars, size_t len, ShellId shellId, ShellCallback *cb) override;
        void endShell(ShellId shellId, ShellCallback *cb) override;
        MegaCli(){
            rxChars=std::string("");
            line=std::string("");
            rxChars.reserve(MAX_LINE_LENGTH);
            line.reserve(MAX_LINE_LENGTH);
        }

    private:
        int executeLine(ShellId shellId, ShellCallback *cb);
        int printPrompt(ShellId shellId, ShellCallback *cb);
        std::vector<AbstractCommand *> commands;
        std::string rxChars;
        std::string line;

        char buf[MAX_LINE_LENGTH + 1];
        size_t pos{0};        // Current cursor position.
        size_t oldpos{0};     // Previous refresh cursor position.
        size_t len{0};        // Current edited line length.
        size_t cols{80};      // Number of columns in terminal.
        int history_index{0}; // The history index we are currently editing.
        EventGroupHandle_t genericEventGroup;
        linenoiseCompletionCallback *completionCallback{nullptr};
        linenoiseHintsCallback *hintsCallback{nullptr};
        linenoiseFreeHintsCallback *freeHintsCallback{nullptr};

        int getCursorPosition(ShellCallback *cb)
        {
            xEventGroupClearBits(this->genericEventGroup, EVBIT_CURSOR_POS_RECEIVED);
            cb->printf("\x1b[6n");
            EventBits_t uxBits = xEventGroupWaitBits(this->genericEventGroup, EVBIT_CURSOR_POS_RECEIVED, pdTRUE, pdFALSE, pdMS_TO_TICKS(3000));
            if ((uxBits & (EVBIT_CURSOR_POS_RECEIVED)) == (EVBIT_CURSOR_POS_RECEIVED))
            {
                return cols;
            }
            else
            {
                return -1;
            }
        }

        /* Try to get the number of columns in the current terminal, or assume 80 if it fails. */
        int getColumns()
        {
            return 80;
        }

        /* Clear the screen. Used to handle ctrl+l */
        void clearScreen(ShellCallback *cb)
        {
            cb->printf("\x1b[H\x1b[2J");
        }

        void beep(ShellCallback *cb)
        {
            cb->printf("\x7");
        }

        /* ============================== Completion ================================ */

        /* Free a list of completion option populated by linenoiseAddCompletion(). */
        void freeCompletions(linenoiseCompletions *lc)
        {
            size_t i;
            for (i = 0; i < lc->len; i++)
                free(lc->cvec[i]);
            if (lc->cvec != NULL)
                free(lc->cvec);
        }

        int completeLine(ShellCallback *cb)
        {
            char c = 0;
            beep(cb);
            return c;
        }

        /* Register a callback function to be called for tab-completion. */
        void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn)
        {
            completionCallback = fn;
        }

        /* Register a hits function to be called to show hits to the user at the
         * right of the prompt. */
        void linenoiseSetHintsCallback(linenoiseHintsCallback *fn)
        {
            hintsCallback = fn;
        }

        /* Register a function to free the hints returned by the hints callback
         * registered with linenoiseSetHintsCallback(). */
        void linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *fn)
        {
            freeHintsCallback = fn;
        }

        /* This function is used by the callback function registered by the user
         * in order to add completion options given the input string when the
         * user typed <tab>. See the example.c source code for a very easy to
         * understand example. */
        void linenoiseAddCompletion(linenoiseCompletions *lc, const char *str)
        {
            size_t len = strlen(str);
            char *copy, **cvec;

            copy = (char *)malloc(len + 1);
            if (copy == NULL)
                return;
            memcpy(copy, str, len + 1);
            cvec = (char **)realloc(lc->cvec, sizeof(char *) * (lc->len + 1));
            if (cvec == NULL)
            {
                free(copy);
                return;
            }
            lc->cvec = cvec;
            lc->cvec[lc->len++] = copy;
        }

        /* =========================== Line editing ================================= */

        /* We define a very simple "append buffer" structure, that is an heap
         * allocated string where we can append to. This is useful in order to
         * write all the escape sequences in a buffer and flush them to the standard
         * output in a single call, to avoid flickering effects. */
        struct abuf
        {
            char *b;
            int len;
        };

        static void abInit(struct abuf *ab)
        {
            ab->b = NULL;
            ab->len = 0;
        }

        static void abAppend(struct abuf *ab, const char *s, int len)
        {
            char *newbuf = (char *)realloc(ab->b, ab->len + len);

            if (newbuf == nullptr)
                return;
            memcpy(newbuf + ab->len, s, len);
            ab->b = newbuf;
            ab->len += len;
        }

        static void abFree(struct abuf *ab)
        {
            free(ab->b);
        }

        /* Helper of refreshSingleLine() and refreshMultiLine() to show hints
         * to the right of the prompt. */
        void refreshShowHints(struct abuf *ab, int plen)
        {
            char seq[64];
            if (hintsCallback && plen + this->len < this->cols)
            {
                int color = -1, bold = 0;
                char *hint = hintsCallback(this->buf, &color, &bold);
                if (hint)
                {
                    int hintlen = strlen(hint);
                    int hintmaxlen = this->cols - (plen + this->len);
                    if (hintlen > hintmaxlen)
                        hintlen = hintmaxlen;
                    if (bold == 1 && color == -1)
                        color = 37;
                    if (color != -1 || bold != 0)
                        snprintf(seq, 64, "\033[%d;%d;49m", bold, color);
                    else
                        seq[0] = '\0';
                    abAppend(ab, seq, strlen(seq));
                    abAppend(ab, hint, hintlen);
                    if (color != -1 || bold != 0)
                        abAppend(ab, "\033[0m", 4);
                    /* Call the function to free the hint returned. */
                    if (freeHintsCallback)
                        freeHintsCallback(hint);
                }
            }
        }

        /* Single line low level line refresh.
         *
         * Rewrite the currently edited line accordingly to the buffer content,
         * cursor position, and number of columns of the terminal. */
        void refreshSingleLine(ShellCallback *cb)
        {
            char seq[64];
            size_t plen = strlen(PROMT_ROOT);
            char *buf = this->buf;
            size_t len = this->len;
            size_t pos = this->pos;
            struct abuf ab;

            while ((plen + pos) >= this->cols)
            {
                buf++;
                len--;
                pos--;
            }
            while (plen + len > this->cols)
            {
                len--;
            }

            abInit(&ab);
            /* Cursor to left edge */
            snprintf(seq, 64, "\r");
            abAppend(&ab, seq, strlen(seq));
            /* Write the prompt and the current buffer content */
            abAppend(&ab, PROMT_ROOT, plen);
            abAppend(&ab, buf, len);
            /* Show hits if any. */
            refreshShowHints(&ab, plen);
            /* Erase to right */
            snprintf(seq, 64, "\x1b[0K");
            abAppend(&ab, seq, strlen(seq));
            /* Move cursor to original position. */
            snprintf(seq, 64, "\r\x1b[%dC", (int)(pos + plen));
            abAppend(&ab, seq, strlen(seq));
            cb->printf("%.*s", ab.len, ab.b);
            abFree(&ab);
        }
    };
}
