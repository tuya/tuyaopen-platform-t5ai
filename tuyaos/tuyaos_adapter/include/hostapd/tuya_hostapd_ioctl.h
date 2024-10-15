#ifndef __TUYA_HOSTAPD_IOCTL_H__
#define __TUYA_HOSTAPD_IOCTL_H__

typedef void * c_addr_t;
typedef unsigned short  ty_sa_family_t;

/*
*	1003.1g requires sa_family_t and that sa_data is char.
*/

struct ty_sockaddr {
 ty_sa_family_t sa_family_t; 		 /* address family, AF_xxx	  */
 char		  sa_data[14];		 /* 14 bytes of protocol address */
};

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define WLAN_EID_TIM_LEN      6

#define SIOCDEVPRIVATE	0x89F0	/* to 89FF */
								 
								 /*
								  *  These 16 ioctl calls are protocol private
								  */
								 
#define SIOCPROTOPRIVATE 0x89E0 /* to 89EF */

#define SIOCIWFIRSTPRIV	                 0x8BE0
#define SIOCIWLASTPRIV	                 0x8BFF


#define IWEVGENIE   0x8C05      /* Generic IE (WPA, RSN, WMM, ..)
                                 * (scan results); This includes id and
                                 * length fields. One IWEVGENIE may
                                 * contain more than one IE. Scan
                                 * results may contain one or more
                                 * IWEVGENIE events. */
#define IWEVMICHAELMICFAILURE 0x8C06    /* Michael MIC failure
                                         * (struct iw_michaelmicfailure)
                                         */
#define IWEVASSOCREQIE  0x8C07  /* IEs used in (Re)Association Request.
                                 * The data includes id and length
                                 * fields and may contain more than one
                                 * IE. This event is required in
                                 * Managed mode if the driver
                                 * generates its own WPA/RSN IE. This
                                 * should be sent just before
                                 * IWEVREGISTERED event for the
                                 * association. */
#define IWEVASSOCRESPIE 0x8C08  /* IEs used in (Re)Association
                                 * Response. The data includes id and
                                 * length fields and may contain more
                                 * than one IE. This may be sent
                                 * between IWEVASSOCREQIE and
                                 * IWEVREGISTERED events for the
                                 * association. */
#define IWEVPMKIDCAND   0x8C09  /* PMKID candidate for RSN
                                 * pre-authentication
                                 * (struct iw_pmkid_cand) */

#define IWEVTXDROP	0x8C00		/* Packet dropped to excessive retry */
#define IWEVQUAL	0x8C01		/* Quality part of statistics (scan) */
#define IWEVCUSTOM	0x8C02		/* Driver specific ascii string */
#define IWEVREGISTERED	0x8C03		/* Discovered a new node (AP mode) */
#define IWEVEXPIRED	0x8C04		/* Expired a node (AP mode) */
#define IWEVGENIE	0x8C05		/* Generic IE (WPA, RSN, WMM, ..)
					 * (scan results); This includes id and
					 * length fields. One IWEVGENIE may
					 * contain more than one IE. Scan
					 * results may contain one or more
					 * IWEVGENIE events. */
#define IWEVMICHAELMICFAILURE 0x8C06	/* Michael MIC failure
					 * (struct iw_michaelmicfailure)
					 */
#define IWEVASSOCREQIE	0x8C07		/* IEs used in (Re)Association Request.
					 * The data includes id and length
					 * fields and may contain more than one
					 * IE. This event is required in
					 * Managed mode if the driver
					 * generates its own WPA/RSN IE. This
					 * should be sent just before
					 * IWEVREGISTERED event for the
					 * association. */
#define IWEVASSOCRESPIE	0x8C08		/* IEs used in (Re)Association
					 * Response. The data includes id and
					 * length fields and may contain more
					 * than one IE. This may be sent
					 * between IWEVASSOCREQIE and
					 * IWEVREGISTERED events for the
					 * association. */
#define IWEVPMKIDCAND	0x8C09		/* PMKID candidate for RSN
					 * pre-authentication
					 * (struct iw_pmkid_cand) */


/* Wireless Identification */
#define SIOCSIWCOMMIT	0x8B00		/* Commit pending changes to driver */
#define SIOCGIWNAME	0x8B01		/* get name == wireless protocol */
/* SIOCGIWNAME is used to verify the presence of Wireless Extensions.
 * Common values : "IEEE 802.11-DS", "IEEE 802.11-FH", "IEEE 802.11b"...
 * Don't put the name of your driver there, it's useless. */

