/**
 * @file tuya_hostapd_driver.h
 * @brief hostapd and wpa_supplicant 无线驱动接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#ifndef TUYA_HOSTAPD_DRIVER_H
#define TUYA_HOSTAPD_DRIVER_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

//#include "tuya_hal_network.h"
#include "tuya_hostapd_ioctl.h"

#ifndef CONFIG_TUYA_SUPPORT
#define CONFIG_TUYA_SUPPORT 1
#endif

#ifndef SSID_MAX_LEN
#define SSID_MAX_LEN 32
#endif

#ifndef TY_MAX_BSS_LIST
#define TY_MAX_BSS_LIST            64
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#define TUYA_PRISM2_HOSTAPD_MAX_BUF_SIZE 1024
#define TUYA_PRISM2_HOSTAPD_RID_HDR_LEN \
((size_t) (&((struct ty_hostapd_param *) 0)->u.rid.data))
#define TUYA_PRISM2_HOSTAPD_GENERIC_ELEMENT_HDR_LEN \
((size_t) (&((struct ty_hostapd_param *) 0)->u.generic_elem.data))

#define TUYA_HOSTAP_CRYPT_ALG_NAME_LEN 16

#define TUYA_HOSTAP_AUTH_OPEN    0
#define TUYA_HOSTAP_AUTH_SHARED  1
#define TUYA_HOSTAP_AUTH_SAE	 3

#define WPAS_MAX_SCAN_SSIDS 2

#define TUYA_WLAN_IS_AP_INTERFACE(vif_index) (vif_index == WF_AP)
#define TUYA_WLAN_IS_STA_INTERFACE(vif_index) (vif_index == WF_STATION)

/**
 * enum wpa_event_type - Event type for wpa_supplicant_event() calls
 */
