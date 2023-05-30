#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include "driver/spi_master.h"
#include <wifi_sta.hh>
#include "sshd.hh"
#include "megacli.hh"
#include "driver/gpio.h"

#include <otacommand.hh>
#include <esp_http_client.h>



#include <lwip/netdb.h>
#include <lwip/dns.h>
#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/api.h>

uint8_t http_scatchpad[2048];

//create on command line just with "ssh-keygen"
constexpr const char *hardcoded_example_host_key =
	R"foo("-----BEGIN OPENSSH PRIVATE KEY-----
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
BAUGBw==
-----END OPENSSH PRIVATE KEY-----)foo";

std::vector<sshd::User> USERS{
	{"klaus", "klaus", false},
	{"root", "root", true}};



#define TAG "MAIN"
class GpioCommand : public CLI::AbstractCommand
{
	int Execute(ShellCallback *cb, int argc, char *argv[]) override
	{
		arg_lit *help = arg_litn(NULL, "help", 0, 1, "display this help and exit");
		arg_int *input = arg_intn("i", "input", "<n>", 0, 1, "read from input");
		arg_int *output = arg_intn("o", "output", "<n>", 0, 1, "write to output");
		arg_int *level = arg_intn("l", "level", "<n>", 0, 1, "level");
		auto end_arg = arg_end(20);

		void *argtable[] = {help, input, output, level, end_arg};

		FILE *fp = funopen(cb, nullptr, &CLI::Static_writefn, nullptr, nullptr);
		int exitcode{0};

		int nerrors = arg_parse(argc, argv, argtable);
		if (help->count > 0)
		{
			cb->printf("Usage: %s\r\n", GetName());
			arg_print_syntax(fp, argtable, "\r\n");
			cb->printf("Set or get GPIO pin\r\n");
			arg_print_glossary(fp, argtable, "  %-25s %s\r\n");
			exitcode = 0;
			goto exit;
		}

		// If the parser returned any errors then display them and exit
		if (nerrors > 0)
		{
			// Display the error details contained in the arg_end struct.
			arg_print_errors(fp, end_arg, GetName());
			fprintf(fp, "\r\nTry '%s --help' for more information.\r\n", GetName());
			exitcode = 1;
			goto exit;
		}
		ESP_LOGI(TAG, "No errors while reading args input=%i, output=%i", input->count, output->count);
		if (input->count > 0)
		{
			int level = gpio_get_level((gpio_num_t)input->ival[0]);
			ESP_LOGI(TAG, "Level of input %i is %i\r\n", input->ival[0], level);
			cb->printf("\r\nLevel of input %i is %i\r\n", input->ival[0], level);
		}
		else if (output->count > 0 && level->count > 0)
		{
			if (gpio_set_level((gpio_num_t)output->ival[0], (uint32_t)level->ival[0]) == ESP_OK)
			{
				cb->printf("\r\nLevel of output %i successfully set to %i\r\n", output->ival[0], level->ival[0]);
			}
			else
			{
				cb->printf("\r\nError while setting output %i\r\n", output->ival[0]);
			}
		}
		else
		{
			cb->printf("Neither input nor output!\r\n");
		}
	exit:
		// deallocate each non-null entry in argtable[]
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		fclose(fp);
		return exitcode;
	}
	const char *GetName() override
	{
		return "gpio";
	}
};

extern "C" void
app_main(void)
{
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(WIFISTA::InitAndRun("smopla", "myosotis2020"));

	std::vector<CLI::AbstractCommand *> custom_commands = {new GpioCommand(), new OTACommand()};
	CLI::MegaCli *cli = new CLI::MegaCli(true, custom_commands);
	cli->InitAndRunCli();
	sshd::SshDemon::InitAndRunSshD(hardcoded_example_host_key, cli, &USERS);

	ip_addr_t addr;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(3000));
		err_t err=netconn_gethostbyname("google.com", &addr);
		if (err!=ERR_OK)
		{
			ESP_LOGE(TAG, "netconn_gethostbyname failed: %d", err);
		}
	}
	
}