/* Basic operations */
#define SIOCSIWNWID	0x8B02		/* set network id (pre-802.11) */
#define SIOCGIWNWID	0x8B03		/* get network id (the cell) */
#define SIOCSIWFREQ	0x8B04		/* set channel/frequency (Hz) */
#define SIOCGIWFREQ	0x8B05		/* get channel/frequency (Hz) */
#define SIOCSIWMODE	0x8B06		/* set operation mode */
#define SIOCGIWMODE	0x8B07		/* get operation mode */
#define SIOCSIWSENS	0x8B08		/* set sensitivity (dBm) */
#define SIOCGIWSENS	0x8B09		/* get sensitivity (dBm) */

/* Informative stuff */
#define SIOCSIWRANGE	0x8B0A		/* Unused */
#define SIOCGIWRANGE	0x8B0B		/* Get range of parameters */
#define SIOCSIWPRIV	0x8B0C		/* Unused */
#define SIOCGIWPRIV	0x8B0D		/* get private ioctl interface info */
#define SIOCSIWSTATS	0x8B0E		/* Unused */
#define SIOCGIWSTATS	0x8B0F		/* Get /proc/net/wireless stats */
/* SIOCGIWSTATS is strictly used between user space and the kernel, and
 * is never passed to the driver (i.e. the driver will never see it). */

/* Spy support (statistics per MAC address - used for Mobile IP support) */
#define SIOCSIWSPY	0x8B10		/* set spy addresses */
#define SIOCGIWSPY	0x8B11		/* get spy info (quality of link) */
#define SIOCSIWTHRSPY	0x8B12		/* set spy threshold (spy event) */
#define SIOCGIWTHRSPY	0x8B13		/* get spy threshold */

/* Access Point manipulation */
#define SIOCSIWAP	0x8B14		/* set access point MAC addresses */
#define SIOCGIWAP	0x8B15		/* get access point MAC addresses */
#define SIOCGIWAPLIST	0x8B17		/* Deprecated in favor of scanning */
#define SIOCSIWSCAN	0x8B18		/* trigger scanning (list cells) */
#define SIOCGIWSCAN	0x8B19		/* get scanning results */

/* 802.11 specific support */
#define SIOCSIWESSID	0x8B1A		/* set ESSID (network name) */
#define SIOCGIWESSID	0x8B1B		/* get ESSID */
#define SIOCSIWNICKN	0x8B1C		/* set node name/nickname */
#define SIOCGIWNICKN	0x8B1D		/* get node name/nickname */
/* As the ESSID and NICKN are strings up to 32 bytes long, it doesn't fit
 * within the 'iwreq' structure, so we need to use the 'data' member to
 * point to a string in user space, like it is done for RANGE... */

/* Other parameters useful in 802.11 and some other devices */
#define SIOCSIWRATE	0x8B20		/* set default bit rate (bps) */
#define SIOCGIWRATE	0x8B21		/* get default bit rate (bps) */
#define SIOCSIWRTS	0x8B22		/* set RTS/CTS threshold (bytes) */
#define SIOCGIWRTS	0x8B23		/* get RTS/CTS threshold (bytes) */
#define SIOCSIWFRAG	0x8B24		/* set fragmentation thr (bytes) */
#define SIOCGIWFRAG	0x8B25		/* get fragmentation thr (bytes) */
#define SIOCSIWTXPOW	0x8B26		/* set transmit power (dBm) */
#define SIOCGIWTXPOW	0x8B27		/* get transmit power (dBm) */
#define SIOCSIWRETRY	0x8B28		/* set retry limits and lifetime */
#define SIOCGIWRETRY	0x8B29		/* get retry limits and lifetime */

/* Encoding stuff (scrambling, hardware security, WEP...) */
#define SIOCSIWENCODE	0x8B2A		/* set encoding token & mode */
#define SIOCGIWENCODE	0x8B2B		/* get encoding token & mode */
/* Power saving stuff (power management, unicast and multicast) */
#define SIOCSIWPOWER	0x8B2C		/* set Power Management settings */
#define SIOCGIWPOWER	0x8B2D		/* get Power Management settings */

