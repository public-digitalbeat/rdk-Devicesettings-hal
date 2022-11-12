#include <stdlib.h>
#include "dsVideoDevice.h"
#include <string.h>
#include "dsError.h"
#include "dsTypes.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>


#define dsVIDEOPORT_INDEX 0
#define	SUCCESS	0
#define	FAILURE	-1
#define BUFF_SIZE 1024
#define BYTE 1
#define SIZE 50

typedef struct _VDEVICEHandle_t {
        int index;
        int DeviceHandle;
	char file_name[SIZE];
}VDEVICEHandle_t;


typedef enum _dsVideodev{
 HDMI_DEV,
 VDevice_MAX
}dsVideodev_t;

enum _screen_modes{
        VIDEO_WIDEOPTION_NORMAL = 0,
        VIDEO_WIDEOPTION_FULL_STRETCH = 1,
        VIDEO_WIDEOPTION_4_3 = 2,
        VIDEO_WIDEOPTION_16_9 = 3,
        VIDEO_WIDEOPTION_NONLINEAR = 4,
        VIDEO_WIDEOPTION_NORMAL_NOSCALEUP = 5,
        VIDEO_WIDEOPTION_4_3_IGNORE = 6,
        VIDEO_WIDEOPTION_4_3_LETTER_BOX = 7,
        VIDEO_WIDEOPTION_4_3_PAN_SCAN = 8,
        VIDEO_WIDEOPTION_4_3_COMBINED = 9,
        VIDEO_WIDEOPTION_16_9_IGNORE = 10,
        VIDEO_WIDEOPTION_16_9_LETTER_BOX = 11,
        VIDEO_WIDEOPTION_16_9_PAN_SCAN = 12,
        VIDEO_WIDEOPTION_16_9_COMBINED = 13,
        VIDEO_WIDEOPTION_MAX = 14

};

static VDEVICEHandle_t _handles[VDevice_MAX]={};
static VDEVICEHandle_t _dsFile;

static const char *dsVideoCodecsCmd = "lsmod | grep \"^amvdec_.*v4l\\b\" | "
		"awk '{ print $1 }' | sed -e 's/amvdec_\\(.*\\)_v4l/\\1/'";

static const char *dsGetHDRCapabilitiesCmd = "find /lib/modules/ -name *.ko | grep dovi.ko";


/**
 * @brief Initialize video device module
 * 
 * This function must initialize all the video devices that exists in the system
 *
 * @param None
 * @return dsError_t Error Code.
 */
dsError_t dsVideoDeviceInit(){
	
	printf("Entering into DeviceInit() in hal\n");
	int i = 0;
	sprintf(_dsFile.file_name,"%s","/sys/class/amhdmitx/amhdmitx0/hdr_cap");
	printf("file name in DSvideoinit():%s\n",_dsFile.file_name);
	_handles[HDMI_DEV].index = dsVIDEOPORT_INDEX;
	
	for(i =0;i<VDevice_MAX;i++)
	{
                if(i==HDMI_DEV)
		{	
			//device file opening for ioctl calls
			if(FAILURE!=(_handles[HDMI_DEV].DeviceHandle = open("/dev/amvideo",O_RDWR))){
	      		printf("/dev/amhdmitx0 file opened successfully\n");
			return dsERR_NONE;
			}
			else    return dsERR_GENERAL;
		}
			
	}
    return dsERR_NONE;
}



/*
 * @brief To get the handle of the video display device 
 * 
 * This function is used to get the  handle for corresponding video device based on index.
 * @param [in] index     The index of the video device file (0, 1, ...)
 * @param [out] *handle  The handle of video display device 
 * @return dsError_t Error code.
 */
dsError_t dsGetVideoDevice(int index, int *handle){

	printf("Entering into dsGetVideoDevice()\n");
	dsError_t ret;
	if (VDevice_MAX< index && handle!=NULL)	
		ret = dsERR_INVALID_PARAM;
	else 
	{
		switch(index) {
        	      	case 0: 
				printf("Index value in switch case:%d\n",index);
				*handle = (int)&_handles[index];
				printf("Got handle for HDMI device DFC file successfully\n");
				ret = dsERR_NONE;
            			break;
           		default:            
            			printf( "operation not supported!\n" );
				ret = dsERR_OPERATION_NOT_SUPPORTED;
				break;
            		
		}
		
	}
   	return ret;
}



/**
 * @brief Get the screen display mode
 * [p
 * This API is used to get the DFC[Decoder format convention used for zoom purpose] setting by its property name.
 *
 * @param [in] handle  Handle for the video device
 * @param [in] *dfc     Type of display mode to be used
 * @return dsError_t Error code.
 */

dsError_t dsGetDFC (int handle, dsVideoZoom_t *dfc){
	
	//stubbed as amstream not applicable for v4l2 
    dsError_t ret = dsERR_NONE;
	return ret;
}



