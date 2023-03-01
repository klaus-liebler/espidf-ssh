#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "minicli.hh"

char next_command[16];
uint8_t next_command_idx = 0;

static void minicli_command_banner(SessionCallback* cb );
static void minicli_command_help(SessionCallback* cb );
static void minicli_command_noop(SessionCallback* cb );

struct minicli_command {
	char *cmd;
	void (*handler)(SessionCallback* cb);
};

struct minicli_command minicli_commands[] = {
	{ "banner", minicli_command_banner},
	{ "help", minicli_command_help},
	{ "", minicli_command_noop},
	{ NULL, NULL }
};

void
minicli_printf(SessionCallback* cb , const char *fmt, ...)
{
	char tmp[512];
	int i;
	va_list args;

	va_start (args, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, args);
	va_end (args);
	for (i = 0; i < sizeof(tmp); i++) {
		if (tmp[i] == 0) {
			return;
		} else {
			if (tmp[i] == '\n')
				cb->printChar('\r');
			cb->printChar(tmp[i]);
		}
	}
}

static void
minicli_command_noop(SessionCallback* cb )
{
}

static const char banner[] = "\n"
	   " _  _     _ _        __      __       _    _\n"
	   "| || |___| | |___    \\ \\    / /__ _ _| |__| |\n"
	   "| __ / -_) | / _ \\_   \\ \\/\\/ / _ \\ '_| / _` |\n"
	   "|_||_\\___|_|_\\___( )   \\_/\\_/\\___/_| |_\\__,_|\n"
	   "                 |/\n"
	   "Welcome to minicli! Type ^D to exit and 'help' for help.\n";


static void
minicli_command_banner(SessionCallback* cb )
{
	minicli_printf(cb, banner);
}

static void
minicli_command_help(SessionCallback* cb)
{
	struct minicli_command *cc;

	cc = minicli_commands;
	while (cc->cmd != NULL) {
		minicli_printf(cb, "	%s\n", cc->cmd);
		cc++;
	}
}

static void
minicli_prompt(SessionCallback* cb )
{
	minicli_printf(cb, "minicli> ");
}

int MiniCli::handleChars(const char *cmd, SessionCallback* cb)
{
	struct minicli_command *cc;

	cc = minicli_commands;
	while (cc->cmd != NULL) {
		if (!strcmp(cmd, cc->cmd)) {
			cc->handler(cb);
			minicli_prompt(cb);
			return 0;
		}
		cc++;
	}
	minicli_printf(cb, "%c? unknown command\n", 7);
	minicli_prompt(cb);
	return 0;
}

int MiniCli::handleChar(char c, SessionCallback* cb)
{
	if (c == '\r') {
		minicli_printf(cb, "\n");
		next_command[next_command_idx] = 0;
		handleChars(next_command, cb);
		next_command_idx = 0;
	} else if (c == 3) {
		// ^C
		minicli_printf(cb, "^C\n");
		next_command[next_command_idx] = 0;
		minicli_prompt(cb);
	} else if (c == 4) {
		// ^D
	} else if (c == 127) {
		// backspace
		if (next_command_idx > 0) {
			minicli_printf(cb, "%c %c", 8, 8);
			next_command_idx--;
		} else {
			minicli_printf(cb, "%c", 7);
		}
	} else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
		if (next_command_idx < sizeof(next_command) - 1) {
			minicli_printf(cb, "%c", c);
			next_command[next_command_idx++] = c;
		}
	} else {
		// invalid chars ignored
	}
	return 1;
}

int MiniCli::beginSession(SessionCallback* cb )
{
	minicli_command_banner(cb);
	minicli_prompt(cb);
	return 0;
}

int MiniCli::endSession(SessionCallback* cb )
{
	return 0;
}