/* WPA : Generic IEEE 802.11 informatiom element (e.g., for WPA/RSN/WMM).
 * This ioctl uses struct iw_point and data buffer that includes IE id and len
 * fields. More than one IE may be included in the request. Setting the generic
 * IE to empty buffer (len=0) removes the generic IE from the driver. Drivers
 * are allowed to generate their own WPA/RSN IEs, but in these cases, drivers
 * are required to report the used IE as a wireless event, e.g., when
 * associating with an AP. */
#define SIOCSIWGENIE	0x8B30		/* set generic IE */
#define SIOCGIWGENIE	0x8B31		/* get generic IE */

/* WPA : IEEE 802.11 MLME requests */
#define SIOCSIWMLME	0x8B16		/* request MLME operation; uses
					 * struct iw_mlme */
/* WPA : Authentication mode parameters */
#define SIOCSIWAUTH	0x8B32		/* set authentication mode params */
#define SIOCGIWAUTH	0x8B33		/* get authentication mode params */

/* WPA : Extended version of encoding configuration */
#define SIOCSIWENCODEEXT 0x8B34		/* set encoding token & mode */
#define SIOCGIWENCODEEXT 0x8B35		/* get encoding token & mode */

/* WPA2 : PMKSA cache management */
#define SIOCSIWPMKSA	0x8B36		/* PMKSA cache operation */

/* --------------------------- SUBTYPES --------------------------- */
/*
 *	For all data larger than 16 octets, we need to use a
 *	pointer to memory allocated in user space.
 */
struct	iw_point
{
  void   	*pointer;	/* Pointer to the data  (in user space) */
  unsigned short		length;		/* number of fields or size in bytes */
  unsigned short		flags;		/* Optional params */
};


/* ------------------------ IOCTL REQUEST ------------------------ */
/*
 *	Generic format for most parameters that fit in an int
 */
struct	iw_param
{
  int		value;		/* The value of the parameter itself */
  unsigned char		fixed;		/* Hardware should not use auto select */
  unsigned char		disabled;	/* Disable the feature */
  unsigned short		flags;		/* Various specifc flags (if any) */
};
/*
 *	A frequency
 *	For numbers lower than 10^9, we encode the number in 'm' and
 *	set 'e' to 0
 *	For number greater than 10^9, we divide it by the lowest power
 *	of 10 to get 'm' lower than 10^9, with 'm'= f / (10^'e')...
 *	The power of 10 is in 'e', the result of the division is in 'm'.
 */
struct	iw_freq
{
	int		m;		/* Mantissa */
	short		e;		/* Exponent */
	unsigned char		i;		/* List index (when in range struct) */
	unsigned char		flags;		/* Flags (fixed/auto) */
};

/*
 *	Quality of the link
 */
struct	iw_quality
{
	unsigned char		qual;		/* link quality (%retries, SNR,
					   %missed beacons or better...) */
	unsigned char		level;		/* signal level (dBm) */
	unsigned char		noise;		/* noise level (dBm) */
	unsigned char		updated;	/* Flags to know if updated */
};

/*
 *	Packet discarded in the wireless adapter due to
 *	"wireless" specific problems...
 *	Note : the list of counter and statistics in net_device_stats
 *	is already pretty exhaustive, and you should use that first.
 *	This is only additional stats...
 */
struct	iw_discarded
{
	unsigned int		nwid;		/* Rx : Wrong nwid/essid */
	unsigned int		code;		/* Rx : Unable to code/decode (WEP) */
	unsigned int		fragment;	/* Rx : Can't perform MAC reassembly */
	unsigned int		retries;	/* Tx : Max MAC retries num reached */
	unsigned int		misc;		/* Others cases */
};

/*
 *	Packet/Time period missed in the wireless adapter due to
 *	"wireless" specific problems...
 */
struct	iw_missed
{
	unsigned int		beacon;		/* Missed beacons/superframe */
};

/*
 * This structure defines the payload of an ioctl, and is used
 * below.
 *
 * Note that this structure should fit on the memory footprint
 * of iwreq (which is the same as ifreq), which mean a max size of
 * 16 octets = 128 bits. Warning, pointers might be 64 bits wide...
 * You should check this when increasing the structures defined
 * above in this file...
 */
union	iwreq_data
{
	/* Config - generic */
	char		name[IFNAMSIZ];
	/* Name : used to verify the presence of  wireless extensions.
	 * Name of the protocol/provider... */

