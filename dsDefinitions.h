#ifndef _DS_DISPLAY_H_
#define _DS_DISPLAY_H_


#define INVALID_HANDLE 0
#define ISNULL(ptr) (ptr==NULL)
#define _RETURN_IF_ERROR(cond, err) do {\
    				if (!(cond)) { \
        			 printf("ERROR at [%s][%d]\r\n", __FILE__, __LINE__);\
        			 return (err); \
    				 } \
				}while(0);
typedef struct _dsDisplayParam_t
{
        dsDisplayEDID_t edidValue;
        dsVideoAspectRatio_t aspectRatio;
        dsVideoPortPortId_t portId;
}dsDisplayParam_t;


dsDisplayParam_t *dsVideoDisplayHandler = '\0';

dsError_t dsDisplayHdlCheck(int handle);

size_t videoDisplayMaxIndex = 0;

#endif


