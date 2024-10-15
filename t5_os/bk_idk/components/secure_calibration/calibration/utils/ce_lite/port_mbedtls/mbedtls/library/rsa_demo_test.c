#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/platform.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/rsa_internal.h"
#include "mbedtls/oid.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"
#include "constant_time_internal.h"
#include "mbedtls/constant_time.h"
#include "mbedtls/bignum.h"
#if defined(MBEDTLS_PKCS1_V21)
#include "mbedtls/md.h"
#endif
#include <stdlib.h>
#include "mbedtls/platform.h"

	

unsigned char public_key_buffer[2048]={
"-----BEGIN PUBLIC KEY-----\r\n" 
"MIIBITANBgkqhkiG9w0BAQEFAAOCAQ4AMIIBCQKCAQEAoX1BDwhKCllM1dTVc5EX\r\n"
"tf1TUly89TwGsKQJ3qz8U3/0Av7/wAoV8xRyN9sAXPUMK74IG9eBd6huDHLCbCAL\r\n"
"9CrUS2nfLLlz0LhdS2aj9RT3QOMRaiafmkPOFIZGY1LUqUYAB3uM0jriSuJG6G3j\r\n"
"5U+xoS0VUE4l9bhuCXJEG61meEa0A4NAgXptL6LAqhvqkRI+yA27fKiG9HBo0GnG\r\n"
"fYo8bopmOgpGPxdqRGDQrsn5sTvpXJfhfMyC297qIubH7K17aTMI3rVseELVlinJ\r\n"
"Jp2t129eyME9DfuHZUrkBqhrE0rNk22jjz0YMyUJLvNGh7cM57xZ6L1+V6NqwsZF\r\n"
"kwICEAE=\r\n"
"-----END PUBLIC KEY-----"
};
unsigned char private_key_buffer[2048] ={
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEoQIBAAKCAQEAoX1BDwhKCllM1dTVc5EXtf1TUly89TwGsKQJ3qz8U3/0Av7/\r\n"
"wAoV8xRyN9sAXPUMK74IG9eBd6huDHLCbCAL9CrUS2nfLLlz0LhdS2aj9RT3QOMR\r\n"
"aiafmkPOFIZGY1LUqUYAB3uM0jriSuJG6G3j5U+xoS0VUE4l9bhuCXJEG61meEa0\r\n"
"A4NAgXptL6LAqhvqkRI+yA27fKiG9HBo0GnGfYo8bopmOgpGPxdqRGDQrsn5sTvp\r\n"
"XJfhfMyC297qIubH7K17aTMI3rVseELVlinJJp2t129eyME9DfuHZUrkBqhrE0rN\r\n"
"k22jjz0YMyUJLvNGh7cM57xZ6L1+V6NqwsZFkwICEAECggEAA4kg1eIbiir7jW+N\r\n"
"bWxB3ANT/3j587k2i7JcfloUK74v2czIygzK8ZIRfz38s9eAVDZd95Y2ymb7bHI1\r\n"
"1QQQNTPwEMW9L1lggahvEOQPkwMzvVS9TcylV6NeuJ3tTvsYrRb4mjnpCIOrfSJl\r\n"
"nZObpYM7Ybl8Wurt1fKZMoUASpzeMe5pQ7tgR/z3e1XbMxnkt6t06jmkHOvA+ld+\r\n"
"JHvnKWbvheQbtXYX4DeOO9B25pvVkMA6bafEKtr5iMjpJmeA8OINvOCQXfAFTo/T\r\n"
"8jTx3xrcWgkGRgVM7TUPoQZ5QMPxVipTQ85XaZd1UNYlLvyzrFLxpu3spzysvnMR\r\n"
"FiuQLQKBgQDZn3BtpiuvZeAoPp3Ot0Dqze8tSFzZl67AP1sgzwv/AXsld/MPnTIh\r\n"
"pW7D1/XbkvXS3dKr394AidLjegQBVXokJ/2XNsEtjz1LTtw24zgE057XsYfAxd/z\r\n"
"lCu2IjvraOkqmjxmYMse2xpT3mTbnNmN2AWXWLwN0B2/ccLKRT+vpwKBgQC996wB\r\n"
"vikw//Fw9pdAkzJXdNfKKXOSs3dm0Flq5pPqC1dfum4tFrzgqBOnlQzum9sgB7JL\r\n"
"PQsr4WnsFfcAdXfgGq3GWVXjM5dTTag8ns8qhxROnpUPT6NGQy3CEHKQYqv9nDYp\r\n"
"FjS6htGlemxia6Ly/JZE0kpxQ5bNhCui6M7YNQKBgC/AsnHpyhntjUoAnjjrSdrr\r\n"
"ZDh8yhRgtJ+H71FOXIw/X0FVLlYX7vHTYgl7XpA4Tc9bGft5evIPHwdc4fLrxAKP\r\n"
"XgLAKIOnpcmaxUj3b2SY6N5pYGWZFIX9tXyx2Gk0g2TTbOrpr/nWGXWxBLKoUnqq\r\n"
"Lxc2VmDiD2Z+YArEZqPzAoGAQRmTnF8orAD68slZwsZIHPh9b7S6EeflryL4dt0b\r\n"
"YcNG/qA9QCCJNW5F2FPR8arRrx+xYjQJU7VIuiDlXBKCruY2rVJKDAAvG9wgqLzj\r\n"
"tJiDrbgAF/4MVpAd7WZM4C/PdzbYIxMrYDmywUyh3J6bMRcDEeX2pOicZyubO2OW\r\n"
"rTECgYANFwi27oxfg7PiRWx4ZqMSF3DA5WDDAFubxHvX87No00vJHD2jW8326CD1\r\n"
"u1i5NXCaTesHpJml98+Czq9kewozOYF6YN/SfJ4jKA/BVR9Fno8+75Vi7T+lt8yS\r\n"
"KAWCOFW9UUuXu9pfEyMsJ9Xys8GpX+MO/C75LfJEIZv+wqyQjQ==\r\n"
"-----END RSA PRIVATE KEY-----"
} ;