	struct iw_point	essid;		/* Extended network name */
	struct iw_param	nwid;		/* network id (or domain - the cell) */
	struct iw_freq	freq;		/* frequency or channel :
					 * 0-1000 = channel
					 * > 1000 = frequency in Hz */

	struct iw_param	sens;		/* signal level threshold */
	struct iw_param	bitrate;	/* default bit rate */
	struct iw_param	txpower;	/* default transmit power */
	struct iw_param	rts;		/* RTS threshold threshold */
	struct iw_param	frag;		/* Fragmentation threshold */
	unsigned int		mode;		/* Operation mode */
	struct iw_param	retry;		/* Retry limits & lifetime */

	struct iw_point	encoding;	/* Encoding stuff : tokens */
	struct iw_param	power;		/* PM duration/timeout */
	struct iw_quality qual;		/* Quality part of statistics */

	struct ty_sockaddr	ap_addr;	/* Access point address */
	struct ty_sockaddr	addr;		/* Destination address (hw/mac) */

	struct iw_param	param;		/* Other small parameters */
	struct iw_point	data;		/* Other large parameters */
};

/*
 * The structure to exchange data for ioctl.
 * This structure is the same as 'struct ifreq', but (re)defined for
 * convenience...
 * Do I need to remind you about structure size (32 octets) ?
 */
struct	iwreq
{
	char	ifr_name[IFNAMSIZ];	/* if name, e.g. "eth0" */

	/* Data part (defined just above) */
	union	iwreq_data	u;
};

/* -------------------------- IOCTL DATA -------------------------- */
/*
 *	For those ioctl which want to exchange mode data that what could
 *	fit in the above structure...
 */

/*
 *	Range of parameters
 */
//#ifndef CONFIG_TUYA_SUPPORT 
#define SCAN_SSID_MAX    2
//#endif
/* Maximum frequencies in the range struct */
#define IW_MAX_FREQUENCIES	32
/* Note : if you have something like 80 frequencies,
 * don't increase this constant and don't fill the frequency list.
 * The user will be able to set by channel anyway... */

/* Maximum bit rates in the range struct */
#define IW_MAX_BITRATES		32

/* Maximum tx powers in the range struct */
#define IW_MAX_TXPOWER		8
/* Note : if you more than 8 TXPowers, just set the max and min or
 * a few of them in the struct iw_range. */

/* Maximum of address that you may set with SPY */
#define IW_MAX_SPY		8

/* Maximum of address that you may get in the
   list of access points in range */
#define IW_MAX_AP		64

/* Maximum size of the ESSID and NICKN strings */
#define IW_ESSID_MAX_SIZE	32
/* Maximum number of size of encoding token available
 * they are listed in the range structure */
#define IW_MAX_ENCODING_SIZES	8

/* Maximum size of the encoding token in bytes */
#define IW_ENCODING_TOKEN_MAX	64	/* 512 bits (for now) */

struct	iw_range
{
	/* Informative stuff (to choose between different interface) */
	unsigned int		throughput;	/* To give an idea... */
	/* In theory this value should be the maximum benchmarked
	 * TCP/IP throughput, because with most of these devices the
	 * bit rate is meaningless (overhead an co) to estimate how
	 * fast the connection will go and pick the fastest one.
	 * I suggest people to play with Netperf or any benchmark...
	 */

	/* NWID (or domain id) */
	unsigned int		min_nwid;	/* Minimal NWID we are able to set */
	unsigned int		max_nwid;	/* Maximal NWID we are able to set */

	/* Old Frequency (backward compat - moved lower ) */
	unsigned short		old_num_channels;
	unsigned char		old_num_frequency;

	/* Scan capabilities */
	unsigned char		scan_capa; 	/* IW_SCAN_CAPA_* bit field */

	/* Wireless event capability bitmasks */
	unsigned int		event_capa[6];

	/* signal level threshold range */
	int		sensitivity;

	/* Quality of link & SNR stuff */
	/* Quality range (link, level, noise)
	 * If the quality is absolute, it will be in the range [0 ; max_qual],
	 * if the quality is dBm, it will be in the range [max_qual ; 0].
	 * Don't forget that we use 8 bit arithmetics... */
	struct iw_quality	max_qual;	/* Quality of the link */
	/* This should contain the average/typical values of the quality
	 * indicator. This should be the threshold between a "good" and
	 * a "bad" link (example : monitor going from green to orange).
	 * Currently, user space apps like quality monitors don't have any
	 * way to calibrate the measurement. With this, they can split
	 * the range between 0 and max_qual in different quality level
	 * (using a geometric subdivision centered on the average).
	 * I expect that people doing the user space apps will feedback
	 * us on which value we need to put in each driver... */
	struct iw_quality	avg_qual;	/* Quality of the link */

