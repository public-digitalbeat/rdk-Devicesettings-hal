#include <string.h>
#include "dsUtl.h"
#include "dsError.h"
#include "dsTypes.h"
#include "dsVideoPort.h"
#include "dsVideoPortSettings.h"
#include "dsVideoResolutionSettings.h"
#include "dsDisplay.h"

#include "dsSysfsUtils.h"
#include "dsDrmUtils.h"
#include "edid-parser.h"
//DRM Headers
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/version.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <libdrm_meson/meson_drm_display.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifndef AML_STB
#define MAX_SIZE 50
#endif

#ifdef AML_STB

/* UDev based DRM hotplug event listener. */
#include <libudev.h>

#define LIBUDEV_EVT_TYPE_KERNEL     "kernel"
#define LIBUDEV_EVT_TYPE_UDEV       "udev"
#define LIBUDEV_SUBSYSTEM_DRM       "drm"
#define LIBUDEV_SUBSYSTEM_EXTCON    "extcon"


/*** Globals ***/
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t drm_refresh_threadId;
bool drmInitialized = false;
//static drmModeRes *res = NULL;  //DRM  Resource ptr
//static drmModeConnector *hdmiConn = NULL; //DRM HDMI connector ptr
static uint32_t connector_id = 0;
bool drmRefreshAlive = false;
static dsHdcpProtocolVersion_t hdcp_version = dsHDCP_VERSION_2X;

static pthread_mutex_t initLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t initCond = PTHREAD_COND_INITIALIZER;

#define WAIT_VIDEO_PORT_INITIALIZE_TIMEOUT_SECONDS  1

#define CHECK_DRM_INITIALIZE(initflag) do { \
    if (!initflag) { \
        pthread_mutex_lock(&initLock); \
        fprintf(stderr, "before func %s initflag %d\n", __FUNCTION__, initflag); \
        while(!initflag){ \
            int ret = 0; \
            struct timeval now; \
            struct timespec timeout; \
            gettimeofday(&now, NULL); \
            timeout.tv_sec = now.tv_sec + WAIT_VIDEO_PORT_INITIALIZE_TIMEOUT_SECONDS; \
            timeout.tv_nsec = now.tv_usec * 1000; \
            ret = pthread_cond_timedwait(&initCond, &initLock, &timeout); \
            if (ret == ETIMEDOUT){ \
                if (!initflag) \
                    fprintf(stderr, "strange!!!!! video port init not finished in 1 seconds\n"); \
                break; \
            } \
            fprintf(stderr, "after func %s initflag %d\n", __FUNCTION__, initflag); \
        } \
        pthread_mutex_unlock(&initLock); \
    } \
}while(0)


static drmModeConnector * getDrmConnector(uint32_t connector_id){
    int drmFD = -1;
    drmModeConnector * conn = NULL;

    CHECK_DRM_INITIALIZE(drmInitialized);

    drmFD = openDefaultDRMDevice();
    if (drmFD < 0) {
        printf("cannot open %s\n", DEFUALT_DRM_DEVICE);
        goto out;
    }

    conn = drmModeGetConnector(drmFD, connector_id);

out:
    closeDefaultDRMDevice(drmFD);
    return conn;
}

#endif /* AML_STB */


/***
 * @Info        : Check if string has substring in it.
 * @param1[in]  : pointer to Haystack
 * @param2[in]  : pointer to Needle
 * @param3[in]  : buffer limit
 * @return      : pointer to the memory of first match
 */
char *strnstr(const char *s, const char *find, size_t slen)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0') {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == '\0' || slen-- < 1)
                    return (NULL);
            } while (sc != c);
            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

static dsHDCPStatusCallback_t hdcpStatusCallback = NULL;
static dsError_t call_hdcpstatus_callback(void);

#ifndef XDG_RUNTIME_DIR
#define XDG_RUNTIME_DIR     "/run"
#endif

typedef struct _VOPHandle_t {
    dsVideoPortType_t vType;
    int index;
    int nativeHandle;
    bool IsEnabled;
} VOPHandle_t;

#define MAX_PORTS 1
static VOPHandle_t _handles[dsVIDEOPORT_TYPE_MAX][MAX_PORTS] = {
};


#define IS_VIDEO_PORT_TYPE_SUPPORT(t)      ( dsVIDEOPORT_TYPE_HDMI == (t) )

/**
 * @brief Check if VideoPort is supported by this device configuration or not.
 *
 * This function check if VideoPort is supported by this device configuration or not.
 *
 * @param [in]  type       Type of video port (e.g. HDMI).
 * @return    true if input videoport is supported otherwise return false.
 */
bool isValidVopHandle(int handle)
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

static dsError_t call_hdcpstatus_callback(void){
    dsHdcpStatus_t hdcpStatus;
    dsError_t hdcpRet = dsERR_NONE;
    int hdcpHandle = 0;
    hdcpRet = dsGetVideoPort(dsVIDEOPORT_TYPE_HDMI, 0, &hdcpHandle);
    if(hdcpRet == dsERR_NONE){
        hdcpRet = dsGetHDCPStatus(hdcpHandle, &hdcpStatus);
        if(hdcpRet == dsERR_NONE){
            if(hdcpStatusCallback && hdcpHandle && isValidVopHandle(hdcpHandle)) {
                hdcpStatusCallback(hdcpHandle, hdcpStatus);
            }
        }
    }

    return hdcpRet;
}

#ifdef AML_STB

/***
  * @Info   : This function checks if DolbyVision driver is loaded or not.
  * @return : bool; true if driver loaded; else false.
  */
static bool isDolbyVisionDriverLoaded(char *pDVKernelModuleName)
{
    bool status = false;
    char cmd[256] = {'\0'};
    /* FIXME: Using 'lsmod | grep `DOLBY_VISION_KERNEL_MODULE_NAME` '*/
    /* Better to confirm DolbyVision driver support using any sysfs entry. */
    snprintf(cmd, sizeof(cmd), "lsmod | grep '^%s'", pDVKernelModuleName);
    DBG("Trying popen '%s'\n", cmd);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        memset(cmd, 0, sizeof(cmd));
        while (fgets(cmd, sizeof(cmd)-1, fp)) {
            DBG("Read: '%s'\n", cmd);
            if (strlen(cmd) && strnstr(cmd, pDVKernelModuleName, strlen(cmd)-1)) {
                status = true;
                break;
            }
        }
        pclose(fp);
    }
    return status;
}

static bool getDvSupportStatus() {
    //return dv support info from current platform device, not display device.
    char buf[1024+1] = {0};
    int fd, len;
    bool ret = false;

    /*bit0: 0-> efuse, 1->no efuse; */
    /*bit1: 1->ko loaded*/
    /*bit2: 1-> value updated*/
    int supportInfo;

    constexpr int dvDriverEnabled = (1 << 2);
    constexpr int dvSupported = ((1 << 0) | (1 << 1) | (1 <<2));

    if ((fd = open(SYSFS_DOLBYVISION_SUPPORT_INFO, O_RDONLY)) < 0) {
        DBG("open %s fail.\n", SYSFS_DOLBYVISION_SUPPORT_INFO);
        return false;
    } else {
        if ((len = read(fd, buf, 1024)) < 0) {
            DBG("read %s error: %s\n", SYSFS_DOLBYVISION_SUPPORT_INFO, strerror(errno));
            ret =  false;
        } else {
            sscanf(buf, "%d", &supportInfo);
            if ((supportInfo & dvDriverEnabled) == 0)
                DBG("dolby vision driver is not ready\n");
            ret = ((supportInfo & dvSupported) == dvSupported) ? true : false;
        }

        close(fd);
        return ret;
    }
}

