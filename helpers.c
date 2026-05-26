/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
#define F_CPU 	14745600
#endif

#include <stdlib.h>


/************************************************************************************
 *                                   Helper function prototypes                     *
 ************************************************************************************/

//-----------------------------------------------------------------------------
//       strstr
//-----------------------------------------------------------------------------
// This function is the standard string search. The library
// does not provide it.  It looks for one string in another string
// and returns a pointer to it if found, otherwise returns NULL. 
//-----------------------------------------------------------------------------
char * strstr(char * haystack, char * needle)
{
char *ptr1, *ptr2;


//printf("Entered strstr routine\n");

// Protect against NULL pointer
if (*needle == 0) return(haystack);
for( ; *haystack; haystack++ )
  {
  // Look for needle in haystack.  If there is a
  // match then this will continue all the way
  // until ptr1 reaches the NULL at the end of needle 
        for(ptr1 = needle, ptr2 = haystack; *ptr1 && (*ptr1 == *ptr2); ++ptr1, ++ptr2);
                            
        // If there is a match then return pointer to needle in haystack
        if(*ptr1 == 0) return(haystack);    
     }

//printf("No match found!\n");

return NULL;            // no matching string found
}
