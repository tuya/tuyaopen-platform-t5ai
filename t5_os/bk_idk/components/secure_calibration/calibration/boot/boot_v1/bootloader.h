#ifndef _BOOT_LOADER_H_
#define _BOOT_LOADER_H_



#define  CRC_LEN_32     1


#ifndef MIN
# define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
# define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif


void control_set_to_flash(void);

void bl_init(void) ;

void legacy_boot_main(void);
void PMU_uninit( void ) ;

int bl_aes_key_handler(void*data, u16 length);
int bl_random_key_handler(void*data, u16 length);
int bl_receive_random_key_over_handler(void*data, u16 length);
int bl_secure_boot_enable_handler(void*data, u16 length);
#if 0
int bl_calibration_init(calibration_t *cali_p);
int bl_read_rsa_pubkey_from_flash(calibration_t *cali_p);
int bl_read_encrypted_aes_from_flash(calibration_t *cali_p);
#endif
void move_aes_bootloader_image(void);
#endif // _BOOT_LOADER_H_