#define DO_RSA_ENCRYPTION    (1)
#define DO_RSA_DECRYPTION    (1)
#define DO_RSA_KEYPAIR       (0)
#define DO_DEBUG_SWITCH      (0)


#define assert_exit(cond, ret) \
    do { if (!(cond)){ \
        printf("  !. assert: failed [line: %d, error: -0x%04X]\n", __LINE__, -ret); \
        goto cleanup; \
    } } while (0)

static void dump_buf(char *info, uint8_t *buf, uint32_t len)
{
    mbedtls_printf("%s", info);
    for (int i = 0; i < len; i++) 
    {
        mbedtls_printf("%s%02X%s", i % 16 == 0 ? "\n     ":" ",  buf[i], i == len - 1 ? "\n":"");
    }
}

static void free_crypto_context(mbedtls_pk_context *public_pk_context,mbedtls_pk_context *private_pk_context,mbedtls_entropy_context *entropy_context,mbedtls_ctr_drbg_context *ctr_drbg_context)
{
    mbedtls_pk_free(private_pk_context);
    mbedtls_pk_free(public_pk_context);
    mbedtls_ctr_drbg_free(ctr_drbg_context);
    mbedtls_entropy_free(entropy_context);
}

/*import the key from the rsa context.*/
static void dump_rsa_key(mbedtls_rsa_context *ctx)
{
    size_t olen;
    uint8_t buf[516];

    mbedtls_printf("\n  +++++++++++++++++ rsa keypair +++++++++++++++++\n\n");
    mbedtls_mpi_write_string(&ctx->N , 16, (char *)buf, sizeof(buf), &olen);
    mbedtls_printf("N: %s\n", buf); 

    mbedtls_mpi_write_string(&ctx->E , 16, (char *)buf, sizeof(buf), &olen);
    mbedtls_printf("E: %s\n", buf);

    mbedtls_mpi_write_string(&ctx->D , 16,(char *) buf, sizeof(buf), &olen);
    mbedtls_printf("D: %s\n", buf);

    mbedtls_mpi_write_string(&ctx->P , 16, (char *)buf, sizeof(buf), &olen);
    mbedtls_printf("P: %s\n", buf);

    mbedtls_mpi_write_string(&ctx->Q , 16, (char *)buf, sizeof(buf), &olen);
    mbedtls_printf("Q: %s\n", buf);

    mbedtls_mpi_write_string(&ctx->DP, 16, (char *)buf, sizeof(buf), &olen);
    mbedtls_printf("DP: %s\n", buf);

    mbedtls_mpi_write_string(&ctx->DQ, 16, (char *)buf, sizeof(buf), &olen);
    mbedtls_printf("DQ: %s\n", buf);

    mbedtls_mpi_write_string(&ctx->QP, 16, (char *)buf, sizeof(buf), &olen);
    mbedtls_printf("QP: %s\n", buf);
    mbedtls_printf("\n  +++++++++++++++++ rsa keypair +++++++++++++++++\n\n");
}