/***
  * @Info   : This funtion will enable DolbyVision core.
  */
#if 0
int enableDolbyVisionCore(void)
{
    /* For current policy, we always use dv as first priority, so
     * enable here. If we need hdr, keep it disable, and enable when
     * customer set dv as prefer.
     */
    int fd = -1;
    int ret = 0;
    drmModeRes * res = NULL;
    fd = openDefaultDRMDevice();
    if (fd < 0) {
        ERR("can not open %s\n", DEFUALT_DRM_DEVICE);
        ret = -1;
        goto err;
    }

    res = drmModeGetResources(fd);
    if (res == NULL) {
        ERR("can not get drmModeRes\n");
        ret = -1;
        goto err;
    }
    if (res && (fd > 0)) {
        bool bDvValid = getDvSupportStatus();
        int value = bDvValid ? 1 : 0;

        for (int i = 0; i < res->count_encoders; i++) {
            drmModeEncoder * enc = drmModeGetEncoder(fd, res->encoders[i]);
            if (enc == nullptr) {
                ERR("failed to get encoder\n");
                continue;
            }

            if (!(enc->possible_crtcs & (1 << i))) {
                drmModeFreeEncoder(enc);
                continue;
            }

            char cmd[256];
            snprintf(cmd, sizeof(cmd) - 1,
                    "export XDG_RUNTIME_DIR=/run; westeros-gl-console set property -s %d:dv_enable:%d 2>&1",
                    enc->crtc_id, value);
#if 1
            FILE * pipe = NULL;
            pipe = popen(cmd, "r");
            if (pipe) {
                char buf[256];
                while(fgets(buf, sizeof(buf), pipe)) {
                    DBG("cmd out: %s\n", buf);
                    memset(buf, 0, sizeof(buf));
                }
                ret = WEXITSTATUS(pclose(pipe));
            }else{
                ERR("popen failed, %d %s\n", errno, strerror(errno));
            }
#else
            ret = WEXITSTATUS(system(cmd));
#endif
            DBG("enable DV core cmd: %s, result: %d errno %d %s\n", cmd, ret, errno, strerror(errno));
            drmModeFreeEncoder(enc);
        }

    }else{
        ERR("input drmModeRes is null, please check fun:%s\n", __FUNCTION__);
    }

err:
    if (res)
        drmModeFreeResources(res);
    if (fd > 0)
        closeDefaultDRMDevice(fd);

    return ret == 0 ? 1 : 0;
}
#else
int enableDolbyVisionCore(void){
    uint32_t retValue = 0x55;
    bool bDvValid = getDvSupportStatus();
    int value = bDvValid ? 1 : 0;

    if (value) {
        meson_drm_set_prop(ENUM_DRM_PROP_HDMI_DV_ENABLE, value);
        meson_drm_get_prop(ENUM_DRM_PROP_HDMI_DV_ENABLE, &retValue);
    }else{
        retValue = value;
    }

    fprintf(stderr, "DSHAL: retValue %d, value %d\n", retValue, value);
    return retValue == value ? 1 : 0;
}
#endif

drmModeConnector * dsDRMGetHDMIConnector(void)
{
    CHECK_DRM_INITIALIZE(drmInitialized);
    return getDrmConnector(connector_id);
}

void drmRefreshConnector(void)
{
    int drmFD = -1;
    drmModeConnector *tempConn = NULL;
    bool acquiredConnector = false;
    drmModeRes * res = NULL;

    CHECK_DRM_INITIALIZE(drmInitialized);

    drmFD = openDefaultDRMDevice();
    if (drmFD < 0) {
        printf("cannot open %s\n", DEFUALT_DRM_DEVICE);
        goto out;
    }

    pthread_mutex_lock(&lock);

    /* retrieve resources */
    res = drmModeGetResources(drmFD);
    if (!res) {
        fprintf(stderr, "cannot retrieve DRM resources (%d): %m\n",
                errno);
        goto out;
    }

    while (!acquiredConnector) {
        for (int i = 0; i < res->count_connectors; ++i) {
            /* get information for each connector */
            tempConn = drmModeGetConnector(drmFD, res->connectors[i]);
            if (!tempConn) {
                fprintf(stderr, "cannot retrieve DRM connector %u:%u (%d): %m\n",
                        i, res->connectors[i], errno);
                continue;
            }
            if (tempConn->connector_type == DRM_MODE_CONNECTOR_HDMIA) {  //Save connector pointer for HDMI Tx
                //hdmiConn = tempConn;
                //tempConn = NULL;
                connector_id = tempConn->connector_id;
                acquiredConnector = true;
                drmModeFreeConnector(tempConn);
                break;
            }
            drmModeFreeConnector(tempConn);
        }
    }

    if (res)
        drmModeFreeResources(res);
out:
    pthread_mutex_unlock(&lock);
    closeDefaultDRMDevice(drmFD);
}

/*  @Info   : This thread notifies HDCP changes of DRM subsystem at boot-up. */
static void * process_hdcp_dolbyVisonEnable(void * arg){
    dsError_t hdcpRet = dsERR_NONE;
    dsHdcpStatus_t hdcpStatus;
    int hdcpHandle = 0;
    int dvEnable = 0;
    int hdcpNotified = 0;
    hdcpRet = dsGetVideoPort(dsVIDEOPORT_TYPE_HDMI, 0, &hdcpHandle);
    if(hdcpRet == dsERR_NONE) {
        bool connected = false;
        hdcpRet = dsIsDisplayConnected(hdcpHandle, &connected);
        if (hdcpRet==dsERR_NONE) {
           int cnt = 0;
           while(1) {
               if ((hdcpNotified == 0) && connected) {
                   hdcpRet = dsGetHDCPStatus(hdcpHandle, &hdcpStatus);
                   if((hdcpRet==dsERR_NONE)&&(hdcpStatus==dsHDCP_STATUS_AUTHENTICATED)) {
                       if(hdcpStatusCallback && hdcpHandle && isValidVopHandle(hdcpHandle)) {
                           hdcpStatusCallback(hdcpHandle, hdcpStatus);
                           hdcpNotified = 1;
                           fprintf(stderr, "DSHAL: spend %d ms to detect hdcp status authenticated\n", cnt*100);
                           //break;
                       }
                   }
               }

               if (dvEnable == 0){
                   dvEnable = enableDolbyVisionCore();
                   if (dvEnable)
                       fprintf(stderr, "DSHAL: spend %d ms to enable dolbyVision Core\n", cnt*100);
               }

               if (dvEnable && hdcpNotified){
                   fprintf(stderr, "DSHAL: spend %d ms to detect hdcp status authenticated and enable dolbyVison core\n", cnt*100);
                   break;
               }
               usleep(100*1000);
               cnt ++;//0.1 seconds per count
               if(cnt >= 3000){//wait 300 seconds, about 5 minutes
                   fprintf(stderr, "thread exit, dvEnable %d, hdcpNotified %d\n", dvEnable, hdcpNotified);
                   break;
               }
           }
        }else{
            fprintf(stderr, "DSHAL: hdmi cable not connected\n");
        }
    }else{
        fprintf(stderr, "DSHAL: get hdmi connect status error\n");
    }
    return NULL;
}


/***
 * @Info   : This thread monitors udev events of DRM subsystem for hotplug events
 *           and if a "change" event is received; it will refresh the display handles.
 */