typedef enum wpa_event_type {
	/**
	 * EVENT_ASSOC - Association completed
	 *
	 * This event needs to be delivered when the driver completes IEEE
	 * 802.11 association or reassociation successfully.
	 * wpa_driver_ops::get_bssid() is expected to provide the current BSSID
	 * after this event has been generated. In addition, optional
	 * EVENT_ASSOCINFO may be generated just before EVENT_ASSOC to provide
	 * more information about the association. If the driver interface gets
	 * both of these events at the same time, it can also include the
	 * assoc_info data in EVENT_ASSOC call.
	 */
	EVENT_ASSOC,

	/**
	 * EVENT_DISASSOC - Association lost
	 *
	 * This event should be called when association is lost either due to
	 * receiving deauthenticate or disassociate frame from the AP or when
	 * sending either of these frames to the current AP. If the driver
	 * supports separate deauthentication event, EVENT_DISASSOC should only
	 * be used for disassociation and EVENT_DEAUTH for deauthentication.
	 * In AP mode, union wpa_event_data::disassoc_info is required.
	 */
	EVENT_DISASSOC,

	/**
	 * EVENT_MICHAEL_MIC_FAILURE - Michael MIC (TKIP) detected
	 *
	 * This event must be delivered when a Michael MIC error is detected by
	 * the local driver. Additional data for event processing is
	 * provided with union wpa_event_data::michael_mic_failure. This
	 * information is used to request new encyption key and to initiate
	 * TKIP countermeasures if needed.
	 */
	EVENT_MICHAEL_MIC_FAILURE,

	/**
	 * EVENT_SCAN_RESULTS - Scan results available
	 *
	 * This event must be called whenever scan results are available to be
	 * fetched with struct wpa_driver_ops::get_scan_results(). This event
	 * is expected to be used some time after struct wpa_driver_ops::scan()
	 * is called. If the driver provides an unsolicited event when the scan
	 * has been completed, this event can be used to trigger
	 * EVENT_SCAN_RESULTS call. If such event is not available from the
	 * driver, the driver wrapper code is expected to use a registered
	 * timeout to generate EVENT_SCAN_RESULTS call after the time that the
	 * scan is expected to be completed. Optional information about
	 * completed scan can be provided with union wpa_event_data::scan_info.
	 */
	EVENT_SCAN_RESULTS,

	/**
	 * EVENT_ASSOCINFO - Report optional extra information for association
	 *
	 * This event can be used to report extra association information for
	 * EVENT_ASSOC processing. This extra information includes IEs from
	 * association frames and Beacon/Probe Response frames in union
	 * wpa_event_data::assoc_info. EVENT_ASSOCINFO must be send just before
	 * EVENT_ASSOC. Alternatively, the driver interface can include
	 * assoc_info data in the EVENT_ASSOC call if it has all the
	 * information available at the same point.
	 */
	EVENT_ASSOCINFO,

	/**
	 * EVENT_INTERFACE_STATUS - Report interface status changes
	 *
	 * This optional event can be used to report changes in interface
	 * status (interface added/removed) using union
	 * wpa_event_data::interface_status. This can be used to trigger
	 * wpa_supplicant to stop and re-start processing for the interface,
	 * e.g., when a cardbus card is ejected/inserted.
	 */
	EVENT_INTERFACE_STATUS,

	/**
	 * EVENT_PMKID_CANDIDATE - Report a candidate AP for pre-authentication
	 *
	 * This event can be used to inform wpa_supplicant about candidates for
	 * RSN (WPA2) pre-authentication. If wpa_supplicant is not responsible
	 * for scan request (ap_scan=2 mode), this event is required for
	 * pre-authentication. If wpa_supplicant is performing scan request
	 * (ap_scan=1), this event is optional since scan results can be used
	 * to add pre-authentication candidates. union
	 * wpa_event_data::pmkid_candidate is used to report the BSSID of the
	 * candidate and priority of the candidate, e.g., based on the signal
	 * strength, in order to try to pre-authenticate first with candidates
	 * that are most likely targets for re-association.
	 *
	 * EVENT_PMKID_CANDIDATE can be called whenever the driver has updates
	 * on the candidate list. In addition, it can be called for the current
	 * AP and APs that have existing PMKSA cache entries. wpa_supplicant
	 * will automatically skip pre-authentication in cases where a valid
	 * PMKSA exists. When more than one candidate exists, this event should
	 * be generated once for each candidate.
	 *
	 * Driver will be notified about successful pre-authentication with
	 * struct wpa_driver_ops::add_pmkid() calls.
	 */
	EVENT_PMKID_CANDIDATE,

	/**
	 * EVENT_STKSTART - Request STK handshake (MLME-STKSTART.request)
	 *
	 * This event can be used to inform wpa_supplicant about desire to set
	 * up secure direct link connection between two stations as defined in
	 * IEEE 802.11e with a new PeerKey mechanism that replaced the original
	 * STAKey negotiation. The caller will need to set peer address for the
	 * event.
	 */
	EVENT_STKSTART,

	/**
	 * EVENT_TDLS - Request TDLS operation
	 *
	 * This event can be used to request a TDLS operation to be performed.
	 */
	EVENT_TDLS,

	/**
	 * EVENT_FT_RESPONSE - Report FT (IEEE 802.11r) response IEs
	 *
	 * The driver is expected to report the received FT IEs from
	 * FT authentication sequence from the AP. The FT IEs are included in
	 * the extra information in union wpa_event_data::ft_ies.
	 */
	EVENT_FT_RESPONSE,

	/**
	 * EVENT_IBSS_RSN_START - Request RSN authentication in IBSS
	 *
	 * The driver can use this event to inform wpa_supplicant about a STA
	 * in an IBSS with which protected frames could be exchanged. This
	 * event starts RSN authentication with the other STA to authenticate
	 * the STA and set up encryption keys with it.
	 */
	EVENT_IBSS_RSN_START,

	/**
	 * EVENT_AUTH - Authentication result
	 *
	 * This event should be called when authentication attempt has been
	 * completed. This is only used if the driver supports separate
	 * authentication step (struct wpa_driver_ops::authenticate).
	 * Information about authentication result is included in
	 * union wpa_event_data::auth.
	 */
	EVENT_AUTH,

	/**
	 * EVENT_DEAUTH - Authentication lost
	 *
	 * This event should be called when authentication is lost either due
	 * to receiving deauthenticate frame from the AP or when sending that
	 * frame to the current AP.
	 * In AP mode, union wpa_event_data::deauth_info is required.
	 */
	EVENT_DEAUTH,

	/**
	 * EVENT_ASSOC_REJECT - Association rejected
	 *
	 * This event should be called when (re)association attempt has been
	 * rejected by the AP. Information about the association response is
	 * included in union wpa_event_data::assoc_reject.
	 */
	EVENT_ASSOC_REJECT,

	/**
	 * EVENT_AUTH_TIMED_OUT - Authentication timed out
	 */
	EVENT_AUTH_TIMED_OUT,

	/**
	 * EVENT_ASSOC_TIMED_OUT - Association timed out
	 */
	EVENT_ASSOC_TIMED_OUT,

	/**
	 * EVENT_WPS_BUTTON_PUSHED - Report hardware push button press for WPS
	 */
	EVENT_WPS_BUTTON_PUSHED,

	/**
	 * EVENT_TX_STATUS - Report TX status
	 */
	EVENT_TX_STATUS,

	/**
	 * EVENT_RX_FROM_UNKNOWN - Report RX from unknown STA
	 */
	EVENT_RX_FROM_UNKNOWN,

	/**
	 * EVENT_RX_MGMT - Report RX of a management frame
	 */
	EVENT_RX_MGMT,

	/**
	 * EVENT_REMAIN_ON_CHANNEL - Remain-on-channel duration started
	 *
	 * This event is used to indicate when the driver has started the
	 * requested remain-on-channel duration. Information about the
	 * operation is included in union wpa_event_data::remain_on_channel.
	 */
	EVENT_REMAIN_ON_CHANNEL,

	/**
	 * EVENT_CANCEL_REMAIN_ON_CHANNEL - Remain-on-channel timed out
	 *
	 * This event is used to indicate when the driver has completed
	 * remain-on-channel duration, i.e., may noot be available on the
	 * requested channel anymore. Information about the
	 * operation is included in union wpa_event_data::remain_on_channel.
	 */
	EVENT_CANCEL_REMAIN_ON_CHANNEL,

	/**
	 * EVENT_RX_PROBE_REQ - Indicate received Probe Request frame
	 *
	 * This event is used to indicate when a Probe Request frame has been
	 * received. Information about the received frame is included in
	 * union wpa_event_data::rx_probe_req. The driver is required to report
	 * these events only after successfully completed probe_req_report()
	 * commands to request the events (i.e., report parameter is non-zero)
	 * in station mode. In AP mode, Probe Request frames should always be
	 * reported.
	 */
	EVENT_RX_PROBE_REQ,

	/**
	 * EVENT_NEW_STA - New wired device noticed
	 *
	 * This event is used to indicate that a new device has been detected
	 * in a network that does not use association-like functionality (i.e.,
	 * mainly wired Ethernet). This can be used to start EAPOL
	 * authenticator when receiving a frame from a device. The address of
	 * the device is included in union wpa_event_data::new_sta.
	 */
	EVENT_NEW_STA,

	/**
	 * EVENT_EAPOL_RX - Report received EAPOL frame
	 *
	 * When in AP mode with hostapd, this event is required to be used to
	 * deliver the receive EAPOL frames from the driver.
	 */
	EVENT_EAPOL_RX,

	/**
	 * EVENT_SIGNAL_CHANGE - Indicate change in signal strength
	 *
	 * This event is used to indicate changes in the signal strength
	 * observed in frames received from the current AP if signal strength
	 * monitoring has been enabled with signal_monitor().
	 */
	EVENT_SIGNAL_CHANGE,

	/**
	 * EVENT_INTERFACE_ENABLED - Notify that interface was enabled
	 *
	 * This event is used to indicate that the interface was enabled after
	 * having been previously disabled, e.g., due to rfkill.
	 */
	EVENT_INTERFACE_ENABLED,

	/**
	 * EVENT_INTERFACE_DISABLED - Notify that interface was disabled
	 *
	 * This event is used to indicate that the interface was disabled,
	 * e.g., due to rfkill.
	 */
	EVENT_INTERFACE_DISABLED,

	/**
	 * EVENT_CHANNEL_LIST_CHANGED - Channel list changed
	 *
	 * This event is used to indicate that the channel list has changed,
	 * e.g., because of a regulatory domain change triggered by scan
	 * results including an AP advertising a country code.
	 */
	EVENT_CHANNEL_LIST_CHANGED,

	/**
	 * EVENT_INTERFACE_UNAVAILABLE - Notify that interface is unavailable
	 *
	 * This event is used to indicate that the driver cannot maintain this
	 * interface in its operation mode anymore. The most likely use for
	 * this is to indicate that AP mode operation is not available due to
	 * operating channel would need to be changed to a DFS channel when
	 * the driver does not support radar detection and another virtual
	 * interfaces caused the operating channel to change. Other similar
	 * resource conflicts could also trigger this for station mode
	 * interfaces. This event can be propagated when channel switching
	 * fails.
	 */
	EVENT_INTERFACE_UNAVAILABLE,

	/**
	 * EVENT_BEST_CHANNEL
	 *
	 * Driver generates this event whenever it detects a better channel
	 * (e.g., based on RSSI or channel use). This information can be used
	 * to improve channel selection for a new AP/P2P group.
	 */
	EVENT_BEST_CHANNEL,

	/**
	 * EVENT_UNPROT_DEAUTH - Unprotected Deauthentication frame received
	 *
	 * This event should be called when a Deauthentication frame is dropped
	 * due to it not being protected (MFP/IEEE 802.11w).
	 * union wpa_event_data::unprot_deauth is required to provide more
	 * details of the frame.
	 */
	EVENT_UNPROT_DEAUTH,

	/**
	 * EVENT_UNPROT_DISASSOC - Unprotected Disassociation frame received
	 *
	 * This event should be called when a Disassociation frame is dropped
	 * due to it not being protected (MFP/IEEE 802.11w).
	 * union wpa_event_data::unprot_disassoc is required to provide more
	 * details of the frame.
	 */
	EVENT_UNPROT_DISASSOC,

	/**
	 * EVENT_STATION_LOW_ACK
	 *
	 * Driver generates this event whenever it detected that a particular
	 * station was lost. Detection can be through massive transmission
	 * failures for example.
	 */
	EVENT_STATION_LOW_ACK,

	/**
	 * EVENT_IBSS_PEER_LOST - IBSS peer not reachable anymore
	 */
	EVENT_IBSS_PEER_LOST,

	/**
	 * EVENT_DRIVER_GTK_REKEY - Device/driver did GTK rekey
	 *
	 * This event carries the new replay counter to notify wpa_supplicant
	 * of the current EAPOL-Key Replay Counter in case the driver/firmware
	 * completed Group Key Handshake while the host (including
	 * wpa_supplicant was sleeping).
	 */
	EVENT_DRIVER_GTK_REKEY,

	/**
	 * EVENT_SCHED_SCAN_STOPPED - Scheduled scan was stopped
	 */
	EVENT_SCHED_SCAN_STOPPED,

	/**
	 * EVENT_DRIVER_CLIENT_POLL_OK - Station responded to poll
	 *
	 * This event indicates that the station responded to the poll
	 * initiated with @poll_client.
	 */
	EVENT_DRIVER_CLIENT_POLL_OK,

	/**
	 * EVENT_EAPOL_TX_STATUS - notify of EAPOL TX status
	 */
	EVENT_EAPOL_TX_STATUS,

	/**
	 * EVENT_CH_SWITCH - AP or GO decided to switch channels
	 *
	 * Described in wpa_event_data.ch_switch
	 * */
	EVENT_CH_SWITCH,

	/**
	 * EVENT_WNM - Request WNM operation
	 *
	 * This event can be used to request a WNM operation to be performed.
	 */
	EVENT_WNM,

	/**
	 * EVENT_CONNECT_FAILED_REASON - Connection failure reason in AP mode
	 *
	 * This event indicates that the driver reported a connection failure
	 * with the specified client (for example, max client reached, etc.) in
	 * AP mode.
	 */
	EVENT_CONNECT_FAILED_REASON,

	/**
	 * EVENT_DFS_RADAR_DETECTED - Notify of radar detection
	 *
	 * A radar has been detected on the supplied frequency, hostapd should
	 * react accordingly (e.g., change channel).
	 */
	EVENT_DFS_RADAR_DETECTED,

	/**
	 * EVENT_DFS_CAC_FINISHED - Notify that channel availability check has been completed
	 *
	 * After a successful CAC, the channel can be marked clear and used.
	 */
	EVENT_DFS_CAC_FINISHED,

	/**
	 * EVENT_DFS_CAC_ABORTED - Notify that channel availability check has been aborted
	 *
	 * The CAC was not successful, and the channel remains in the previous
	 * state. This may happen due to a radar beeing detected or other
	 * external influences.
	 */
	EVENT_DFS_CAC_ABORTED,

	/**
	 * EVENT_DFS_NOP_FINISHED - Notify that non-occupancy period is over
	 *
	 * The channel which was previously unavailable is now available again.
	 */
	EVENT_DFS_NOP_FINISHED,

	/**
	 * EVENT_SURVEY - Received survey data
	 *
	 * This event gets triggered when a driver query is issued for survey
	 * data and the requested data becomes available. The returned data is
	 * stored in struct survey_results. The results provide at most one
	 * survey entry for each frequency and at minimum will provide one
	 * survey entry for one frequency. The survey data can be os_malloc()'d
	 * and then os_free()'d, so the event callback must only copy data.
	 */
	EVENT_SURVEY,

	/**
	 * EVENT_SCAN_STARTED - Scan started
	 *
	 * This indicates that driver has started a scan operation either based
	 * on a request from wpa_supplicant/hostapd or from another application.
	 * EVENT_SCAN_RESULTS is used to indicate when the scan has been
	 * completed (either successfully or by getting cancelled).
	 */
	EVENT_SCAN_STARTED,

	/**
	 * EVENT_AVOID_FREQUENCIES - Received avoid frequency range
	 *
	 * This event indicates a set of frequency ranges that should be avoided
	 * to reduce issues due to interference or internal co-existence
	 * information in the driver.
	 */
	EVENT_AVOID_FREQUENCIES,

	/**
	 * EVENT_NEW_PEER_CANDIDATE - new (unknown) mesh peer notification
	 */
	EVENT_NEW_PEER_CANDIDATE,

	/**
	 * EVENT_ACS_CHANNEL_SELECTED - Received selected channels by ACS
	 *
	 * Indicates a pair of primary and secondary channels chosen by ACS
	 * in device.
	 */
	EVENT_ACS_CHANNEL_SELECTED,

	/**
	 * EVENT_DFS_CAC_STARTED - Notify that channel availability check has
	 * been started.
	 *
	 * This event indicates that channel availability check has been started
	 * on a DFS frequency by a driver that supports DFS Offload.
	 */
	EVENT_DFS_CAC_STARTED,
     /**
      * EVENT_P2P_LO_STOP - Notify that P2P listen offload is stopped
      */
     EVENT_P2P_LO_STOP,

     /**
	 * EVENT_EXTERNAL_AUTH - This event interface is used by host drivers
	 * that do not define separate commands for authentication and
	 * association (~WPA_DRIVER_FLAGS_SME) but offload the 802.11
	 * authentication to wpa_supplicant. This event carries all the
	 * necessary information from the host driver for the authentication to
	 * happen.
	 */
	EVENT_EXTERNAL_AUTH,
} TY_WLAN_EVENT_TYPE_T;

