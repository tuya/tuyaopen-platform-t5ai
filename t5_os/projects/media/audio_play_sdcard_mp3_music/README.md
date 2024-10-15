## Project：audio

## Life Cycle：2023-07-15 ~~ 2024-01-15

## Application：media audio_play_sdcard_mp3_music

## Special Macro Configuration Description:
1、CONFIG_AUDIO_DAC          			// CONFIG AUDIO DAC ENABLE
2、CONFIG_ASDF       					// CONFIG AUDIO Software Development Framework
3、CONFIG_ASDF_ONBOARD_SPEAKER_STREAM   // CONFIG onboard speaker stream
4、CONFIG_ASDF_FATFS_STREAM				// CONFIG Fatfs stream
5、CONFIG_ASDF_MP3_DECODER				// CONFIG mp3 decoder

## Complie Command:
1、make bk7256 PROJECT=media/audio_play_sdcard_mp3_music
2、make bk7258 PROJECT=media/audio_play_sdcard_mp3_music

## CPU:
1、BK7256: CPU0
2、BK7258：CPU0 + CPU1

## Media: AUDIO