	/* Rates */
	unsigned char		num_bitrates;	/* Number of entries in the list */
	int		bitrate[IW_MAX_BITRATES];	/* list, in bps */

	/* RTS threshold */
	int		min_rts;	/* Minimal RTS threshold */
	int		max_rts;	/* Maximal RTS threshold */

	/* Frag threshold */
	int		min_frag;	/* Minimal frag threshold */
	int		max_frag;	/* Maximal frag threshold */

	/* Power Management duration & timeout */
	int		min_pmp;	/* Minimal PM period */
	int		max_pmp;	/* Maximal PM period */
	int		min_pmt;	/* Minimal PM timeout */
	int		max_pmt;	/* Maximal PM timeout */
	unsigned short		pmp_flags;	/* How to decode max/min PM period */
	unsigned short		pmt_flags;	/* How to decode max/min PM timeout */
	unsigned short		pm_capa;	/* What PM options are supported */

	/* Encoder stuff */
	unsigned short	encoding_size[IW_MAX_ENCODING_SIZES];	/* Different token sizes */
	unsigned char	num_encoding_sizes;	/* Number of entry in the list */
	unsigned char	max_encoding_tokens;	/* Max number of tokens */
	/* For drivers that need a "login/passwd" form */
	unsigned char	encoding_login_index;	/* token index for login token */

	/* Transmit power */
	unsigned short		txpower_capa;	/* What options are supported */
	unsigned char		num_txpower;	/* Number of entries in the list */
	int		txpower[IW_MAX_TXPOWER];	/* list, in bps */

	/* Wireless Extension version info */
	unsigned char		we_version_compiled;	/* Must be WIRELESS_EXT */
	unsigned char		we_version_source;	/* Last update of source */

	/* Retry limits and lifetime */
	unsigned short		retry_capa;	/* What retry options are supported */
	unsigned short		retry_flags;	/* How to decode max/min retry limit */
	unsigned short		r_time_flags;	/* How to decode max/min retry life */
	int		min_retry;	/* Minimal number of retries */
	int		max_retry;	/* Maximal number of retries */
	int		min_r_time;	/* Minimal retry lifetime */
	int		max_r_time;	/* Maximal retry lifetime */

	/* Frequency */
	unsigned short		num_channels;	/* Number of channels [0; num - 1] */
	unsigned char		num_frequency;	/* Number of entry in the list */
	struct iw_freq	freq[IW_MAX_FREQUENCIES];	/* list */
	/* Note : this frequency list doesn't need to fit channel numbers,
	 * because each entry contain its channel index */

	unsigned int		enc_capa;	/* IW_ENC_CAPA_* bit field */
};

/* ----------------------- WIRELESS EVENTS ----------------------- */
/*
 * Wireless events are carried through the rtnetlink socket to user
 * space. They are encapsulated in the IFLA_WIRELESS field of
 * a RTM_NEWLINK message.
 */

/*
 * A Wireless Event. Contains basically the same data as the ioctl...
 */
struct iw_event
{
	unsigned short		len;			/* Real length of this stuff */
	unsigned short		cmd;			/* Wireless IOCTL */
	union iwreq_data	u;		/* IOCTL fixed payload */
};

/* Size of the Event prefix (including padding and alignement junk) */
#define IW_EV_LCP_LEN	(sizeof(struct iw_event) - sizeof(union iwreq_data))
/* Size of the various events */
#define IW_EV_CHAR_LEN	(IW_EV_LCP_LEN + IFNAMSIZ)
#define IW_EV_UINT_LEN	(IW_EV_LCP_LEN + sizeof(unsigned int))
#define IW_EV_FREQ_LEN	(IW_EV_LCP_LEN + sizeof(struct iw_freq))
#define IW_EV_PARAM_LEN	(IW_EV_LCP_LEN + sizeof(struct iw_param))
#define IW_EV_ADDR_LEN	(IW_EV_LCP_LEN + sizeof(struct sockaddr))
#define IW_EV_QUAL_LEN	(IW_EV_LCP_LEN + sizeof(struct iw_quality))