static void* drm_refresh_thread(void *arg)
{
    struct udev *udev = NULL;
    struct udev_device *dev = NULL;
    struct udev_monitor *mon = NULL;

    /* create udev object */
    udev = udev_new();
    if (!udev) {
        printf("drm_refresh_thread: Can't create udev monitor for DRM,\n");
    } else {
        mon = udev_monitor_new_from_netlink(udev, LIBUDEV_EVT_TYPE_KERNEL);
        if (mon) {
            int fd = -1;
            fd_set fds;
            struct timeval tv;
            int ret;

            fd = udev_monitor_get_fd(mon);
            if (fd < 0) {
                printf("ERROR[%s:%d] udev_monitor_get_fd failed\n", __FUNCTION__, __LINE__);
            } else {
                if (udev_monitor_filter_add_match_subsystem_devtype(mon, LIBUDEV_SUBSYSTEM_DRM, NULL) < 0) {
                    printf("ERROR[%s:%d] udev_monitor_filter_add_match_subsystem_devtype failed\n", __FUNCTION__, __LINE__);
                } else {
                    if (udev_monitor_enable_receiving(mon) < 0) {
                        printf("ERROR[%s:%d] udev_monitor_enable_receiving failed\n", __FUNCTION__, __LINE__);
                    } else {
                        while (drmRefreshAlive) {
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
                                        drmRefreshConnector();
                                        call_hdcpstatus_callback();
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
    pthread_exit(NULL);
    return NULL;
}

#endif /* AML_STB */

dsError_t  dsVideoPortInit()
{
    dsError_t ret = dsERR_NONE;
    _handles[dsVIDEOPORT_TYPE_HDMI][0].vType  = dsVIDEOPORT_TYPE_HDMI;
    _handles[dsVIDEOPORT_TYPE_HDMI][0].nativeHandle = dsVIDEOPORT_TYPE_HDMI;
    _handles[dsVIDEOPORT_TYPE_HDMI][0].index = 0;
    _handles[dsVIDEOPORT_TYPE_HDMI][0].IsEnabled = true;


#ifdef AML_STB

    int drmFD = -1;
    drmModeConnector *hdmiConn = NULL;
    pthread_mutex_lock(&initLock);
    if(!drmInitialized) {
        bool acquiredConnector = false;
        drmModeRes * res = NULL;
        drmFD = openDefaultDRMDevice();
        if (drmFD < 0) {
            printf("cannot open %s\n", DEFUALT_DRM_DEVICE);
            ret = dsERR_GENERAL;
            goto out;
        }
        /* retrieve resources */
        res = drmModeGetResources(drmFD);
        if (!res) {
            fprintf(stderr, "cannot retrieve DRM resources (%d): %m\n",
                    errno);
            ret = dsERR_GENERAL;
            goto out;
        }

        while(!acquiredConnector) {
            for (int i = 0; i < res->count_connectors; ++i) {
                /* get information for each connector */
                hdmiConn = drmModeGetConnector(drmFD, res->connectors[i]);
                if (!hdmiConn) {
                    fprintf(stderr, "cannot retrieve DRM connector %u:%u (%d): %m\n",
                            i, res->connectors[i], errno);
                    continue;
                }
                if (hdmiConn->connector_type == DRM_MODE_CONNECTOR_HDMIA) {  //Save connector pointer for HDMI Tx
                    DBG("Acquired connector\n");
                    acquiredConnector = true;
                    connector_id = hdmiConn->connector_id;
                    drmModeFreeConnector(hdmiConn);
                    break;
                }

                drmModeFreeConnector(hdmiConn);
                continue;
            }
        }

        drmRefreshAlive = true;
        int err = pthread_create (&drm_refresh_threadId, NULL, drm_refresh_thread, NULL);
        if(err) {
            printf("DSHAL : Failed to Ceate DRM refresh Thread ....\r\n");
            drm_refresh_threadId = -1;
            ret = dsERR_GENERAL;
        }

        pthread_t notify_threadId = -1;
        err = pthread_create(&notify_threadId, NULL, process_hdcp_dolbyVisonEnable, NULL);
        if (err){
            fprintf(stderr, "DSHAL: Failed to create notify hdcp status thread\n");
            ret = dsERR_GENERAL;
        }else{
            err = pthread_detach(notify_threadId);
            if (err){
                fprintf(stderr, "DSHAL: detach notify hdcp status thread error\n");
                ret = dsERR_GENERAL;
            }
        }

        drmInitialized = true;
        pthread_cond_signal(&initCond);
        //call_hdcpstatus_callback();
        DBG("Total number of available connectors : %d\n", res->count_connectors);

        if (res)
            drmModeFreeResources(res);
    }
    pthread_mutex_unlock(&initLock);
out:
    closeDefaultDRMDevice(drmFD);
#endif /* AML_STB */
    DBG("valid handle:\n");
    int i = 0;
    for(i = 0; i < dsVIDEOPORT_TYPE_MAX; i ++){
        fprintf(stderr, "port: %d, handle: 0x%x\n", i, (int)&_handles[i][0]);
    }
    return ret;
}


dsError_t  dsGetVideoPort(dsVideoPortType_t type, int index, int *handle)
{
    dsError_t ret = dsERR_NONE;
#ifdef AML_STB
    CHECK_DRM_INITIALIZE(drmInitialized);
#endif
    if (index != 0 || NULL == handle || !dsVideoPortType_isValid(type) || !IS_VIDEO_PORT_TYPE_SUPPORT(type)) {
        fprintf(stderr, "in dsGetVideoPort invalid parameter\n");
        ret = dsERR_INVALID_PARAM;
    }
#ifdef AML_STB
    if (ret == dsERR_NONE && drmInitialized) {
#else
	if (ret == dsERR_NONE ) {
#endif
        *handle = (int)&_handles[type][index];
    }else{
        fprintf(stderr, "in dsGetVideoPort can not get valid handle\n");
        ret = dsERR_GENERAL;
    }
    DBG("dsGetVideoPort handle : 0x%x, index: %d\n", *handle, index);
    return ret;
}

dsError_t dsIsVideoPortEnabled(int handle, bool *enabled)
{
    dsError_t ret = dsERR_NONE;
#ifdef AML_STB
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;

    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }
    if (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI ) {
        DBG("HDMI TX port is : %d\n",vopHandle->IsEnabled);
        *enabled = vopHandle->IsEnabled;
    }
#endif
    return ret;
}

dsError_t  dsEnableVideoPort(int handle, bool enabled)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;
#ifdef AML_STB
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;
    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }

    if(vopHandle->vType == dsVIDEOPORT_TYPE_HDMI) {
        char buffer[512] = {'\0'};
        if (enabled == false) {
            DBG("Disabling LCD.................\n");
            snprintf(buffer, sizeof(buffer)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set display enable 0 | grep \"Response\"",
                    XDG_RUNTIME_DIR);
        } else {
            DBG("Enabling  LCD.................\n");
            snprintf(buffer, sizeof(buffer)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set display enable 1 | grep \"Response\"",
                    XDG_RUNTIME_DIR);
        }
        /* FIXME: popen in use */
        FILE* fp = popen(buffer, "r");
        if (NULL != fp) {
            char output[64] = {'\0'};
            while (fgets(output, sizeof(output)-1, fp)) {
                if (strlen(output) && strnstr(output, "[0:", 20)) {
                    //set IsEnabled flag if command was executed successfully
                    vopHandle->IsEnabled = enabled;
                    ret = dsERR_NONE;
                } else {
                    ret = dsERR_GENERAL;
                }
            }
            pclose(fp);
        } else {
            printf("DS_HAL: popen failed\n");
            ret = dsERR_GENERAL;
        }

    }
#endif
    return ret;
}

dsError_t  dsIsDisplayConnected(int handle, bool *connected)
{
    dsError_t ret = dsERR_NONE;
#ifdef AML_STB
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;

    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }

    char strStatus[13] = {'\0'};
    //drmModeConnector *conn;
    if (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI ) {
        amsysfs_get_sysfs_str(SYSFS_DRM_CARD0_HDMISTATUS, strStatus, sizeof(strStatus));
        //    conn = drmModeGetConnector(drmFD, hdmiConn->connector_id);
        //   if(conn->connection == DRM_MODE_CONNECTED) {
        if(strncmp(strStatus,"connected",9) == 0) {
            *connected = true;
        }
        else {
            *connected = false;
        }
        DBG("dsIsDisplayConnected: %d, %s, len %d\n", *connected, strStatus, strlen(strStatus));
    }
#else
    *connected = true; //Display always connected for Panel
#endif
    return ret;
}

dsError_t  dsEnableDTCP(int handle, bool contentProtect)
{
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t  dsEnableHDCP(int handle, bool contentProtect,
        char *hdcpKey, size_t keySize)
{
    dsError_t ret = dsERR_NONE;
    return ret;
}

dsError_t  dsIsDTCPEnabled (int handle, bool* pContentProtected)
{
    return dsERR_OPERATION_NOT_SUPPORTED;
}

/**
 * @brief Register a callback function to listen for HDCP status.
 *
 * This function registers a callback function for getting the HDCP status on HDMI Ports
 *
 * @note Application should install at most one callback function per handle. Multiple listeners are supported at application layer and thus not required in HAL implementation.
 * @param [in] handle    Handle for the output video port
 * @param cb    The callback function
 * @return dsError_t Error code.
 */

dsError_t dsRegisterHdcpStatusCallback(int handle, dsHDCPStatusCallback_t cb)
{
    dsError_t error = dsERR_NONE;

    if( NULL == cb )
        error = dsERR_INVALID_PARAM;

    hdcpStatusCallback = cb;

    call_hdcpstatus_callback();

    return error;
}


dsError_t  dsIsHDCPEnabled (int handle, bool* pContentProtected)
{
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *)handle;

    if (pContentProtected && isValidVopHandle(handle) && (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI)) {
#ifdef AML_STB
        char buffer[64] = {'\0'};
        amsysfs_get_sysfs_str(SYSFS_HDMITX_HDCPMODE, buffer, sizeof(buffer)-1);
        DBG("buffer : '%s'\n", buffer);
        if (strnstr(buffer, "22", sizeof(buffer))) {
            *pContentProtected = true;
        } else if (strnstr(buffer, "14", sizeof(buffer))) {
            *pContentProtected = true;
        } else {
            *pContentProtected = false;
        }
#else /* !AML_STB */
        *pContentProtected = false;
#endif /* !AML_STB */
    } else {
        ret = dsERR_INVALID_PARAM;
    }
    return ret;
}

dsError_t dsGetResolution(int handle, dsVideoPortResolution_t *resolution)
{
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;
    DBG("Enter...\n");
#ifdef AML_STB
    CHECK_DRM_INITIALIZE(drmInitialized);
#endif
    if (!isValidVopHandle(handle)) {
        fprintf(stderr, "dsGetResolution invalid handle 0x%x\n", handle);
        return dsERR_INVALID_PARAM;
    }

#ifdef AML_STB
    int drmFD = -1;
    drmModeConnector *hdmiConn = NULL;
    drmConnectorModes mode;
    drmModeRes * res = NULL;
    const char *resName = "unknown resolution";

    drmFD = openDefaultDRMDevice();
    if (drmFD < 0) {
        printf("cannot open %s\n", DEFUALT_DRM_DEVICE);
        ret = dsERR_GENERAL;
        goto out;
    }

    res = drmModeGetResources(drmFD);
    if (!res) {
        fprintf(stderr, "cannot retrieve DRM resources (%d): %m\n",
                errno);
        ret = dsERR_GENERAL;
        goto out;
    }

    //    const char *resName = NULL;
    if (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI ) {
        if((hdmiConn = drmModeGetConnector(drmFD, connector_id)) &&
                getCurrentDRMResolution (drmFD, res, hdmiConn, &mode)) {
            switch(mode) {
                case drmMode_480i :
                    resName = "480i";
                    break;
                case drmMode_480p :
                    resName = "480p";
                    break;
                case drmMode_576p :
                    resName = "576p50";
                    break;
                case drmMode_720p24 :
                    resName = "720p24";
                    break;
                case drmMode_720p25 :
                    resName = "720p25";
                    break;
                case drmMode_720p30 :
                    resName = "720p30";
                    break;
                case drmMode_720p50 :
                    resName = "720p50";
                    break;
                case drmMode_720p :
                    resName = "720p60";
                    break;
                case drmMode_1080i :
                    resName = "1080i60";
                    break;
                case drmMode_1080i50 :
                    resName = "1080i50";
                    break;
                case drmMode_1080p24 :
                    resName = "1080p24";
                    break;
                case drmMode_1080p25 :
                    resName = "1080p25";
                    break;
                case drmMode_1080p30 :
                    resName = "1080p30";
                    break;
                case drmMode_1080p50 :
                    resName = "1080p50";
                    break;
                case drmMode_1080p :
                    resName = "1080p60";
                    break;
                case drmMode_3840x2160p24 :
                    resName = "2160p24";
                    break;
                case drmMode_3840x2160p25 :
                    resName = "2160p25";
                    break;
                case drmMode_3840x2160p30 :
                    resName = "2160p30";
                    break;
                case drmMode_3840x2160p50 :
                    resName = "2160p50";
                    break;
                case drmMode_3840x2160p60 :
                    resName = "2160p60";
                    break;
                case drmMode_4096x2160p24 :
                    resName = "4096x2160p24";
                    break;
                case drmMode_4096x2160p25 :
                    resName = "4096x2160p25";
                    break;
                case drmMode_4096x2160p30 :
                    resName = "4096x2160p30";
                    break;
                case drmMode_4096x2160p50 :
                    resName = "4096x2160p50";
                    break;
                case drmMode_4096x2160p60 :
                    resName = "4096x2160p60";
                    break;
                default:
                    if (hdmiConn->modes) {
                        printf(" DS_HAL unrecognized resoulution: %s, mode %d\n", hdmiConn->modes[0].name, mode);
                    }
                    break;
            }
            DBG("AML_STB dsGetResolution  : %s\n", resName);
            strncpy(resolution->name, resName, sizeof(resolution->name));
        }else{
            ERR("getCurrentDRMResolution failed \n");
        }
    }

out:
    drmModeFreeConnector(hdmiConn);
    if (res)
        drmModeFreeResources(res);
    closeDefaultDRMDevice(drmFD);
#else
    //    TODO: To be made API driven
    strncpy(resolution->name, "2160p60", sizeof(resolution->name));
    printf(" DS_HAL dsGetResolution for Panel : %s\n", resolution->name);
#endif
    return ret;
}

dsError_t  dsSetResolution(int handle, dsVideoPortResolution_t *resolution, bool persist)
{
    dsError_t ret = dsERR_NONE;
    int ResolutionCnt = sizeof(kResolutions)/sizeof(dsVideoPortResolution_t);
    int i=0;
    printf(" DS_HAL: dsSetResolution %s, To Persist: %d\n",resolution->name, persist);
#ifdef AML_STB

    if (!resolution) {
        ret = dsERR_INVALID_PARAM;
        return ret;
    }

    for (i = 0; i < ResolutionCnt; i++) {
        if ((kResolutions[i].pixelResolution == resolution->pixelResolution) &&
            (kResolutions[i].interlaced == resolution->interlaced) &&
            (kResolutions[i].aspectRatio == resolution->aspectRatio) &&
            (kResolutions[i].stereoScopicMode == resolution->stereoScopicMode))
            {
                break;
            }
    }
    if (i == ResolutionCnt)
    {
        return dsERR_INVALID_PARAM;
    }
    else {
        char cmdBuf[512] = {'\0'};
        int width = -1, height = -1;
        int rate = 60;
        char interlaced = 'n';

        if (sscanf (resolution->name, "%dx%dp%d", &width, &height, &rate) == 3)
        {
            interlaced = 'p';
        }
        else if (sscanf(resolution->name, "%dx%di%d", &width, &height, &rate ) == 3)
        {
            interlaced = 'i';
        }
        else if (sscanf(resolution->name, "%dx%dx%d", &width, &height, &rate ) == 3)
        {
            interlaced = 'p';
        }
        else if (sscanf(resolution->name, "%dx%d", &width, &height ) == 2)
        {
            interlaced = 'p';
        }
        else if (sscanf(resolution->name, "%dp%d", &height, &rate ) == 2)
        {
            interlaced = 'p';
            width= -1;
        }
        else if (sscanf(resolution->name, "%di%d", &height, &rate ) == 2)
        {
            interlaced = 'i';
            width= -1;
        }
        else if (sscanf(resolution->name, "%d%c", &height, &interlaced ) == 2)
        {
           width= -1;
           rate = 60;
        }

        //if width is missing, set it manualy
        if ( height > 0 )
        {
           if ( width < 0 )
           {
              switch ( height )
              {
                 case 480:
                 case 576:
                    width= 720;
                    break;
                 case 720:
                    width= 1280;
                    break;
                 case 1080:
                    width= 1920;
                    break;
                 case 1440:
                    width= 2560;
                    break;
                 case 2160:
                    width= 3840;
                    break;
                 case 2880:
                    width= 5120;
                    break;
                 case 4320:
                    width= 7680;
                    break;
                 default:
                    break;
              }
           }
        }

        snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set mode %dx%d%c%d | grep \"Response\"",
                XDG_RUNTIME_DIR, width,height,interlaced,rate);
        DBG("AML_STB Executing '%s'\n", cmdBuf);
        /* FIXME: popen in use */
        FILE* fp = popen(cmdBuf, "r");
        if (NULL != fp) {
            char output[64] = {'\0'};
            while (fgets(output, sizeof(output)-1, fp)) {
                if (strlen(output) && strnstr(output, "[0:", 20)) {
                    ret = dsERR_NONE;
                } else {
                    ret = dsERR_GENERAL;
                }
            }
            pclose(fp);
        } else {
            printf("DS_HAL: popen failed\n");
            ret = dsERR_GENERAL;
        }
    }
#endif
    return ret;
}

dsError_t dsSetBackgroundColor(int handle, dsVideoBackgroundColor_t color)
{
    dsError_t ret = dsERR_NONE;
    printf("DS_HAL: dsSetBackgroundColor %d\n",color);
#ifdef AML_STB
    if (!isValidVopHandle(handle)) {
        fprintf(stderr, "dsSetBackgroundColor invalid handle 0x%x\n", handle);
        return dsERR_INVALID_PARAM;
    }
    char cmdBuf[512] = {'\0'};
    if (color == dsVIDEO_BGCOLOR_BLUE)
    {
        snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set video-backcolor blue | grep \"Response\"",
            XDG_RUNTIME_DIR);
        printf("DS_HAL: dsSetBackgroundColor blue: %d\n",color);
    }
    else if ((color == dsVIDEO_BGCOLOR_BLACK) || (color == dsVIDEO_BGCOLOR_NONE))
    {
        snprintf(cmdBuf, sizeof(cmdBuf)-1, "export XDG_RUNTIME_DIR=%s;westeros-gl-console set video-backcolor black | grep \"Response\"",
            XDG_RUNTIME_DIR);
        printf("DS_HAL: dsSetBackgroundColor black: %d\n",color);
    }
    else
    {
        printf("DS_HAL: dsSetBackgroundColor invalid \n");
        return dsERR_INVALID_PARAM;
    }
    DBG("AML_STB Executing '%s'\n", cmdBuf);
    /* FIXME: popen in use */
    FILE* fp = popen(cmdBuf, "r");
    if (NULL != fp) {
        char output[64] = {'\0'};
        while (fgets(output, sizeof(output)-1, fp)) {
            if (strlen(output) && strnstr(output, "[0:", 20)) {
                ret = dsERR_NONE;
            } else {
                ret = dsERR_GENERAL;
            }
        }
        pclose(fp);
    } else {
        printf("DS_HAL: popen failed\n");
        ret = dsERR_GENERAL;
    }

#endif
    return ret;
}

/**
 * @brief Get current HDCP status
 *
 * This function will give the current HDCP status
 *
 * @param [in] handle    Handle for the output video port
 * @param [out] status   HDCP status
 * @return dsError_t Error code.
 */

dsError_t dsGetHDCPStatus(int handle, dsHdcpStatus_t *status)
{
    dsError_t error = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *)handle;
#ifdef AML_STB
    if (status && isValidVopHandle(handle) && vopHandle->vType == dsVIDEOPORT_TYPE_HDMI) {
        char buffer[64] = {'\0'};
        bool authenticated = false;
        amsysfs_get_sysfs_str(SYSFS_HDMITX_HDCPMODE, buffer, sizeof(buffer)-1);
        printf("dsGetHDCPStatus: buffer : '%s'\n", buffer);
        /* Expecting [succeed/fail] : sample [14 : succeed] or [22 : fail] */
        if (strnstr(buffer, "succeed", sizeof(buffer))) {
            authenticated = true;
        } else {
            authenticated = false;
        }
        if (strnstr(buffer, "22", sizeof(buffer)) && !authenticated) {
            /* 1.4 is authenticated but only 2.2 authentication failed. */
            authenticated = true;
        }
        if (authenticated) {
            *status = dsHDCP_STATUS_AUTHENTICATED;
        } else {
            *status = dsHDCP_STATUS_AUTHENTICATIONFAILURE;
        }
    } else {
	*status = dsHDCP_STATUS_AUTHENTICATIONFAILURE;
        error = dsERR_INVALID_PARAM;
    }
#endif /* !AML_STB */
    return error;
}