/*get the public key from the rsa context.*/
static int get_pubkey_from_rsa_context(mbedtls_rsa_context *rsa_context)
{
    mbedtls_pk_context pk_context;
	int result;

    mbedtls_printf("%s enter! \n", __FUNCTION__);
    
    mbedtls_pk_init(&pk_context);
    //set the context type of the PK to RSA.
    mbedtls_pk_setup(&pk_context,mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    //get the RSA field of the PK context and set RSA context.
    mbedtls_rsa_copy(mbedtls_pk_rsa(pk_context),rsa_context);
    //get the public key.
    result = mbedtls_pk_write_pubkey_pem(&pk_context,public_key_buffer,sizeof(public_key_buffer));
    assert_exit(result == 0, result); 

    mbedtls_printf("%s\n", public_key_buffer);   
    mbedtls_printf("%s over! \n", __FUNCTION__);
    
cleanup:

	mbedtls_pk_free(&pk_context);
    return 0;
}

/*get the private key from the rsa context.*/
int get_privkey_from_rsa_context(mbedtls_rsa_context *rsa_context)
{
    mbedtls_pk_context pk_context;
	int result = 0;

    mbedtls_printf("%s enter! \n", __FUNCTION__);
    
    mbedtls_pk_init(&pk_context);
    //set the context type of the PK to RSA.
    mbedtls_pk_setup(&pk_context,mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    //get the RSA field of the PK context and set RSA context.
    mbedtls_rsa_copy(mbedtls_pk_rsa(pk_context),rsa_context);
    //get the private key.
    result = mbedtls_pk_write_key_pem(&pk_context,private_key_buffer,sizeof(private_key_buffer));
    assert_exit(result == 0, result); 
  
    mbedtls_printf("%s\n", private_key_buffer);
    mbedtls_printf("%s over! \n", __FUNCTION__);
cleanup:

    mbedtls_pk_free(&pk_context);

    return result;
}


int auto_generate_keypair()
{
    int ret;
    mbedtls_rsa_context ctx;    	
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "simple_rsa_test";

    mbedtls_entropy_init(&entropy);	
    mbedtls_ctr_drbg_init(&ctr_drbg);

	mbedtls_rsa_init(&ctx, MBEDTLS_RSA_PKCS_V15, 0); 
   /* mbedtls_entropy_add_source(&entropy, entropy_source, NULL,
                               MBEDTLS_ENTROPY_MAX_GATHER,
                               MBEDTLS_ENTROPY_SOURCE_STRONG);*/
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, 
                                    (const uint8_t *) pers, strlen(pers));
    assert_exit(ret == 0, ret);
    
    mbedtls_printf("\n  . setup rng ... ok\n");

    mbedtls_printf("\n  ! RSA Generating large primes may take minutes! \n");
	//…˙≥…RSA√‹‘ø
    ret = mbedtls_rsa_gen_key(&ctx, mbedtls_ctr_drbg_random, 
                                        &ctr_drbg,
                                        2048,
                                        65537);
    assert_exit(ret == 0, ret);                                    
    mbedtls_printf("\n  1. RSA generate key ... ok\n");
    dump_rsa_key(&ctx); 

    get_pubkey_from_rsa_context(&ctx); 
    get_privkey_from_rsa_context(&ctx);
    
cleanup:
    mbedtls_ctr_drbg_free(&ctr_drbg);
    
    mbedtls_entropy_free(&entropy);
    mbedtls_rsa_free(&ctx); 
 	mbedtls_printf("\n  5. RSA Compare results and plaintext  over\n");
 	
    return ret;

}

uint8_t encrypt_out[2048/8];
int do_rsa_public_encrypt_test(void)
{
    int ret = 0;
    //size_t olen = 0;
	int keylen;
    mbedtls_rsa_context ctx; 
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "simple_rsa_test";
    const char *msg = "Hello, World test!";
    
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const uint8_t *) pers, strlen(pers));
    assert_exit(ret == 0, ret);
    mbedtls_printf("\n  . setup rng ... ok\n");


    mbedtls_pk_context public_pk_context;
    keylen = strlen((const char *)public_key_buffer);
    mbedtls_printf("\n  .keylen £∫%d \n",keylen);  
    mbedtls_pk_init(&public_pk_context);

    ret = mbedtls_pk_parse_public_key(&public_pk_context, (const unsigned char *)public_key_buffer, keylen +1);
    if(ret !=0)
    {
    	mbedtls_printf("\n ret £∫%d ,parse the public key fail\n",ret);  
    }
    else
    {
    	mbedtls_printf("\n  .ret £∫%d parse the public key success\n",ret);  
    }

    mbedtls_rsa_context *public_rsa_context = mbedtls_pk_rsa(public_pk_context);
    mbedtls_rsa_set_padding(public_rsa_context,MBEDTLS_RSA_PKCS_V15,0);
    ret = mbedtls_rsa_pkcs1_encrypt(public_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg,MBEDTLS_RSA_PUBLIC,strlen(msg),(const unsigned char *)msg,encrypt_out);
    assert_exit(ret == 0, ret);                              
    dump_buf("\n  2. RSA encryption ... ok", encrypt_out, sizeof(encrypt_out));