struct wmm_params {
	unsigned char info_bitmap;
	unsigned char uapsd_queues;
};

struct ty_wpa_driver_scan_ssid {
    /**
     * ssid - specific SSID to scan for (ProbeReq)
     * %NULL or zero-length SSID is used to indicate active scan
     * with wildcard SSID.
     */
    const unsigned char *ssid;
    /**
     * ssid_len: Length of the SSID in octets
     */
    size_t ssid_len;
};

/**
 * struct external_auth - External authentication trigger parameters
 *
 * These are used across the external authentication request and event
 * interfaces.
 * @action: Action type / trigger for external authentication. Only significant
 *	for the event interface.
 * @bssid: BSSID of the peer with which the authentication has to happen. Used
 *	by both the request and event interface.
 * @ssid: SSID of the AP. Used by both the request and event interface.
 * @ssid_len: SSID length in octets.
 * @key_mgmt_suite: AKM suite of the respective authentication. Optional for
 *	the request interface.
 * @status: Status code, %WLAN_STATUS_SUCCESS for successful authentication,
 *	use %WLAN_STATUS_UNSPECIFIED_FAILURE if wpa_supplicant cannot give
 *	the real status code for failures. Used only for the request interface
 *	from user space to the driver.
 * @pmkid: Generated PMKID as part of external auth exchange (e.g., SAE).
 */