/***
 * Info : Get STB HDCP protocol version.
 */
dsError_t dsGetHDCPProtocol(int handle, dsHdcpProtocolVersion_t *protocolVersion)
{
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *)handle;

    if (protocolVersion && isValidVopHandle(handle) && vopHandle->vType == dsVIDEOPORT_TYPE_HDMI) {
#ifdef AML_STB
        /* STB shall always return 2.2 */
        *protocolVersion = dsHDCP_VERSION_2X;
        DBG("protocolVersion: '%d'\n", (int)*protocolVersion);
#else /* !AML_STB */
        ret = dsERR_OPERATION_NOT_SUPPORTED;
#endif /* !AML_STB */
    } else {
        ret = dsERR_INVALID_PARAM;
        printf("Error: Non HDMI Port or protocolVersion is NULL\n");
    }
    return ret;
}

/***
 * Info : Get Receiver/TV HDCP protocol version.
 */
dsError_t dsGetHDCPReceiverProtocol (int handle, dsHdcpProtocolVersion_t *protocolVersion)
{
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *)handle;

    if (protocolVersion && isValidVopHandle(handle) && vopHandle->vType == dsVIDEOPORT_TYPE_HDMI) {
#ifdef AML_STB
        char buffer[64] = {'\0'};
        /* dsHDCP_VERSION_1X is mandatory. */
        *protocolVersion = dsHDCP_VERSION_1X;

        amsysfs_get_sysfs_str(SYSFS_HDMITX_HDCPVERSION, buffer, sizeof(buffer)-1);
        printf("dsGetHDCPReceiverProtocol: buffer : '%s'\n", buffer);
        if (strnstr(buffer, "22", sizeof(buffer))) {
            *protocolVersion = dsHDCP_VERSION_2X;
        } else if (strnstr(buffer, "14", sizeof(buffer))) {
            *protocolVersion = dsHDCP_VERSION_1X;
        } else {
            /* Shall not reach here... */
            ret = dsERR_INVALID_PARAM;
            printf("AML_STB Error:Non HDMI Port or protocolVersion is NULL\n");
        }
#else /* !AML_STB */
        *protocolVersion = dsHDCP_VERSION_1X;
#endif /* !AML_STB */
    } else {
        ret = dsERR_INVALID_PARAM;
        printf("Error:Non HDMI Port or protocolVersion is NULL\n");
    }
    return ret;
}