cleanup:
    mbedtls_ctr_drbg_free(&ctr_drbg);
    
    mbedtls_entropy_free(&entropy);
    mbedtls_rsa_free(&ctx);
 	mbedtls_printf("\n  55. RSA Compare results and plaintext  over\n");
 	
    return ret;

}


int do_rsa_private_decrypt_test(void)
{
    int ret = 0;
    uint8_t outxx[2048/8];
	int keylen;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "simple_rsa_test";
    const char *msg = "Hello, World test!";
    
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const uint8_t *) pers, strlen(pers));
    assert_exit(ret == 0, ret);
    mbedtls_printf("\n  . setup rng ... ok\n");


    mbedtls_pk_context private_pk_context;
    keylen = strlen((const char *)private_key_buffer);
    mbedtls_printf("\n  .keylen £∫%d \n",keylen);  
    mbedtls_pk_init(&private_pk_context);

    ret = mbedtls_pk_parse_key(&private_pk_context, (const unsigned char *)private_key_buffer, keylen +1, NULL, 0);
    if(ret !=0)
    {
    	mbedtls_printf("\n ret £∫%d ,parse the private key fail\n",ret);  
    }
    else
    {
    	mbedtls_printf("\n  .ret £∫%d parse the private key success\n",ret);  
    }

    mbedtls_rsa_context *private_rsa_context = mbedtls_pk_rsa(private_pk_context);

    size_t decrypt_len = 0;
    mbedtls_rsa_set_padding(private_rsa_context,MBEDTLS_RSA_PKCS_V15,0);
    
	ret = mbedtls_rsa_pkcs1_decrypt(private_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg,MBEDTLS_RSA_PRIVATE,&decrypt_len,encrypt_out,outxx,sizeof(outxx));
	
	outxx[decrypt_len] = 0;
    mbedtls_printf("\n  3. RSA decryption ... ok\n     %s\n", outxx);

    ret = memcmp(outxx, msg, decrypt_len);
    assert_exit(ret == 0, ret);      
    mbedtls_printf("\n  4. RSA Compare results and plaintext ... ok\n");
cleanup:
    mbedtls_ctr_drbg_free(&ctr_drbg);
    
    mbedtls_entropy_free(&entropy);
 	mbedtls_printf("\n  555. RSA Compare results and plaintext  over\n");
 	
    return ret;

}


int demo1_run_rsa_encryption_decryption_separate(void)
{
	int ret;

	mbedtls_printf("\n %s enter!\n",__FUNCTION__);
	ret = auto_generate_keypair();
	if(ret !=0)
	{
 		mbedtls_printf("\n auto_generate_keypair fail\n");
	}
	else
	{
 		mbedtls_printf("\n 1. auto_generate_keypair  ok\n");
	}

	ret = do_rsa_public_encrypt_test();
	if(ret != 0)
	{
 		mbedtls_printf("\n do_rsa_public_encrypt_test fail\n");
	}
	else
	{
 		mbedtls_printf("\n 2. do_rsa_public_encrypt_test ok\n");
	}

	ret = do_rsa_private_decrypt_test();
	if(ret != 0)
	{
 		mbedtls_printf("\n do_rsa_private_encrypt_test fail\n");
	}
	else
	{
 		mbedtls_printf("\n 3. do_rsa_private_encrypt_test ok\n");
	}
	
	return ret ;
}


