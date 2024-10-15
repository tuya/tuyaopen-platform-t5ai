/*
 *==========================================================================
 *
 *      crc.h
 *
 *      Interface for the CRC algorithms.
 *
 *==========================================================================
 *####ECOSGPLCOPYRIGHTBEGIN####
 * -------------------------------------------
 * This file is part of eCos, the Embedded Configurable Operating System.
 * Copyright (C) 2002 Andrew Lunn
 *
 * eCos is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 or (at your option) any later version.
 *
 * eCos is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with eCos; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * As a special exception, if other files instantiate templates or use macros
 * or inline functions from this file, or you compile this file and link it
 * with other works to produce a work based on this file, this file does not
 * by itself cause the resulting work to be covered by the GNU General Public
 * License. However the source code for this file must still be made available
 * in accordance with section (3) of the GNU General Public License.
 *
 * This exception does not invalidate any other reasons why a work based on
 * this file might be covered by the GNU General Public License.
 *
 * Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
 * at http: *sources.redhat.com/ecos/ecos-license/
 * -------------------------------------------
 *####ECOSGPLCOPYRIGHTEND####
 *==========================================================================
 *#####DESCRIPTIONBEGIN####
 *
 * Author(s):    Andrew Lunn
 * Contributors: Andrew Lunn
 * Date:         2002-08-06
 * Purpose:
 * Description:
 *
 * This code is part of eCos (tm).
 *
 *####DESCRIPTIONEND####
 *
 *==========================================================================
 */

#ifndef _SERVICES_CRC_CRC_H_
#define _SERVICES_CRC_CRC_H_

/******************CRC-16/XMODEM       x16+x12+x5+1******************************

*******************************************************************************/

/**
 * @brief             caculate the CRC16 result for each byte
 *
 * @param crcIn       The context to save CRC16 result.
 * @param byte        each byte of input data.
 *
 * @retval            return the CRC16 result
 */


typedef struct {
    u16 crc;
} CRC16_Context;

/**
 * @brief             initialize the CRC16 Context
 *
 * @param inContext   holds CRC16 result
 *
 * @retval            none
 */
void CRC16_Init( CRC16_Context *inContext );


/**
 * @brief             Caculate the CRC16 result
 *
 * @param inContext   holds CRC16 result during caculation process
 * @param inSrc       input data
 * @param inLen       length of input data
 *
 * @retval            none
 */
void CRC16_Update( CRC16_Context *inContext, const void *inSrc, size_t inLen );


/**
 * @brief             output CRC16 result
 *
 * @param inContext   holds CRC16 result
 * @param outResutl   holds CRC16 final result
 *
 * @retval            none
 */
void CRC16_Final( CRC16_Context *inContext, u16 *outResult );

/**
  * @}
  */

/**
  * @}
  */

#endif /* _SERVICES_CRC_CRC_H_ */