struct external_auth {
	enum {
		EXT_AUTH_START,
		EXT_AUTH_ABORT,
	} action;
	unsigned char *bssid;
	unsigned char *ssid;
	size_t ssid_len;
	unsigned int key_mgmt_suite;
	unsigned short status;
	unsigned char *pmkid;
};


/**
 * union wpa_event_data - Additional data for wpa_supplicant_event() calls
 */
typedef union wpa_event_data {
	/**
	 * struct assoc_info - Data for EVENT_ASSOC and EVENT_ASSOCINFO events
	 *
	 * This structure is optional for EVENT_ASSOC calls and required for
	 * EVENT_ASSOCINFO calls. By using EVENT_ASSOC with this data, the
	 * driver interface does not need to generate separate EVENT_ASSOCINFO
	 * calls.
	 */
	struct assoc_info {
		/**
		 * reassoc - Flag to indicate association or reassociation
		 */
		int reassoc;

		/**
		 * req_ies - (Re)Association Request IEs
		 *
		 * If the driver generates WPA/RSN IE, this event data must be
		 * returned for WPA handshake to have needed information. If
		 * wpa_supplicant-generated WPA/RSN IE is used, this
		 * information event is optional.
		 *
		 * This should start with the first IE (fixed fields before IEs
		 * are not included).
		 */
		const unsigned char *req_ies;

		/**
		 * req_ies_len - Length of req_ies in bytes
		 */
		size_t req_ies_len;

		/**
		 * resp_ies - (Re)Association Response IEs
		 *
		 * Optional association data from the driver. This data is not
		 * required WPA, but may be useful for some protocols and as
		 * such, should be reported if this is available to the driver
		 * interface.
		 *
		 * This should start with the first IE (fixed fields before IEs
		 * are not included).
		 */
		const unsigned char *resp_ies;

		/**
		 * resp_ies_len - Length of resp_ies in bytes
		 */
		size_t resp_ies_len;

		/**
		 * beacon_ies - Beacon or Probe Response IEs
		 *
		 * Optional Beacon/ProbeResp data: IEs included in Beacon or
		 * Probe Response frames from the current AP (i.e., the one
		 * that the client just associated with). This information is
		 * used to update WPA/RSN IE for the AP. If this field is not
		 * set, the results from previous scan will be used. If no
		 * data for the new AP is found, scan results will be requested
		 * again (without scan request). At this point, the driver is
		 * expected to provide WPA/RSN IE for the AP (if WPA/WPA2 is
		 * used).
		 *
		 * This should start with the first IE (fixed fields before IEs
		 * are not included).
		 */
		const unsigned char *beacon_ies;

		/**
		 * beacon_ies_len - Length of beacon_ies */
		size_t beacon_ies_len;

		/**
		 * freq - Frequency of the operational channel in MHz
		 */
		unsigned int freq;

		/**
		 * wmm_params - WMM parameters used in this association.
		 */
		struct wmm_params wmm_params;

		/**
		 * addr - Station address (for AP mode)
		 */
		const unsigned char *addr;

		/**
		 * The following is the key management offload information
		 * @authorized
		 * @key_replay_ctr
		 * @key_replay_ctr_len
		 * @ptk_kck
		 * @ptk_kek_len
		 * @ptk_kek
		 * @ptk_kek_len
		 */

		/**
		 * authorized - Status of key management offload,
		 * 1 = successful
		 */
		int authorized;

		/**
		 * key_replay_ctr - Key replay counter value last used
		 * in a valid EAPOL-Key frame
		 */
		const unsigned char *key_replay_ctr;

		/**
		 * key_replay_ctr_len - The length of key_replay_ctr
		 */
		size_t key_replay_ctr_len;

		/**
		 * ptk_kck - The derived PTK KCK
		 */
		const unsigned char *ptk_kck;

		/**
		 * ptk_kek_len - The length of ptk_kck
		 */
		size_t ptk_kck_len;

		/**
		 * ptk_kek - The derived PTK KEK
		 */
		const unsigned char *ptk_kek;

		/**
		 * ptk_kek_len - The length of ptk_kek
		 */
		size_t ptk_kek_len;
	} assoc_info;

	/**
	 * struct disassoc_info - Data for EVENT_DISASSOC events
	 */
	struct disassoc_info {
		/**
		 * addr - Station address (for AP mode)
		 */
		const unsigned char *addr;

		/**
		 * reason_code - Reason Code (host byte order) used in
		 *	Deauthentication frame
		 */
		unsigned short reason_code;

		/**
		 * ie - Optional IE(s) in Disassociation frame
		 */
		const unsigned char *ie;

		/**
		 * ie_len - Length of ie buffer in octets
		 */
		size_t ie_len;

		/**
		 * locally_generated - Whether the frame was locally generated
		 */
		int locally_generated;
	} disassoc_info;

	/**
	 * struct deauth_info - Data for EVENT_DEAUTH events
	 */
	struct deauth_info {
		/**
		 * addr - Station address (for AP mode)
		 */
		const unsigned char *addr;

		/**
		 * reason_code - Reason Code (host byte order) used in
		 *	Deauthentication frame
		 */
		unsigned short reason_code;

		/**
		 * ie - Optional IE(s) in Deauthentication frame
		 */
		const unsigned char *ie;

		/**
		 * ie_len - Length of ie buffer in octets
		 */
		size_t ie_len;

		/**
		 * locally_generated - Whether the frame was locally generated
		 */
		int locally_generated;
	} deauth_info;
#ifndef CONFIG_TUYA_SUPPORT

	/**
	 * struct michael_mic_failure - Data for EVENT_MICHAEL_MIC_FAILURE
	 */
	struct michael_mic_failure {
		int unicast;
		const unsigned char *src;
	} michael_mic_failure;

	/**
	 * struct interface_status - Data for EVENT_INTERFACE_STATUS
	 */
	struct interface_status {
		char ifname[100];
		enum {
			EVENT_INTERFACE_ADDED, EVENT_INTERFACE_REMOVED
		} ievent;
	} interface_status;

	/**
	 * struct pmkid_candidate - Data for EVENT_PMKID_CANDIDATE
	 */
	struct pmkid_candidate {
		/** BSSID of the PMKID candidate */
		unsigned char bssid[ETH_ALEN];
		/** Smaller the index, higher the priority */
		int index;
		/** Whether RSN IE includes pre-authenticate flag */
		int preauth;
	} pmkid_candidate;

	/**
	 * struct stkstart - Data for EVENT_STKSTART
	 */
	struct stkstart {
		unsigned char peer[ETH_ALEN];
	} stkstart;

	/**
	 * struct tdls - Data for EVENT_TDLS
	 */
	struct tdls {
		unsigned char peer[ETH_ALEN];
		enum {
			TDLS_REQUEST_SETUP,
			TDLS_REQUEST_TEARDOWN,
			TDLS_REQUEST_DISCOVER,
		} oper;
		unsigned short reason_code; /* for teardown */
	} tdls;

	/**
	 * struct wnm - Data for EVENT_WNM
	 */
	struct wnm {
		unsigned char addr[ETH_ALEN];
		enum {
			WNM_OPER_SLEEP,
		} oper;
		enum {
			WNM_SLEEP_ENTER,
			WNM_SLEEP_EXIT
		} sleep_action;
		int sleep_intval;
		unsigned short reason_code;
		unsigned char *buf;
		unsigned short buf_len;
	} wnm;

	/**
	 * struct ft_ies - FT information elements (EVENT_FT_RESPONSE)
	 *
	 * During FT (IEEE 802.11r) authentication sequence, the driver is
	 * expected to use this event to report received FT IEs (MDIE, FTIE,
	 * RSN IE, TIE, possible resource request) to the supplicant. The FT
	 * IEs for the next message will be delivered through the
	 * struct wpa_driver_ops::update_ft_ies() callback.
	 */
	struct ft_ies {
		const unsigned char *ies;
		size_t ies_len;
		int ft_action;
		unsigned char target_ap[ETH_ALEN];
		/** Optional IE(s), e.g., WMM TSPEC(s), for RIC-Request */
		const unsigned char *ric_ies;
		/** Length of ric_ies buffer in octets */
		size_t ric_ies_len;
	} ft_ies;

	/**
	 * struct ibss_rsn_start - Data for EVENT_IBSS_RSN_START
	 */
	struct ibss_rsn_start {
		unsigned char peer[ETH_ALEN];
	} ibss_rsn_start;
#endif
	/**
	 * struct auth_info - Data for EVENT_AUTH events
	 */
	struct auth_info {
		unsigned char peer[ETH_ALEN];
		unsigned char bssid[ETH_ALEN];
		unsigned short auth_type;
		unsigned short auth_transaction;
		unsigned short status_code;
		const unsigned char *ies;
		size_t ies_len;
	} auth;

	/**
	 * struct assoc_reject - Data for EVENT_ASSOC_REJECT events
	 */
	struct assoc_reject {
		/**
		 * bssid - BSSID of the AP that rejected association
		 */
		const unsigned char *bssid;

		/**
		 * resp_ies - (Re)Association Response IEs
		 *
		 * Optional association data from the driver. This data is not
		 * required WPA, but may be useful for some protocols and as
		 * such, should be reported if this is available to the driver
		 * interface.
		 *
		 * This should start with the first IE (fixed fields before IEs
		 * are not included).
		 */
		const unsigned char *resp_ies;

		/**
		 * resp_ies_len - Length of resp_ies in bytes
		 */
		size_t resp_ies_len;

		/**
		 * status_code - Status Code from (Re)association Response
		 */
		unsigned short status_code;
	} assoc_reject;

	struct timeout_event {
		unsigned char addr[ETH_ALEN];
	} timeout_event;

	/**
	 * struct tx_status - Data for EVENT_TX_STATUS events
	 */
	struct tx_status {
		unsigned short type;
		unsigned short stype;
		const unsigned char *dst;
		const unsigned char *data;
		size_t data_len;
		int ack;
	} tx_status;
#ifndef CONFIG_TUYA_SUPPORT

	/**
	 * struct rx_from_unknown - Data for EVENT_RX_FROM_UNKNOWN events
	 */
	struct rx_from_unknown {
		const unsigned char *bssid;
		const unsigned char *addr;
		int wds;
	} rx_from_unknown;
#endif
	/**
	 * struct rx_mgmt - Data for EVENT_RX_MGMT events
	 */
	struct rx_mgmt {
		const unsigned char *frame;
		size_t frame_len;
		unsigned int datarate;

		/**
		 * drv_priv - Pointer to store driver private BSS information
		 *
		 * If not set to NULL, this is used for comparison with
		 * hostapd_data->drv_priv to determine which BSS should process
		 * the frame.
		 */
		void *drv_priv;

		/**
		 * freq - Frequency (in MHz) on which the frame was received
		 */
		int freq;

		/**
		 * ssi_signal - Signal strength in dBm (or 0 if not available)
		 */
		int ssi_signal;
	} rx_mgmt;
#ifndef CONFIG_TUYA_SUPPORT

	/**
	 * struct remain_on_channel - Data for EVENT_REMAIN_ON_CHANNEL events
	 *
	 * This is also used with EVENT_CANCEL_REMAIN_ON_CHANNEL events.
	 */
	struct remain_on_channel {
		/**
		 * freq - Channel frequency in MHz
		 */
		unsigned int freq;

		/**
		 * duration - Duration to remain on the channel in milliseconds
		 */
		unsigned int duration;
	} remain_on_channel;
#endif
	/**
	 * struct scan_info - Optional data for EVENT_SCAN_RESULTS events
	 * @aborted: Whether the scan was aborted
	 * @freqs: Scanned frequencies in MHz (%NULL = all channels scanned)
	 * @num_freqs: Number of entries in freqs array
	 * @ssids: Scanned SSIDs (%NULL or zero-length SSID indicates wildcard
	 *	SSID)
	 * @num_ssids: Number of entries in ssids array
	 */
	struct scan_info {
		int aborted;
		const int *freqs;
		size_t num_freqs;
		struct ty_wpa_driver_scan_ssid ssids[WPAS_MAX_SCAN_SSIDS];
		size_t num_ssids;
	} scan_info;

	/**
	 * struct rx_probe_req - Data for EVENT_RX_PROBE_REQ events
	 */
	struct rx_probe_req {
		/**
		 * sa - Source address of the received Probe Request frame
		 */
		const unsigned char *sa;

		/**
		 * da - Destination address of the received Probe Request frame
		 *	or %NULL if not available
		 */
		const unsigned char *da;

		/**
		 * bssid - BSSID of the received Probe Request frame or %NULL
		 *	if not available
		 */
		const unsigned char *bssid;

		/**
		 * ie - IEs from the Probe Request body
		 */
		const unsigned char *ie;

		/**
		 * ie_len - Length of ie buffer in octets
		 */
		size_t ie_len;

		/**
		 * signal - signal strength in dBm (or 0 if not available)
		 */
		int ssi_signal;
	} rx_probe_req;

	/**
	 * struct new_sta - Data for EVENT_NEW_STA events
	 */
	struct new_sta {
		const unsigned char *addr;
	} new_sta;

	/**
	 * struct eapol_rx - Data for EVENT_EAPOL_RX events
	 */
	struct eapol_rx {
		const unsigned char *src;
		const unsigned char *data;
		size_t data_len;
	} eapol_rx;
#ifndef CONFIG_TUYA_SUPPORT

	/**
	 * signal_change - Data for EVENT_SIGNAL_CHANGE events
	 */
	struct wpa_signal_info signal_change;

	/**
	 * struct best_channel - Data for EVENT_BEST_CHANNEL events
	 * @freq_24: Best 2.4 GHz band channel frequency in MHz
	 * @freq_5: Best 5 GHz band channel frequency in MHz
	 * @freq_overall: Best channel frequency in MHz
	 *
	 * 0 can be used to indicate no preference in either band.
	 */
	struct best_channel {
		int freq_24;
		int freq_5;
		int freq_overall;
	} best_chan;
#endif
	struct unprot_deauth {
		const unsigned char *sa;
		const unsigned char *da;
		unsigned short reason_code;
	} unprot_deauth;

	struct unprot_disassoc {
		const unsigned char *sa;
		const unsigned char *da;
		unsigned short reason_code;
	} unprot_disassoc;
#ifndef CONFIG_TUYA_SUPPORT

	/**
	 * struct low_ack - Data for EVENT_STATION_LOW_ACK events
	 * @addr: station address
	 */
	struct low_ack {
		unsigned char addr[ETH_ALEN];
	} low_ack;

	/**
	 * struct ibss_peer_lost - Data for EVENT_IBSS_PEER_LOST
	 */
	struct ibss_peer_lost {
		unsigned char peer[ETH_ALEN];
	} ibss_peer_lost;

	/**
	 * struct driver_gtk_rekey - Data for EVENT_DRIVER_GTK_REKEY
	 */
	struct driver_gtk_rekey {
		const unsigned char *bssid;
		const unsigned char *replay_ctr;
	} driver_gtk_rekey;

	/**
	 * struct client_poll - Data for EVENT_DRIVER_CLIENT_POLL_OK events
	 * @addr: station address
	 */
	struct client_poll {
		unsigned char addr[ETH_ALEN];
	} client_poll;

	/**
	 * struct eapol_tx_status
	 * @dst: Original destination
	 * @data: Data starting with IEEE 802.1X header (!)
	 * @data_len: Length of data
	 * @ack: Indicates ack or lost frame
	 *
	 * This corresponds to hapd_send_eapol if the frame sent
	 * there isn't just reported as EVENT_TX_STATUS.
	 */
	struct eapol_tx_status {
		const unsigned char *dst;
		const unsigned char *data;
		int data_len;
		int ack;
	} eapol_tx_status;

	/**
	 * struct ch_switch
	 * @freq: Frequency of new channel in MHz
	 * @ht_enabled: Whether this is an HT channel
	 * @ch_offset: Secondary channel offset
	 * @ch_width: Channel width
	 * @cf1: Center frequency 1
	 * @cf2: Center frequency 2
	 */
	struct ch_switch {
		int freq;
		int ht_enabled;
		int ch_offset;
		enum chan_width ch_width;
		int cf1;
		int cf2;
	} ch_switch;

	/**
	 * struct connect_failed - Data for EVENT_CONNECT_FAILED_REASON
	 * @addr: Remote client address
	 * @code: Reason code for connection failure
	 */
	struct connect_failed_reason {
		unsigned char addr[ETH_ALEN];
		enum {
			MAX_CLIENT_REACHED,
			BLOCKED_CLIENT
		} code;
	} connect_failed_reason;



	/**
	 * struct dfs_event - Data for radar detected events
	 * @freq: Frequency of the channel in MHz
	 */
	struct dfs_event {
		int freq;
		int ht_enabled;
		int chan_offset;
		enum chan_width chan_width;
		int cf1;
		int cf2;
	} dfs_event;

	/**
	 * survey_results - Survey result data for EVENT_SURVEY
	 * @freq_filter: Requested frequency survey filter, 0 if request
	 *	was for all survey data
	 * @survey_list: Linked list of survey data (struct freq_survey)
	 */
	struct survey_results {
		unsigned int freq_filter;
		struct dl_list survey_list; /* struct freq_survey */
	} survey_results;

	/**
	 * channel_list_changed - Data for EVENT_CHANNEL_LIST_CHANGED
	 * @initiator: Initiator of the regulatory change
	 * @type: Regulatory change type
	 * @alpha2: Country code (or "" if not available)
	 */
	struct channel_list_changed {
		enum reg_change_initiator initiator;
		enum reg_type type;
		char alpha2[3];
	} channel_list_changed;

	/**
	 * freq_range - List of frequency ranges
	 *
	 * This is used as the data with EVENT_AVOID_FREQUENCIES.
	 */
	struct wpa_freq_range_list freq_range;
#endif
    
#ifdef CONFIG_MESH
	/**
	 * struct mesh_peer
	 *
	 * @peer: Peer address
	 * @ies: Beacon IEs
	 * @ie_len: Length of @ies
	 *
	 * Notification of new candidate mesh peer.
	 */
	struct mesh_peer {
		const unsigned char *peer;
		const unsigned char *ies;
		size_t ie_len;
	} mesh_peer;
#endif

#ifdef CONFIG_ACS
	/**
	 * struct acs_selected_channels - Data for EVENT_ACS_CHANNEL_SELECTED
	 * @pri_channel: Selected primary channel
	 * @sec_channel: Selected secondary channel
	 * @vht_seg0_center_ch: VHT mode Segment0 center channel
	 * @vht_seg1_center_ch: VHT mode Segment1 center channel
	 * @ch_width: Selected Channel width by driver. Driver may choose to
	 *	change hostapd configured ACS channel width due driver internal
	 *	channel restrictions.
	 * hw_mode: Selected band (used with hw_mode=any)
	 */
	struct acs_selected_channels {
		unsigned char pri_channel;
		unsigned char sec_channel;
		unsigned char vht_seg0_center_ch;
		unsigned char vht_seg1_center_ch;
		unsigned short ch_width;
		enum hostapd_hw_mode hw_mode;
	} acs_selected_channels;
#endif    
//#if CONFIG_EXTERNAL_AUTH_SAE
	/* For EVENT_EXTERNAL_AUTH */
	struct external_auth external_auth;
//#endif
}TY_WLAN_EVENT_DATA_S;

