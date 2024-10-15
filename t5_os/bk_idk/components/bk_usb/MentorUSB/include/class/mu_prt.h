/*****************************************************************************
 *                                                                           *
 *      Copyright Mentor Graphics Corporation 2006                           *
 *                                                                           *
 *                All Rights Reserved.                                       *
 *                                                                           *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION            *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS              *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                               *
 *                                                                           *
 ****************************************************************************/

/*
 * MUSB-MicroSW mass-storage definitions.
 * $Revision: 1.5 $
 */

#ifndef __MUSB_PRT_H__
#define __MUSB_PRT_H__

/* bInterfaceProtocol */
#define MGC_PRT_UNDEFINED_PROTOCOL           0x00
#define MGC_PRT_UNI_DIR_PROTOCOL           0x01
#define MGC_PRT_BI_DIR_PROTOCOL           0x02
#define MGC_PRT_BI_1284_PROTOCOL           0x03
#define MGC_PRT_VENDOR_SPECIFIC_PROTOCOL           0xFF

/* bInterfaceSubClass */
#define MGC_PRT_PRT_SUBCLASS           0x01

#endif	/* multiple inclusion protection */
