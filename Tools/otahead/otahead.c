#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __WIN32__
#include <netinet/in.h>		/* for host / network byte order conversions	*/
#include <sys/mman.h>
#else
#include <windows.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __WIN32__
typedef unsigned int __u32;
typedef unsigned short __u16;

#define SWAP_SHORT(x) \
    ((__u16)( \
        (((__u16)(x) & (__u16)0x00ff) << 8) | \
        (((__u16)(x) & (__u16)0xff00) >> 8) ))

#define SWAP_LONG(x) \
    ((__u32)( \
        (((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
        (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
        (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
        (((__u32)(x) & (__u32)0xff000000UL) >> 24) ))

#define     ntohs(a)    SWAP_SHORT(a)
#define     htons(a)    SWAP_SHORT(a)

#define     ntohl(a)    SWAP_LONG(a)
#define     htonl(a)    SWAP_LONG(a)
#endif  /* __WIN32__ */

typedef struct ota_header {
	uint16_t    icrc;
    uint16_t    isize;
} image_header_t;

#define SIM_CODE_HEADER_LEN     26
#define SIM_CODE_TAIL_LEN       26

char *cmdname=NULL;

image_header_t header;
image_header_t *hdr = &header;

// Table of CRC constants - implements x^16+x^12+x^5+1
static const uint16_t crc16_tab[] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49, 
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067, 
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0, 
};

/* CRC16_CCITT */
uint16_t crc16(uint16_t crc, const unsigned char *buf, int len)
{
    int i;
    uint16_t cksum = crc;

    for (i = 0;  i < len;  i++)
    {
        cksum = crc16_tab[((cksum>>8) ^ *buf++) & 0xFF] ^ (cksum << 8);
    }

    return cksum;
}
 
void usage (void)
{
	fprintf (stderr, "Usage:\n"
			         "    %s zbnode.sim\n", cmdname);

	exit (EXIT_FAILURE);
}

int main (int argc, char **argv)
{
    
#ifndef __WIN32__
    int imgfd;
    int simfd;
#else
    HANDLE imghandle, imgmap;
    HANDLE simhandle, simmap;
    DWORD writeSize;
#endif
    char *zb_sim_file;
    char zb_img_file[64]={0};
    unsigned char *ptr;
    unsigned char *tail;
    struct stat sbuf;
    uint32_t actsimsize;
    uint16_t isize;
    uint16_t icrc;
    
    cmdname = *argv++;

    if (argc != 2)
        usage();
 
    zb_sim_file = *argv;
    if ( (zb_sim_file[strlen(zb_sim_file)-4] != '.') ||
         (strcmp(&zb_sim_file[strlen(zb_sim_file)-3], "sim") != 0) )
    {
        fprintf (stderr, "Error: command needs a simple-code file\n");
        exit (EXIT_FAILURE);
    }
    
    strncpy(zb_img_file, zb_sim_file, strlen(zb_sim_file)-3);
    strcat(zb_img_file, "img");

    /*
     * Open simple-code file.
     */
#ifndef __WIN32__ 
    simfd = open(zb_sim_file, O_RDONLY|O_BINARY);
    if (simfd < 0)
#else
    simhandle = CreateFile(zb_sim_file, GENERIC_READ,0,0,OPEN_EXISTING,0,0);
    if (simhandle == INVALID_HANDLE_VALUE)
#endif
    {
		fprintf (stderr, "%s: Can't open %s: %s\n",
			cmdname, zb_sim_file, strerror(errno));
		exit (EXIT_FAILURE);
	}
	
	/*
     * Stat simple-code file.
     */
#ifndef __WIN32__ 
    if (fstat(simfd, &sbuf) < 0)
#else
    if (stat(zb_sim_file, &sbuf) < 0)
#endif
    {
        fprintf (stderr, "%s: Can't stat %s: %s\n",
            cmdname, zb_sim_file, strerror(errno));
#ifndef __WIN32__
        close(simfd);
#else
        CloseHandle(simhandle);
#endif
        exit (EXIT_FAILURE);
    }
    
    if ((unsigned)sbuf.st_size < SIM_CODE_HEADER_LEN + SIM_CODE_TAIL_LEN)
    {
        fprintf (stderr,
            "%s: Bad size: \"%s\" is no valid simle code\n",
            cmdname, zb_sim_file);
#ifndef __WIN32__
        close(simfd);
#else
        CloseHandle(simhandle);
#endif
        exit (EXIT_FAILURE);
    }
    
    /*
     * Map simple-code file.
     */
#ifndef __WIN32__
    ptr = (unsigned char *)mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, simfd, 0);

    if ((caddr_t)ptr == (caddr_t)-1)
    {
        fprintf (stderr, "%s: Can't read %s: %s\n",
            cmdname, zb_sim_file, strerror(errno));
        close(simfd);
        exit (EXIT_FAILURE);
    }
#else
    simmap=CreateFileMapping(simhandle,0,PAGE_READONLY|SEC_COMMIT,0,0,0);

    if (!simmap) 
    {
        fprintf (stderr, "%s: Can't creat map file for %s: %s\n",
            cmdname, zb_sim_file, strerror(errno));
        CloseHandle(simhandle);
        exit (EXIT_FAILURE);
    }

    ptr = MapViewOfFile(simmap,FILE_MAP_READ,0,0,0);
    if (ptr == NULL)
    {
        fprintf (stderr, "%s: Can't map %s: %s\n",
            cmdname, zb_sim_file, strerror(errno));
        CloseHandle(simmap);
        CloseHandle(simhandle);
        exit (EXIT_FAILURE);
    }
#endif	
    
    tail = ptr + ((unsigned)sbuf.st_size - 1 - SIM_CODE_TAIL_LEN);
    
    while( (*tail == 0xFF) && (tail > ptr + SIM_CODE_HEADER_LEN) )
        tail --;
    
    actsimsize = tail + 1 - ptr - SIM_CODE_HEADER_LEN;
    actsimsize = (actsimsize + 3) & 0xFFFFFFFC;
    
    memset (hdr, 0, sizeof(image_header_t));
    isize=((htons(actsimsize/4)>>8)&0x00FF) + ((htons(actsimsize/4)<<8)&0xFF00);
    icrc=crc16(0, ptr + SIM_CODE_HEADER_LEN, actsimsize);
    icrc=((htons(icrc)>>8)&0x00FF) + ((htons(icrc)<<8)&0xFF00);
    hdr->isize = isize;
    hdr->icrc = icrc;

    /*
     * Open image file.
     */
#ifndef __WIN32__ 
	imgfd = open(zb_img_file, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0666);

	if (imgfd < 0)
	{
		fprintf (stderr, "%s: Can't open %s: %s\n",
			cmdname, zb_img_file, strerror(errno));
        
        close(simfd);
        
		exit (EXIT_FAILURE);
	}
#else
    imghandle = CreateFile(zb_img_file, GENERIC_READ|GENERIC_WRITE,0,0,OPEN_ALWAYS,0,0);

    if (imghandle == INVALID_HANDLE_VALUE)
    {
		fprintf (stderr, "%s: Can't open %s: %s\n",
			cmdname, zb_img_file, strerror(errno));
        CloseHandle(simmap);
        CloseHandle(simhandle);
		exit (EXIT_FAILURE);
	}
#endif

    /*
     * wirte image header to image file
	 */

#ifndef __WIN32__
	if (write(imgfd, hdr, sizeof(image_header_t)) != sizeof(image_header_t))
#else
    WriteFile(imghandle,hdr,sizeof(image_header_t),&writeSize,0);
    if (writeSize != sizeof(image_header_t))
#endif
    {
		fprintf (stderr, "%s: Write error on  %s: %s\n",
			cmdname, zb_img_file, strerror(errno));
#ifndef __WIN32__
        close(simfd);
        close(imgfd);
#else
        CloseHandle(simmap);
        CloseHandle(simhandle);
        CloseHandle(imghandle);
#endif
		exit (EXIT_FAILURE);
	}

    /* copy simple code to image file */	
#ifndef __WIN32__
	if (write(imgfd, ptr + SIM_CODE_HEADER_LEN, actsimsize) != actsimsize)
#else
    WriteFile(imghandle,ptr+SIM_CODE_HEADER_LEN,actsimsize,&writeSize,0);
    if (writeSize != actsimsize)
#endif
    {
		fprintf (stderr, "%s: Write error on  %s: %s\n",
			cmdname, zb_img_file, strerror(errno));
#ifndef __WIN32__
        close(simfd);
        close(imgfd);
#else
        CloseHandle(simmap);
        CloseHandle(simhandle);
        CloseHandle(imghandle);
#endif
		exit (EXIT_FAILURE);
	}

#ifndef __WIN32__
    (void) munmap((void *)ptr, sbuf.st_size);
    close(simfd);
#else
    UnmapViewOfFile(ptr); 
    CloseHandle(simmap);
    CloseHandle(simhandle);
#endif    

	/* wait for writing */
#ifndef __WIN32__
#if defined(_POSIX_SYNCHRONIZED_IO) && !defined(__sun__) && !defined(__FreeBSD__)
	(void) fdatasync (imgfd);
#else
	(void) fsync (imgfd);
#endif
#else
    FlushFileBuffers(imghandle);
#endif 
   	
	/*
     * Stat image file.
     */
#ifndef __WIN32__ 
    if (fstat(imgfd, &sbuf) < 0)
#else
    if (stat(zb_img_file, &sbuf) < 0)
#endif
    {
        fprintf (stderr, "%s: Can't stat %s: %s\n",
            cmdname, zb_img_file, strerror(errno));
#ifndef __WIN32__
        close(imgfd);
#else
        CloseHandle(imghandle);
#endif
        exit (EXIT_FAILURE);
    }
    
    /*
     * Map image file.
     */
#ifndef __WIN32__
    ptr = (unsigned char *)mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, imgfd, 0);

    if ((caddr_t)ptr == (caddr_t)-1)
    {
        fprintf (stderr, "%s: Can't read %s: %s\n",
            cmdname, zb_img_file, strerror(errno));
        close(imgfd);
        exit (EXIT_FAILURE);
    }
#else
    imgmap=CreateFileMapping(imghandle,0,PAGE_READONLY|SEC_COMMIT,0,0,0);

    if (!imgmap) 
    {
        fprintf (stderr, "%s: Can't creat map file for %s: %s\n",
            cmdname, zb_img_file, strerror(errno));
        CloseHandle(imghandle);
        exit (EXIT_FAILURE);
    }

    ptr = MapViewOfFile(imgmap,FILE_MAP_READ,0,0,0);
    if (ptr == NULL)
    {
        fprintf (stderr, "%s: Can't map %s: %s\n",
            cmdname, zb_img_file, strerror(errno));
        CloseHandle(imgmap);
        CloseHandle(imghandle);
        exit (EXIT_FAILURE);
    }
#endif
 
#ifndef __WIN32__
    (void) munmap((void *)ptr, sbuf.st_size);
#else
    UnmapViewOfFile(ptr);
#endif
   
	/* wait for writing */
#ifndef __WIN32__
#if defined(_POSIX_SYNCHRONIZED_IO) && !defined(__sun__) && !defined(__FreeBSD__)
	(void) fdatasync (imgfd);
#else
	(void) fsync (imgfd);
#endif
#else
    FlushFileBuffers(imghandle);
#endif 
    
#ifndef __WIN32__
    close(imgfd);
#else
    CloseHandle(imgmap);
    CloseHandle(imghandle);
#endif

    return 0;
  
}
