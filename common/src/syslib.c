/*
 * Copyrite (C) 2019, Lamp
 *
 * 文件名称：syslib.c
 *
 * 文件说明：常用函数实现库
 *
 */

#include "pub_def.h"
#include "syslib.h"
   
/********************************************
* Defines *
********************************************/




/********************************************
* Typedefs *
********************************************/



/********************************************
* Globals *
********************************************/ 



/********************************************
* Function defines *
********************************************/
char *itoa(int val, char *str, int radix)
{
    char a[20];
    char *p;
    uint8_t u8tmp;
    
    int i = 0;
    
    if (val < 0)
        return NULL;
    
    if ( (radix != 10) && (radix != 16))
        return NULL;
      
    do{
        u8tmp = val%radix;
        
        a[i ++] = I2A(u8tmp);
        val /= radix;
    }while(val > 0);
    
    p = str;
    while(i > 0)
    {
        *p ++ = a[i-1];
        
        i --;
    }
    
    *p = '\0';
    
    return str;
    
}

int atoi(const char *str)
{
    int val = 0;
    
    int flag = 0;
    
    if (str == NULL)
        return 0;
    
    while(*str == ' ')
        str ++;
    
    if ( (*str == '-') || (*str == '+') )
    {
        if (*str == '-')
            flag = 1;
        
        str ++;
    }
    
    while ( (*str >= '0') && (*str <= '9') )
    {
        val = val*10;
        val += (*str - '0');
        
        str++;
    }
    
    return val*(flag?-1:1);
}

int htoi(const char *str)
{
    int val = 0;
    
    if (str == NULL)
        return 0;
    
    if ( (str[0] == '0') &&
         ( (str[1] == 'x') || (str[1] == 'X') ) )
        str += 2;

    while (1)
    {
        if( (*str >= '0') && (*str <= '9') )
        {
            val = val*16;
            val += (*str - '0');
        }
        else if( (*str >= 'A') && (*str <= 'F') )
        {
            val = val*16;
            val += (*str + 10 - 'A');
        }
        else if( (*str >= 'a') && (*str <= 'f') )
        {
            val = val*16;
            val += (*str + 10 - 'a');
        }
        else
            break;
    }

    return val;
}

