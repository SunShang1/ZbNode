/*
 * ush_cmd.h
 * head file for ushell
 */

#ifndef __USH_CMD_H__
#define __USH_CMD_H__

/********************************************
* Defines *
********************************************/
#define USH_CMD_SIZE        50
#define USH_MAXARGS         16


#define ATOH(c)                        ((c>='a')?(c-'a'+10):((c>='A')?(c-'A'+10):(c-'0')))
/********************************************
* Typedefs *
********************************************/
struct cmd_tbl_s
{
    const char * const name;      /* Command Name			*/
    int	    (*cmd)(struct cmd_tbl_s *, int, char *[]);  /*  handle */
    const char * const usage;	    /* Usage message	(short)	*/
};

typedef struct cmd_tbl_s	cmd_tbl_t;


/********************************************
* Function *
********************************************/
void ush_system_cmd_tbl_init(void);

int do_help(cmd_tbl_t * cmdtp, int argc, char *argv[]);

int do_addneighbor(cmd_tbl_t * cmdtp, int argc, char *argv[]);
int do_listneighbor(cmd_tbl_t * cmdtp, int argc, char *argv[]);
int do_delneighbor(cmd_tbl_t * cmdtp, int argc, char *argv[]);

int do_listroute(cmd_tbl_t * cmdtp, int argc, char *argv[]);

int do_info(cmd_tbl_t * cmdtp, int argc, char *argv[]);

int do_hello(cmd_tbl_t * cmdtp, int argc, char *argv[]);

int do_reboot(cmd_tbl_t * cmdtp, int argc, char *argv[]);

#endif
