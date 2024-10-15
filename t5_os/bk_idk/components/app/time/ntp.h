#ifndef _NTP_H_
#define _NTP_H_

/**
 * Get the UTC time from NTP server
 *
 * @note this function is not reentrant
 *
 * @return >0: success, current UTC time
 *         =0: get failed
 */
time_t ntp_get_time(char *host, uint32_t *frag_val);

/**
 * Get the local time from NTP server
 *
 * @return >0: success, current local time, offset timezone by NTP_TIMEZONE
 *         =0: get failed
 */
time_t ntp_get_local_time(uint32_t *frag_val);

/**
 * Sync current local time to RTC by NTP
 *
 * @return >0: success, current local time, offset timezone by NTP_TIMEZONE
 *         =0: sync failed
 */
time_t ntp_sync_to_rtc(void);

#endif /* _NTP_H_ */
