#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dsTypes.h"
#include "dsUtl.h"
#include "dsDisplay.h"
#include "dsVideoPort.h"
#include "dsVideoResolutionSettings.h"
#include "dsDrmUtils.h"
//#include "dsS905D.h"
#include "edid-parser.h"
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "dsSysfsUtils.h"
#include "audio_if.h"

#ifdef AML_STB

/* UDev based DRM hotplug event listener. */
#include <libudev.h>

#define LIBUDEV_EVT_TYPE_KERNEL     "kernel"
#define LIBUDEV_EVT_TYPE_UDEV       "udev"
#define LIBUDEV_SUBSYSTEM_DRM       "drm"
#define LIBUDEV_SUBSYSTEM_EXTCON    "extcon"

extern drmModeConnector * dsDRMGetHDMIConnector();
extern void drmRefreshConnector();
dsVideoPortResolution_t *HdmiSupportedResolution=NULL;
static unsigned int supportedResCount = 0;

#endif /* AML_STB */

typedef struct _VDISPHandle_t {
    dsVideoPortType_t vType;
    int index;
    int nativeHandle;
} VDISPHandle_t;

#define MAX_PORTS 1
static VDISPHandle_t _handles[dsVIDEOPORT_TYPE_MAX][MAX_PORTS] = {
};

/*struct SuppResolutionlist Supported_VIC[21] = {
  {3,     "480p"},
  {18,    "576p50"},
  {4,     "720p"},
  {19,    "720p50"},
  {5,     "1080i"},
  {16,    "1080p"},
  {20,    "1080i50"},
  {31,    "1080p50"},
  {33,    "1080p25"},
  {32,    "1080p24"},
  {34,    "1080p30"},
  {95,    "2160p30"},
  {94,    "2160p25"},
  {93,    "2160p24"},
  {96,    "2160p50"},
  {97,    "2160p"},
  {98,    "smpte24hz"},
  {99,    "smpte25hz"},
  {100,   "smpte30hz"},
  {101,   "smpte50hz"},
  {102,   "smpte60hz"},
  };*/


bool isValidVdispHandle(int handle)
{
    uint8_t type, i;
    for ( type = 0; dsVIDEOPORT_TYPE_MAX > type; type ++) {
        for ( i = 0; MAX_PORTS > i; i ++) {
            if ( (int)&_handles[type][i] == handle )
                return true;
        }
    }
    return false;
}

bool isMonitoringAlive = false;
static pthread_t ds_hpd_threadId; // Use for HDMI Hotplug
#ifdef AML_STB
static void* ds_hotplug_thread(void *arg);
#endif
dsDisplayEventCallback_t _halcallback = NULL;

dsError_t  dsDisplayInit()
{
    dsError_t ret = dsERR_NONE;
    int err;
    int type;
    int index;
    _handles[dsVIDEOPORT_TYPE_HDMI][0].vType  = dsVIDEOPORT_TYPE_HDMI;
    _handles[dsVIDEOPORT_TYPE_HDMI][0].nativeHandle = dsVIDEOPORT_TYPE_HDMI;
    _handles[dsVIDEOPORT_TYPE_HDMI][0].index = 0;

#ifdef AML_STB
    type = dsVIDEOPORT_TYPE_HDMI;
    index = 0;
    isMonitoringAlive = true;
    err = pthread_create (&ds_hpd_threadId, NULL,ds_hotplug_thread, (void *)&(_handles[type][index]));
    if(err) {
        printf("DSHAL : Failed to Ceate HDMI Hot Plug Thread ....\r\n");
        ret = dsERR_GENERAL;
        ds_hpd_threadId = -1;
    }
#endif
    return ret;
}