/***
 * Info : Get current used HDCP protocol version.
 */
dsError_t dsGetHDCPCurrentProtocol (int handle, dsHdcpProtocolVersion_t *protocolVersion)
{
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *)handle;

    if (protocolVersion && isValidVopHandle(handle) && vopHandle->vType == dsVIDEOPORT_TYPE_HDMI) {
#ifdef AML_STB
        char buffer[64] = {'\0'};
        /* dsHDCP_VERSION_1X is mandatory. */
        *protocolVersion = dsHDCP_VERSION_1X;

        amsysfs_get_sysfs_str(SYSFS_HDMITX_HDCPVERSION, buffer, sizeof(buffer)-1);
        DBG("buffer : '%s'\n", buffer);
        if (strnstr(buffer, "22", sizeof(buffer)) && !strnstr(buffer, "fail", sizeof(buffer))) {
            *protocolVersion = dsHDCP_VERSION_2X;
        } else if (strnstr(buffer, "14", sizeof(buffer))) {
            *protocolVersion = dsHDCP_VERSION_1X;
        } else {
            /* Shall not reach here... */
            ret = dsERR_INVALID_PARAM;
            printf("AML_STB Error: Non HDMI Port or protocolVersion is NULL\n");
        }
#else /* !AML_STB */
        ret = dsERR_OPERATION_NOT_SUPPORTED;
#endif /* !AML_STB */
    } else {
        ret = dsERR_INVALID_PARAM;
        printf("Error: Non HDMI Port or protocolVersion is NULL\n");
    }
    return ret;
}