/**
 * @brief Set the screen display mode
 * 
 * This API is used to set the DFC[Decoder format convention used for zoom purpose] setting by its property name.
 *
 * @param [in] handle  Handle for the video device
 * @param [in] dfc     Type of display mode to be used
 * @return dsError_t Error code.
 */

dsError_t dsSetDFC (int handle, dsVideoZoom_t dfc){
	//stubbed as amstream not applicable for v4l2 
    dsError_t ret = dsERR_NONE;
	return ret;
	
}

dsError_t dsGetHDRCapabilities(int handle,int *capabilities){
#if 1
    (void) handle;
    int outVar = dsHDRSTANDARD_NONE;
    dsError_t ret = dsERR_NONE;
    int sys_ret = -1;

    if (capabilities != NULL)
    {
        sys_ret = system(dsGetHDRCapabilitiesCmd);

        if (sys_ret == 0) //dovi.ko file found
        {
            outVar |= dsHDRSTANDARD_HDR10;
            outVar |= dsHDRSTANDARD_HLG;
            outVar |= dsHDRSTANDARD_DolbyVision;
        }
        else //dovi.ko file not found
        {
            outVar |= dsHDRSTANDARD_HDR10;
            outVar |= dsHDRSTANDARD_HLG;
        }

        //set capabilities
        *capabilities = outVar;
    }
    else
    {
        ret = dsERR_INVALID_PARAM;
    }

    printf(" DS_HAL: %s dsGetHDRCapabilities capabilities %d\n",__func__, *capabilities);
    return ret;

#else
       	printf("Entering into dsGetHDRCapabilities\n");
	int SMPTEST2084_Value;
        int out_var = dsHDRSTANDARD_NONE;
        char bin_val[BYTE + 1];
        char *Fetch_ptr;
        char strHdr[BUFF_SIZE] = {0};
	dsError_t ret;
        int d;
	
       	VDEVICEHandle_t *vDevHandle = (VDEVICEHandle_t *) handle;
	if(NULL != vDevHandle && capabilities!= NULL){
		printf("file name:%s\n",_dsFile.file_name);
        	d = amsysfs_get_sysfs_str(_dsFile.file_name,strHdr,sizeof(strHdr));
		printf("d:%d\n",d);
        	printf("/sys/devices/virtual/amhdmitx/amhdmitx0/hdr_cap file opened successfully\n");
        }
	else return dsERR_INVALID_PARAM;
	
       
	printf("strHdr for after all time:%s\n",strHdr);
       	
	Fetch_ptr = strstr(strHdr,"SMPTE ST 2084");
        Fetch_ptr = Fetch_ptr+15;
        memcpy(bin_val,Fetch_ptr,BYTE);
        bin_val[1]='\0';
        SMPTEST2084_Value = atoi(bin_val);
	printf("Binary value of SMPTEST2084 is:%d\n",SMPTEST2084_Value);
        
	if(SMPTEST2084_Value ==1)
        {
                printf("out_var:%d\n",out_var);
                out_var = out_var | dsHDRSTANDARD_HDR10;
                printf("out_var:%d\n",out_var);
		*capabilities = out_var;
		ret = dsERR_NONE;
		
        }
        else
        {
                printf("SMPTE ST 2048 having zero binary type\n");
		printf("out_var:%d\n",out_var);
		*capabilities = out_var;
		ret = dsERR_NONE;
        }
	
        

	return ret;

#endif
}

/**
 * @brief Terminate the usage of video Device module
 * 
 * This function will reset the data structs used within this module and release the video display specific handles
 *
 * @param None
 * @return dsError_t Error code.
 */

dsError_t dsVideoDeviceTerm ()
{
	printf("entering into dsVideoDeviceTerm()\n");
   	
	if(SUCCESS==close(_handles[HDMI_DEV].DeviceHandle))	return dsERR_NONE;
	else	return dsERR_GENERAL;

}	

dsError_t dsGetSupportedVideoCodingFormats (int handle, unsigned int *supported_formats)
{
	(void) handle;
	FILE *fd = NULL;
	char buf[64];

	printf("entering into dsGetSupportedVideoCodingFormats\n");
	if (NULL == (fd = popen(dsVideoCodecsCmd, "r")))
	{
		return dsERR_GENERAL;
	}

	while (fgets(buf, sizeof(buf), fd) != NULL)
	{
		if (strcasestr(buf, "mpeg12") != NULL)
		{
			*supported_formats |= dsVIDEO_CODEC_MPEG2;
		}
		else if (strcasestr(buf, "mpeg4") != NULL)
		{
			*supported_formats |= dsVIDEO_CODEC_MPEG4PART10;
		}
		else if (strcasestr(buf, "h265") != NULL)
		{
			*supported_formats |= dsVIDEO_CODEC_MPEGHPART2;
		}
	}
	pclose(fd);

	return dsERR_NONE;
}

