/*
 * ush_cmd.c
 * description: it is part of ush
 */

#include <stdio.h> 
#include <string.h>
#include "pub_def.h"
#include "ush_cmd.h"
#include "ushell.h"
#include "BosMem.h"

#ifdef USHELL
/********************************************
* Defines *
********************************************/


/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals * 
********************************************/
const cmd_tbl_t UShCmdTab[] = 
{
    // help command
    {
        "help",
        do_help,
        "help\t\t\t- print online help\n"
    },
    
    //MAC
    {
        "info",
        do_info,
        "info\t\t\t\t- print device info\n"
    },
    
    // add one node to the neighbor table
    {
        "addneighbor",
        do_addneighbor,
        "addneighbor\t\t\t- add one node to the neighbor table\n"
    },
    
    // list all nodes in neighbor table
    {
        "listneighbor",
        do_listneighbor,
        "listneighbor\t\t\t- list all nodes in neighbor table\n"
    },
    
    // delet one node from the neighbor table
    {
        "delneighbor",
        do_delneighbor,
        "delneighbor\t\t\t- delet one node from the neighbor table\n"
    },

//    // list all nodes in the sMAC black table
//    {
//        "listblk",
//        do_listblk,
//        "listblk\t\t\t- list all nodes in the sMAC black table\n"
//    },
//
    //NWK
    // list the sNwk route table
    {
        "listroute",
        do_listroute,
        "listroute\t\t- list the sNwk route table\n"
    },
    
    //APP
    {
        "hello",
        do_hello,
        "hello dst\t\t- say hello to dest address\n"
    },
    
    {
        "reboot",
        do_reboot,
        "reboot\t\t- reboot the system\n"
    },
    
};

cmd_tbl_t  *__ushell_cmd_start;
cmd_tbl_t  *__ushell_cmd_end;

uint16_t cmd_rep_len;
char *cmd_rep_buf;


/********************************************
* Function *
********************************************/
/* 
 * This function will initialize the ushell command table
 */
void ush_system_cmd_tbl_init(void)
{
    __ushell_cmd_start = (cmd_tbl_t  *)&UShCmdTab[0];
    __ushell_cmd_end = (cmd_tbl_t  *)&UShCmdTab[sizeof(UShCmdTab) / sizeof(cmd_tbl_t)];
}

int do_help(cmd_tbl_t * cmdtp, int argc, char *argv[])
{
    int i;
    int rcode=0;
    
    if (argc == 1)  /* show list of commands */
    {
        int cmd_items = __ushell_cmd_end - __ushell_cmd_start;
        cmd_tbl_t **cmd_array;
        int i, j, swaps;
        
        cmd_array = BosMalloc(cmd_items*sizeof(cmd_tbl_t *));
        if (cmd_array == NULL)
            return -1;
        
        /* Make array of commands from .uboot_cmd section */
		cmdtp = __ushell_cmd_start;
		for (i = 0; i < cmd_items; i++)
        {
			cmd_array[i] = cmdtp++;
		}
        
        /* Sort command list (trivial bubble sort) */
		for (i = cmd_items - 1; i > 0; --i)
        {
			swaps = 0;
			for (j = 0; j < i; ++j)
            {
				if (strcmp (cmd_array[j]->name,
					    cmd_array[j + 1]->name) > 0)
                {
					cmd_tbl_t *tmp;
					tmp = cmd_array[j];
					cmd_array[j] = cmd_array[j + 1];
					cmd_array[j + 1] = tmp;
					++swaps;
				}
			}
			if (!swaps)
				break;
		}
        
        /* print short help (usage) */
		for (i = 0; i < cmd_items; i++)
        {
			const char *usage = cmd_array[i]->usage;

			if (usage == NULL)
				continue;
            
			printf("%s", cmd_array[i]->usage);
		}
        
        BosFree(cmd_array);
        
		return 0;
    }
    
    /*
	 * command help (long version)
	 */
    cmd_rep_buf = BosMalloc(128);
    if (cmd_rep_buf == NULL)
        return - BOS_ENOMEM;
    
	for (i = 1; i < argc; ++i)
    {
		if ((cmdtp = find_cmd (argv[i])) != NULL)
        {
			if (cmdtp->usage)
				printf("%s", cmdtp->usage);
		}
        else
        {
			printf ("Unknown command '%s' - try 'help'"
				    " without arguments for list of all"
				    " known commands\n",
                    argv[i]);
            
			rcode = 1;
		}
	}
    BosFree(cmd_rep_buf);
	return rcode;
}

#endif