dsError_t dsSupportedTvResolutions(int handle, int *resolutions)
{
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;
    printf(" DS_HAL (%s)\n",__func__);

#ifdef AML_STB
    drmModeConnector *hdmiConn = NULL;
    CHECK_DRM_INITIALIZE(drmInitialized);

    if (resolutions && isValidVopHandle(handle) && (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI) &&
            (hdmiConn = getDrmConnector(connector_id))) {
        drmConnectorModes supportedModes[drmMode_Max] = {drmMode_Unknown};
        getSupportedDRMResolutions(hdmiConn, supportedModes);
        for(int i = 0; i<drmMode_Max; i++ ) {
            switch(supportedModes[i]) {
                case drmMode_480i:
                    *resolutions |= dsTV_RESOLUTION_480i;
                    break;
                case drmMode_480p:
                    *resolutions |= dsTV_RESOLUTION_480p;
                    break;
                case drmMode_576i:
                    *resolutions |= dsTV_RESOLUTION_576i;
                    break;
                case drmMode_576p:
                    *resolutions |= dsTV_RESOLUTION_576p;
                    break;
                case drmMode_720p:
                case drmMode_720p24:
                case drmMode_720p25:
                case drmMode_720p30:
                case drmMode_720p50:
                    *resolutions |= dsTV_RESOLUTION_720p;
                    break;
                case drmMode_1080i:
                    *resolutions |= dsTV_RESOLUTION_1080i;
                    break;
                case drmMode_1080p:
                case drmMode_1080p24:
                case drmMode_1080p25:
                case drmMode_1080p30:
                case drmMode_1080p50:
                    *resolutions |= dsTV_RESOLUTION_1080p;
                    break;
                case drmMode_3840x2160p24:
                case drmMode_3840x2160p25:
                case drmMode_3840x2160p30:
                case drmMode_3840x2160p50:
                case drmMode_4096x2160p24:
                case drmMode_4096x2160p25:
                case drmMode_4096x2160p30:
                case drmMode_4096x2160p50:
                    *resolutions |= dsTV_RESOLUTION_2160p30;
                case drmMode_3840x2160p60:
                case drmMode_4096x2160p60:
                    *resolutions |= dsTV_RESOLUTION_2160p60;
                    break;
                default:
                    *resolutions |= dsTV_RESOLUTION_480p;
                    break;
            }
        }
    }
    else {
        ret = dsERR_INVALID_PARAM;
    }

    drmModeFreeConnector(hdmiConn);
#else
    *resolutions |= dsTV_RESOLUTION_2160p60;
    printf("DS_HAL:%s: Supported TV Resolutions for Panel:  %x\n",__func__,*resolutions);
#endif
    return ret;
}

dsError_t dsGetTVHDRCapabilities(int handle, int *capabilities)
{
    *capabilities = dsHDRSTANDARD_NONE;
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;
#ifdef AML_STB
    CHECK_DRM_INITIALIZE(drmInitialized);
#endif
    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }

