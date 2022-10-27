#include "dsHost.h"
#include "dsError.h"
#include "dsTypes.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "Amvideoutils.h"
#include "Amsysfsutils.h"

#define SUCCESS 0
#define SIZE 9
#define File_Size 50

typedef struct _HOSTHandle_t {
        uint32_t Ver_Num;
}HostHandle_t;

enum _HW_Operations{
TEMP,
VER_NUM,
MAX_OPR
};



static HostHandle_t _dsHal = {0x1000};


dsError_t dsHostInit()
{
	return dsERR_NONE;
}

dsError_t dsSetHostPowerMode(int newPower)
{
	/**** Currently not invoking from DS ****/
	printf("Entered into SetHost power mode\n");
	dsError_t ret;  
	char Temp_File[File_Size];
	
	
	sprintf(Temp_File,"%s","/sys/power/state");
        printf("file name in DSvideoinit():%s\n",Temp_File);
	printf("newPower value is %d\n",newPower);

	switch(newPower) {
                        case 1:
				amsysfs_set_sysfs_str(Temp_File,"wake");
                                break;
                        case 2:
                                amsysfs_set_sysfs_str(Temp_File,"mem");
                                break;
			}
	
	ret = dsERR_NONE;
	
	return ret;

}

dsError_t dsGetHostPowerMode(int *currPower)
{
	/**** Currently not invoking from DS ****/
	printf("Entered into Get hostpower mode in hal\n");
	dsError_t ret;
	char strPower[SIZE] = {0};
	char Temp_File[File_Size];
	
	if(currPower!=NULL)
	{
		sprintf(Temp_File,"%s","/sys/power/state");
        	printf("file name in DSvideoinit():%s\n",Temp_File);
		amsysfs_get_sysfs_str(Temp_File,strPower, sizeof(strPower));
        	printf("strPower in hal:%s\n",strPower);
		if((strcmp(strPower,"mem"))==0){
			*currPower = 2;
			 ret = dsERR_NONE;
		}else{
                        *currPower = 1;
			ret = dsERR_NONE;
		}
                                
	}
	else ret = dsERR_INVALID_PARAM;
	
	return ret;
	
}

dsError_t dsHostTerm()
{
	/****Not supported from RDK****/
	return dsERR_NONE;
}

dsError_t dsGetPreferredSleepMode(dsSleepMode_t *pMode)
{
	printf("ENtering into dsGetPrefereedSleepMode\n");
	/****operation not supported from SOC****/
        return dsERR_NONE;
}

dsError_t dsSetPreferredSleepMode(dsSleepMode_t mode)
{
	printf("Entering into dsSetPreferredSLeepMode\n");
	/****operation not supported from SOC*****/
        return dsERR_NONE;

}
/*****
@brief This API is used to get the temparature from the soc
*****/
dsError_t dsGetCPUTemperature(float *cpuTemperature)
{
	printf("Entering into dsGetCPUTemparature trinadh\n");
	char strTemp[SIZE]; 
        float temp_value;
	dsError_t ret;
	char Temp_File[File_Size];
	
	
	if(cpuTemperature!=NULL)
	{
		sprintf(Temp_File,"%s","/sys/class/thermal/thermal_zone0/temp");
        	printf("file name in DSvideoinit():%s\n",Temp_File);
		amsysfs_get_sysfs_str(Temp_File,strTemp, sizeof(strTemp));
                temp_value = atof(strTemp);
		printf("strTemp in hal:%s\n",strTemp);
		temp_value = temp_value/1000;
		printf("temp_value in hal:%f\n",temp_value);
		*cpuTemperature = temp_value;
		printf("cpuTemparature:%f\n",*cpuTemperature);
        	return dsERR_NONE;
	}
	else ret = dsERR_INVALID_PARAM;
	return ret;
}
/*****
@brief This API is used to get the DS-Hal version
*****/
dsError_t dsGetVersion(uint32_t *versionNumber)
{
	printf("Entering into dsGetVersion\n");
	dsError_t ret;
	
	if(versionNumber != NULL)
	{
		printf("getting hal version in ds-hal\n");
		printf("_dsHal.Ver_Num in GetVersion is:%x\n",_dsHal.Ver_Num);
		*versionNumber = _dsHal.Ver_Num;
		return dsERR_NONE;
	}	
	else ret = dsERR_INVALID_PARAM;	
	return ret;
}