/* using the public key  to encrypt, and using the private key to decrypt*/
int demo2_mbedtls_rsa_test_with_public_private_key(void)
{

    const char *personalization = "rsa_encrypt_descrtpt_test!!!!"; //personalized initial value,used to initialize the pseudorandom unmber generator,can be set to any value.
    const char *plain_text = "welcome to rsa world!!!!";
    mbedtls_pk_context public_pk_context;
    mbedtls_pk_context private_pk_context;
    mbedtls_rsa_context  rsa_context;
    mbedtls_entropy_context entropy_context;
    mbedtls_ctr_drbg_context ctr_drbg_context;
    unsigned char encrypt_text[256];
    unsigned char decrypt_text[256];
    int ret;

    mbedtls_entropy_init(&entropy_context);
    mbedtls_ctr_drbg_init(&ctr_drbg_context);
    mbedtls_rsa_init(&rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);
    
    mbedtls_printf("\n  %s enter!!!\n",__FUNCTION__);
    
    //mbedtls_pk_setup(&pk_context,mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    //mbedtls_entropy_add_source(&entropy_context,get_clock_for_entropy,NULL,MBEDTLS_ENTROPY_MIN_PLATFORM,MBEDTLS_ENTROPY_SOURCE_STRONG);
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg_context, mbedtls_entropy_func, &entropy_context, (const unsigned char *)personalization,strlen(personalization));
    assert_exit(ret == 0, ret); 

#if DO_RSA_KEYPAIR
    mbedtls_printf("\n  . setup rng ... ok\n");
    mbedtls_printf("\n  ! RSA Generating large primes may take minutes! \n");

	//generate RSA key.
    ret = mbedtls_rsa_gen_key(&rsa_context, mbedtls_ctr_drbg_random, //random number gereration interface.
                                        &ctr_drbg_context, 			//random number structure
                                        2048, 						//modulus bit length
                                        0x01001);					//public index.

    assert_exit(ret == 0, ret); 
    mbedtls_printf("\n  1. RSA generate key ... ok\n");
    dump_rsa_key(&rsa_context); 

    get_pubkey_from_rsa_context(&rsa_context); //get the encryption key
    get_privkey_from_rsa_context(&rsa_context);//get the decryption key.
#endif

#if DO_RSA_ENCRYPTION
	//do encryption operation. 
   	int keylen = strlen((const char *)public_key_buffer);
    mbedtls_printf("\n  .keylen £∫%d \n",keylen);  
    mbedtls_pk_init(&public_pk_context);
    ret = mbedtls_pk_parse_public_key(&public_pk_context, (const unsigned char *)public_key_buffer, keylen +1); 	//parse the public key
    assert_exit(ret == 0, ret); 
    
    mbedtls_printf("succeeded to parse public key !\n");
 

    mbedtls_rsa_context *public_rsa_context = mbedtls_pk_rsa(public_pk_context);
    //set the padding mode .the default is v15.if using v21,need to display setting
    mbedtls_rsa_set_padding(public_rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);
    ret = mbedtls_rsa_pkcs1_encrypt(public_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg_context,MBEDTLS_RSA_PUBLIC,strlen(plain_text),(const unsigned char *)plain_text,encrypt_text);
    assert_exit(ret == 0, ret);                              
    dump_buf("\n  2. RSA encryption ... ok", encrypt_text, sizeof(encrypt_text));
#endif

#if DO_RSA_DECRYPTION
	//do decryption operation. 
    keylen = strlen((const char *)private_key_buffer);
    mbedtls_printf("\n  .keylen £∫%d \n",keylen);  
    mbedtls_pk_init(&private_pk_context);
    ret = mbedtls_pk_parse_key(&private_pk_context, (const unsigned char *)private_key_buffer, keylen +1, NULL, 0); 	//parse the private key.
    assert_exit(ret == 0, ret);
    
    mbedtls_printf("\n  .ret £∫%d parse the private key success\n",ret);  

    mbedtls_rsa_context *private_rsa_context = mbedtls_pk_rsa(private_pk_context);
    //plaintext length
    size_t decrypt_len = 0;
     //set the padding mode .the default is v15.if using v21,need to display setting
    mbedtls_rsa_set_padding(private_rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);
    
	ret = mbedtls_rsa_pkcs1_decrypt(private_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg_context,MBEDTLS_RSA_PRIVATE,&decrypt_len,encrypt_text,decrypt_text,sizeof(decrypt_text));
	
	decrypt_text[decrypt_len] = 0;
    mbedtls_printf("\n  3. RSA decryption ... ok\n     %s\n", decrypt_text);

    ret = memcmp(decrypt_text, plain_text, decrypt_len);
    assert_exit(ret == 0, ret);      
    mbedtls_printf("\n  4. RSA Compare results and plaintext ... ok\n");
#endif


