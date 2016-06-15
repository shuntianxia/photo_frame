#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Photo_frame

$(NAME)_SOURCES := photo_frame.c \
				   http_download.c \
				   cJSON/cJSON.c \
				   photo_frame_dct.c \
				   utils.h \
				   wifi_setup.c \
				   easy_setup_wiced.c \
				   easy_setup/easy_setup.c \
				   proto/cooee.c \
				   proto/neeze.c \
				   proto/akiss.c \
				   proto/changhong.c
				   
$(NAME)_INCLUDES := ./cJSON ./proto ./easy_setup
                      
GLOBAL_DEFINES :=

WIFI_CONFIG_DCT_H := wifi_config_dct.h

APPLICATION_DCT := photo_frame_dct.c
