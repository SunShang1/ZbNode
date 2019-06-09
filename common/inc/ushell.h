/*
 * ushell.h
 * head file for ushell.c
 */

#ifndef __USHELL_H__
#define __USHELL_H__

#include "ush_cmd.h"


typedef struct
{
	char line[USH_CMD_SIZE];

	int uart;
    
} ushell_t;

void ush_set_device(const char* device_name);
int ush_system_init(void);
cmd_tbl_t *find_cmd (const char *cmd);   
    
    
#endif
