#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include "driver/spi_master.h"
#include <softap.hh>
#include "sshd.hh"
#include "megacli.hh"
#include "driver/gpio.h"
#include "i2c.hh"
#define TAG "MAIN"

static void
initialize_nvs()
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK( nvs_flash_erase() );
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
}

const char *hardcoded_example_host_key =
	"-----BEGIN OPENSSH PRIVATE KEY-----\n"
	"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\n"
	"-----END OPENSSH PRIVATE KEY-----\n";

std::vector<sshd::User> USERS{
	{"klaus", "klaus", false},
	{"root", "root", true}
};

class I2CDetectCommand:public CLI::AbstractCommand{
	int Execute(ShellCallback *cb, int argc, char *argv[]) override{
			arg_int *busindexArg = arg_intn("F", nullptr, "<n>", 0, 2, "bus index");
            auto end_arg = arg_end(2);
			int busindex{0};

            void *argtable[] = {busindexArg, end_arg};

			FILE *fp = funopen(cb, nullptr, &CLI::Static_writefn, nullptr, nullptr);
			int exitcode{0};

            int nerrors = arg_parse(argc, argv, argtable);
			if (nerrors > 0)
            {
                // Display the error details contained in the arg_end struct.
                arg_print_errors(fp, end_arg, GetName());
                exitcode = 1;
                goto exit;
            }
			if(busindexArg->count>0){
				busindex=busindexArg->ival[0];
			}
			if(I2C::Discover((i2c_port_t)busindex, fp)!=ESP_OK){
				cb->printf("Error while i2cdetect!\r\n");
			}
        exit:
            // deallocate each non-null entry in argtable[]
            arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
            fclose(fp);
            return exitcode;
		
	}

	const char *GetName() override{
		return "i2cdetect";
	}
};

class GpioCommand:public CLI::AbstractCommand{
        int Execute(ShellCallback *cb, int argc, char *argv[]) override{
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
			if(input->count>0){
				int level = gpio_get_level((gpio_num_t)input->ival[0]);
				ESP_LOGI(TAG, "Level of input %i is %i\r\n", input->ival[0], level);
				cb->printf("\r\nLevel of input %i is %i\r\n", input->ival[0], level);
			}
			else if(output->count>0 && level->count>0){
				if(gpio_set_level((gpio_num_t)output->ival[0], (uint32_t)level->ival[0])==ESP_OK){
					cb->printf("\r\nLevel of output %i successfully set to %i\r\n", output->ival[0], level->ival[0]);
				}else{
					cb->printf("\r\nError while setting output %i\r\n", output->ival[0]);
				}
			}
			else{
				cb->printf("Neither input nor output!\r\n");
			}
        exit:
            // deallocate each non-null entry in argtable[]
            arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
            fclose(fp);
            return exitcode;
		}
        const char *GetName() override{
			return "gpio";
		}
};

extern "C" void
app_main(void)
{
	initialize_nvs();
	SOFTAP::init(false, "test", "testtest");
	std::vector<CLI::AbstractCommand*> custom_commands = {new GpioCommand(), new I2CDetectCommand()};
	CLI::MegaCli *cli = new CLI::MegaCli(true, custom_commands);
	sshd::SshDemon::InitAndRunSshD(hardcoded_example_host_key, cli, &USERS);
}

