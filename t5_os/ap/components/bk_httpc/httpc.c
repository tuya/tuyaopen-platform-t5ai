#include "mbedtls/ssl.h"
#include "mbedtls/x509_crl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/platform.h"
//#include <http_parser.h>
//#include <http_header.h>
//#include <http_utils.h>
#include <components/system.h>
//#include <bk_rom_md5.h>
/*********************************tls begin**************************************/
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <lwip/sockets.h>
#include <netdb.h>

#include "httpc.h"
#define TAG "httpc"
#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
#include "polarssl/ssl.h"
#include "polarssl/memory.h"
#include "polarssl/base64.h"

struct httpc_tls {
	ssl_context ctx;                 /*!< Context for PolarSSL */
	x509_crt ca;                     /*!< CA certificates */
	x509_crt cert;                   /*!< Certificate */
	pk_context key;                  /*!< Private key */
};

static int _verify_func(void *data, x509_crt *crt, int depth, int *flags)
{
	char buf[1024];
	x509_crt_info(buf, sizeof(buf) - 1, "", crt);

	if(*flags)
		printf("\n[HTTPC] ERROR: certificate verify\n%s\n", buf);
	else
		printf("\n[HTTPC] Certificate verified\n%s\n", buf);

	return 0;
}

#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
#include "mbedtls/ssl.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/base64.h"

struct httpc_tls {
	mbedtls_ssl_context ctx;         /*!< Context for mbedTLS */
	mbedtls_ssl_config conf;         /*!< Configuration for mbedTLS */
	mbedtls_x509_crt ca;             /*!< CA certificates */
	mbedtls_x509_crt cert;           /*!< Certificate */
	mbedtls_pk_context key;          /*!< Private key */
};

static int _verify_func(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
	/* To avoid gcc warnings */
	( void ) data;
	( void ) depth;
	
	char buf[1024];
	mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);

	if(*flags)
		printf("\n[HTTPC] ERROR: certificate verify\n%s\n", buf);
	else
		printf("\n[HTTPC] Certificate verified\n%s\n", buf);

	return 0;
}

static void* _calloc_func(size_t nmemb, size_t size)
{
	size_t mem_size;
	void *ptr = NULL;

	mem_size = nmemb * size;
	ptr = os_malloc(mem_size);

	if(ptr)
		memset(ptr, 0, mem_size);

	return ptr;
}
#endif /* HTTPC_USE_POLARSSL */
static int httpc_get_random(unsigned char *buf, size_t sz)
{
	int r;
	int len;

	while (sz > 0) {
		len = sizeof(r) > sz ? sz : sizeof(r);
		r = rand();
		os_memcpy(buf, &r, len);
		buf += len;
		sz -= len;
	}

	return 0;
}

static int _random_func(void *p_rng, unsigned char *output, size_t output_len)
{
	/* To avoid gcc warnings */
	( void ) p_rng;
	httpc_get_random(output, output_len);
	//rtw_get_random_bytes(output, output_len);
	return 0;
}