dsError_t dsGetVideoCodecInfo (int handle, dsVideoCodingFormat_t codec, dsVideoCodecInfo_t *info)
{
	(void) handle;
	printf("entering into dsGetVideoCodecInfo() %d \n", codec);
	if (codec == dsVIDEO_CODEC_MPEGHPART2)
	{
		info->num_entries = 2;
		info->entries[0].profile = dsVIDEO_CODEC_HEVC_PROFILE_MAIN;
		info->entries[0].level = 5.1;
		info->entries[1].profile = dsVIDEO_CODEC_HEVC_PROFILE_MAIN10;
		info->entries[1].level = 5.1;
	}
	else
	{
		info->num_entries = 0;
	}

	return dsERR_NONE;
}


dsError_t westerosReadWrite(char *cmd, char *response)
{
    char buffer[256] = {0};
    FILE *fd;

    fd = popen(cmd , "r");

    if (fd == NULL) {
            fprintf(stderr, "Could not open pipe.\n");
            return dsERR_INVALID_PARAM;
    }

    // Read process output
    while (fgets(buffer, 256, fd) != NULL)
    {
            printf("%s", buffer);
    }

    strcpy(response, buffer);
    pclose(fd);
    return dsERR_NONE;
}


void removeChar(char *str, char garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}

dsError_t dsSetFRFMode(int handle, int framerate)
{
    char *ptr =NULL;
    char resp[256] = {0};
    char buf[256] ={0};

    printf(" %s :  framerate= %d \r\n",__FUNCTION__, framerate);
#ifndef TMP_XDG_RUNTIME_DIR
    sprintf(buf, "export XDG_RUNTIME_DIR=/run; westeros-gl-console set auto-frm-mode %d", framerate);
#else
    sprintf(buf, "export XDG_RUNTIME_DIR=/tmp; westeros-gl-console set auto-frm-mode %d", framerate);
#endif

    westerosReadWrite(buf, resp);

    printf("%s\n", resp);
    ptr = strstr(resp, "auto-frm-mode");
    if (ptr != NULL) {
        if (framerate == atoi(ptr+14)) {
                return dsERR_NONE;
        }
    }
    return dsERR_INVALID_PARAM;
}

dsError_t dsGetFRFMode(int handle, int *frfmode)
{
    char *ptr =NULL;
    char resp[256] = {0};
    char buf[256] ={0};

    printf("%s :  \r\n",__FUNCTION__);
#ifndef TMP_XDG_RUNTIME_DIR
    memcpy(buf, "export XDG_RUNTIME_DIR=/run; westeros-gl-console get auto-frm-mode", 66);
#else
    memcpy(buf, "export XDG_RUNTIME_DIR=/tmp; westeros-gl-console get auto-frm-mode", 66);
#endif

    westerosReadWrite(buf, resp);

    printf("%s\n", resp);
    ptr = strstr(resp, "auto-frm-mode");
    if (ptr == NULL) {
        return dsERR_INVALID_PARAM;
    }else{
        *frfmode = atoi(ptr+13);
    }
    return dsERR_NONE;
}

dsError_t dsGetCurrentDisplayframerate(int handle, char *framerate)
{
    char *ptr =NULL;
    char resp[256] = {0};
    char buf[256] ={0};

    printf("%s : \r\n",__FUNCTION__);
#ifndef TMP_XDG_RUNTIME_DIR
    memcpy(buf, "export XDG_RUNTIME_DIR=/run; westeros-gl-console get mode", 57);
#else
    memcpy(buf, "export XDG_RUNTIME_DIR=/tmp; westeros-gl-console get mode", 57);
#endif

    westerosReadWrite(buf, resp);

    printf("%s\n", resp);

    //removes character 'p' from response
    removeChar(resp, 'p');

    ptr = strstr(resp, "mode");

    if (ptr == NULL) {
        return dsERR_INVALID_PARAM;
    }else{
    // strncpy(framerate, ptr+5, 13);
    strncpy(framerate, ptr+5, strlen(ptr) - 6);
    }
    return dsERR_NONE;
}

dsError_t dsSetDisplayframerate(int handle, char *mode)
{
    char *ptr =NULL;
    char resp[256] = {0};
    char buf[256] ={0};

    char resolution[256] ={0};

    // memcpy(resolution, mode, strlen(mode)+1);
    //add char 'p' after second 'X'

    ptr = strstr(mode,"x");
    ptr++;
    ptr = strstr(ptr,"x");

    strncpy(resolution, mode, strlen(mode)-strlen(ptr));
    strcat (resolution,"p");
    strcat (resolution,ptr);

    printf("%s :  mode= %s \r\n",__FUNCTION__, resolution);

#ifndef TMP_XDG_RUNTIME_DIR
    sprintf(buf, "export XDG_RUNTIME_DIR=/run; westeros-gl-console set mode %s", resolution);
#else
    sprintf(buf, "export XDG_RUNTIME_DIR=/tmp; westeros-gl-console set mode %s", resolution);
#endif

    westerosReadWrite(buf, resp);

    printf("%s\n", resp);

    ptr = strstr(resp, mode);
    if (ptr != NULL) {
            return dsERR_NONE;
    }
    return dsERR_INVALID_PARAM;
}