#ifdef AML_STB
    int drmFD = -1;
    drmModeConnectorPtr hdmiConn = NULL;
    unsigned char* edid_buffer = NULL;
    edid_data_t* data_ptr = NULL;
    int length = 0;
    drmConnectorModes supportedModes[drmMode_Max] = {drmMode_Unknown};
    drmModePropertyPtr props = NULL;
    drmModePropertyBlobPtr blob = NULL;

    drmFD = openDefaultDRMDevice();
    if (drmFD < 0) {
        printf("%s:%d: cannot open %s\n", __func__, __LINE__, DEFUALT_DRM_DEVICE);
        ret = dsERR_GENERAL;
        goto out;
    }

    data_ptr = (edid_data_t*)malloc(sizeof(struct edid_data_t));
    if((hdmiConn = drmModeGetConnector(drmFD, connector_id)) && (hdmiConn->connection == DRM_MODE_CONNECTED)) {
        for (int i=0; i<hdmiConn->count_props;i++) {
            props = drmModeGetProperty(drmFD, hdmiConn->props[i]);
            if(props && !strcmp(props->name,"EDID") && (props->flags & DRM_MODE_PROP_BLOB)) {
                blob = drmModeGetPropertyBlob(drmFD, hdmiConn->prop_values[i]);
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

    if (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI ) {
        edid_status_e status = EDID_Parse(edid_buffer, length, data_ptr);
        printf(" DS_HAL: %s hdr_capabilities %d\n",__func__, data_ptr->hdr_capabilities);
        if (data_ptr->hdr_capabilities) {
            if (data_ptr->hdr_capabilities & HDR_standard_HDR10) *capabilities |= dsHDRSTANDARD_HDR10;
            if (data_ptr->hdr_capabilities & HDR_standard_HLG) *capabilities |= dsHDRSTANDARD_HLG;
            printf(" DS_HAL: dsGetTVHDRCapabilities add hdr capabilities %d\n", *capabilities);
        }
        printf(" DS_HAL: %s dv_capabilities %d\n",__func__, data_ptr->dv_capabilities);
        if (data_ptr->dv_capabilities && isDolbyVisionDriverLoaded(DOLBY_VISION_KERNEL_MODULE_NAME)) {
            *capabilities |= dsHDRSTANDARD_DolbyVision;
            printf(" DS_HAL: dsGetTVHDRCapabilities add dv capabilities %d\n", *capabilities);
        }
    }

out:
    free(edid_buffer);
    free(data_ptr);
    closeDefaultDRMDevice(drmFD);
    drmModeFreeConnector(hdmiConn);
#else
    *capabilities |= dsHDRSTANDARD_HDR10;
    *capabilities |= dsHDRSTANDARD_HLG;
    *capabilities |= dsHDRSTANDARD_DolbyVision;
#endif
    return ret;
}

dsError_t dsSetActiveSource(int handle)
{
    dsError_t ret = dsERR_OPERATION_NOT_SUPPORTED;
    return ret;
}

dsError_t dsIsVideoPortActive(int handle, bool *active)
{
    printf(" DS_HAL: %s\n",__func__);
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;
#ifdef AML_STB
    CHECK_DRM_INITIALIZE(drmInitialized);
#endif
    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }

#ifdef AML_STB
    int drmFD = -1;
    drmModePropertyPtr props = NULL;
    drmModeConnectorPtr hdmiConn = NULL;

    drmFD = openDefaultDRMDevice();
    if (drmFD < 0) {
        printf("%s:%d: cannot open %s\n", __func__, __LINE__, DEFUALT_DRM_DEVICE);
        ret = dsERR_GENERAL;
        goto out;
    }

    if (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI ) {
        if((hdmiConn = drmModeGetConnector(drmFD, connector_id)) && (hdmiConn->connection == DRM_MODE_CONNECTED)) {
            printf(" DS_HAL: HDMI Connected\n");
            for(int i=0; i<hdmiConn->count_props; i++){
                props = drmModeGetProperty(drmFD, hdmiConn->props[i]);
                if(props && !strcmp(props->name,"DPMS")){
                    printf(" DS_HAL: Current DPMS value: %d\n",hdmiConn->prop_values[i]);
                    if(hdmiConn->prop_values[i] == DRM_MODE_DPMS_ON) {
                        *active = true;
                    }
                    else {
                        *active = false;
                    }
                    break;
                }
            }
            drmModeFreeProperty(props);
        }
        else {
            *active = false;
        }
    }
out:
    closeDefaultDRMDevice(drmFD);
    drmModeFreeConnector(hdmiConn);
#else
    printf("DS_HAL: %s Return True for Panel\n",__func__);
    *active = true;
#endif
    return ret;
}

dsError_t dsIsOutputHDR(int handle, bool *hdr)
{
    dsError_t ret = dsERR_NONE;
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;
    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }
    //TODO: Need to check hdr_capabilities
    //Now we hack to support always so Amazon content server will send HDR10 or DV
    *hdr = true;
    return ret;
}

dsError_t dsResetOutputToSDR()
{
    dsError_t ret = dsERR_NONE;
    return ret;
}

dsError_t  dsVideoPortTerm()
{
    dsError_t ret = dsERR_NONE;
#ifdef AML_STB
    if(drmInitialized) {
        /* free resources */
        printf(" DS_HAL: Close down DRM syste\n");
        //drmModeFreeConnector(hdmiConn);
        //drmModeFreeResources(res);
        drmRefreshAlive = false;
        drmInitialized = false;
        if ((-1 != drm_refresh_threadId) && (0 != pthread_join(drm_refresh_threadId, NULL))) {
            printf("ERROR[%s:%d] : pthread_join drm_refresh_thread failed\n", __FUNCTION__, __LINE__);
            ret = dsERR_GENERAL;
        }
    }
#endif
    return ret;
}

dsError_t dsSetHdmiPreference(int handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol)
{
    dsError_t ret = dsERR_NONE;
    /* FIXME: identify possiblility of setting preffered HDMI mode. */
    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }
    if (!hdcpCurrentProtocol) {
        ret = dsERR_INVALID_PARAM;
    }

    if ((*hdcpCurrentProtocol == dsHDCP_VERSION_1X) || (*hdcpCurrentProtocol == dsHDCP_VERSION_2X))
    {
        hdcp_version = *hdcpCurrentProtocol;
        ret = dsERR_NONE;
    }
    else
    {
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}

dsError_t dsGetHdmiPreference(int handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol)
{
    dsError_t ret = dsERR_NONE;
    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }

    if (!hdcpCurrentProtocol) {
        ret = dsERR_INVALID_PARAM;
    } else {
        /* FIXME: identify possiblility of getting preffered HDMI mode.
           Assuming STB always go for 2.2 */
        *hdcpCurrentProtocol = hdcp_version;
    }
    return ret;
}

dsError_t dsGetCurrentOutputSettings(int handle,
    dsHDRStandard_t* video_eotf, dsDisplayMatrixCoefficients_t* matrix_coefficients,
    dsDisplayColorSpace_t* color_space, unsigned int* color_depth, dsDisplayQuantizationRange_t* quantization_range){
    dsError_t ret = dsERR_INVALID_PARAM;
    VOPHandle_t *vopHandle = (VOPHandle_t *)handle;

    if (isValidVopHandle(handle) && \
        video_eotf && matrix_coefficients && color_space && color_depth &&
        (vopHandle->vType == dsVIDEOPORT_TYPE_HDMI)) {
#ifdef AML_STB
        char buffer[32] = {'\0'};
        char * colorSpace = NULL;
        char * colorDepth = NULL;
        char * p = NULL;
        amsysfs_get_sysfs_str(SYSFS_HDMITX_ATTR, buffer, sizeof(buffer)-1);
        p = strchr(buffer, ',');
        *p = '\0';
        colorSpace = buffer;
        colorDepth = ++p;
        fprintf(stderr, "colorSpace %s, colorDepth %s\n", colorSpace, colorDepth);
        if (!strcasecmp(colorSpace, "rgb")){
            *color_space = dsDISPLAY_COLORSPACE_RGB;
        }else if (!strcasecmp(colorSpace, "422")){
            *color_space = dsDISPLAY_COLORSPACE_YCbCr422;
        }else if (!strcasecmp(colorSpace, "444")){
            *color_space = dsDISPLAY_COLORSPACE_YCbCr444;
        }else if (!strcasecmp(colorSpace, "420")){
            *color_space = dsDISPLAY_COLORSPACE_YCbCr420;
        }else{
            *color_space = dsDISPLAY_COLORSPACE_AUTO;
        }

        *color_depth = strtol(colorDepth, NULL, 10);

        amsysfs_get_sysfs_str(SYSFS_HDMITX_HDR_STATUS, buffer, sizeof(buffer)-1);
        if(!strcasecmp(buffer, "SDR")){
            *video_eotf = dsHDRSTANDARD_NONE;
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_BT_709;
        }else if(!strcasecmp(buffer, "hdr10-gamma_hlg")){
            *video_eotf = dsHDRSTANDARD_HLG;
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_BT_2020_CL;
        }else if(!strcasecmp(buffer, "HDR10-GAMMA_ST2084") ||
                !strcasecmp(buffer, "HDR10-others") ||
                strcasecmp(buffer, "HDR10Plus-VSIF")){
            *video_eotf = dsHDRSTANDARD_HDR10;
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_BT_2020_CL;
        }else if(!strcasecmp(buffer, "DolbyVision-Lowlatency") ||
                !strcasecmp(buffer, "DolbyVision-Std")){
            *video_eotf = dsHDRSTANDARD_DolbyVision;
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_UNKNOWN;
        }
#endif
        ret = dsERR_NONE;
    }

    return ret;
}