void *httpc_tls_new(int *sock, char *client_cert, char *client_key, char *ca_certs)
{
#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
	int ret = 0;
	struct httpc_tls *tls = NULL;

	memory_set_own(pvPortMalloc, vPortFree);
	tls = (struct httpc_tls *) malloc(sizeof(struct httpc_tls));

	if(tls) {
		ssl_context *ssl = &tls->ctx;

		memset(tls, 0, sizeof(struct httpc_tls));
		x509_crt_init(&tls->ca);
		x509_crt_init(&tls->cert);
		pk_init(&tls->key);
		if((ret = ssl_init(ssl))!=0){
                        printf("\n[HTTPC] ERROR: ssl_init %d\n", ret);
                        ret = -1;
                        goto exit;
		} 
		ssl_set_endpoint(ssl, SSL_IS_CLIENT);
		ssl_set_authmode(ssl, SSL_VERIFY_NONE);
		ssl_set_rng(ssl, _random_func, NULL);
		ssl_set_bio(ssl, net_recv, sock, net_send, sock);

		if(client_cert && client_key) {
			if((ret = x509_crt_parse(&tls->cert, (const unsigned char *) client_cert, strlen(client_cert))) != 0) {
				printf("\n[HTTPC] ERROR: x509_crt_parse %d\n", ret);
				ret = -1;
				goto exit;
			}

			if((ret = pk_parse_key(&tls->key, (const unsigned char *) client_key, strlen(client_key), NULL, 0)) != 0) {
				printf("\n[HTTPC] ERROR: pk_parse_key %d\n", ret);
				ret = -1;
				goto exit;
			}

			if((ret = ssl_set_own_cert(ssl, &tls->cert, &tls->key)) != 0) {
				printf("\n[HTTPC] ERROR: ssl_set_own_cert %d\n", ret);
				ret = -1;
				goto exit;
			}
		}

		if(ca_certs) {
			// set trusted ca certificates next to client certificate
			if((ret = x509_crt_parse(&tls->ca, (const unsigned char *) ca_certs, strlen(ca_certs))) != 0) {
				printf("\n[HTTPC] ERROR: x509_crt_parse %d\n", ret);
				ret = -1;
				goto exit;
			}

			ssl_set_ca_chain(ssl, &tls->ca, NULL, NULL);
			ssl_set_authmode(ssl, SSL_VERIFY_REQUIRED);
			ssl_set_verify(ssl, _verify_func, NULL);
		}
	}
	else {
		printf("\n[HTTPC] ERROR: malloc\n");
		ret = -1;
		goto exit;
	}

exit:
	if(ret && tls) {
		ssl_free(&tls->ctx);
		x509_crt_free(&tls->ca);
		x509_crt_free(&tls->cert);
		pk_free(&tls->key);
		free(tls);
		tls = NULL;
	}

	return (void *) tls;
#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
	int ret = 0;
	struct httpc_tls *tls = NULL;

	mbedtls_platform_set_calloc_free(_calloc_func, free);
	tls = (struct httpc_tls *) malloc(sizeof(struct httpc_tls));

	if(tls) {
		mbedtls_ssl_context *ssl = &tls->ctx;
		mbedtls_ssl_config *conf = &tls->conf;

		memset(tls, 0, sizeof(struct httpc_tls));
		mbedtls_x509_crt_init(&tls->ca);
		mbedtls_x509_crt_init(&tls->cert);
		mbedtls_pk_init(&tls->key);
		mbedtls_ssl_init(ssl);
		mbedtls_ssl_config_init(conf);

		if((ret = mbedtls_ssl_config_defaults(conf,
				MBEDTLS_SSL_IS_CLIENT,
				MBEDTLS_SSL_TRANSPORT_STREAM,
				MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {

			printf("\n[HTTPC] ERROR: mbedtls_ssl_config_defaults %d\n", ret);
			ret = -1;
			goto exit;
		}

		mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
		mbedtls_ssl_conf_rng(conf, _random_func, NULL);
		if(client_cert && client_key) {
			if((ret = mbedtls_x509_crt_parse(&tls->cert, (const unsigned char *) client_cert, strlen(client_cert) + 1)) != 0) {
				printf("\n[HTTPC] ERROR: mbedtls_x509_crt_parse %d\n", ret);
				ret = -1;
				goto exit;
			}

			if((ret = mbedtls_pk_parse_key(&tls->key, (const unsigned char *) client_key, strlen(client_key) + 1, NULL, 0)) != 0) {
				printf("\n[HTTPC] ERROR: mbedtls_pk_parse_key %d\n", ret);
				ret = -1;
				goto exit;
			}

			if((ret = mbedtls_ssl_conf_own_cert(conf, &tls->cert, &tls->key)) != 0) {
				printf("\n[HTTPC] ERROR: mbedtls_ssl_conf_own_cert %d\n", ret);
				ret = -1;
				goto exit;
			}
		}

		if(ca_certs) {
			// set trusted ca certificates next to client certificate
			if((ret = mbedtls_x509_crt_parse(&tls->ca, (const unsigned char *) ca_certs, strlen(ca_certs) + 1)) != 0) {
				printf("\n[HTTPC] ERROR: mbedtls_x509_crt_parse %d\n", ret);
				ret = -1;
				goto exit;
			}

			mbedtls_ssl_conf_ca_chain(conf, &tls->ca, NULL);
			mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
			mbedtls_ssl_conf_verify(conf, _verify_func, NULL);
		}

		if((ret = mbedtls_ssl_setup(ssl, conf)) != 0) {
			printf("\n[HTTPC] ERROR: mbedtls_ssl_setup %d\n", ret);
			ret = -1;
			goto exit;
		}

		mbedtls_ssl_set_bio(ssl, sock, mbedtls_net_send, mbedtls_net_recv, NULL);
	}
	else {
		printf("\n[HTTPC] ERROR: malloc\n");
		ret = -1;
		goto exit;
	}

exit:
	if(ret && tls) {
		mbedtls_ssl_free(&tls->ctx);
		mbedtls_ssl_config_free(&tls->conf);
		mbedtls_x509_crt_free(&tls->ca);
		mbedtls_x509_crt_free(&tls->cert);
		mbedtls_pk_free(&tls->key);
		free(tls);
		tls = NULL;
	}

	return (void *) tls;
#endif
}

void httpc_tls_free(void *tls_in)
{
	struct httpc_tls *tls = (struct httpc_tls *) tls_in;

#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
	ssl_free(&tls->ctx);
	x509_crt_free(&tls->ca);
	x509_crt_free(&tls->cert);
	pk_free(&tls->key);
	free(tls);
#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
	mbedtls_ssl_free(&tls->ctx);
	mbedtls_ssl_config_free(&tls->conf);
	mbedtls_x509_crt_free(&tls->ca);
	mbedtls_x509_crt_free(&tls->cert);
	mbedtls_pk_free(&tls->key);
	free(tls);
#endif
}

int httpc_tls_handshake(void *tls_in, char *host)
{
	struct httpc_tls *tls = (struct httpc_tls *) tls_in;

#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
	int ret = 0;

	ssl_set_hostname(&tls->ctx, host);

	if((ret = ssl_handshake(&tls->ctx)) != 0) {
		printf("\n[HTTPC] ERROR: ssl_handshake %d\n", ret);
		ret = -1;
	}
	else {
		printf("\n[HTTPC] Use ciphersuite %s\n", ssl_get_ciphersuite(&tls->ctx));
	}

	return ret;
#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
	int ret = 0;

	mbedtls_ssl_set_hostname(&tls->ctx, host);

	if((ret = mbedtls_ssl_handshake(&tls->ctx)) != 0) {
		printf("\n[HTTPC] ERROR: mbedtls_ssl_handshake %d\n", ret);
		ret = -1;
	}
	else {
		printf("\n[HTTPC] Use ciphersuite %s\n", mbedtls_ssl_get_ciphersuite(&tls->ctx));
	}

	return ret;
#endif
}

void httpc_tls_close(void *tls_in)
{
	struct httpc_tls *tls = (struct httpc_tls *) tls_in;

#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
	ssl_close_notify(&tls->ctx);
#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
	mbedtls_ssl_close_notify(&tls->ctx);
#endif
}

int httpc_tls_read(void *tls_in, uint8_t *buf, size_t buf_len)
{
	struct httpc_tls *tls = (struct httpc_tls *) tls_in;

#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
	return ssl_read(&tls->ctx, buf, buf_len);
#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
	return mbedtls_ssl_read(&tls->ctx, buf, buf_len);
#endif
}

int httpc_tls_write(void *tls_in, uint8_t *buf, size_t buf_len)
{
	struct httpc_tls *tls = (struct httpc_tls *) tls_in;

#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
	return ssl_write(&tls->ctx, buf, buf_len);
#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
	return mbedtls_ssl_write(&tls->ctx, buf, buf_len);
#endif
}

int httpc_base64_encode(uint8_t *data, size_t data_len, char *base64_buf, size_t buf_len)
{
#if (HTTPC_USE_TLS == HTTPC_TLS_POLARSSL)
	int ret = 0;

	if((ret = base64_encode(base64_buf, &buf_len, data, data_len)) != 0) {
		printf("\n[HTTPC] ERROR: base64_encode %d\n", ret);
		ret = -1;
	}

	return ret;
#elif (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
	int ret = 0;
	size_t output_len = 0;

	if((ret = mbedtls_base64_encode((unsigned char*)base64_buf, buf_len, &output_len, data, data_len)) != 0) {
		printf("\n[HTTPC] ERROR: mbedtls_base64_encode %d\n", ret);
		ret = -1;
	}

	return ret;
#endif
}
/***********************************************tls end****************************************************/

#define RCV_HEADER_SIZE                5*1024
#define RCV_BUFF_SIZE                  5*1024
#define SEND_HEADER_SIZE               1024

struct httpc_conn *httpc_conn_new(uint8_t secure, char *client_cert, char *client_key, char *ca_certs)
{
	struct httpc_conn *conn = malloc(sizeof(struct httpc_conn));
	if (conn == NULL) {
		BK_LOGE(TAG,"%s: create failed, no memory!\r\n",__func__);
		return NULL;
	}
	// Initialize other members
	conn->response.header = NULL;
	conn->response.header_len = 0;
	conn->response.version = NULL;
	conn->response.version_len = 0;
	conn->response.status = NULL;
	conn->response.status_len = 0;
	conn->response.content_type = NULL;
	conn->response.content_type_len = 0;
	conn->response.content_len = 0;
	conn->request_header = NULL;
	conn->host = NULL;
	conn->port = 0;
	conn->user_password = NULL;

	if (secure) {
		struct httpc_tls *tls = (struct httpc_tls *) httpc_tls_new(&conn->sock, client_cert, client_key, ca_certs);
		if(tls == NULL) {
			BK_LOGE(TAG, "FAIL to httpc_tls_new\r\n");
		}
		conn->tls = tls;
		return conn;
	} else {
		conn->tls = NULL;
	}

	return conn;
}

struct timeval* bk_utils_ms_to_timeval(int timeout_ms, struct timeval *tv)
{
	if (timeout_ms == -1) {
		return NULL;
	}
	tv->tv_sec = timeout_ms / 1000;
	tv->tv_usec = (timeout_ms - (tv->tv_sec * 1000)) * 1000;
	return tv;
}

static void bk_httpc_hex_dump(char *s, int length) {
	BK_LOGE(TAG, "begin to print (length=%d):\r\n", length);
	for (int index = 0; index < length; index++)
	{
		BK_LOG_RAW("%c", s[index]);
	}
	BK_LOGD(NULL, "\r\n");
}

static bk_err_t httpc_tls_hostname_to_fd(const char *host, size_t hostlen, int port, struct sockaddr_storage *address, int* fd)
{
	struct addrinfo *address_info;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	char *use_host = strndup(host, hostlen);
	if (!use_host) {
		return -1;
	}

	BK_LOGV(TAG, "host:%s: strlen %lu\r\n", use_host, (unsigned long)hostlen);
	int res = getaddrinfo(use_host, NULL, &hints, &address_info);
	if (res != 0 || address_info == NULL) {
		BK_LOGE(TAG, "couldn't get hostname for :%s: "
					  "getaddrinfo() returns %d, addrinfo=%p\r\n", use_host, res, address_info);
		free(use_host);
		return -1;
	}
	free(use_host);
	*fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
	if (*fd < 0) {
		BK_LOGE(TAG, "Failed to create socket (family %d socktype %d protocol %d)\r\n", address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
		freeaddrinfo(address_info);
		return -1;
	}

	if (address_info->ai_family == AF_INET) {
		struct sockaddr_in *p = (struct sockaddr_in *)address_info->ai_addr;
		p->sin_port = htons(port);
		BK_LOGE(TAG, "[sock=%d] Resolved IPv4 address: %s\r\n", *fd, ipaddr_ntoa((const ip_addr_t*)&p->sin_addr.s_addr));
		memcpy(address, p, sizeof(struct sockaddr ));
	}
#if CONFIG_LWIP_IPV6
	else if (address_info->ai_family == AF_INET6) {
		struct sockaddr_in6 *p = (struct sockaddr_in6 *)address_info->ai_addr;
		p->sin6_port = htons(port);
		p->sin6_family = AF_INET6;
		BK_LOGV(TAG, "[sock=%d] Resolved IPv6 address: %s\r\n", *fd, ip6addr_ntoa((const ip6_addr_t*)&p->sin6_addr));
		memcpy(address, p, sizeof(struct sockaddr_in6 ));
	}
#endif
	else {
		BK_LOGE(TAG, "Unsupported protocol family %d\r\n", address_info->ai_family);
		close(*fd);
		freeaddrinfo(address_info);
		return -1;
	}

	freeaddrinfo(address_info);
	return BK_OK;
}

int httpc_conn_connect(struct httpc_conn *conn, char *host, uint16_t port, uint32_t timeout)
{
	conn->port = port;
	conn->host = os_strdup(host);
	struct sockaddr_storage address;
	if (port == 443) {
		bk_err_t ret = httpc_tls_hostname_to_fd(host, strlen(host), port, &address, &conn->sock);
		BK_LOGE(TAG, "[sock=%d] Connecting to server. HOST: %s, Port: %d ret: %d\r\n", conn->sock, host, port, ret);
		ret = connect(conn->sock, (struct sockaddr *)&address, sizeof(struct sockaddr));
		if (ret < 0) {
	        BK_LOGE(TAG, "Fail to connect ret:%d\r\n", ret);
	        return -1;
	    }
		else
			BK_LOGE(TAG, "success to connect ret:%d\r\n", ret);

		ret = httpc_tls_handshake(conn->tls, host);
		if (ret < 0) {
	        BK_LOGE(TAG, "Fail to handshake ret:%d\r\n", ret);
	        return -1;
	    }
		else
			BK_LOGE(TAG, "success to handshake ret:%d\r\n", ret);
	}
#if 1
	struct timeval tv;
	bk_utils_ms_to_timeval(timeout, &tv);
	if (setsockopt(conn->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
		BK_LOGE(TAG, "Fail to setsockopt SO_RCVTIMEO\r\n");
		return -1;
	}
	if (setsockopt(conn->sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
		BK_LOGE(TAG, "Fail to setsockopt SO_SNDTIMEO\r\n");
		return -1;
	}
#endif
	return BK_OK;
}

void httpc_conn_free(struct httpc_conn *conn)
{
	if (!conn)
		return;
	if (conn->tls)
	{
		httpc_tls_free(conn->tls);
	}
	if (conn->sock >= 0)
	{
		closesocket(conn->sock);
		conn->sock = -1;
	}
	return;
}

void httpc_conn_close(struct httpc_conn *conn)
{
	if (conn && conn->tls)
	{
		httpc_tls_close(conn->tls);
	}
    return;	
}

void httpc_free(void *ptr)
{
	struct httpc_conn *conn = (struct httpc_conn *)ptr;

	if (!conn)
		return;
	if (conn->request_header)
	{
		os_free(conn->request_header);
		conn->request_header = NULL;
	}
	if (conn->response.header)
	{
		os_free(conn->response.header);
		conn->response.header = NULL;
	}
	if (conn->response.version)
	{
		os_free(conn->response.version);
		conn->response.version = NULL;
	}
	if (conn->response.status)
	{
		os_free(conn->response.status);
		conn->response.status = NULL;
	}
	if (conn->response.content_type)
	{
		os_free(conn->response.content_type);
		conn->response.content_type = NULL;
	}

	if (conn->host)
	{
		os_free(conn->host);
		conn->host = NULL;
	}
	if (conn)
	{
		os_free(conn);
		conn = NULL;
	}	
	return;
}

int httpc_conn_setup_user_password_v1(struct httpc_conn *conn, char *user, char *password) {
    if (user == NULL || password == NULL) {
        return -1;
    }
	if (!conn)
		return -1;
    size_t user_password_len = strlen(user) + strlen(password) + 2;
    conn->user_password = (char *)malloc(user_password_len);
    if (conn->user_password == NULL) {
        BK_LOGE(TAG,"%s: create failed, no memory!\r\n",__func__);
        return -1;
    }

    snprintf(conn->user_password, user_password_len, "%s:%s", user, password);
    return 0;
}

int httpc_conn_setup_user_password_v2(struct httpc_conn *conn, char *user, char *password)
{
	int out;
	char *user_info = NULL;
	char *digest = NULL;
	size_t n = 0;
	asprintf(&user_info, "%s:%s", user, password);
	mbedtls_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));
	digest = calloc(1, 6 + n + 1);
	strcpy(digest, "Basic ");
	mbedtls_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
	if (!conn) {
		os_free(digest);
		return -1;
	}
    conn->user_password = (char *)malloc(n);
    if (conn->user_password == NULL) {
        BK_LOGE(TAG,"%s: create failed, no memory!\r\n",__func__);
		os_free(digest);
		os_free(conn->user_password);
        return -1;
    }
    snprintf(conn->user_password, n, "%s", (unsigned char *)digest + 6);
	os_free(digest);
	os_free(conn->user_password);
	/*TODO more*/

    return 0;

}


void httpc_setup_debug(uint8_t debug)
{
	if (debug)
		BK_LOGD(NULL, "%s\r\n", __func__);
}

void httpc_conn_dump_header(struct httpc_conn *conn)
{
    BK_LOGE(TAG, "HTTP Version: %s %d\n", conn->response.version,  (int) strtol((char *)conn->response.version, NULL, 10));
    BK_LOGE(TAG, "Status: %s\n", conn->response.status);
    BK_LOGE(TAG, "Content-Type: %s\n", conn->response.content_type);
    BK_LOGE(TAG, "Content-Length: %d\n", conn->response.content_len);	
}

int httpc_request_write_header_start(struct httpc_conn *conn, char *method, char *resource, char *content_type, size_t content_len)
{
    conn->request_header = (uint8_t *)malloc(SEND_HEADER_SIZE);
    if (conn->request_header == NULL) {
        BK_LOGE(TAG,"%s: create failed, no memory!\r\n",__func__);
        return -1;
    }
	if (resource)
    	snprintf((char *)conn->request_header, SEND_HEADER_SIZE, "%s %s HTTP/1.1\r\n", method, resource);
	else
		snprintf((char *)conn->request_header, SEND_HEADER_SIZE, "%s / HTTP/1.1\r\n", method);

    if (content_type != NULL) {
        snprintf((char *)conn->request_header + strlen((char *)conn->request_header), SEND_HEADER_SIZE - strlen((char *)conn->request_header),
                 "Content-Type: %s\r\n", content_type);
    }

    if (content_len > 0) {
        snprintf((char *)conn->request_header + strlen((char *)conn->request_header), SEND_HEADER_SIZE - strlen((char *)conn->request_header),
                 "Content-Length: %zu\r\n", content_len);
    }
    snprintf((char *)conn->request_header + strlen((char *)conn->request_header), SEND_HEADER_SIZE - strlen((char *)conn->request_header),
             "Host: %s\r\n", conn->host);
    return 0;
}

int httpc_request_write_header(struct httpc_conn *conn, char *name, char *value) {
    snprintf((char *)conn->request_header + strlen((char *)conn->request_header), SEND_HEADER_SIZE - strlen((char *)conn->request_header),
             "%s: %s\r\n", name, value);
    return 0;
}

int httpc_request_write_header_finish(struct httpc_conn *conn) {
    snprintf((char *)conn->request_header + strlen((char *)conn->request_header), SEND_HEADER_SIZE - strlen((char *)conn->request_header),
             "\r\n");
    int bytes_written = httpc_tls_write(conn->tls, conn->request_header, strlen((char *)conn->request_header));
    if (bytes_written < 0) {
        BK_LOGE(TAG,"%s: create failed, no memory!\r\n",__func__);
		free(conn->request_header);
		return -1;
    }
	else
		BK_LOGE(TAG,"write_header_finish, bytes_written:%d\r\n", bytes_written);
	if(conn->request_header) {
    	os_free(conn->request_header);
		conn->request_header = NULL;
	}
    return bytes_written;
}

static int httpc_ssl_base_poll_read(struct httpc_conn *conn, int timeout_ms)
{
	int ret = -1;
	int remain = 0;
	struct timeval timeout;
	struct httpc_tls *tls = (struct httpc_tls *) conn->tls;
	fd_set readset;
	fd_set errset;
	FD_ZERO(&readset);
	FD_ZERO(&errset);
	FD_SET(conn->sock, &readset);
	FD_SET(conn->sock, &errset);

	if (tls && (remain = mbedtls_ssl_get_bytes_avail(&tls->ctx)) > 0) {
		BK_LOGV(TAG, "remain data in cache, need to read again\r\n");
		return remain;
	}
	else
		BK_LOGV(TAG, "NO data available\r\n");
	ret = select(conn->sock + 1, &readset, NULL, &errset, bk_utils_ms_to_timeval(timeout_ms, &timeout));
	if (ret > 0 && FD_ISSET(conn->sock, &errset)) {
		int sock_errno = 0;
		uint32_t optlen = sizeof(sock_errno);
		getsockopt(conn->sock, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen);
		BK_LOGE(TAG, "poll_read select error %d, errno = %s, fd = %d\r\n", sock_errno, strerror(sock_errno), conn->sock);
		ret = -1;
	}
	return ret;
}

static int httpc_ssl_base_poll_write(struct httpc_conn *conn, int timeout_ms)
{
	int ret = -1;
	struct timeval timeout;
	fd_set writeset;
	fd_set errset;
	FD_ZERO(&writeset);
	FD_ZERO(&errset);
	FD_SET(conn->sock, &writeset);
	FD_SET(conn->sock, &errset);
	ret = select(conn->sock + 1, &writeset, NULL, &errset, bk_utils_ms_to_timeval(timeout_ms, &timeout));
	if (ret > 0 && FD_ISSET(conn->sock, &errset)) {
		int sock_errno = 0;
		uint32_t optlen = sizeof(sock_errno);
		getsockopt(conn->sock, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen);
		BK_LOGE(TAG, "poll_write select error %d, errno = %s, fd = %d\r\n", sock_errno, strerror(sock_errno), conn->sock);
		ret = -1;
	}
	return ret;
}

int httpc_request_write_data(struct httpc_conn *conn, uint8_t *data, size_t data_len)
{
    int bytes_write = 0;
    int total_write = 0;
    int left = data_len;

    BK_ASSERT(conn);

    if (conn->sock < 0)
    {
        return -1;
    }

    if (data_len == 0)
    {
        return 0;
    }

    /* send all of data on the buffer. */
    do
    {
        bytes_write = httpc_tls_write(conn->tls, (void *)((char *)data + total_write), left);
        if (bytes_write <= 0)
        {
#if (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
            if(conn->tls &&
                (bytes_write == MBEDTLS_ERR_SSL_WANT_READ || bytes_write == MBEDTLS_ERR_SSL_WANT_WRITE))
            {
                continue;
            }
#endif
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                /* send timeout */
                if (total_write)
                {
                    return total_write;
                }
                continue;
                /* TODO: whether return the TIMEOUT*/
            }
            else
            {
                closesocket(conn->sock);
                conn->sock = -1;

                if (total_write == 0)
                {
                    return -1;
                }
                break;
            }
        }

        left -= bytes_write;
        total_write += bytes_write;
		BK_LOGE(TAG, "%s, total_write: %d\r\n", __func__, total_write);
    }
    while (left);

    return total_write;	
}

int httpc_response_is_status(struct httpc_conn *conn, char *status)
{
	if (os_strncmp((char *)conn->response.status, status, conn->response.status_len) == 0) {
		return 1;
	}
	else
		return 0;
}

int httpc_response_get_header_field(struct httpc_conn *conn, char *field, char **value)
{
    char *field_start, *value_start, *value_end;
	char * header_begin = (char *)conn->response.header;
    field_start = strstr(header_begin, field);
    if (field_start == NULL) {
		conn->response.header = (uint8_t *)header_begin;
		BK_LOGE(TAG,"%s: %d no such field\r\n",__func__, __LINE__);
        return -1;
    }

    value_start = strchr(field_start, ':');
    if (value_start == NULL) {
		conn->response.header = (uint8_t *)header_begin;
		BK_LOGE(TAG,"%s: %d this field no value\r\n",__func__, __LINE__);
        return -1;
    }
    value_start += 2;

    value_end = strstr(value_start, "\n");
    if (value_end == NULL) {
		conn->response.header = (uint8_t *)header_begin;
		BK_LOGE(TAG,"%s LINE:%d value_end null\r\n", __func__, __LINE__);
        return -1;
    }
    int value_length = value_end - value_start;
    *value = malloc(value_length + 1);
    if (*value == NULL) {
		conn->response.header = (uint8_t *)header_begin;
        BK_LOGE(TAG,"%d alloc fail\r\n", __LINE__);
        return -1;
    }
    strncpy(*value, value_start, value_length);
    (*value)[value_length] = '\0';
	conn->response.header = (uint8_t *)header_begin;

    return 0;
}

static int httpc_read_line(struct httpc_conn *conn, char *buffer, int size)
{
    int rc, count = 0;
    char ch = 0, last_ch = 0;
	struct httpc_tls *tls = (struct httpc_tls *) conn->tls;
    BK_ASSERT(conn);
    BK_ASSERT(buffer);

    while (count < size)
    {
        rc = httpc_tls_read(tls, (unsigned char *) &ch, 1);
#if (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
        if (tls && (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            continue;
        }
#endif
        if (rc <= 0)
            return rc;

        if (ch == '\n' && last_ch == '\r')
            break;

        buffer[count++] = ch;

        last_ch = ch;
    }

    if (count > size)
    {
        BK_LOGE(TAG,"read line failed. The line data length is out of buffer size(%d)!\r\n", count);
        return -1;
    }

    return count;
}

int httpc_response_read_header(struct httpc_conn *conn)
{
	int rc;
	char *mime_buffer = NULL;
    conn->response.header = malloc(RCV_HEADER_SIZE);
    if (conn->response.header == NULL) {
        BK_LOGE(TAG," create failed, no memory for session header!\r\n");
        return -1;
    }

    while (1)
    {
        mime_buffer = (char *)conn->response.header + conn->response.header_len;
        rc = httpc_read_line(conn, mime_buffer, RCV_HEADER_SIZE - conn->response.header_len);
        if (rc < 0)
            break;
        if (rc == 0)
            break;
        if ((rc == 1) && (mime_buffer[0] == '\r'))
        {
            mime_buffer[0] = '\0';
            break;
        }

        mime_buffer[rc - 1] = '\n';
        conn->response.header_len += rc;

        if (conn->response.header_len >= RCV_HEADER_SIZE)
        {
            BK_LOGE(TAG,"not enough header buffer size(%d)!\r\n", RCV_HEADER_SIZE);
            return -1;
        }
    }
	conn->response.header = (uint8_t *)mime_buffer - conn->response.header_len;
	BK_LOGE(TAG,"conn->response.header_len:%d\r\n", conn->response.header_len);
	//bk_httpc_hex_dump((char *)conn->response.header, conn->response.header_len);

	httpc_response_get_header_field(conn, "Content-Type", (char **)&conn->response.content_type);
	char *content_len = NULL;
	httpc_response_get_header_field(conn, "Content-Length", &content_len);
	if (content_len) {
		conn->response.content_len = atoi(content_len);
		os_free(content_len);
		content_len = NULL;
	}
	int i = 0;
	char * header_v = (char *)conn->response.header;
	char * header_s = (char *)conn->response.header;
	char * header_temp = (char *)conn->response.header;
    if (strstr(header_v, "HTTP/1."))
    {
        char *ptr = header_v;

        ptr += strlen("HTTP/1.");

        while (*ptr && (*ptr == ' ' || *ptr == '\t'))
            ptr++;

        for (i = 0; ((ptr[i] != ' ') && (ptr[i] != '\t')); i++);
        ptr[i] = '\0';

		conn->response.version = (uint8_t *)os_strdup(ptr);
    }
    if (strstr(header_s, "HTTP/1."))
    {
        char *ptr = header_s;
		int len=0;
        ptr += strlen("HTTP/1.x ");
       // while (*ptr && (*ptr == ' ' || *ptr == '\t') && (*ptr != '\n')) {
        while (*ptr && (*ptr != '\n')) {
            ptr++;
			len++;
        }
		conn->response.status = os_malloc(len + 1);
		os_memset(conn->response.status, 0, len + 1);
		if (len && ptr)
			os_memcpy(conn->response.status, ptr-len, len);
		conn->response.status[len] = '\0';
		//conn->response.status = (uint8_t *)os_strdup(ptr);
    }
	conn->response.header = (uint8_t *)header_temp;
    return 0;

}

int httpc_response_read_data(struct httpc_conn *conn, uint8_t *data, size_t data_len)
{

    int bytes_read = 0;
    int total_read = 0;
    int left;

    BK_ASSERT(conn);

    if (conn->sock < 0)
    {
        return -1;
    }

    if (data_len == 0)
    {
        return 0;
    }

    if (conn->response.content_len < 0)
    {
		return -1;
    }
	left = data_len;
    do
    {
		int poll=0;
		/*
		 * Read until: there is an error, we've read "size" bytes or the remote
		 * side has closed the connection.
		 */
		if ((poll = httpc_ssl_base_poll_read(conn, 10)) <= 0) {
			BK_LOGE(TAG, "ssl_base_poll_read no data already\r\n");
			return total_read;;
		}
		else
			BK_LOGV(TAG, "ssl_base_poll_read:%d\r\n", poll);

        bytes_read = httpc_tls_read(conn->tls, (void *)((char *)data + total_read), left);
        if (bytes_read <= 0)
        {
			BK_LOGE(TAG,"%s bytes_read <=0 :%d\r\n", __func__, bytes_read);
#if (HTTPC_USE_TLS == HTTPC_TLS_MBEDTLS)
            if(conn->tls &&
                (bytes_read == MBEDTLS_ERR_SSL_WANT_READ || bytes_read == MBEDTLS_ERR_SSL_WANT_WRITE))
            {
                continue;
            }

#endif
            BK_LOGE(TAG,"receive data error(%d).\r\n", bytes_read);

            if (total_read)
            {
                break;
            }
            else
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    /* recv timeout */
                    BK_LOGD(TAG,"receive data timeout.\r\n");
                    return -1;
                }
                else
                {
                    closesocket(conn->sock);
                    conn->sock = -1;
                    return 0;
                }
            }
        }

        left -= bytes_read;
        total_read += bytes_read;
		BK_LOGE(TAG,"%s bytes_read:%d total_read:%d\r\n", __func__, bytes_read, total_read);
    }
    while (left);

    return total_read;
}

#if 0
/******************************************************demo*****************************************************/
#define USE_HTTPS    1
#define SERVER_HOST  "192.168.32.250"//"www.rt-thread.com"//"www.baidu.com"

static void example_httpc_post(char *url)
{
	BK_LOGE(TAG, "------Example: HTTPC post------\r\n");
	struct httpc_conn *conn = NULL;
#if 1
	const char *host_addr = 0;
	const char *path_ptr;
	const char *request;
	char *req_url = NULL;
	const char *port_ptr;
	char port_str[6] = "80";
	int host_addr_len = 0;
	int url_len = strlen(url);
	if (strncmp(url, "http://", 7) == 0)
	{
		host_addr = url + 7;
	}
	else if (strncmp(url, "https://", 8) == 0)
	{
		host_addr = url + 8;
	}
	else
	{
		return;
	}
	path_ptr = strstr(host_addr, "/");
	request = path_ptr ? path_ptr : "/";

	if (request)
	{
		req_url = strdup(request);
	}

	port_ptr = strstr(host_addr + host_addr_len, ":");
	if (port_ptr && path_ptr && (port_ptr < path_ptr))
	{
		int port_len = path_ptr - port_ptr - 1;

		strncpy(port_str, port_ptr + 1, port_len);
		port_str[port_len] = '\0';
	}

	if (port_ptr && (!path_ptr))
	{
		strcpy(port_str, port_ptr + 1);
	}
	if (!host_addr_len)
	{

		if ((port_ptr && path_ptr && (port_ptr < path_ptr) )|| (port_ptr && (!path_ptr)))
		{
			host_addr_len = port_ptr - host_addr;
		}
		else if (path_ptr)
		{
			host_addr_len = path_ptr - host_addr;
		}
		else
		{
			host_addr_len = strlen(host_addr);
		}

	}

	if ((host_addr_len < 1) || (host_addr_len > url_len))
	{
		return;
	}

	char *host_addr_new = os_malloc(host_addr_len + 1);

	if (!host_addr_new)
	{
		return;
	}

	memcpy(host_addr_new, host_addr, host_addr_len);
	host_addr_new[host_addr_len] = '\0';
	BK_LOGE(TAG,"host_addr_new:%s port_str:%d\r\n", host_addr_new, port_str);
#endif

#if USE_HTTPS
	conn = httpc_conn_new(HTTPC_SECURE_TLS, NULL, NULL, NULL);
#else
	conn = httpc_conn_new(HTTPC_SECURE_NONE, NULL, NULL, NULL);
#endif
	if(conn) {
#if USE_HTTPS
		if(httpc_conn_connect(conn, host_addr_new, 443, 0) == 0) {
#else
		if(httpc_conn_connect(conn, SERVER_HOST, 80, 0) == 0) {
#endif
			/* HTTP POST request */
			char *post_data = "param1=test_data1&param2=test_data2";
			BK_LOGE(TAG,"----------------BEGIN WRITE HEADER------------\r\n");
			// start a header and add Host (added automatically), Content-Type and Content-Length (added by input param)
			httpc_request_write_header_start(conn, "POST", req_url, NULL, strlen(post_data));//"/post"
			// add other header fields if necessary
			//httpc_request_write_header(conn, "Connection", "close");
			// finish and send header
			httpc_request_write_header_finish(conn);
			// send http body
			httpc_request_write_data(conn, (uint8_t *)post_data, strlen(post_data));
			BK_LOGE(TAG,"----------------BEGIN READ HEADER------------\r\n");
			// receive response header
			if(httpc_response_read_header(conn) == 0) {
				httpc_conn_dump_header(conn);

				// receive response body
				if(httpc_response_is_status(conn, "200 OK")) {
					//uint8_t buf[RCV_BUFF_SIZE];
					uint8_t *buf = (uint8_t *)malloc(RCV_BUFF_SIZE);
					int read_size = 0, total_size = 0;
					BK_LOGE(TAG,"----------------BEGIN READ PAYLOAD---------------\r\n");
					while(1) {
						memset(buf, 0, RCV_BUFF_SIZE);
						read_size = httpc_response_read_data(conn, buf, RCV_BUFF_SIZE - 1);

						if(read_size > 0) {
							total_size += read_size;
							bk_httpc_hex_dump((char *)buf, read_size);
						}
						else {
							break;
						}

						if(conn->response.content_len && (total_size >= conn->response.content_len))
							break;
					}
					if (buf) {
						os_free(buf);
						buf = NULL;
					}
				}
			}
		}
		else {
			BK_LOGD(NULL, "\nERROR: httpc_conn_connect\n");
		}
	}
	BK_LOGE(TAG,"----------------BEGIN CLOSE---------------\r\n");
	httpc_conn_close(conn);
	httpc_conn_free(conn);
	httpc_free(conn);
}

static void example_httpc_get(char *url)
{
	BK_LOGE(TAG, "-------Example: HTTPC get-----\r\n");
	struct httpc_conn *conn = NULL;
#if 1
    const char *host_addr = 0;
	const char *path_ptr;
	const char *request;
	char *req_url = NULL;
	const char *port_ptr;
	char port_str[6] = "80";
	int host_addr_len = 0;
	int url_len = strlen(url);
    if (strncmp(url, "http://", 7) == 0)
    {
        host_addr = url + 7;
    }
    else if (strncmp(url, "https://", 8) == 0)
    {
        host_addr = url + 8;
    }
    else
    {
        return;
    }
    path_ptr = strstr(host_addr, "/");
    request = path_ptr ? path_ptr : "/";

    if (request)
    {
        req_url = strdup(request);
    }

    port_ptr = strstr(host_addr + host_addr_len, ":");
    if (port_ptr && path_ptr && (port_ptr < path_ptr))
    {
        int port_len = path_ptr - port_ptr - 1;

        strncpy(port_str, port_ptr + 1, port_len);
        port_str[port_len] = '\0';
    }

    if (port_ptr && (!path_ptr))
    {
        strcpy(port_str, port_ptr + 1);
    }
    if (!host_addr_len)
    {

        if ((port_ptr && path_ptr && (port_ptr < path_ptr) )|| (port_ptr && (!path_ptr)))
        {
            host_addr_len = port_ptr - host_addr;
        }
        else if (path_ptr)
        {
            host_addr_len = path_ptr - host_addr;
        }
        else
        {
            host_addr_len = strlen(host_addr);
        }

    }

    if ((host_addr_len < 1) || (host_addr_len > url_len))
    {
        return;
    }

    char *host_addr_new = os_malloc(host_addr_len + 1);

    if (!host_addr_new)
    {
        return;
    }

    memcpy(host_addr_new, host_addr, host_addr_len);
    host_addr_new[host_addr_len] = '\0';
	//BK_LOGE(TAG,"host_addr_new:%s port_str:%d\r\n", host_addr_new, port_str);
#endif

#if USE_HTTPS
	conn = httpc_conn_new(HTTPC_SECURE_TLS, NULL, NULL, NULL);
#else
	conn = httpc_conn_new(HTTPC_SECURE_NONE, NULL, NULL, NULL);
#endif
	if(conn) {
#if USE_HTTPS
		if(httpc_conn_connect(conn, host_addr_new, 443, 0) == 0) {
#else
		if(httpc_conn_connect(conn, SERVER_HOST, 80, 0) == 0) {
#endif
			/* HTTP GET request */
			BK_LOGE(TAG,"----------------BEGIN WRITE HEADER------------\r\n");
			httpc_request_write_header_start(conn, "GET", req_url, NULL, 0);
			httpc_request_write_header(conn, "User-Agent", "BEKEN HTTP Client/1.0");
			// finish and send header
			httpc_request_write_header_finish(conn);
			BK_LOGE(TAG,"----------------BEGIN READ HEADER---------------\r\n");
			// receive response header
			if(httpc_response_read_header(conn) == 0) {
				httpc_conn_dump_header(conn);

				// receive response body
				if(httpc_response_is_status(conn, "200 OK")) {
					//uint8_t buf[RCV_BUFF_SIZE];
					uint8_t *buf = (uint8_t *)malloc(RCV_BUFF_SIZE);
					int read_size = 0, total_size = 0;
					BK_LOGE(TAG,"----------------BEGIN READ PAYLOAD---------------\r\n");
					while(1) {
						memset(buf, 0, RCV_BUFF_SIZE);
						read_size = httpc_response_read_data(conn, buf, RCV_BUFF_SIZE - 1);
						if(read_size > 0) {
							BK_LOGE(TAG,"httpc get response data: \r\n");
							total_size += read_size;
							bk_httpc_hex_dump((char *)buf, read_size);
						}
						else {
							break;
						}

						if(conn->response.content_len && (total_size >= conn->response.content_len))
							break;
					}
						if (buf) {
							os_free(buf);
							buf = NULL;
						}
				}
			}
		}
		else {
			printf("\nERROR: httpc_conn_connect\n");
		}
	}
	BK_LOGE(TAG,"----------------BEGIN CLOSE---------------\r\n");
	httpc_conn_close(conn);
	httpc_conn_free(conn);
	httpc_free((void *)conn);
}
extern char *httpc_url;
static void example_httpc_thread(void *param)
{
	printf("\nExample: HTTPC\n");
	/* test GET to http://httpbin.org/get?param1=test_data1&param2=test_data2 */
	example_httpc_get(httpc_url);
	/* test POST to http://httpbin.org/post with data of param1=test_data1&param2=test_data2 */
	example_httpc_post(httpc_url);
	rtos_delete_thread(NULL);
}

void example_httpc(void)
{
	if( rtos_create_thread(NULL, 7, "example_httpc_thread",
		(beken_thread_function_t)example_httpc_thread, 6*1024, 0) != 0) {
		BK_LOGD(NULL, "%s (example_httpc_thread) failed\r\n", __func__);
	}
}
#endif
