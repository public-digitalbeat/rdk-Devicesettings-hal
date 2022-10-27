#include <stdio.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "dsVideoResolutionSettings.h"


typedef enum _drmConnectorModes {
    drmMode_Unknown = 0,
    drmMode_480i,
    drmMode_480p,
    drmMode_576i,
    drmMode_576p,
    drmMode_720p,
    drmMode_720p24,
    drmMode_720p25,
    drmMode_720p30,
    drmMode_720p50,
    drmMode_1024p,
    drmMode_1080i,
    drmMode_1080i50,
    drmMode_1080p,
    drmMode_1080p24,
    drmMode_1080p25,
    drmMode_1080p30,
    drmMode_1080p50,
    drmMode_3840x2160p24,
    drmMode_3840x2160p25,
    drmMode_3840x2160p30,
    drmMode_3840x2160p50,
    drmMode_3840x2160p60,
    drmMode_4096x2160p24,
    drmMode_4096x2160p25,
    drmMode_4096x2160p30,
    drmMode_4096x2160p50,
    drmMode_4096x2160p60,
    drmMode_Max

} drmConnectorModes;

#define DEFUALT_DRM_DEVICE "/dev/dri/card0"
int openDefaultDRMDevice(void);
void closeDefaultDRMDevice(int drmFD);

int getDefaultDRMResolution (drmModeConnector *conn, drmConnectorModes *drmResolution) ;
int getSupportedDRMResolutions (drmModeConnector *conn, drmConnectorModes *drmResolution);
int getCurrentDRMResolution (int drmFD, drmModeRes *res, drmModeConnector *conn, drmConnectorModes *drmResolution);
drmConnectorModes getDRMConnectorModeInfo(dsVideoResolution_t pixelRes, dsVideoFrameRate_t f_rate, bool interlaced);