dsError_t  dsGetDisplay(dsVideoPortType_t vType, int index, int *handle)
{
    dsError_t ret = dsERR_NONE;
    if (index != 0  || !dsVideoPortType_isValid(vType) || handle == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    if (vType != dsVIDEOPORT_TYPE_HDMI) {
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    if (ret == dsERR_NONE) {
        *handle = (int)&_handles[vType][index];
    }

    return ret;
}

dsError_t dsGetDisplayAspectRatio(int handle, dsVideoAspectRatio_t *aspect)
{
    dsError_t ret = dsERR_NONE;
    return ret;
}


#ifdef AML_STB
int getHDMISupportedResolutions()
{
    int ret = 1;
    int count = dsUTL_DIM(kResolutions);
    HdmiSupportedResolution=(dsVideoPortResolution_t*)malloc(sizeof(dsVideoPortResolution_t)*count);
    drmModeConnector* conn = dsDRMGetHDMIConnector();
    drmConnectorModes supportedModes[drmMode_Max] = {drmMode_Unknown};
    getSupportedDRMResolutions(conn, supportedModes);

    supportedResCount = 0;
    for (int i=0; i<count; i++) {
        dsVideoPortResolution_t *rdkRes = &kResolutions[i];
        drmConnectorModes drmMode = getDRMConnectorModeInfo(rdkRes->pixelResolution, rdkRes->frameRate, rdkRes->interlaced);
        if( drmMode != drmMode_Unknown ) {
            for (int j=0; j<drmMode_Max; j++) {
                if (drmMode == supportedModes[j]) {
                    HdmiSupportedResolution[supportedResCount] = kResolutions[i];
                    DBG("Supported Resolution '%s'\n", HdmiSupportedResolution[supportedResCount].name);
                    supportedResCount++;
                    break;
                }
            }
        }
    }
    drmModeFreeConnector(conn);
    return ret;
}
#endif



#ifdef AML_STB
/***
  * @Info   : isDisplaySinkHDMI checks if the Display Sink is HDMI or not.
  * @return : true if HDMI; else false.
  */
bool isDisplaySinkHDMI(void)
{
    /* Could not get this info from raw edid; hence using parsed info. */
    /* FIXME: popen in use */
    char buffer[64] = {'\0'};
    bool ret = false;
    snprintf(buffer, sizeof(buffer)-1, "cat %s | grep Vendor", SYSFS_HDMITX_EDID_PARSED);

    FILE* fp = popen(buffer, "r");
    if (NULL != fp) {
        char output[64] = {'\0'};
        while (fgets(output, sizeof(output)-1, fp)) {
            if (strlen(output) && strnstr(output, "HDMI", 63)) {
                ret = true;
            } else {
                ret = false;
            }
        }
        pclose(fp);
    } else {
        printf("isSinkHDMI: popen failed\n");
    }
    return ret;
}
#endif

dsError_t dsGetEDID(int handle, dsDisplayEDID_t *edid)
{
    dsError_t ret = dsERR_NONE;
    VDISPHandle_t *vDispHandle = (VDISPHandle_t *) handle;
    if (vDispHandle == NULL)
    {
        ret = dsERR_NONE;
        printf("DIsplay Handle is NULL .......... \r\n");
        return ret;
    }
#ifdef AML_STB
    int drmFD = -1;
    unsigned char* edid_buffer = NULL;
    int length = 0;
    edid_data_t* data_ptr = NULL;
    drmModeConnector *conn = NULL;
    drmModePropertyPtr props = NULL;
    drmModePropertyBlobPtr blob = NULL;

    drmFD = openDefaultDRMDevice();
    if (drmFD < 0) {
        printf("%s:%d: cannot open %s\n", __func__, __LINE__, DEFUALT_DRM_DEVICE);
        ret = dsERR_GENERAL;
        goto out;
    }

    data_ptr = (edid_data_t*)calloc(1, sizeof(struct edid_data_t));
    conn = dsDRMGetHDMIConnector();
    if((conn->connection == DRM_MODE_CONNECTED)) {
        for (int i=0; i<conn->count_props;i++) {
            props = drmModeGetProperty(drmFD, conn->props[i]);
            if(props && !strcmp(props->name,"EDID") && (props->flags & DRM_MODE_PROP_BLOB)) {
                blob = drmModeGetPropertyBlob(drmFD, conn->prop_values[i]);
                if(blob) {
                    length = blob->length;
                    edid_buffer = (unsigned char*) malloc(blob->length);
                    DBG("Copying raw EDID\n");
                    if(edid_buffer != NULL) {
                        for (int j=0;j<blob->length;j++) {
                            memcpy(edid_buffer+j, blob->data+j, 1);
                        }
                    }
                }
                drmModeFreePropertyBlob(blob);
                break;
            }
        }
        drmModeFreeProperty(props);

    }
    else {
        ret = dsERR_INVALID_STATE;
        goto out;
    }
    if (vDispHandle->vType == dsVIDEOPORT_TYPE_HDMI && vDispHandle->index == 0) {
        edid_status_e status = EDID_Parse(edid_buffer, length, data_ptr);
        printf(" DS_HAL EDID Parse Status %d\n", status);
        DBG("manufacturer_name %s\n", data_ptr->manufacturer_name);
        DBG("monitor_name %s\n", data_ptr->monitor_name);
        DBG("serial_number %d\n", data_ptr->serial_number);
        DBG("product_code %d\n", data_ptr->product_code);
        DBG("hdr_capabilities %d\n", data_ptr->hdr_capabilities);
        DBG("dv_capabilities %d\n", data_ptr->dv_capabilities);
        DBG("manufacture_week %d\n", data_ptr->manufacture_week);
        DBG("manufacture_year %d\n", data_ptr->manufacture_year);
        DBG("physical_address_a %d\n", data_ptr->physical_address_a);
        DBG("physical_address_b %d\n", data_ptr->physical_address_b);
        DBG("physical_address_c %d\n", data_ptr->physical_address_c);
        DBG("physical_address_d %d\n", data_ptr->physical_address_d);
        DBG("colorimetry_info %d\n", data_ptr->colorimetry_info);
        edid->productCode = data_ptr->product_code;
        edid->serialNumber = data_ptr->serial_number;
        edid->manufactureYear = data_ptr->manufacture_year;
        edid->manufactureWeek = data_ptr->manufacture_week;
        edid->physicalAddressA = data_ptr->physical_address_a;
        edid->physicalAddressB = data_ptr->physical_address_b;
        edid->physicalAddressC = data_ptr->physical_address_c;
        edid->physicalAddressD = data_ptr->physical_address_d;
        /* See https://wiki.rdkcentral.com/display/FORUMS/EDID+structure+description. */
        edid->hdmiDeviceType = isDisplaySinkHDMI();
        memset(((char*)edid->monitorName), 0, sizeof((char*)edid->monitorName));
        strcpy(((char*)edid->monitorName),((char*)data_ptr->monitor_name));

        getHDMISupportedResolutions();
        printf("number of supported resolutions: %d .......... \r\n",supportedResCount);
        for (size_t i = 0; i < supportedResCount; i++)
        {
            edid->suppResolutionList[edid->numOfSupportedResolution] = HdmiSupportedResolution[i];
            edid->numOfSupportedResolution++;
        }

    }
    free(HdmiSupportedResolution);
    HdmiSupportedResolution = NULL;
out:
    free(edid_buffer);
    free(data_ptr);
    closeDefaultDRMDevice(drmFD);
    drmModeFreeConnector(conn);
#endif
    return ret;
}

dsError_t dsGetEDIDBytes(int handle, unsigned char **edid, int *length) {
    dsError_t ret = dsERR_NONE;
    VDISPHandle_t *vDispHandle = (VDISPHandle_t *) handle;
    unsigned char *edid_buffer;
    *length = 0;

#ifdef AML_STB
    int drmFD = -1;
    drmModeConnector *conn = NULL;
    drmModePropertyPtr props = NULL;
    drmModePropertyBlobPtr blob = NULL;

    if (edid == NULL || length == NULL) {
        printf("[%s] invalid params\n", __FUNCTION__);
        return dsERR_INVALID_PARAM;
    }
    else if (vDispHandle == NULL || vDispHandle != &_handles[dsVIDEOPORT_TYPE_HDMI][0])
    {
        printf("[%s] invalid handle\n", __FUNCTION__);
        return dsERR_INVALID_PARAM;
    }

    drmFD = openDefaultDRMDevice();
    if (drmFD < 0) {
        printf("cannot open %s\n", DEFUALT_DRM_DEVICE);
        ret = dsERR_GENERAL;
        goto out;
    }

    conn = dsDRMGetHDMIConnector();
    if((conn->connection == DRM_MODE_CONNECTED)) {
        for (int i=0; i<conn->count_props;i++) {
            props = drmModeGetProperty(drmFD, conn->props[i]);
            if(props && !strcmp(props->name,"EDID") && (props->flags & DRM_MODE_PROP_BLOB)) {
                blob = drmModeGetPropertyBlob(drmFD, conn->prop_values[i]);
                if(blob) {
                    *length = blob->length;
                    edid_buffer = (unsigned char*) malloc(blob->length);
                    DBG("Copying raw EDID\n");
                    if(edid_buffer != NULL) {
                        for (int j=0;j<blob->length;j++) {
                            memcpy(edid_buffer+j, blob->data+j, 1);
                        }
                        *edid = edid_buffer;
                    }
                }
                drmModeFreePropertyBlob(blob);
                break;
            }
        }
        drmModeFreeProperty(props);

    }
    else {
        ret = dsERR_INVALID_STATE;
        goto out;
    }

out:
    closeDefaultDRMDevice(drmFD);
    drmModeFreeConnector(conn);
#endif
    return ret;
}

dsError_t  dsDisplayTerm()
{
    dsError_t ret = dsERR_NONE;
    isMonitoringAlive = false;
    _halcallback = NULL;
    if ((-1 != ds_hpd_threadId) && (0 != pthread_join(ds_hpd_threadId, NULL))) {
        printf("ERROR[%s:%d] : pthread_join ds_hpd_threadId failed\n", __FUNCTION__, __LINE__);
        ret = dsERR_GENERAL;
    }
    return ret;
}

dsError_t dsRegisterDisplayEventCallback(int handle, dsDisplayEventCallback_t cb)
{
    dsError_t ret = dsERR_NONE;

    if(cb == NULL)
        ret = dsERR_INVALID_PARAM;
    if(ret == dsERR_NONE) {
        /* Register The call Back */
        _halcallback = cb;
    }

    return ret;
}

#ifdef AML_STB
static int ds_notify_audio_hdmi_hotplug(bool isConnect)
{
    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);
    char setting[32];

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", err);
        return err;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return err;
    }
    /*1024 means AUDIO_DEVICE_OUT_HDMI=0x400u*/
    snprintf(setting, sizeof(setting), "%s=1024", isConnect? "connect": "disconnect");
    err = device->set_parameters(device, setting);

    audio_hw_unload_interface(device);

    return err;
}

