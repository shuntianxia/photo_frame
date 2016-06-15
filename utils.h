/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

 
/******************************************************
 *                    Constants
 ******************************************************/


/******************************************************
 *                   Enumerations
 ******************************************************/


/******************************************************
 *                 Type Definitions
 ******************************************************/
 

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
static inline int reboot()
{
    WPRINT_APP_INFO( ( "Rebooting...\n" ) );
    host_rtos_delay_milliseconds( 1000 );

    wiced_framework_reboot();

    /* Never reached */
    return 0;
}

static inline void print_ip_address(wiced_ip_address_t *ip_addr)
{
	WPRINT_APP_INFO (("%u.%u.%u.%u\n", (unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >> 24 ) & 0xff ),
										(unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >> 16 ) & 0xff ),
										(unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >>  8 ) & 0xff ),
										(unsigned char) ( ( GET_IPV4_ADDRESS(*ip_addr) >>  0 ) & 0xff )
										 ));
}

#ifdef __cplusplus
} /* extern "C" */
#endif