typedef void (*FUNC_2PARAM_PTR) (void *arg, unsigned char vif_idx);
typedef int (*FUNC_MGMT_FRAME_HOOK_PTR) (int vif_idx, unsigned char *buf, int len);
typedef int (*FUNC_WLAN_AUTH_EVENT_HOOK_PTR) (void *ctx, TY_WLAN_EVENT_TYPE_T type, TY_WLAN_EVENT_DATA_S *data);

/* struct wpa_scan_res */
struct wpa_scan_res {
	unsigned int flags;
	unsigned char bssid[ETH_ALEN];
	int freq;
	unsigned short beacon_int;
	unsigned short caps;
	int qual;
	int noise;
	int level;
	unsigned long long tsf;
	unsigned int age;
	unsigned int est_throughput;
	int snr;
	unsigned long long parent_tsf;
	unsigned char tsf_bssid[ETH_ALEN];
	size_t ie_len;
	size_t beacon_ie_len;
	/* Followed by ie_len + beacon_ie_len octets of IE data */
};

struct wpa_reltime {
    long sec;
    long usec;
};

/* struct wpa_scan_results */
struct wpa_scan_results {
	struct wpa_scan_res **res;
	size_t num;
	struct wpa_reltime fetch_time;
};

/* struct hostapd_freq_params */
struct hostapd_freq_params {
	/**
	 * mode - Mode/band (HOSTAPD_MODE_IEEE80211A, ..)
	 */
	//enum hostapd_hw_mode mode;
	int mode;

