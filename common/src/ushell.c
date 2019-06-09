/*
 * ushell.c
 */
#include <stdio.h> 
#include <string.h>
#include "kernel.h"
#include "ushell.h"
#include "board.h"
#include "BosMem.h"
#include "sysTask.h"

#ifdef USHELL
/********************************************
* Defines *
********************************************/
#define USH_READ_LINE_EVENT            0x01
#define USH_RUN_COMMAND_EVENT          0x02


#define USHELL_RECHO
/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals * 
********************************************/
ushell_t _ushell;
ushell_t *ushell = NULL;

char *ush_prompt = "sNet>";

extern cmd_tbl_t  *__ushell_cmd_start;
extern cmd_tbl_t  *__ushell_cmd_end;
/********************************************
* Function defines *
********************************************/
__task void uShellTask(event_t event);

static int parse_line (char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < USH_MAXARGS)
    {
        /* skip any white space */
		while ((*line == ' ') || (*line == '\t'))
        {
			++line;
		}

		if (*line == '\0')
        {	/* end of line, no more args	*/
			argv[nargs] = NULL;

			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t'))
        {
			++line;
		}

		if (*line == '\0')
        {	
            /* end of line, no more args	*/
			argv[nargs] = NULL;

			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	//rt_kprintf ("** Too many args (max. %d) **\n", CFG_MAXARGS);

    return (nargs);
}

cmd_tbl_t *find_cmd (const char *cmd)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = __ushell_cmd_start;	/*Init value */
	const char *p;
	int len;
	int n_found = 0;

	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

	for (cmdtp = __ushell_cmd_start;
	     cmdtp != __ushell_cmd_end;
	     cmdtp++)
    {
		if (strncmp (cmd, cmdtp->name, len) == 0)
        {
			if (len == strlen (cmdtp->name))
				return cmdtp;	/* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
	}
    
	if (n_found == 1)
    {
        /* exactly one match */
		return cmdtp_temp;
	}

	return NULL;	/* not found or ambiguous command */
}

/* 
 * This function will initialize ushell
 */
int ush_system_init(void)
{
    struct uartOpt_t opt;

    ush_system_cmd_tbl_init();

    ushell = &_ushell;
    memset(ushell, 0, sizeof(ushell_t));    
    ushell->uart = CONSOLE_UART;
    
    BosEventSend(TASK_USHELL_ID, USH_READ_LINE_EVENT);

    return 0;
}


/* */
__task void uShellTask(event_t event)
{
    if (event == USH_READ_LINE_EVENT)
    {
        static char *p = NULL;
        static unsigned int n = 0;         /* buffer index		*/
        
        char c;
        
        if (p == NULL)
        {
            //sent prompt
            if (ush_prompt)
            {
                halUartSend(ushell->uart, (uint8_t *)ush_prompt, strlen(ush_prompt));
            }
            
            p = ushell->line;
            n = 0;
        }
        
        if (BOS_EOK != halUartGetChar(ushell->uart, &c))
        {
            BosEventSend(TASK_USHELL_ID, USH_READ_LINE_EVENT);
            return;
        }
        
        if (c == '\r')
        {
            char next;
            uint32_t delay = 0;
            
            while(delay ++ < 1000)
            {            
                if (BOS_EOK == halUartGetChar(ushell->uart, &next))
                {
                    c = next;
                    break;
                }
            }
        }
        
        if ((c == '\r') || (c == '\n'))
        {
            *p = '\0';

#ifdef USHELL_RECHO            
            halUartSend(ushell->uart, "\n", 1);
#endif
            
            BosEventSend(TASK_USHELL_ID, USH_RUN_COMMAND_EVENT);
            p = NULL;
        }
        else if (c == '\0')
            BosEventSend(TASK_USHELL_ID, USH_READ_LINE_EVENT);
        else
        {
            /* be a normal character */
            if (n < USH_CMD_SIZE - 2)
            {
#ifdef USHELL_RECHO 
                halUartPutChar(ushell->uart, c);
#endif
                *p++ = c;
                ++n;
            }
#ifdef USHELL_RECHO
            else
                halUartPutChar(ushell->uart, '\a');
#endif
            
            BosEventSend(TASK_USHELL_ID, USH_READ_LINE_EVENT);
        }   
    }
    else if (event == USH_RUN_COMMAND_EVENT)
    {
        cmd_tbl_t *cmdtp;
        char *token;                    /* start of token in cmdbuf	*/
        char *sep;                      /* end of token (separator) in cmdbuf */
        char *str;
        char *argv[USH_MAXARGS + 1];    /* NULL terminated	*/
        int argc, inquotes;

        str = (char *)ushell->line;
        
        while(*str)
        {
            /*
             * Find separator, or string end
             * Allow simple escape of ';' by writing "\;"
             */
            for (inquotes = 0, sep = str; *sep; sep++)
            {
                if ((*sep=='\'') &&
                    (*(sep-1) != '\\'))
                    inquotes=!inquotes;

                if (!inquotes &&
                    (*sep == ';') &&	/* separator		*/
                    ( sep != str) &&	/* past string start	*/
                    (*(sep-1) != '\\'))	/* and NOT escaped	*/
                    break;
            }
            
            /*
             * Limit the token to data between separators
             */
            token = str;
            if (*sep)
            {
                str = sep + 1;	/* start of command for next pass */
                *sep = '\0';
            }
            else
                str = sep;	/* no more commands for next pass */
            
            /* Extract arguments */
            if ((argc = parse_line (token, argv)) == 0)
            {
                continue;
            }
        
            /* Look up command in command table */
            if ((cmdtp = find_cmd(argv[0])) == NULL)
            {
                int len;
                char *tmpbuf = BosMalloc(128);
                
                if (tmpbuf != NULL)
                {    
                    len=sprintf(tmpbuf,
                                "Unknown command '%s' - try 'help'\n",
                                argv[0]);
                    halUartSend(ushell->uart, (uint8_t *)tmpbuf, len);
                    
                    BosFree(tmpbuf);
                }
                
                continue;
            }
     
            /* OK - call function to do the command */
            (cmdtp->cmd)(cmdtp, argc, argv);
            
        }
  
        BosEventSend(TASK_USHELL_ID, USH_READ_LINE_EVENT);
    }
}


#endif

