#include <string.h>
#include "tuya_tools.h"


#define __TOLOWER(c) ((('A' <= (c))&&((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c))


SIZE_T tuya_strlen(const CHAR_T *str)
{
    return strlen(str);
}

CHAR_T *tuya_strcpy(CHAR_T *dst, const CHAR_T *src)
{
    return strcpy(dst, src);
}

CHAR_T *tuya_strcat(CHAR_T* dst, const CHAR_T* src)
{
    return strcat(dst, src);
}



INT_T tuya_strncasecmp(const CHAR_T *s1, const CHAR_T *s2, size_t n)
{
    const UCHAR_T *p1 = (const UCHAR_T *) s1;
    const UCHAR_T *p2 = (const UCHAR_T *) s2;
    INT_T result = 0;
    INT_T cnt = 0;

    if (p1 == p2) {
        return 0;
    }

    while (((result = __TOLOWER(*p1) - __TOLOWER(*p2)) == 0)) {
        if(++cnt >= n) {
            break;
        }

        if (*p1++ == '\0') {
            result = -1;
            break;
        }

        if(*p2++ == '\0') {
            result = 1;
            break;
        }
    }

    return result;
}

INT_T tuya_strcmp(const CHAR_T *src,const CHAR_T *dst)
{
    INT_T ret = 0;

    while(!(ret  =  *(UCHAR_T *)src - *(UCHAR_T *)dst) && *dst) {
        ++src,++dst;
    }

    if(ret < 0) {
        ret = -1;
    } else if(ret > 0) {
        ret = 1;
    }

    return ret;
}


UCHAR_T tuya_asc2hex(CHAR_T asccode)
{
    UCHAR_T ret;

    if('0'<=asccode && asccode<='9')
        ret=asccode-'0';
    else if('a'<=asccode && asccode<='f')
        ret=asccode-'a'+10;
    else if('A'<=asccode && asccode<='F')
        ret=asccode-'A'+10;
    else
        ret = 0;

    return ret;
}

VOID_T tuya_ascs2hex(UCHAR_T *hex, UCHAR_T *ascs, INT_T srclen)
{
    UCHAR_T l4,h4;
    INT_T i,lenstr;
    lenstr = srclen;

    if(lenstr%2) {
        lenstr -= (lenstr%2);
    }

    if(lenstr==0){
        return;
    }

    for(i=0; i < lenstr; i+=2) {
        h4 = tuya_asc2hex(ascs[i]);
        l4 = tuya_asc2hex(ascs[i+1]);
        hex[i/2]=(h4<<4)+l4;
    }
}

VOID_T tuya_hex2str(UCHAR_T *str, UCHAR_T *hex, INT_T hexlen)
{
    CHAR_T ddl,ddh;
    INT_T i;

    for (i=0; i<hexlen; i++) {
        ddh = 48 + hex[i] / 16;
        ddl = 48 + hex[i] % 16;
        if (ddh > 57) ddh = ddh + 7;
        if (ddl > 57) ddl = ddl + 7;
        str[i*2] = ddh;
        str[i*2+1] = ddl;
    }

    str[hexlen*2] = '\0';
}

BOOL_T tuya_str2num(UINT_T *number, const CHAR_T *str, UINT8_T strlen)
{
    UINT_T value = 0;
    UINT8_T i;

    for (i=0; i<strlen; i++) {
        if(str[i]>='0' && str[i]<='9') {
            value = value*10 + (str[i]-'0');
        }
        else {
            return FALSE;
        }
    }
    *number = value;
    return TRUE;
}

UINT_T tuya_intArray2int(UINT8_T *intArray, UINT_T index, UINT8_T len)
{
    if(index >= len) {
        return (UINT_T)-1;
    }
    
	UINT_T num = 0;
    UINT8_T i = 0;
	for (i = index; i < index+len; i++) {
		num = (num*10) + intArray[i];
	}

	return num;
}

UINT_T tuya_int2intArray(UINT_T num, UINT8_T *intArray, UINT8_T len)
{
    UINT8_T i = 0;
    UINT_T tmp = 0;
    
    tmp = num;
    do {
        tmp /= 10;
        i++;
    } while(tmp != 0);
    
    if(len < i) {
        return 0;
    }
    
    tmp = num;
    for(i = 0; tmp != 0; i++) {
        intArray[i] = tmp % 10;
        tmp /= 10;
    }
    
    tuya_buff_reverse(intArray, i);
    
    return i;
}

VOID_T tuya_buff_reverse(UINT8_T *buf, UINT16_T len)
{
    UINT8_T* p_tmp = buf;
    UINT8_T  tmp;
    UINT16_T i;

    for(i=0; i<len/2; i++) {
        tmp = *(p_tmp+i);
        *(p_tmp+i) = *(p_tmp+len-1-i);
        *(p_tmp+len-1-i) = tmp;
    }
}

VOID_T tuya_data_reverse(UINT8_T *dst, UINT8_T *src, UINT16_T srclen)
{
    UINT16_T i;
    UINT16_T max_len = srclen;

    for(i=0; i<srclen; i++) {
        dst[i] = src[--max_len];
    }
}

// input: str->string
//        index->reverse index,start from 0
//        ch->find CHAR_T
// return: find position
INT_T tuya_find_char_with_reverse_idx(const CHAR_T *str, const INT_T index, const CHAR_T ch)
{
    if(NULL == str) {
        return -1;
    }

    INT_T len = strlen(str);
    if(index >= len) {
        return -1;
    }

    INT_T i = 0;
    for(i = (len-1-index);i >= 0;i--) {
        if(str[i] == ch) {
            return i;
        }
    }

    return -2;
}

VOID_T tuya_byte_sort(UCHAR_T is_ascend, UCHAR_T *buf, INT_T len)
{
    INT_T i,j;
    UCHAR_T tmp = 0;

    for(j = 1;j < len;j++) {
        for(i = 0;i < len-j;i++) {
            if(is_ascend) {
                if(buf[i] > buf[i+1]) {
                    tmp = buf[i];
                    buf[i] = buf[i+1];
                    buf[i+1] = tmp;
                }
            }
            else {
                if(buf[i] < buf[i+1]) {
                    tmp = buf[i];
                    buf[i] = buf[i+1];
                    buf[i+1] = tmp;
                }
            }
        }
    }
}

UINT_T tuya_bit1_count(UINT_T num)
{
    UINT_T count = 0;

    for (count = 0; num; ++count) {
        num &= (num -1); // clear lower bit
    }

    return count;
}

UINT_T tuya_leading_zeros_count(UINT_T num)
{
    UINT_T count = 0;
    UINT_T temp = ~num;

    while(temp & 0x80000000) {
        temp <<= 1;
        count++;
    }

    return count;
}

UINT8_T tuya_check_sum8(UINT8_T *buf, UINT_T len)
{
    UINT8_T sum = 0;
    UINT_T idx=0;
    for(idx=0; idx<len; idx++) {
        sum += buf[idx];
    }
    return sum;
}

UINT16_T tuya_check_sum16(UINT8_T *buf, UINT_T len)
{
    UINT16_T sum = 0;
    UINT_T idx=0;
    for(idx=0; idx<len; idx++) {
        sum += buf[idx];
    }
    return sum;
}