dsError_t dsGetMatrixCoefficients(int handle, dsDisplayMatrixCoefficients_t *matrix_coefficients)
{
    dsError_t ret = dsERR_INVALID_PARAM;
    VOPHandle_t *vopHandle = (VOPHandle_t *)handle;

    char buffer[32] = {'\0'};

    if (isValidVopHandle(handle) && matrix_coefficients )
    {
        #ifdef AML_STB
        amsysfs_get_sysfs_str(SYSFS_HDMITX_HDR_STATUS, buffer, sizeof(buffer)-1);

        if (!strcasecmp(buffer, "SDR")) {
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_BT_709;
        }else if(!strcasecmp(buffer, "hdr10-gamma_hlg")){
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_BT_2020_CL;
        }else if(!strcasecmp(buffer, "HDR10-GAMMA_ST2084") ||
                !strcasecmp(buffer, "HDR10-others") ||
                strcasecmp(buffer, "HDR10Plus-VSIF")){
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_BT_2020_CL;
        }else if(!strcasecmp(buffer, "DolbyVision-Lowlatency") ||
                !strcasecmp(buffer, "DolbyVision-Std")){
            *matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_UNKNOWN;
        }

        ret = dsERR_NONE;
        #endif //AML_STB
    }
    return ret;
}

dsError_t dsGetColorDepth(int handle, unsigned int* color_depth)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    (void) handle;
    dsError_t ret = dsERR_NONE;
    #ifdef AML_STB
    int err = 0;
    char buffer[256];
    int cnt = 0;
    if (color_depth == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    *color_depth = 0;

    if ( dsERR_NONE == ret ) {
        FILE * fp = NULL;

        fp = fopen(SYSFS_HDMITX_ATTR, "r");
        if (fp) {
            char * str = NULL;
            char * pch = NULL;
            cnt = 0;
            while (str = fgets(buffer, sizeof(buffer), fp)) {
                int i = 0;
                //in the string read from file, contain '\0', should remove it
                while (str[i] != '\n') {
                    if (str[i] == '\0') {
                        str[i] = ' ';
                    }
                    i ++;
                    if (i > 256) {
                        fprintf(stderr, "error!!! too much characters in %s\n", str);
                        break;
                    }
                }
                //buffer example: 420,10bit
                pch = strchr(buffer,',');

                if (pch) {
                    pch++; //move pointer after ','
                    //convert 10bit to iteger value
                    *color_depth = (unsigned int) strtol(pch,NULL,10);
                }
                cnt ++;
                if (cnt > 64) {
                    fprintf(stderr, "error!!! read 64 strings, perhaps too much strings in %s\n", SYSFS_HDMITX_ATTR);
                    break;
                }
            }
            fclose(fp);
        }else{
            fprintf(stderr, "open file %s error %s\n", SYSFS_HDMITX_ATTR, strerror(errno));
            ret = dsERR_GENERAL;
        }
    }
    #endif
    return ret;
}
dsError_t dsGetColorSpace(int handle, dsDisplayColorSpace_t* color_space)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    (void) handle;
    dsError_t ret = dsERR_NONE;
#ifdef AML_STB
    int err = 0;
    char buffer[256];
    int cnt = 0;
    if (color_space == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    *color_space = dsDISPLAY_COLORSPACE_UNKNOWN;

    if (dsERR_NONE == ret) {
        FILE * fp = NULL;

        fp = fopen(SYSFS_HDMITX_ATTR, "r");
        if (fp) {
            char * str = NULL;
            cnt = 0;
            while (str = fgets(buffer, sizeof(buffer), fp)) {
                int i = 0;
                //in the string read from file, contain '\0', should remove it
                while (str[i] != '\n') {
                    if (str[i] == '\0') {
                        str[i] = ' ';
                    }
                    i ++;
                    if (i > 256) {
                        fprintf(stderr, "error!!! too much characters in %s\n", str);
                        break;
                    }
                }
                //buffer example: 420,10bit
                if (strcasestr(str, "420")) {
                    *color_space = dsDISPLAY_COLORSPACE_YCbCr420;
                }
                if (strcasestr(str, "444")) {
                    *color_space = dsDISPLAY_COLORSPACE_YCbCr444;
                }
                if (strcasestr(str, "422")) {
                    *color_space = dsDISPLAY_COLORSPACE_YCbCr422;
                }
                if (strcasestr(str, "RGB")) {
                    *color_space = dsDISPLAY_COLORSPACE_RGB;
                }
                if (strcasestr(str, "auto")) {
                    *color_space = dsDISPLAY_COLORSPACE_AUTO;
                }
                cnt ++;
                if (cnt > 64) {
                    fprintf(stderr, "error!!! read 64 strings, perhaps too much strings in %s\n", SYSFS_HDMITX_ATTR);
                    break;
                }
            }
            fclose(fp);
        }else{
            fprintf(stderr, "open file %s error %s\n", SYSFS_HDMITX_ATTR, strerror(errno));
            ret = dsERR_GENERAL;
        }
    }
#endif
    return ret;
}

dsError_t dsIsDisplaySurround(int handle, bool *surround)
{
    (void) handle;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    (void) handle;
    dsError_t ret = dsERR_NONE;
#ifdef AML_STB
    int err = 0;
    char buffer[256];
    int cnt = 0;
    if (surround == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    *surround = false;

    if (dsERR_NONE == ret) {
        FILE * fp = NULL;

        fp = fopen(SYSFS_HDMITX_ATTR, "r");
        if (fp) {
            char * str = NULL;
            cnt = 0;
            while (str = fgets(buffer, sizeof(buffer), fp)) {
                int i = 0;
                //in the string read from file, contain '\0', should remove it
                while (str[i] != '\n') {
                    if (str[i] == '\0') {
                        str[i] = ' ';
                    }
                    i ++;
                    if (i > 256) {
                        fprintf(stderr, "error!!! too much characters in %s\n", str);
                        break;
                    }
                }
                //surround is supported if number of channels is greater than 2
                const char delim[2] = ",";
                char * token = NULL;
                int num_channels=0;

                token = strtok(buffer,delim);

                token = strtok(NULL,delim);

                if (token != NULL) {
                    //expected token example: "8 ch", or " 6 ch"
                    num_channels = (unsigned int) strtol(token,NULL,10);
                    if (num_channels > 2) {
                        *surround = true;
                        break;
                    }
                }

                cnt ++;
                if (cnt > 64) {
                    fprintf(stderr, "error!!! read 64 strings, perhaps too much strings in %s\n", SYSFS_HDMITX_ATTR);
                    break;
                }
            }
            fclose(fp);
        }else{
            fprintf(stderr, "open file %s error %s\n", SYSFS_HDMITX_ATTR, strerror(errno));
            ret = dsERR_GENERAL;
        }
    }
#endif
    return ret;
}

/**
 * @brief Get current video Electro-Optical Transfer Function (EOT) value;
 *
 * @param[in]  handle -  Handle of the display device.
 * @param[out] video_eotf - pointer to EOFF value
 *
 * @return Device Settings error code
 * @retval dsERR_NONE on success
 * @retval dsERR_GENERAL General failure.
 */
dsError_t dsGetVideoEOTF(int handle, dsHDRStandard_t *video_eotf)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    VOPHandle_t *vopHandle = (VOPHandle_t *) handle;

    if (!isValidVopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }

    if (video_eotf == NULL) {
       return dsERR_INVALID_PARAM;
    }

    *video_eotf = (dsHDRStandard_t)(dsHDRSTANDARD_NONE|dsHDRSTANDARD_HLG|dsHDRSTANDARD_HDR10|dsHDRSTANDARD_DolbyVision);

    printf("*video_eotf = %d\n",*video_eotf);

    return dsERR_NONE;
}
