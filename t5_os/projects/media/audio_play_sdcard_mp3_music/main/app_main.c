#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "audio_play.h"
#include "media_service.h"
#include <driver/pwr_clk.h>


extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);


#if ((CONFIG_SYS_CPU0 && CONFIG_SOC_BK7256XX) || (CONFIG_SYS_CPU1 && CONFIG_SOC_BK7236XX))
static void cli_audio_play_sdcard_mp3_music_help(void)
{
	os_printf("audio_play_sdcard_mp3_music {start|stop file_name} \r\n");
}

void cli_audio_play_sdcard_mp3_music_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 2 && argc != 3) {
		cli_audio_play_sdcard_mp3_music_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
#if 0
		bk_set_printf_sync(true);
		extern void bk_enable_white_list(int enabled);
		bk_enable_white_list(1);
//		bk_disable_mod_printf("AUDIO_PIPELINE", 0);
//		bk_disable_mod_printf("AUDIO_ELEMENT", 0);
//		bk_disable_mod_printf("AUDIO_EVENT", 0);
//		bk_disable_mod_printf("AUDIO_MEM", 0);
		bk_disable_mod_printf("FATFS_STREAM", 0);
		bk_disable_mod_printf("MP3_DECODER", 0);
		bk_disable_mod_printf("ONBOARD_SPEAKER", 0);
#endif
		if (BK_OK != audio_play_sdcard_mp3_music_start(argv[2]))
			os_printf("start audio play sdcard mp3 music fail \n");
		else
			os_printf("start audio play sdcard mp3 music ok \n");
	}else if (os_strcmp(argv[1], "stop") == 0) {
		audio_play_sdcard_mp3_music_stop();
	} else {
		cli_audio_play_sdcard_mp3_music_help();
	}

}


#define AUDIO_PLAY_SDCARD_MP3_MUSIC_CMD_CNT  (sizeof(s_audio_play_sdcard_mp3_music_commands) / sizeof(struct cli_command))
static const struct cli_command s_audio_play_sdcard_mp3_music_commands[] =
{
	{"audio_play_sdcard_mp3_music", "audio_play_sdcard_mp3_music {start|stop file_name}", cli_audio_play_sdcard_mp3_music_cmd},
};

int cli_audio_play_sdcard_mp3_music_init(void)
{
	os_printf("cli_audio_play_sdcard_mp3_music_init \n");

	return cli_register_commands(s_audio_play_sdcard_mp3_music_commands, AUDIO_PLAY_SDCARD_MP3_MUSIC_CMD_CNT);
}


void user_app_main(void)
{
	cli_audio_play_sdcard_mp3_music_init();
}
#else
void user_app_main(void)
{
	//bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);
}
#endif		//#if (CONFIG_SYS_CPU0 && CONFIG_SOC_BK7256XX)


int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
	media_service_init();

#if (CONFIG_SYS_CPU0)
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);
#endif

#if (CONFIG_ASDF_WORK_CPU1 && CONFIG_SYS_CPU1)
	cli_audio_play_sdcard_mp3_music_init();
#endif
	return 0;
}