/******
@brief This API is used to set the DS-hal version
******/
dsError_t dsSetVersion(uint32_t versionNumber)
{
	printf("Entering into dsSetVersion\n");
	dsError_t ret;
	
        printf("setting hal version in ds-hal\n");
	printf("_dsHal.Ver_Num in setVersion is:%x\n",_dsHal.Ver_Num);
	printf("versionNumber received in setversion is %x\n",versionNumber);
        _dsHal.Ver_Num = versionNumber;
	printf("_dsHal.Ver_Num in setVersion is:%x\n",_dsHal.Ver_Num);
	ret = dsERR_NONE;
        
      	return ret;
}


#define MAX_BUF_LEN 255
#define MAX_COMMAND_SIZE 100
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

const char defaultManufacturerOUI[] = "S905X4";
const char defaultSerialNumber[] = "0000000000000000";


static const struct amlSocFamily_t {
    uint32_t idMask;
    const char *Class;
    const char *Chip;
    const char *DeviceName;

} amlSocFamily[] = {
    /* 'FF' in idMask makes logical AND passable for match */
    { 0x28ff10, "MBX", "G12A",  "S905D2" },
    { 0x28ff30, "MBX", "G12A",  "S905Y2" },
    { 0x28ff40, "MBX", "G12A",  "S905X2" },
    { 0x29ffff, "MBX", "G12B",  "T931G"  },
    { 0x29ffff, "MBX", "G12B",  "S922X"  },
    { 0x32fa02, "MBX", "SC2",   "S905X4" },
    { 0x320b04, "MBX", "C2",    "S905C2" },
    { 0x370a02, "MBX", "S4",    "S805X2" },
    { 0x370a03, "MBX", "S4",    "S905Y4" },
    { 0x3a0a03, "MBX", "S4D",   "S905Y4D" },
    { 0x3a0c04, "MBX", "C3",    "S905C3" },
};

static const char* getChipsetFromId(char *pSlNo)
{
    int i;
    char slNo[8] = {'\0'};
    strncpy(slNo, pSlNo, 6);
    uint32_t chipID = strtoul(slNo, NULL, 16);
    for (i = 0; i < ARRAY_SIZE(amlSocFamily); i++) {
        if (chipID == (chipID & amlSocFamily[i].idMask)) {
            return(amlSocFamily[i].DeviceName);
        }
    }
    return defaultManufacturerOUI;
}

dsError_t dsGetSocIDFromSDK(char *socID)
{
    char cmd[MAX_COMMAND_SIZE];
    char buffer[MAX_BUF_LEN];
    char out_buffer[MAX_BUF_LEN];
    FILE *sfp = NULL;

    if (socID)
    {
        /* Extract Serial Number and map chipset. */
        memset(cmd, 0, sizeof(char) * 100);
        memset(buffer, 0, MAX_BUF_LEN);
        memset(out_buffer, 0, MAX_BUF_LEN);

        /* retrieving tag Serial from /proc/cpuinfo */
        sprintf(cmd, "cat /proc/cpuinfo | grep Serial | sed -e 's/.*: //g'");
        if ((sfp = popen(cmd, "r")) == NULL)
        {
            printf("popen failed.");
            strncpy(buffer, defaultSerialNumber, 6);
        }
        if (sfp)
        {
            fgets(buffer, sizeof(buffer), sfp);
            pclose(sfp);
            if (strlen(buffer) < 6) {
                strncpy(buffer, defaultSerialNumber, 6);
            }
            strcpy(out_buffer, getChipsetFromId(buffer));
        }

        memcpy(socID,out_buffer,strlen(out_buffer)+1);

        printf("SocID Info= %s\t len=%d\n", out_buffer, strlen(out_buffer)+1);

        return dsERR_NONE;
    }
    return dsERR_INVALID_PARAM;
}