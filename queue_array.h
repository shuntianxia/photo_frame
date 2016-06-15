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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#define MAX_BUF_SIZE 4*1024
#define UART_BUFF_SIZE 5*1024

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/


/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct uart_transport_s{
	int len;
	char data[MAX_BUF_SIZE];
}uart_transport_t;

typedef uart_transport_t q_elem_t;

typedef struct queue{
	int size;
	int maxsize;
	int front;
	int rear;
	q_elem_t *array;
}queue_t;
/******************************************************
 *                    Structures
 ******************************************************/



/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif

