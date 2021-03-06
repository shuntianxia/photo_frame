/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "photo_frame_dct.h"
#include "wiced_framework.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/
//static smart_strip_app_dct_t app_dct_local;

DEFINE_APP_DCT(photo_frame_app_dct_t)
{
	.work_mode = 0
};

/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t load_app_data(photo_frame_app_dct_t* app_dct)
{
	photo_frame_app_dct_t* dct_app;
	if(	wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( *dct_app ) ) != WICED_SUCCESS)
	{
        return WICED_ERROR;
    }
	*app_dct = *dct_app;
	wiced_dct_read_unlock( dct_app, WICED_TRUE );
	return WICED_SUCCESS;
}

void store_app_data(photo_frame_app_dct_t* app_dct)
{
	wiced_dct_write( (const void*) app_dct, DCT_APP_SECTION, 0, sizeof(photo_frame_app_dct_t) );
}

wiced_result_t load_wifi_data(platform_dct_wifi_config_t* wifi_dct)
{
	platform_dct_wifi_config_t* dct_wifi_config;
	/* get the wi-fi config section for modifying, any memory allocation required would be done inside wiced_dct_read_lock() */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof( *dct_wifi_config ) );

    *wifi_dct = *dct_wifi_config;

    /* release the read lock */
    wiced_dct_read_unlock( dct_wifi_config, WICED_TRUE );
    return WICED_SUCCESS;
}

void store_wifi_data(platform_dct_wifi_config_t* wifi_dct)
{
	wiced_dct_write( (const void*) wifi_dct, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
}