cleanup:  

    free_crypto_context(&public_pk_context,&private_pk_context,&entropy_context,&ctr_drbg_context);

    return 0;
}


/* using the private key  to encrypt, and using the public key to decrypt*/
int demo2_with_private_key_encrypt_public_key_decrypt(void)
{

    const char *personalization = "rsa_encrypt_descrtpt_test!!!!"; //personalized initial value,used to initialize the pseudorandom unmber generator,can be set to any value.
    const char *plain_text = "welcome to the rsa world!!!!";
    mbedtls_pk_context public_pk_context;
    mbedtls_pk_context private_pk_context;
    mbedtls_rsa_context  rsa_context;
    mbedtls_entropy_context entropy_context;
    mbedtls_ctr_drbg_context ctr_drbg_context;
    unsigned char encrypt_text[256];
    unsigned char decrypt_text[256];
    int ret;
	int keylen;

    mbedtls_entropy_init(&entropy_context);
    mbedtls_ctr_drbg_init(&ctr_drbg_context);
    mbedtls_rsa_init(&rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);
    
    mbedtls_printf("\n  %s enter!!!\n",__FUNCTION__);
    
    //mbedtls_pk_setup(&pk_context,mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    //mbedtls_entropy_add_source(&entropy_context,get_clock_for_entropy,NULL,MBEDTLS_ENTROPY_MIN_PLATFORM,MBEDTLS_ENTROPY_SOURCE_STRONG);
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg_context, mbedtls_entropy_func, &entropy_context, (const unsigned char *)personalization,strlen(personalization));
    assert_exit(ret == 0, ret); 

#if DO_RSA_KEYPAIR
    mbedtls_printf("\n  . setup rng ... ok\n");
    mbedtls_printf("\n  ! RSA Generating large primes may take minutes! \n");

	//generate RSA key.
    ret = mbedtls_rsa_gen_key(&rsa_context, mbedtls_ctr_drbg_random, //random number gereration interface.
                                        &ctr_drbg_context, 			//random number structure
                                        2048, 						//modulus bit length
                                        0x01001);					//public index.

    assert_exit(ret == 0, ret); 
    mbedtls_printf("\n  1. RSA generate key ... ok\n");
    dump_rsa_key(&rsa_context); 

    get_pubkey_from_rsa_context(&rsa_context); //get the encryption key
    get_privkey_from_rsa_context(&rsa_context);//get the decryption key.
#endif

#if DO_RSA_ENCRYPTION
	//do encryption operation. 
    keylen = strlen((const char *)private_key_buffer);
    mbedtls_printf("\n  .keylen £∫%d \n",keylen);  
    mbedtls_pk_init(&private_pk_context);
    ret = mbedtls_pk_parse_key(&private_pk_context, (const unsigned char *)private_key_buffer, keylen +1, NULL, 0); 	//parse the private key.
    assert_exit(ret == 0, ret);
    
    mbedtls_printf("\n  .ret £∫%d parse the private key success\n",ret);  

    mbedtls_rsa_context *private_rsa_context = mbedtls_pk_rsa(private_pk_context);
    //plaintext length
    size_t decrypt_len = 0;
     //set the padding mode .the default is v15.if using v21,need to display setting
    mbedtls_rsa_set_padding(private_rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);
 
    ret = mbedtls_rsa_pkcs1_encrypt(private_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg_context,MBEDTLS_RSA_PRIVATE,strlen(plain_text),(const unsigned char *)plain_text,encrypt_text);
    assert_exit(ret == 0, ret);                              
    dump_buf("\n  2. RSA encryption ... ok", encrypt_text, sizeof(encrypt_text));
#endif

#if DO_RSA_DECRYPTION
	//do decryption operation. 
   	keylen = strlen((const char *)public_key_buffer);
    mbedtls_printf("\n  .keylen £∫%d \n",keylen);  
    mbedtls_pk_init(&public_pk_context);
    ret = mbedtls_pk_parse_public_key(&public_pk_context, (const unsigned char *)public_key_buffer, keylen +1); 	//parse the public key
    
    mbedtls_printf("succeeded to parse public key !\n");
    mbedtls_rsa_context *public_rsa_context = mbedtls_pk_rsa(public_pk_context);
    //set the padding mode .the default is v15.if using v21,need to display setting
    mbedtls_rsa_set_padding(public_rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);

	ret = mbedtls_rsa_pkcs1_decrypt(public_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg_context,MBEDTLS_RSA_PUBLIC,&decrypt_len,encrypt_text,decrypt_text,sizeof(decrypt_text));
	
	decrypt_text[decrypt_len] = 0;
    mbedtls_printf("\n  3. RSA decryption ... ok\n     %s\n", decrypt_text);

    ret = memcmp(decrypt_text, plain_text, decrypt_len);
    assert_exit(ret == 0, ret);      
    mbedtls_printf("\n  4. RSA Compare results and plaintext ... ok\n");