	/**
	 * freq - Primary channel center frequency in MHz
	 */
	int freq;

	/**
	 * channel - Channel number
	 */
	int channel;

	/**
	 * ht_enabled - Whether HT is enabled
	 */
	int ht_enabled;

	/**
	 * sec_channel_offset - Secondary channel offset for HT40
	 *
	 * 0 = HT40 disabled,
	 * -1 = HT40 enabled, secondary channel below primary,
	 * 1 = HT40 enabled, secondary channel above primary
	 */
	int sec_channel_offset;

	/**
	 * vht_enabled - Whether VHT is enabled
	 */
	int vht_enabled;

	/**
	 * center_freq1 - Segment 0 center frequency in MHz
	 *
	 * Valid for both HT and VHT.
	 */
	int center_freq1;

	/**
	 * center_freq2 - Segment 1 center frequency in MHz
	 *
	 * Non-zero only for bandwidth 80 and an 80+80 channel
	 */
	int center_freq2;

	/**
	 * bandwidth - Channel bandwidth in MHz (20, 40, 80, 160)
	 */
	int bandwidth;
};

/* struct beacon_data */
struct beacon_data {
	unsigned char *head, *tail;
	unsigned char *beacon_ies;
	unsigned char *proberesp_ies;
	unsigned char *assocresp_ies;
	unsigned char *probe_resp;

