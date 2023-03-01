#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "minicli.hh"
#include <vector>
#include <argtable3/argtable3.h>
#include "esp_system.h"
#include "esp_vfs_dev.h"
#include "esp_log.h"

#define TAG "CLI"

namespace CLI
{
	static int Static_writefn(void *data, const char *buffer, int size)
	{
		ShellCallback *cb = static_cast<ShellCallback *>(data);
		cb->printf("%.*s", size, buffer);
		return 0;
	}

	class HelloCommand : public AbstractCommand
	{
		int Execute(ShellCallback *cb, int argc, char *argv[]) override
		{
			cb->printf("\r\nHello World!\r\n");
			return 0;
		}
		const char *GetName() override
		{
			return "hello";
		}
	};

	class HelpCommand : public AbstractCommand
	{
		int Execute(ShellCallback *cb, int argc, char *argv[]) override
		{
			cb->printf("\r\nI am the help command\r\n");
			return 0;
		}
		const char *GetName() override
		{
			return "help";
		}
	};

	class SystemInfoCommand : public AbstractCommand
	{
		const char *GetName() override
		{
			return "systeminfo";
		}

		int Execute(ShellCallback *cb, int argc, char *argv[]) override
		{
			uint32_t res = esp_get_free_heap_size();

			int exitcode = 0;
			ESP_LOGI(TAG, "We have %i arguments", argc);
			for (int i = 0; i < argc; i++)
			{
				ESP_LOGI(TAG, "Argument %i is %s", i, argv[i]);
			}

			arg_lit *help = arg_litn(NULL, "help", 0, 1, "display this help and exit");
			arg_lit *version = arg_litn(NULL, "version", 0, 1, "display version info and exit");
			arg_int *level = arg_intn(NULL, "level", "<n>", 0, 1, "foo value");
			arg_lit *verb = arg_litn("v", "verbose", 0, 1, "verbose output");
			arg_file *o = arg_filen("o", NULL, "myfile", 0, 1, "output file");
			arg_file *file = arg_filen(NULL, NULL, "<file>", 1, 100, "input files");
			auto end_arg = arg_end(20);

			void *argtable[] = {help, version, level, verb, o, file, end_arg};

			FILE *fp = funopen(cb, nullptr, &Static_writefn, nullptr, nullptr);

			int nerrors = arg_parse(argc, argv, argtable);
			if (help->count > 0)
			{
				fprintf(stdout, "Usage: %s", GetName());
				arg_print_syntax(stdout, argtable, "\r\n");
				fprintf(stdout, "Demonstrate command-line parsing in argtable3.\r\n");
				arg_print_glossary(stdout, argtable, "  %-25s %s\r\n");
				exitcode = 0;
				goto exit;
			}

			// If the parser returned any errors then display them and exit
			if (nerrors > 0)
			{
				// Display the error details contained in the arg_end struct.
				arg_print_errors(fp, end_arg, GetName());
				fprintf(fp, "\r\nTry '%s --help' for more information.\n", GetName());
				exitcode = 1;
				goto exit;
			}

			cb->printf("Free Heap is %lu, level value was %i\r\n", res, level->ival[0]);
		exit:
			// deallocate each non-null entry in argtable[]
			arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
			fclose(fp);
			return exitcode;
		}
	};

	MiniCli::MiniCli()
	{
		this->commands = {new HelloCommand(), new HelpCommand(), new SystemInfoCommand()};
	}

	int MiniCli::printPrompt(ShellId shellId, ShellCallback *cb)
	{
		if (cb->IsPrivilegedUser())
		{
			cb->printf("\r\n ROOT> ");
		}
		else
		{
			cb->printf("\r\n USER> ");
		}
		return 0;
	}

	int MiniCli::handleChars(const char *chars, size_t len, ShellId shellId, ShellCallback *cb)
	{
		for (int i = 0; i < len; i++)
		{
			char c = chars[i];
			if (c == '\r') // carriage return
			{
				cb->printf("\r\n");
				line[this->pos] = '\0';
				executeLine(shellId, cb);
				pos = 0;
			}
			else if(c='\033'){//suppress escape sequences
				//do nothing
			}
			else if (c == 127) // backspace
			{
				if (pos > 0)
				{
					cb->printf("%c %c", 8, 8); // Backspace Space Backspace!
					pos--;
				}
				else
				{
					cb->printf("%c", 7); // bell sound, because we can not delete the prompt!
				}
			}
			else
			{
				if (pos < MAX_LINE_LENGTH)
				{
					cb->printf("%c", c); // prints single character
					line[pos] = c;
					pos++;
				}
			}
		}

		return 0;
	}

	int MiniCli::executeLine(ShellId shellId, ShellCallback *cb)
	{
		int argc = 0;
		char *p = strtok(this->line, " ");
		char *argv[16];
		while (p != NULL)
		{
			argv[argc++] = p;
			p = strtok(NULL, " ");
		}
		for (auto &cmd : this->commands)
		{
			if (0 == strcmp(cmd->GetName(), argv[0]))
			{
				return cmd->Execute(cb, argc, argv);
			}
		}
		cb->printf(COLOR_BRIGHT_RED "Unknown Command: %s", line);
		return -1;
	}

	ShellId MiniCli::beginShell(ShellCallback *cb)
	{
		ShellId shellId = 0;
		printPrompt(shellId, cb);
		return shellId;
	}

	void MiniCli::endShell(ShellId shellId, ShellCallback *cb)
	{
		return;
	}
}