#endif


cleanup:  

    free_crypto_context(&public_pk_context,&private_pk_context,&entropy_context,&ctr_drbg_context);

    return 0;
}


int demo3_auto_generate_keypair_do_rsa_test(void)
{
    int ret;
    size_t olen = 0;
    uint8_t out[2048/8];

    mbedtls_rsa_context ctx;    		//RSA key structure
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;	//randon structure
    const char *personalization = "simple_rsa_test";
    const char *plain_text = "Hello, World test!";

    mbedtls_printf("\n  %s enter!!!\n",__FUNCTION__);

    mbedtls_entropy_init(&entropy);		//initial the entropt structure
    mbedtls_ctr_drbg_init(&ctr_drbg);	//initial the random structure

    mbedtls_rsa_init(&ctx, MBEDTLS_RSA_PKCS_V15, 0);     //initial the rsa structure
   /* mbedtls_entropy_add_source(&entropy, entropy_source, NULL,
                               MBEDTLS_ENTROPY_MAX_GATHER,
                               MBEDTLS_ENTROPY_SOURCE_STRONG);*/
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    (const uint8_t *) personalization, strlen(personalization));// update the seed based on the personalization string.
    assert_exit(ret == 0, ret);
    mbedtls_printf("\n  . setup rng ... ok\n");

    mbedtls_printf("\n  ! RSA Generating large primes may take minutes! \n");
    //generate the RSA key
    ret = mbedtls_rsa_gen_key(&ctx, mbedtls_ctr_drbg_random, 		//random number gereration interface.
                                        &ctr_drbg, 					//random number structure
                                        2048, 						//modulus bit length
                                        65537);						//public index.
    assert_exit(ret == 0, ret);
    mbedtls_printf("\n  1. RSA generate key ... ok\n");
    dump_rsa_key(&ctx); 

    //RSA encryption
    ret = mbedtls_rsa_pkcs1_encrypt(&ctx, mbedtls_ctr_drbg_random, 	//random number gereration interface.
                            &ctr_drbg,          					//random number structure
                            MBEDTLS_RSA_PUBLIC, 					//public key operation
                            strlen(plain_text),        				//plain_text length
                            (const unsigned char *)plain_text,      //input the plain_text pointer
                            out);               					//output ciphertext pointer

    assert_exit(ret == 0, ret);
    dump_buf("\n  2. RSA encryption ... ok", out, sizeof(out));
    //RSA decryption
    ret = mbedtls_rsa_pkcs1_decrypt(&ctx, mbedtls_ctr_drbg_random,	//random number gereration interface.
    						&ctr_drbg,          					//random number structure
                            MBEDTLS_RSA_PRIVATE, 					//private key operation
                            &olen,           						//output length
                            out,             						//input ciphertext pointer
                            out,             						//output plaintext pointer
                            sizeof(out));    						//maximum output plaintext array length
    assert_exit(ret == 0, ret);

    out[olen] = 0;
    mbedtls_printf("\n  3. RSA decryption ... ok\n     %s\n", out);

    ret = memcmp(out, plain_text, olen);
    assert_exit(ret == 0, ret);
    mbedtls_printf("\n  4. RSA Compare results and plaintext ... ok\n");


cleanup:
    mbedtls_ctr_drbg_free(&ctr_drbg);	//release the random structure
    mbedtls_entropy_free(&entropy); 	//release the entropt structure
    mbedtls_rsa_free(&ctx);          	//release the rsa structure
    mbedtls_printf("\n  5. RSA Compare results and plaintext  over\n");

    return ret;
}


int rsa_decrypt_with_public_key(unsigned char *encrypt_text, const char *pubkey_buf,unsigned char *decrypt_text)
{
    const char *personalization = "rsa_encrypt_descrtpt_test!!!!"; //personalized initial value,used to initialize the pseudorandom unmber generator,can be set to any value.
    mbedtls_pk_context public_pk_context;
    mbedtls_rsa_context  rsa_context;
    mbedtls_entropy_context entropy_context;
    mbedtls_ctr_drbg_context ctr_drbg_context;
    //unsigned char encrypt_text[256];
    int ret;
    int keylen;
    size_t decrypt_len = 0;

    mbedtls_entropy_init(&entropy_context);
    mbedtls_ctr_drbg_init(&ctr_drbg_context);
    mbedtls_rsa_init(&rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);

    mbedtls_printf("\n  %s enter!!!\n",__FUNCTION__);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg_context, mbedtls_entropy_func, &entropy_context, (const unsigned char *)personalization,strlen(personalization));
    assert_exit(ret == 0, ret);