/* iw_point events are special. First, the payload (extra data) come at
 * the end of the event, so they are bigger than IW_EV_POINT_LEN. Second,
 * we omit the pointer, so start at an offset. */
#define IW_EV_POINT_OFF (((char *) &(((struct iw_point *) NULL)->length)) - \
			  (char *) NULL)
#define IW_EV_POINT_LEN	(IW_EV_LCP_LEN + sizeof(struct iw_point) - \
			 IW_EV_POINT_OFF)

/* netdevice private ioctls (used, e.g., with iwpriv from user space) */

/* New wireless extensions API - SET/GET convention (even ioctl numbers are
 * root only)
 */
#define TUYA_IOCTL_SETPARAM (SIOCIWFIRSTPRIV + 0)
#define TUYA_IOCTL_GETPARAM (SIOCIWFIRSTPRIV + 1)
#define TUYA_IOCTL_WRITEMIF (SIOCIWFIRSTPRIV + 2)
#define TUYA_IOCTL_READMIF (SIOCIWFIRSTPRIV + 3)
#define TUYA_IOCTL_MONITOR (SIOCIWFIRSTPRIV + 4)
#define TUYA_IOCTL_RESET (SIOCIWFIRSTPRIV + 6)
#define TUYA_IOCTL_INQUIRE (SIOCIWFIRSTPRIV + 8)
#define TUYA_IOCTL_WDS_ADD (SIOCIWFIRSTPRIV + 10)
#define TUYA_IOCTL_WDS_DEL (SIOCIWFIRSTPRIV + 12)
#define TUYA_IOCTL_SET_RID_WORD (SIOCIWFIRSTPRIV + 14)
#define TUYA_IOCTL_MACCMD (SIOCIWFIRSTPRIV + 16)
#define TUYA_IOCTL_ADDMAC (SIOCIWFIRSTPRIV + 18)
#define TUYA_IOCTL_DELMAC (SIOCIWFIRSTPRIV + 20)
#define TUYA_IOCTL_KICKMAC (SIOCIWFIRSTPRIV + 22)

/* following are not in SIOCGIWPRIV list; check permission in the driver code
 */
#define TUYA_IOCTL_DOWNLOAD (SIOCDEVPRIVATE + 13)
#define TUYA_IOCTL_HOSTAPD (SIOCDEVPRIVATE + 14)

#define  TUYA_RUN_TIME_MODE

#ifdef TUYA_RUN_TIME_MODE
enum
{
        CLR_CNT = 0,
        START_CNT = 1,
        STOP_CNT = 2,
};

enum
{
        RATE_1M = 0,
        RATE_2M,
        RATE_5_5M,
        RATE_11M,

        RATE_6M,
        RATE_9M,
        RATE_12M,
        RATE_18M,
        RATE_24M,
        RATE_36M,
        RATE_48M,
        RATE_54M,

		RATE_MCS0,
        RATE_MCS1,
        RATE_MCS2,
        RATE_MCS3,
        RATE_MCS4,
        RATE_MCS5,
        RATE_MCS6,
        RATE_MCS7,
};

struct tuya_statistic
{
		int vif_idx;
		int flag;
        int tx_total;
        int tx_ok;
        int tx_failed;
        int tx_retry;
        int tx_full;
		int tx_empty;

        int rx_ok;
        int rx_fcs;
    	int rx_full;
		int rx_empty;
};

#endif