/**
 *  @Info   : HDMI Hot plug polling Thread; monitors udev events
 *            of DRM subsystem for "change" action hotplug events.
 */
static void* ds_hotplug_thread(void *arg)
{
    if (arg == NULL) {
        printf("ERROR[%s:%d] argument NULL\n", __FUNCTION__, __LINE__);
    } else {
        bool isConnected = false;
        bool wasConnected = false;
        dsDisplayEvent_t displayEvent = dsDISPLAY_EVENT_DISCONNECTED;
        VDISPHandle_t *param = (VDISPHandle_t*)arg;;
        unsigned char  eventData=0; /* event data - a place holder as of now*/
        int videoPortHandle = 0;
        dsError_t err = dsERR_NONE;

        struct udev *udev = NULL;
        struct udev_device *dev = NULL;
        struct udev_monitor *mon = NULL;

        /* create udev object */
        udev = udev_new();
        if (!udev) {
            printf("ERROR[%s:%d] Can't create udev monitor for DRM,\n", __FUNCTION__, __LINE__);
        } else {
            mon = udev_monitor_new_from_netlink(udev, LIBUDEV_EVT_TYPE_KERNEL);
            if (mon) {
                int fd = -1;
                fd_set fds;
                struct timeval tv;
                int ret;

                if ((fd = udev_monitor_get_fd(mon)) < 0) {
                    printf("ERROR[%s:%d] udev_monitor_get_fd failed,\n", __FUNCTION__, __LINE__);
                } else {
                    if (udev_monitor_filter_add_match_subsystem_devtype(mon, LIBUDEV_SUBSYSTEM_DRM, NULL) < 0) {
                        printf("ERROR[%s:%d] udev_monitor_filter_add_match_subsystem_devtype failed,\n", __FUNCTION__, __LINE__);
                    } else {
                        if (udev_monitor_enable_receiving(mon) < 0) {
                            printf("ERROR[%s:%d] udev_monitor_enable_receiving\n", __FUNCTION__, __LINE__);
                        } else {
                            while (isMonitoringAlive) {
                                FD_ZERO(&fds);
                                FD_SET(fd, &fds);
                                tv.tv_sec = 15; /* FIXME: fine tune the select timeout. */
                                tv.tv_usec = 0;

                                ret = select(fd+1, &fds, NULL, NULL, &tv);
                                if (ret > 0 && FD_ISSET(fd, &fds)) {
                                    dev = udev_monitor_receive_device(mon);
                                    if (dev) {
                                        DBG("I: ACTION=%s\n", udev_device_get_action(dev));
                                        DBG("I: DEVNAME=%s\n", udev_device_get_sysname(dev));
                                        DBG("I: DEVPATH=%s\n", udev_device_get_devpath(dev));
                                        if (!strcmp(udev_device_get_action(dev), "change")) {
                                            /* drm card0 event: hotplug */
                                            if (!videoPortHandle) {
                                                err = dsGetVideoPort(param->vType, param->index, &videoPortHandle);
                                                if (err != dsERR_NONE) {
                                                    printf("%s:%d: dsGetVideoPort[%d][%d] failed(%d)\n",
                                                            __func__, __LINE__, param->vType, param->index, err);
                                                }
                                            }
                                            if (err == dsERR_NONE) {
                                                err = dsIsDisplayConnected(videoPortHandle, &isConnected);
                                                if (err != dsERR_NONE) {
                                                    printf("%s:%d: dsIsDisplayConnected failed(%d)\n", __func__, __LINE__, err);
                                                }
                                            }
                                            if ((wasConnected != isConnected) && (err == dsERR_NONE)) {
                                                wasConnected = isConnected;
                                                displayEvent = isConnected ? dsDISPLAY_EVENT_CONNECTED : dsDISPLAY_EVENT_DISCONNECTED;
                                                printf("Send %s HDMI Hot Plug Event !!!\n",
                                                        (displayEvent == dsDISPLAY_EVENT_CONNECTED? "Connect":"DisConnect"));
                                                ds_notify_audio_hdmi_hotplug(displayEvent == dsDISPLAY_EVENT_CONNECTED? true: false);
                                                if (_halcallback) {
                                                    _halcallback((int)param, displayEvent, &eventData);
                                                }
                                            } else {
                                                /* TODO: Maybe no change or err != dsERR_NONE. Handle accordingly. */
                                            }
                                        }
                                        /* free dev */
                                        udev_device_unref(dev);
                                    }
                                } else {
                                    /* TODO: Select timeout or error; handle accordingly. */
                                }
                            }
                        }
                    }
                }
            } else {
                printf("ERROR[%s:%d] udev_monitor_new_from_netlink failed\n", __FUNCTION__, __LINE__);
            }
        }
        udev = NULL;
        dev = NULL;
        mon = NULL;
    }
    pthread_exit(NULL);
    return NULL;
}
#endif