#if DO_RSA_DECRYPTION
    //do decryption operation.
   	keylen = strlen((const char *)pubkey_buf);
    mbedtls_printf("\n  .keylen £∫%d \n",keylen);
    mbedtls_pk_init(&public_pk_context);
    ret = mbedtls_pk_parse_public_key(&public_pk_context, (const unsigned char *)pubkey_buf, keylen +1); 	//parse the public key

    mbedtls_printf("succeeded to parse public key !\n");
    mbedtls_rsa_context *public_rsa_context = mbedtls_pk_rsa(public_pk_context);
    //set the padding mode .the default is v15.if using v21,need to display setting

    mbedtls_rsa_set_padding(public_rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);

    ret = mbedtls_rsa_pkcs1_decrypt(public_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg_context,MBEDTLS_RSA_PUBLIC,&decrypt_len,encrypt_text,decrypt_text,sizeof(decrypt_text));

    decrypt_text[decrypt_len] = 0;
    mbedtls_printf("\n  3. RSA decryption ... ok\n     %s\n", decrypt_text);

    mbedtls_printf("\n  4. RSA Compare results and plaintext ... ok\n");
#endif

cleanup:

    free_crypto_context(&public_pk_context,NULL,&entropy_context,&ctr_drbg_context);

    return 0;

}


int rsa_decrypt_with_private_key(unsigned char *encrypt_text, const char *pubkey_buf,unsigned char *decrypt_text)
{
    const char *personalization = "rsa_encrypt_descrtpt_test!!!!"; //personalized initial value,used to initialize the pseudorandom unmber generator,can be set to any value.
    mbedtls_pk_context private_pk_context;
    mbedtls_rsa_context  rsa_context;
    mbedtls_entropy_context entropy_context;
    mbedtls_ctr_drbg_context ctr_drbg_context;
    int ret;
    int keylen;

    mbedtls_printf("%s enter!!!\n",__FUNCTION__);

    mbedtls_entropy_init(&entropy_context);
    mbedtls_ctr_drbg_init(&ctr_drbg_context);
    mbedtls_rsa_init(&rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg_context, mbedtls_entropy_func, &entropy_context, (const unsigned char *)personalization,strlen(personalization));
    assert_exit(ret == 0, ret);

#if DO_RSA_DECRYPTION
    //do decryption operation. 
    keylen = strlen((const char *)pubkey_buf);
    mbedtls_printf("keylen £∫%d \n",keylen);

    mbedtls_pk_init(&private_pk_context);
    ret = mbedtls_pk_parse_key(&private_pk_context, (const unsigned char *)pubkey_buf, keylen +1, NULL, 0); 	//parse the private key.
    assert_exit(ret == 0, ret);

    mbedtls_printf("ret £∫%d parse the private key success\n",ret);

    mbedtls_rsa_context *private_rsa_context = mbedtls_pk_rsa(private_pk_context);
    //plaintext length
    size_t decrypt_len = 0;
     //set the padding mode .the default is v15.if using v21,need to display setting
    mbedtls_rsa_set_padding(private_rsa_context,MBEDTLS_RSA_PKCS_V15,MBEDTLS_MD_SHA256);

    ret = mbedtls_rsa_pkcs1_decrypt(private_rsa_context,mbedtls_ctr_drbg_random,&ctr_drbg_context,MBEDTLS_RSA_PRIVATE,&decrypt_len,encrypt_text,decrypt_text,256);

    decrypt_text[decrypt_len] = 0;

    mbedtls_printf("RSA decrypt_len :%d. decryption ... ok\n     %s\n",decrypt_len, decrypt_text);

#if DO_DEBUG_SWITCH
    for(int i= 0;i <decrypt_len;i++)
    {
        mbedtls_printf("decrypt_text[%d] =0x%x \n", i, decrypt_text[i]);
    }
#endif

#endif

cleanup:

    free_crypto_context(NULL,&private_pk_context,&entropy_context,&ctr_drbg_context);

    return 0;

}