	size_t head_len, tail_len;
	size_t beacon_ies_len;
	size_t proberesp_ies_len;
	size_t assocresp_ies_len;
	size_t probe_resp_len;
};

/* struct csa_settings */
struct csa_settings {
	unsigned char cs_count;
	unsigned char block_tx;

	struct hostapd_freq_params freq_params;
	struct beacon_data beacon_csa;
	struct beacon_data beacon_after;

	unsigned short counter_offset_beacon;
	unsigned short counter_offset_presp;
};

/* struct prism2_hostapd_param */
typedef struct ty_hostapd_param {
	unsigned int cmd;
	unsigned char sta_addr[ETH_ALEN];
    unsigned char vif_idx;

	union {
		struct {
			unsigned char *own_addr;
			unsigned char *sta_addr;
		} poll_null_data;

		struct {
			FUNC_2PARAM_PTR cb;
			void *arg;
		} reg_scan_cfm;

		struct {
			FUNC_2PARAM_PTR cb;
			void *arg;
		} reg_assoc_cfm;

		struct {
			FUNC_2PARAM_PTR cb;
			void *arg;
		} reg_auth_cfm;

		struct {
			FUNC_2PARAM_PTR cb;
			void *arg;
		} reg_disassoc_evt, reg_deauth_evt;

		struct {
			unsigned short aid;
			unsigned short capability;
			unsigned char tx_supp_rates;
			unsigned char plink_state;
			unsigned char  set;
			void *arg;
		} add_sta;

        struct {
            unsigned int type;
			unsigned char *indx_ptr;
		} add_if;

		struct {
			unsigned int inactive_sec;
		} get_info_sta;

        struct {
            void *beacon;
            unsigned short bcn_len;
            unsigned short head_len;
            unsigned char tim_len;
            unsigned int pairwise_ciphers;
            unsigned int group_cipher;
            unsigned int key_mgmt_suites;
            unsigned int auth_algs;
            int *basic_rates;
            unsigned char *ssid;
            unsigned char ssid_len;
            unsigned char privacy;
        } bcn_change;

		struct {
			unsigned char alg[TUYA_HOSTAP_CRYPT_ALG_NAME_LEN];
			unsigned int flags;
			unsigned int err;
			unsigned char idx;
			unsigned char seq[8]; /* sequence counter (set: RX, get: TX) */
			unsigned short key_len;
			unsigned char *key;
		} crypt;

		struct {
			unsigned int flags_and;
			unsigned int flags_or;
		} set_flags_sta;

		struct {
			unsigned short rid;
			unsigned short len;
			unsigned char data[0];
		} rid;

		struct {
			unsigned char len;
			unsigned char data[0];
		} generic_elem;

		struct {
#define MLME_STA_DEAUTH 0
#define MLME_STA_DISASSOC 1
			unsigned short cmd;
			unsigned short reason_code;
		} mlme;

		struct {
			struct {
				unsigned char ssid_len;
				unsigned char ssid[SSID_MAX_LEN];
				unsigned char bssid[ETH_ALEN];
			}ssids[SCAN_SSID_MAX];
			unsigned char ssids_num;
			int freqs[14];
		} scan_req;

		struct wpa_scan_results *scan_rst;

		struct {
			unsigned char bssid[ETH_ALEN];
			unsigned char ssid_len;
			unsigned char ssid[SSID_MAX_LEN];
			unsigned short freq;
			unsigned char proto;
			unsigned int key_mgmt_suite;
			unsigned int pairwise_suite;
			unsigned int group_suite;
			unsigned char auth_alg;
			unsigned char mfp;
			unsigned short ie_len;
			unsigned int ie_buf[64];
            unsigned short bcn_len;
            unsigned char bcn_buf[0];
		} assoc_req;

        struct {
            unsigned char bssid[ETH_ALEN];
            unsigned char ssid[SSID_MAX_LEN];
            unsigned char ssid_len;
            unsigned short freq;
            unsigned char auth_alg;
            unsigned short ie_len;
            unsigned char ie[128];
            unsigned char *wep_key[4];
            unsigned int wep_key_len[4];
            int          wep_tx_keyidx;
            unsigned short sae_data_len;
            unsigned char sae_data[0];
        } authen_req;

		struct {
			unsigned int reason;
		} disconnect_req;

		struct {
			unsigned int freq;
			unsigned int csa_cnt;
            struct csa_settings *settings;
		} chan_switch;

		struct {
			FUNC_2PARAM_PTR cb;
			void *arg;
		} reg_csa_event;

		struct {
			unsigned char bssid[ETH_ALEN];
			unsigned char ssid[SSID_MAX_LEN];
		} bss_info;

        struct {
			int state;
		} oper_state;

		struct {
			unsigned short status;
		} external_auth_status;

        struct {
            FUNC_MGMT_FRAME_HOOK_PTR cb;
            void *arg;
        } reg_mgmt_s;
            
        struct {
            FUNC_WLAN_AUTH_EVENT_HOOK_PTR cb;
            void *arg;
        } reg_event_s;

        struct hostapd_freq_params freq;

        struct {
            unsigned short noack;
            unsigned short len;
            unsigned char *buf;
        } send_mlme_s;

        struct {
            char rssi;
            char rssi6;
        } softap_stats_s;

        struct {
            unsigned char ssid_len;
            unsigned char ssid[SSID_MAX_LEN+1];
        } ssid_cfg_s;

        unsigned int value;
        struct tuya_statistic mac_statistic;

        struct {
            unsigned short max_inactivity;
            unsigned char  inactivity_poll;
            unsigned short inactivity_poll_cnt;
        } sta_inact_settings;
	} u;
} TY_WLAN_PARAMS_S;

#define TUYA_HOSTAP_CRYPT_FLAG_SET_TX_KEY          BIT(0)
#define TUYA_HOSTAP_CRYPT_FLAG_PERMANENT           BIT(1)

#define TUYA_HOSTAP_CRYPT_ERR_UNKNOWN_ALG          2
#define TUYA_HOSTAP_CRYPT_ERR_UNKNOWN_ADDR         3
#define TUYA_HOSTAP_CRYPT_ERR_CRYPT_INIT_FAILED    4
#define TUYA_HOSTAP_CRYPT_ERR_KEY_SET_FAILED       5
#define TUYA_HOSTAP_CRYPT_ERR_TX_KEY_SET_FAILED    6
#define TUYA_HOSTAP_CRYPT_ERR_CARD_CONF_FAILED     7

#define TUYA_WPA_DRIVER_FLAGS_SME		           BIT(0)
#endif /* TUYA_HOSTAPD_DRIVER_H */