/* TUYA_IOCTL_SETPARAM ioctl() subtypes: */
enum {
	TUYA_PARAM_PTYPE = 1,
	TUYA_PARAM_TXRATECTRL = 2,
	TUYA_PARAM_BEACON_INT = 3,
	TUYA_PARAM_PSEUDO_IBSS = 4,
	TUYA_PARAM_ALC = 5,
	TUYA_PARAM_TXPOWER = 6,
	TUYA_PARAM_DUMP = 7,
	TUYA_PARAM_OTHER_AP_POLICY = 8,
	TUYA_PARAM_AP_MAX_INACTIVITY = 9,
	TUYA_PARAM_AP_BRIDGE_PACKETS = 10,
	TUYA_PARAM_DTIM_PERIOD = 11,
	TUYA_PARAM_AP_NULLFUNC_ACK = 12,
	TUYA_PARAM_MAX_WDS = 13,
	TUYA_PARAM_AP_AUTOM_AP_WDS = 14,
	TUYA_PARAM_AP_AUTH_ALGS = 15,
	TUYA_PARAM_MONITOR_ALLOW_FCSERR = 16,
	TUYA_PARAM_HOST_ENCRYPT = 17,
	TUYA_PARAM_HOST_DECRYPT = 18,
	TUYA_PARAM_BUS_MASTER_THRESHOLD_RX = 19,
	TUYA_PARAM_BUS_MASTER_THRESHOLD_TX = 20,
	TUYA_PARAM_HOST_ROAMING = 21,
	TUYA_PARAM_BCRX_STA_KEY = 22,
	TUYA_PARAM_IEEE_802_1X = 23,
	TUYA_PARAM_ANTSEL_TX = 24,
	TUYA_PARAM_ANTSEL_RX = 25,
	TUYA_PARAM_MONITOR_TYPE = 26,
	TUYA_PARAM_WDS_TYPE = 27,
	TUYA_PARAM_HOSTSCAN = 28,
	TUYA_PARAM_AP_SCAN = 29,
	TUYA_PARAM_ENH_SEC = 30,
	TUYA_PARAM_IO_DEBUG = 31,
	TUYA_PARAM_BASIC_RATES = 32,
	TUYA_PARAM_OPER_RATES = 33,
	TUYA_PARAM_HOSTAPD = 34,
	TUYA_PARAM_HOSTAPD_STA = 35,
	TUYA_PARAM_WPA = 36,
	TUYA_PARAM_PRIVACY_INVOKED = 37,
	TUYA_PARAM_TKIP_COUNTERMEASURES = 38,
	TUYA_PARAM_DROP_UNENCRYPTED = 39,
	TUYA_PARAM_SCAN_CHANNEL_MASK = 40,
    /* Below Extend by TUYA */
	TUYA_PARAM_COUNTRY_CODE = 200,
	TUYA_PARAM_CHANNEL = 203,
	TUYA_PARAM_CALIBRATED = 204,
	TUYA_PARAM_LP_MODE = 205,
	TUYA_PARAM_MACADDR = 206,
	TUYA_PARAM_STA_CNN_STATUS = 207,
	TUYA_PARAM_SEND_RAW_FRAME = 208,
	TUYA_PARAM_RCV_RAW_FRAME = 209,
	TUYA_PARAM_LOW_POWER = 210,

#ifdef TUYA_RUN_TIME_MODE	
	TUYA_PARAM_SET_POWER = 211,
	TUYA_PARAM_SET_TX_FIXRATE = 212,
	TUYA_PARAM_SET_GI = 213,
	TUYA_PARAM_SET_BW = 214,
	TUYA_PARAM_SET_RTS = 215,
	TUYA_PARAM_START_STATISTIC = 216,
	TUYA_PARAM_STOP_STATISTIC = 217,
	TUYA_PARAM_CLEAR_STATISTIC = 218,
	TUYA_PARAM_GET_TX_STATUS = 219,
	TUYA_PARAM_GET_RX_STATUS = 220,
#endif	

};

enum { TUYA_HOSTAP_ANTSEL_DO_NOT_TOUCH = 0, TUYA_HOSTAP_ANTSEL_DIVERSITY = 1,
       TUYA_HOSTAP_ANTSEL_LOW = 2, TUYA_HOSTAP_ANTSEL_HIGH = 3 };


/* TUYA_IOCTL_MACCMD ioctl() subcommands: */
enum { TUYA_AP_MAC_CMD_POLICY_OPEN = 0, TUYA_AP_MAC_CMD_POLICY_ALLOW = 1,
       TUYA_AP_MAC_CMD_POLICY_DENY = 2, TUYA_AP_MAC_CMD_FLUSH = 3,
       TUYA_AP_MAC_CMD_KICKALL = 4 };


/* TUYA_IOCTL_DOWNLOAD ioctl() dl_cmd: */
enum {
	TUYA_DOWNLOAD_VOLATILE = 1 /* RAM */,
	/* Note! Old versions of prism2_srec have a fatal error in CRC-16
	 * calculation, which will corrupt all non-volatile downloads.
	 * TUYA_DOWNLOAD_VOLATILE used to be 2, but it is now 3 to
	 * prevent use of old versions of prism2_srec for non-volatile
	 * download. */
	TUYA_DOWNLOAD_NON_VOLATILE = 3 /* FLASH */,
	TUYA_DOWNLOAD_VOLATILE_GENESIS = 4 /* RAM in Genesis mode */,
	/* Persistent versions of volatile download commands (keep firmware
	 * data in memory and automatically re-download after hw_reset */
	TUYA_DOWNLOAD_VOLATILE_PERSISTENT = 5,
	TUYA_DOWNLOAD_VOLATILE_GENESIS_PERSISTENT = 6,
};

/* TUYA_IOCTL_HOSTAPD ioctl() cmd: */
enum {
	TUYA_HOSTAPD_FLUSH = 1,
	TUYA_HOSTAPD_ADD_STA = 2,
	TUYA_HOSTAPD_REMOVE_STA = 3,
	TUYA_HOSTAPD_GET_INFO_STA = 4,
	TUYA_HOSTAPD_RESET_TXEXC_STA = 5,
	TUYA_SET_ENCRYPTION = 6,
	TUYA_GET_ENCRYPTION = 7,
	TUYA_HOSTAPD_SET_FLAGS_STA = 8,
	TUYA_HOSTAPD_GET_RID = 9,
	TUYA_HOSTAPD_SET_RID = 10,
	TUYA_HOSTAPD_SET_ASSOC_AP_ADDR = 11,
	TUYA_HOSTAPD_SET_GENERIC_ELEMENT = 12,
	TUYA_HOSTAPD_MLME = 13,
	TUYA_HOSTAPD_SCAN_REQ = 14,
	TUYA_HOSTAPD_STA_CLEAR_STATS = 15,
	TUYA_HOSTAPD_GET_SCAN_RESULT = 16,
	TUYA_HOSTAPD_ASSOC_REQ = 17,
	TUYA_HOSTAPD_ASSOC_ACK = 18,

	TUYA_HOSTAPD_REG_SCAN_CALLBACK = 19,
	TUYA_HOSTAPD_REG_ASSOC_CALLBACK = 20,
	TUYA_HOSTAPD_REG_DISASSOC_CALLBACK = 21,
	TUYA_HOSTAPD_REG_DEAUTH_CALLBACK = 22,
	TUYA_HOSTAPD_REG_APP_START = 23,

	TUYA_HOSTAPD_DISCONN_REQ = 24,
	TUYA_HOSTAPD_WPA_INIT_VIF = 25,
	TUYA_HOSTAPD_WPA_DEINIT_VIF = 26,
	TUYA_HOSTAPD_START_APM = 27,
	TUYA_HOSTAPD_SET_AP_BCN = 28,
    TUYA_HOSTAPD_GET_BSS_INFO = 29,
    TUYA_HOSTAPD_CHANNEL_SWITCH = 30,
	TUYA_HOSTAPD_STOP_APM = 31,

	TUYA_HOSTAPD_REG_CSA_CALLBACK = 32,
	TUYA_HOSTAPD_POLL_CLIENT_NULL_DATA = 33,
	TUYA_HOSTAPD_JOIN_MESH = 34,
	TUYA_HOSTAPD_LEAVE_MESH = 35,
	TUYA_HOSTAPD_AUTHENTICATE = 36,
	TUYA_HOSTAPD_REG_AUTH_CALLBACK = 37,
	TUYA_HOSTAPD_SET_OPER_STATE = 38,
	TUYA_HOSTAPD_EXTERNAL_AUTH_STATUS = 39,
    /* Below Extend by TUYA */
    TUYA_HOSTAPD_GET_HW_FEATURES = 200,
    TUYA_HOSTAPD_SCAN_ALL_AP = 201,
    TUYA_HOSTAPD_SCAN_SPECIAL_AP = 202,
    TUYA_HOSTAPD_MGMT_HOOK_CALLBACK = 203,
    TUYA_HOSTAPD_SET_SSID = 204,
    TUYA_HOSTAPD_EVENT_HOOK_CALLBACK = 205,
    TUYA_HOSTAPD_GET_SOFTAP_STATS = 206,
    TUYA_HOSTAPD_GET_WPA_CAPACITY = 207,
    TUYA_HOSTAPD_SET_STA_KEEPALIVE = 208,
    TUYA_HOSTAPD_SCAN_CANCEL=209,
};

#endif /* __TUYA_HOSTAPD_IOCTL_H__ */
