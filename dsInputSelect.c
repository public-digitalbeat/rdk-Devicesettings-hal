//Considering HDMI SUPPORT will be on, there by default for TVs
// and COMPOSITE IN could be optional


//#define HAS_HDMI_IN_SUPPORT 1
#ifdef HAS_HDMI_IN_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "TvClientWrapper.h"
#include <dbus/dbus.h>
#include "dsHdmiIn.h"
#define HDMI_IN_PORT_INFO    "/dev/hdmirx0"
#define HDMI_IN_ARC_CAP_PORT (1)

#ifdef HAS_COMPOSITE_IN_SUPPORT
#include "dsCompositeIn.h"
#define AVIN_DETECT_PATH    "/dev/avin_detect"
#define AVIN_CHANNEL_MAX 2
#endif

/* Enable for debug tracing */
#define INPUT_SELECT_DEBUG
#ifdef INPUT_SELECT_DEBUG
#define INPUT_SELECT_TRACE(m) printf m
#else
#define INPUT_SELECT_TRACE(m)
#endif

#define INPUT_SELECT_ERROR(m) printf m
#define INPUT_SELECT_INFO(m) printf m

static pthread_mutex_t inputSelectMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t inputPortMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t inputSwitch_threadId;
bool isPresenting = false;
static tv_source_input_t selectedSource = SOURCE_INVALID;
struct TvClientWrapper_t *tvclient = NULL;

typedef enum _input_port_type_t
{
    eINPUT_PORT_TYPE_HDMI,
    eINPUT_PORT_TYPE_COMPOSITE,
    eINPUT_PORT_TYPE_INVALID
} input_port_type_t;


dsHdmiInConnectCB_t _halCBHDMI = NULL;
dsHdmiInSignalChangeCB_t _halSigChangeCB = NULL;
dsHdmiInStatusChangeCB_t _halStatusChangeCB = NULL;
static const int HDMI_DETECT_STATUS_BIT_A = 0x01;
static const int HDMI_DETECT_STATUS_BIT_B = 0x02;
static const int HDMI_DETECT_STATUS_BIT_C = 0x04;
static dsHdmiInStatus_t* hdmiInStatus = NULL;

static dsHdmiInPort_t TvClientPortTodsHDMIPort(tv_source_input_t tvPort)
{
    dsHdmiInPort_t ret = dsHDMI_IN_PORT_NONE;

    switch (tvPort)
    {
    case SOURCE_HDMI1:
        ret = dsHDMI_IN_PORT_0;
        break;
    case SOURCE_HDMI2:
        ret = dsHDMI_IN_PORT_1;
        break;
    case SOURCE_HDMI3:
        ret = dsHDMI_IN_PORT_2;
        break;
    default:
        ret = dsHDMI_IN_PORT_NONE;
    }

    return ret;
}

#ifdef HAS_COMPOSITE_IN_SUPPORT

enum avin_status_e {
    AVIN_STATUS_IN = 0,
    AVIN_STATUS_OUT = 1,
    AVIN_STATUS_UNKNOW = 2,
};
enum avin_channel_e {
    AVIN_CHANNEL1 = 0,
    AVIN_CHANNEL2 = 1,
};

struct report_data_s {
    enum avin_channel_e channel;
    enum avin_status_e status;
};


dsCompositeInConnectCB_t _halCBComposite = NULL;
static dsCompositeInStatus_t* compositeInStatus = NULL;

static dsCompositeInPort_t TvClientPortTodsCOMPOSITEPort(tv_source_input_t tvPort)
{
    dsCompositeInPort_t ret = dsCOMPOSITE_IN_PORT_NONE;

    switch (tvPort)
    {
    case SOURCE_AV1:
        ret = dsCOMPOSITE_IN_PORT_0;
        break;
    case SOURCE_AV2:
        ret = dsCOMPOSITE_IN_PORT_1;
        break;
    default:
        ret = dsCOMPOSITE_IN_PORT_NONE;
    }

    return ret;
}
#endif


static int SetOsdBlankStatus(const char *path, int cmd);
static int IntWriteSysfs(const char *path, int value);
static int StringWriteSysfs(const char *path, const char *cmd);


static input_port_type_t GetInputPortType(tv_source_input_t tvPort)
{
    input_port_type_t portType = eINPUT_PORT_TYPE_INVALID;
    switch(tvPort)
    {
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3:
        {
            portType = eINPUT_PORT_TYPE_HDMI;
            break;
        }
        case SOURCE_AV1:
        case SOURCE_AV2:
        {
            portType = eINPUT_PORT_TYPE_COMPOSITE;
            break;
        }
        default:
            break;
    }

    return portType;
}


static void TvEventCallback(event_type_t eventType, void *eventData)
{
    INPUT_SELECT_TRACE(("%s: eventType: %d.\n", __FUNCTION__, eventType));
    switch (eventType)
    {
        case TV_EVENT_TYPE_SIGLE_DETECT:
            {
                SignalDetectCallback_t *signalDetectEvent = ( SignalDetectCallback_t *)(eventData);
                INPUT_SELECT_TRACE(("%s: source: %d, signalFmt: %d, transFmt: %d, status: %d, isDVI: %d.\n", __FUNCTION__,
                            signalDetectEvent->SourceInput,
                            signalDetectEvent->SignalFmt,
                            signalDetectEvent->TransFmt,
                            signalDetectEvent->SignalStatus,
                            signalDetectEvent->isDviSignal));

		dsHdmiInPort_t port = dsHDMI_IN_PORT_0;
		dsHdmiInSignalStatus_t status = dsHDMI_IN_SIGNAL_STATUS_NONE;

                switch(signalDetectEvent->SourceInput) {
                    case SOURCE_HDMI1:
                        port = dsHDMI_IN_PORT_0;
                        break;

                    case SOURCE_HDMI2:
                        port = dsHDMI_IN_PORT_1;
                        break;

                    case SOURCE_HDMI3:
                        port = dsHDMI_IN_PORT_2;
                        break;

                    default:
                        break;
                }

                pthread_mutex_lock(&inputPortMutex);
                switch(signalDetectEvent->SignalStatus) {
                        case TVIN_SIG_STATUS_NULL:
                                status = dsHDMI_IN_SIGNAL_STATUS_NONE;
                                break;
                        case TVIN_SIG_STATUS_NOSIG:
                                status = dsHDMI_IN_SIGNAL_STATUS_NOSIGNAL;
                                break;
                        case TVIN_SIG_STATUS_UNSTABLE:
                                status = dsHDMI_IN_SIGNAL_STATUS_UNSTABLE;
                                break;
                        case TVIN_SIG_STATUS_NOTSUP:
                                status = dsHDMI_IN_SIGNAL_STATUS_NOTSUPPORTED;
                                break;
                        case TVIN_SIG_STATUS_STABLE:
                                status = dsHDMI_IN_SIGNAL_STATUS_STABLE;
                                isPresenting = true;
                                if(eINPUT_PORT_TYPE_HDMI == GetInputPortType(signalDetectEvent->SourceInput) && hdmiInStatus != NULL) {
                                    hdmiInStatus->isPresented = isPresenting;
                                }
#ifdef HAS_COMPOSITE_IN_SUPPORT
				else if(eINPUT_PORT_TYPE_COMPOSITE == GetInputPortType(signalDetectEvent->SourceInput) && compositeInStatus != NULL ) {
                                    compositeInStatus->isPresented = isPresenting;
				}
#endif
                                break;
                        default:
                                break;
                }

		if(eINPUT_PORT_TYPE_HDMI == GetInputPortType(signalDetectEvent->SourceInput)) {
                    if(_halSigChangeCB != NULL) {
                        INPUT_SELECT_INFO(("HDMI In Signal Changed!!! HDMI In Source: %d, Signal Status: %d\n",port,status));
                        _halSigChangeCB(port, status);
                    }

                    if((_halStatusChangeCB != NULL) && (status == dsHDMI_IN_SIGNAL_STATUS_STABLE)) {
                        INPUT_SELECT_INFO(("%s HDMI In Status Changed!!! HDMI In Source: %d, isPresented: %d\n",__PRETTY_FUNCTION__,hdmiInStatus->activePort,hdmiInStatus->isPresented));
                        _halStatusChangeCB(*hdmiInStatus);
                    }
		}

		pthread_mutex_unlock(&inputPortMutex);
                IntWriteSysfs("/sys/class/video/disable_video", 0);
                break;
            }
        case TV_EVENT_TYPE_SOURCE_CONNECT:
            {
                SourceConnectCallback_t *sourceConnectEvent = (SourceConnectCallback_t *)(eventData);
                INPUT_SELECT_TRACE(("%s: source: %d, connectStatus: %d\n", __FUNCTION__,
                            sourceConnectEvent->SourceInput, sourceConnectEvent->ConnectionState));

                bool isConnected = sourceConnectEvent->ConnectionState;
                pthread_mutex_lock(&inputPortMutex);
                if(selectedSource != SOURCE_INVALID) {
                    if((isConnected == 0) && (isPresenting == true)) {
                    if(selectedSource == sourceConnectEvent->SourceInput) {
                            StopTv(tvclient, selectedSource);
                            isPresenting = false;
                    }
                        else {
                           INPUT_SELECT_INFO(("Non active HDMI In Source:%d unplugged !!!   ActiveSource: %d\n",sourceConnectEvent->SourceInput, selectedSource));
                        }
                    }
                    else {
                        if(selectedSource == sourceConnectEvent->SourceInput) {
                            StartTv(tvclient, selectedSource);
                            isPresenting = true;
                        }
                        else {
                            INPUT_SELECT_INFO(("HDMI In Source changed!!! PreviousSource:%d  CurrentSource: %d\n",selectedSource,sourceConnectEvent->SourceInput));
                        }
                    }
                }
                if(eINPUT_PORT_TYPE_HDMI == GetInputPortType(sourceConnectEvent->SourceInput) && hdmiInStatus != NULL)
                    hdmiInStatus->isPresented = isPresenting;

#ifdef HAS_COMPOSITE_IN_SUPPORT
                if(eINPUT_PORT_TYPE_COMPOSITE == GetInputPortType(sourceConnectEvent->SourceInput) && compositeInStatus != NULL) {
                    compositeInStatus->isPresented = isPresenting;
		}
#endif
                pthread_mutex_unlock(&inputPortMutex);

                if(_halCBHDMI != NULL) {
                    switch(sourceConnectEvent->SourceInput) {
                        case SOURCE_HDMI1:
                            _halCBHDMI(dsHDMI_IN_PORT_0,isConnected);
                            break;

                        case SOURCE_HDMI2:
                            _halCBHDMI(dsHDMI_IN_PORT_1,isConnected);
                            break;

                        case SOURCE_HDMI3:
                            _halCBHDMI(dsHDMI_IN_PORT_2,isConnected);
                            break;

                        default:
                            break;
                    }
                }



                if(eINPUT_PORT_TYPE_HDMI == GetInputPortType(sourceConnectEvent->SourceInput)) {
                    if((_halStatusChangeCB != NULL) && (isConnected == 0)) {
                         INPUT_SELECT_INFO(("%s HDMI In Status Changed!!! HDMI In Source: %d, isPresented: %d\n",__PRETTY_FUNCTION__,hdmiInStatus->activePort,hdmiInStatus->isPresented));
                         _halStatusChangeCB(*hdmiInStatus);
                    }
                }
#ifdef HAS_COMPOSITE_IN_SUPPORT
                if(_halCBComposite != NULL) {
                    switch(sourceConnectEvent->SourceInput) {
                        case SOURCE_AV1:
                            _halCBComposite(dsCOMPOSITE_IN_PORT_0, isConnected);
		            compositeInStatus->isPortConnected[dsCOMPOSITE_IN_PORT_0] = isConnected;
                            break;
                        case SOURCE_AV2:
                            _halCBComposite(dsCOMPOSITE_IN_PORT_1, isConnected);
		            compositeInStatus->isPortConnected[dsCOMPOSITE_IN_PORT_1] = isConnected;
                            break;
                        default:
                            break;
                    }
                }
#endif
                break;
            }
        default:
            INPUT_SELECT_TRACE(("%s invalid event!\n", __FUNCTION__));
            break;
    }
    return;
}

static int SetOsdBlankStatus(const char *path, int cmd)
{
    int fd;
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0777);

    if (fd >= 0)
    {
        sprintf(bcmd, "%d", cmd);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }
    return -1;
}

static int IntWriteSysfs(const char *path, int value)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0777);

    if (fd >= 0) {
        sprintf(bcmd,"%d",value);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

static int StringWriteSysfs(const char *path, const char *cmd)
{
    int fd;
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0777);

    if (fd >= 0) {
        write(fd, cmd, strlen(cmd));
        close(fd);
        return 0;
    }

    return -1;
}

static int DisplayInit()
{
    SetOsdBlankStatus("/sys/class/graphics/fb0/osd_display_debug", 1);
    SetOsdBlankStatus("/sys/class/graphics/fb0/blank", 1);
    return 0;
}


static void* input_switching_thread(void *arg) {
    clock_t time_start;
    time_start = clock();

    pthread_mutex_lock(&inputPortMutex);

    dsError_t err = dsERR_NONE;
    tv_source_input_t tvCliPort = *((tv_source_input_t*) arg);
    free(arg);

    INPUT_SELECT_TRACE(("%s: thread ID:%d ---> tvCliPort=%d\n", __PRETTY_FUNCTION__,pthread_self(), tvCliPort));
    input_port_type_t portType = GetInputPortType(tvCliPort);

    //Stop request
    if (SOURCE_INVALID == tvCliPort)
    {
        //Check if source is selected
        if ((selectedSource != SOURCE_INVALID))
        {
            INPUT_SELECT_TRACE(("%s - Invoking StopTv(%d)\n", __PRETTY_FUNCTION__, selectedSource));
            if (0 != StopTv(tvclient, selectedSource))
            {
                INPUT_SELECT_ERROR(("%s : StopTv failed for source port %d\n", __PRETTY_FUNCTION__, selectedSource));
                err = dsERR_GENERAL;
            }
            else
            {
                selectedSource = SOURCE_INVALID;
                isPresenting = false;
                INPUT_SELECT_INFO(("%s StopTv successful for source port %d\n", __PRETTY_FUNCTION__, tvCliPort));
		if(eINPUT_PORT_TYPE_HDMI == portType && hdmiInStatus != NULL) {
                     hdmiInStatus->isPresented = isPresenting;
		     if(_halStatusChangeCB != NULL) {
                         INPUT_SELECT_INFO(("%s HDMI In Status Changed!!! HDMI In Source: %d, isPresented: %d\n",__PRETTY_FUNCTION__,hdmiInStatus->activePort, hdmiInStatus->isPresented));
                         _halStatusChangeCB(*hdmiInStatus);
                     }
		}
#ifdef HAS_COMPOSITE_IN_SUPPORT
		else if(eINPUT_PORT_TYPE_COMPOSITE == portType && compositeInStatus != NULL) {
                    compositeInStatus->isPresented = isPresenting;
		}
#endif
            }
        }
    }
    else
    {
        //stop currently presenting source
        if ((selectedSource != SOURCE_INVALID) && (selectedSource != tvCliPort))
        {
            if (0 != StopTv(tvclient, selectedSource))
            {
                INPUT_SELECT_ERROR(("%s : StopTv failed for source port %d\n", __PRETTY_FUNCTION__, selectedSource));
                err = dsERR_GENERAL;
            }
            else
            {
                selectedSource = SOURCE_INVALID;
                isPresenting = false;
                INPUT_SELECT_INFO(("%s StopTv successful for source port %d\n", __PRETTY_FUNCTION__, tvCliPort));
		if(eINPUT_PORT_TYPE_HDMI == portType && hdmiInStatus != NULL) {
                         hdmiInStatus->isPresented = isPresenting;
		     if(_halStatusChangeCB != NULL) {
                         INPUT_SELECT_INFO(("%s HDMI In Status Changed!!! HDMI In Source: %d, isPresented: %d\n",__PRETTY_FUNCTION__,hdmiInStatus->activePort, hdmiInStatus->isPresented));
                         _halStatusChangeCB(*hdmiInStatus);
                     }
		}
#ifdef HAS_COMPOSITE_IN_SUPPORT
		else if(eINPUT_PORT_TYPE_COMPOSITE == portType && compositeInStatus != NULL) {
                    compositeInStatus->isPresented = isPresenting;
		}
#endif
            }
        }
        //Switch Input to selected port
        INPUT_SELECT_TRACE(("%s - Invoking StartTv(%d)\n", __PRETTY_FUNCTION__, tvCliPort));
                SetOsdBlankStatus("/sys/class/graphics/fb0/blank", 0);
        if (0 != StartTv(tvclient, tvCliPort))
        {
            INPUT_SELECT_ERROR(("%s : StartTv failed for source port %d\n", __PRETTY_FUNCTION__, tvCliPort));
            err = dsERR_GENERAL;
        }
        else
        {
            selectedSource = tvCliPort;
            INPUT_SELECT_INFO(("%s StartTv successful for source port %d\n", __PRETTY_FUNCTION__, selectedSource));
	    if(eINPUT_PORT_TYPE_HDMI == portType && hdmiInStatus != NULL) {
                hdmiInStatus->activePort = TvClientPortTodsHDMIPort(tvCliPort);
	    }
#ifdef HAS_COMPOSITE_IN_SUPPORT
	    else if (eINPUT_PORT_TYPE_COMPOSITE == portType && compositeInStatus != NULL) {
		compositeInStatus->activePort = TvClientPortTodsCOMPOSITEPort(tvCliPort);
	    }
#endif
        }
    }

    time_start = clock() - time_start;
    double time_taken = ((double)time_start)/(CLOCKS_PER_SEC); // in sec

    INPUT_SELECT_INFO(("%s: thread ID:%d  time taken:%fsec <--- error=%d \n", __PRETTY_FUNCTION__, pthread_self(), time_taken, err));

    pthread_mutex_unlock(&inputPortMutex);
    pthread_exit(NULL);
    return NULL;
}

static dsError_t SelectPort(tv_source_input_t tvCliPort)
{
    dsError_t ret = dsERR_NONE;
    IntWriteSysfs("/sys/class/video/video_global_output", 1);

    int *arg = (int*) malloc(sizeof(*arg));
    *arg = tvCliPort;

    pthread_mutex_lock(&inputSelectMutex);
    if (tvclient != NULL)
    {
        pthread_t inputSwitch_threadId = 0;
        int err = pthread_create(&inputSwitch_threadId,NULL,input_switching_thread, arg);
        if(err != 0){
            INPUT_SELECT_ERROR(("%s : input_switching_thread creation failed!!! \n", __PRETTY_FUNCTION__));
            free(arg);
            ret = dsERR_GENERAL;
        }
        else {
            err = pthread_detach(inputSwitch_threadId);
            if(err != 0) {
                INPUT_SELECT_ERROR(("%s : input_switching_thread detach failed!!! \n", __PRETTY_FUNCTION__));
            }
        }
    }
    else
    {
        INPUT_SELECT_ERROR(("%s : TvClient object is NULL \n", __PRETTY_FUNCTION__));
        ret = dsERR_GENERAL;
    }
    /* Release the HDMI Input mutex */
    pthread_mutex_unlock(&inputSelectMutex);
    INPUT_SELECT_TRACE(("%s <--- error=%d\n", __PRETTY_FUNCTION__, ret));
    return ret;
}


static dsError_t ScaleInputVideo(int32_t x, int32_t y, int32_t width, int32_t height)
{
    INPUT_SELECT_TRACE(("DS_HAL %s -----> \n",__PRETTY_FUNCTION__));
    dsError_t ret = dsERR_NONE;
    if((x < 0)||(y < 0)||(width < 0)||(height < 0)){
    INPUT_SELECT_ERROR(("%s : Negative Numbers are Not Valid\n", __PRETTY_FUNCTION__));
    ret = dsERR_INVALID_PARAM;
        return ret;
     }

    FILE* f = fopen("/sys/class/video/axis", "w");
    if (f == NULL) {
    INPUT_SELECT_ERROR(("%s : Unable to open the file for writing\n", __PRETTY_FUNCTION__));
        ret = dsERR_GENERAL;
        return ret;
    }

    INPUT_SELECT_TRACE(("DS_HAL %s ----->  x: %d, y: %d, w: %d, h: %d\n",__PRETTY_FUNCTION__,x,y,width,height));
    if(fprintf(f, "%d %d %d %d", x, y, width+x, height+y) < 0 ) {
        INPUT_SELECT_ERROR(("%s : Failed to update video axis sysfs\n", __PRETTY_FUNCTION__));
        ret = dsERR_INVALID_PARAM;
    }

    fclose(f);
    return ret;
}


/*
 *                                 HDMI IN specific functions start here
 *  =============================================================================================================
 */

static tv_source_input_t dsHalHDMIPortToTvClientPort(dsHdmiInPort_t ePort)
{
    tv_source_input_t ret = SOURCE_INVALID;

    switch (ePort)
    {
    case dsHDMI_IN_PORT_0:
        ret = SOURCE_HDMI1;
        break;
    case dsHDMI_IN_PORT_1:
        ret = SOURCE_HDMI2;
        break;
    case dsHDMI_IN_PORT_2:
        ret = SOURCE_HDMI3;
        break;
    default:
        ret = SOURCE_INVALID;
    }

    return ret;
}

static int checkHdmiInPortStatus() {

    int fp;
    int hdmi_status = 0;
    fp = open(HDMI_IN_PORT_INFO, O_RDONLY);
    if(fp != -1) {
        read(fp, (void*) &hdmi_status, sizeof(int));
    }
    else {
        INPUT_SELECT_ERROR(("%s: HDMI Source Info file not present !!!\n",__PRETTY_FUNCTION__));
        return -1;
    }
    close(fp);

    INPUT_SELECT_TRACE(("%s  ---> hdmi_status : %d \n", __PRETTY_FUNCTION__, hdmi_status));

    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_A) == HDMI_DETECT_STATUS_BIT_A) {
        hdmiInStatus->isPortConnected[dsHDMI_IN_PORT_0] = true;
        INPUT_SELECT_TRACE(("%s  ---> dsHDMI_IN_PORT_0 connected\n", __PRETTY_FUNCTION__));
    } else {
        hdmiInStatus->isPortConnected[dsHDMI_IN_PORT_0] = false;
    }

    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_B) == HDMI_DETECT_STATUS_BIT_B) {
        hdmiInStatus->isPortConnected[dsHDMI_IN_PORT_1] = true;
        INPUT_SELECT_TRACE(("%s  ---> dsHDMI_IN_PORT_1 connected\n", __PRETTY_FUNCTION__));
    } else {
        hdmiInStatus->isPortConnected[dsHDMI_IN_PORT_1] = false;
    }

    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_C) == HDMI_DETECT_STATUS_BIT_C) {
        INPUT_SELECT_TRACE(("%s  ---> dsHDMI_IN_PORT_2 connected\n", __PRETTY_FUNCTION__));
        hdmiInStatus->isPortConnected[dsHDMI_IN_PORT_2] = true;
    } else {
        hdmiInStatus->isPortConnected[dsHDMI_IN_PORT_2] = false;
    }

    return 0;
}

bool dsIsHdmiARCPort (int iPort) {
    bool eRet = false;
    if (HDMI_IN_ARC_CAP_PORT == iPort) {
        eRet = true;
    }
    return eRet;
}

dsError_t dsHdmiInInit(void)
{
    dsError_t dsErr = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s  ---> \n", __PRETTY_FUNCTION__));
    pthread_mutex_lock(&inputSelectMutex);
    if (tvclient == NULL)
    {
        INPUT_SELECT_TRACE(("%s  Initiializing tvclient observer \n", __PRETTY_FUNCTION__));
        tvclient = GetInstance();
        DisplayInit();
        setTvEventCallback(TvEventCallback);
    }
    if (NULL == tvclient)
    {
        dsErr = dsERR_GENERAL;
    }

    if(NULL == hdmiInStatus)
    {
        hdmiInStatus = (dsHdmiInStatus_t*) malloc(sizeof(dsHdmiInStatus_t));
    }
    pthread_mutex_lock(&inputPortMutex);
    int err = checkHdmiInPortStatus();
    if(err != -1) {
       hdmiInStatus->isPresented = false;
       hdmiInStatus->activePort = dsHDMI_IN_PORT_NONE;
    }
    else {
        dsErr = dsERR_GENERAL;
    }
    pthread_mutex_unlock(&inputPortMutex);
    pthread_mutex_unlock(&inputSelectMutex);
    INPUT_SELECT_TRACE(("%s  <--- error=%d\n", __PRETTY_FUNCTION__, dsErr));
    return dsErr;
}

dsError_t dsHdmiInTerm(void)
{
    dsError_t ret = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    pthread_mutex_lock(&inputSelectMutex);
    if ((selectedSource != SOURCE_INVALID) && (eINPUT_PORT_TYPE_HDMI == GetInputPortType(selectedSource)))
    {
        if (0 != StopTv(tvclient, selectedSource))
        {
            INPUT_SELECT_ERROR(("%s : StopTv failed for source port %d\n", __PRETTY_FUNCTION__, selectedSource));
            ret = dsERR_GENERAL;
        }
        else
        {
            selectedSource = SOURCE_INVALID;
            INPUT_SELECT_INFO(("%s StopTv successful for source port %d\n", __PRETTY_FUNCTION__, selectedSource));
        }
    }
    free(hdmiInStatus);
    hdmiInStatus = NULL;
    pthread_mutex_unlock(&inputSelectMutex);
    INPUT_SELECT_TRACE(("%s <--- error=%d\n", __PRETTY_FUNCTION__, ret));
    return ret;
}

dsError_t dsHdmiInGetNumberOfInputs(uint8_t *pNumberOfInputs)
{
    dsError_t ret = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s ---> pNumberOfInputs=0x%08X\n", __PRETTY_FUNCTION__, (uint32_t)pNumberOfInputs));
    *pNumberOfInputs = 3;
    INPUT_SELECT_TRACE(("%s <--- NumberOfInputs=%d error=%d\n", __PRETTY_FUNCTION__, (uint32_t)(*pNumberOfInputs), ret));
    return ret;
}

dsError_t dsHdmiInGetStatus(dsHdmiInStatus_t *pStatus)
{
    dsError_t ret = dsERR_NONE;
    pthread_mutex_lock(&inputSelectMutex);
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));

    pthread_mutex_lock(&inputPortMutex);
    int err = checkHdmiInPortStatus();
    if(err != -1) {
        memcpy(pStatus, hdmiInStatus, sizeof(dsHdmiInStatus_t));
    }
    else {
        ret = dsERR_GENERAL;
    }
    pthread_mutex_unlock(&inputPortMutex);

    pthread_mutex_unlock(&inputSelectMutex);
    return ret;
}


dsError_t dsHdmiInSelectPort(dsHdmiInPort_t ePort)
{
    dsError_t ret = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s ---> ePort=%d\n", __PRETTY_FUNCTION__, ePort));
    tv_source_input_t tvCliPort = dsHalHDMIPortToTvClientPort(ePort);
    ret = SelectPort(tvCliPort);
    return ret;
}

dsError_t dsHdmiInScaleVideo(int32_t x, int32_t y, int32_t width, int32_t height)
{
    dsError_t ret = ScaleInputVideo(x, y, width, height);
    return ret;
}

dsError_t dsHdmiInSelectZoomMode(dsVideoZoom_t requestedZoomMode)
{
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    return (dsERR_GENERAL);
}

dsError_t dsHdmiInPauseAudio()
{
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    return (dsERR_GENERAL);
}

dsError_t dsHdmiInResumeAudio()
{
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    return (dsERR_GENERAL);
}

dsError_t dsHdmiInGetCurrentVideoMode(dsVideoPortResolution_t *resolution)
{
    INPUT_SELECT_TRACE(("%s --->\n", __PRETTY_FUNCTION__));
    return (dsERR_GENERAL);
}

dsError_t dsHdmiInRegisterConnectCB(dsHdmiInConnectCB_t CBFunc)
{
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    dsError_t ret = dsERR_NONE;

    if(CBFunc == NULL){
        ret = dsERR_INVALID_PARAM;
    }
    if(ret == dsERR_NONE) {
        /* Register The call Back */
        _halCBHDMI = CBFunc;
    }

    return ret;
}


dsError_t dsHdmiInRegisterSignalChangeCB(dsHdmiInSignalChangeCB_t CBFunc)
{
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    dsError_t ret = dsERR_NONE;

    if(CBFunc == NULL){
	ret = dsERR_INVALID_PARAM;
    }
    if(ret == dsERR_NONE) {
	/* Register The call Back */
	_halSigChangeCB = CBFunc;
    }

    return ret;
}

dsError_t dsHdmiInRegisterStatusChangeCB(dsHdmiInStatusChangeCB_t CBFunc)
{
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    dsError_t ret = dsERR_NONE;

    if(CBFunc == NULL){
        ret = dsERR_INVALID_PARAM;
    }
    if(ret == dsERR_NONE) {
        /* Register The call Back */
        _halStatusChangeCB = CBFunc;
    }

    return ret;
}

/*
 *                                 End of HDMI IN specific functions
 *  =============================================================================================================
 */


/*
 *                                 COMPOSITE IN specific functions start here
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifdef HAS_COMPOSITE_IN_SUPPORT

static tv_source_input_t dsHalCOMPOSITEPortToTvClientPort(dsCompositeInPort_t ePort)
{
    tv_source_input_t ret = SOURCE_INVALID;

    switch (ePort)
    {
    case dsCOMPOSITE_IN_PORT_0:
        ret = SOURCE_AV1;
        break;
    default:
        ret = SOURCE_INVALID;
    }

    return ret;
}

static int checkCompositeInPortStatus() {

  //Opening the AVIN_DETECT_PATH casuses issues now
  //will update the status with connect callback for now
  //until amlogic provides a fix
/*
    int avinfp;
    avinfp = open(AVIN_DETECT_PATH, O_RDONLY);

    struct report_data_s status[AVIN_CHANNEL_MAX];
    if(avinfp != -1) {
        read(avinfp, (void *)(&status), sizeof(struct report_data_s) * AVIN_CHANNEL_MAX);
        INPUT_SELECT_ERROR(("%s: status[0].status %d  status[1].status %d !!!\n",__PRETTY_FUNCTION__, status[0].status, status[1].status));
    }
    else {
        INPUT_SELECT_ERROR(("%s: COMPOSITE Source Info file not present !!!\n",__PRETTY_FUNCTION__));
        return -1;
    }
    close(avinfp);

    INPUT_SELECT_TRACE(("%s  ---> Checking Composite in status MAX ports %d\n", __PRETTY_FUNCTION__, AVIN_CHANNEL_MAX));
    for (int i = 0; i <  AVIN_CHANNEL_MAX; i++) {
        if (status[i].channel == AVIN_CHANNEL1 && compositeInStatus != NULL) {
            compositeInStatus->isPortConnected[dsCOMPOSITE_IN_PORT_0] = (AVIN_STATUS_IN == status[i].status);
            INPUT_SELECT_TRACE(("%s  ---> dsCOMPOSITE_IN_PORT_0 connected = %d\n", __PRETTY_FUNCTION__,compositeInStatus->isPortConnected[dsCOMPOSITE_IN_PORT_0]));
        } else if (status[i].channel == AVIN_CHANNEL2 && compositeInStatus != NULL) {
            compositeInStatus->isPortConnected[dsCOMPOSITE_IN_PORT_1] = (AVIN_STATUS_IN == status[i].status);
            INPUT_SELECT_TRACE(("%s  ---> dsCOMPOSITE_IN_PORT_1 connected = %d\n", __PRETTY_FUNCTION__,compositeInStatus->isPortConnected[dsCOMPOSITE_IN_PORT_1]));
        }
    }//end for
*/
    return 0;
}

dsError_t dsCompositeInInit(void)
{
    dsError_t dsErr = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s  ---> \n", __PRETTY_FUNCTION__));
    pthread_mutex_lock(&inputSelectMutex);
    if (tvclient == NULL)
    {
        INPUT_SELECT_TRACE(("%s  Initiializing tvclient observer \n", __PRETTY_FUNCTION__));
        tvclient = GetInstance();
        DisplayInit();
        setTvEventCallback(TvEventCallback);
    }
    if (NULL == tvclient)
    {
        dsErr = dsERR_GENERAL;
    }

    if(NULL == compositeInStatus)
    {
        compositeInStatus = (dsCompositeInStatus_t*) malloc(sizeof(dsCompositeInStatus_t));
    }
    pthread_mutex_lock(&inputPortMutex);
    int err = checkCompositeInPortStatus();
    if(err != -1) {
       compositeInStatus->isPresented = false;
       compositeInStatus->activePort = dsCOMPOSITE_IN_PORT_NONE;
    }
    else {
        dsErr = dsERR_GENERAL;
    }
    pthread_mutex_unlock(&inputPortMutex);
    pthread_mutex_unlock(&inputSelectMutex);
    INPUT_SELECT_TRACE(("%s  <--- error=%d\n", __PRETTY_FUNCTION__, dsErr));
    return dsErr;
}

dsError_t dsCompositeInTerm(void)
{
    dsError_t ret = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    pthread_mutex_lock(&inputSelectMutex);
    if ((selectedSource != SOURCE_INVALID) && (eINPUT_PORT_TYPE_COMPOSITE == GetInputPortType(selectedSource)))
    {
        if (0 != StopTv(tvclient, selectedSource))
        {
            INPUT_SELECT_ERROR(("%s : StopTv failed for source port %d\n", __PRETTY_FUNCTION__, selectedSource));
            ret = dsERR_GENERAL;
        }
        else
        {
            selectedSource = SOURCE_INVALID;
            INPUT_SELECT_INFO(("%s StopTv successful for source port %d\n", __PRETTY_FUNCTION__, selectedSource));
        }
    }
    if(compositeInStatus)
    {
        free(compositeInStatus);
        compositeInStatus = NULL;
    }
    pthread_mutex_unlock(&inputSelectMutex);
    INPUT_SELECT_TRACE(("%s <--- error=%d\n", __PRETTY_FUNCTION__, ret));
    return ret;
}

dsError_t dsCompositeInGetNumberOfInputs(uint8_t *pNumberOfInputs)
{
    dsError_t ret = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s ---> pNumberOfInputs=0x%08X\n", __PRETTY_FUNCTION__, (uint32_t)pNumberOfInputs));
    *pNumberOfInputs = 1;
    INPUT_SELECT_TRACE(("%s <--- NumberOfInputs=%d error=%d\n", __PRETTY_FUNCTION__, (uint32_t)(*pNumberOfInputs), ret));
    return ret;
}

dsError_t dsCompositeInGetStatus(dsCompositeInStatus_t *pStatus)
{
    dsError_t ret = dsERR_NONE;
    pthread_mutex_lock(&inputSelectMutex);
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));

    pthread_mutex_lock(&inputPortMutex);
    int err = checkCompositeInPortStatus();
    if(err != -1) {
        memcpy(pStatus, compositeInStatus, sizeof(dsCompositeInStatus_t));
    }
    else {
        ret = dsERR_GENERAL;
    }
    pthread_mutex_unlock(&inputPortMutex);

    pthread_mutex_unlock(&inputSelectMutex);
    return ret;
}


dsError_t dsCompositeInSelectPort(dsCompositeInPort_t ePort)
{
    dsError_t ret = dsERR_NONE;
    INPUT_SELECT_TRACE(("%s ---> ePort=%d\n", __PRETTY_FUNCTION__, ePort));
    tv_source_input_t tvCliPort = dsHalCOMPOSITEPortToTvClientPort(ePort);
    ret = SelectPort(tvCliPort);
    return ret;
}

dsError_t dsCompositeInScaleVideo(int32_t x, int32_t y, int32_t width, int32_t height)
{
    dsError_t ret = ScaleInputVideo(x, y, width, height);
    return ret;
}

dsError_t dsCompositeInRegisterConnectCB(dsCompositeInConnectCB_t CBFunc)
{
    INPUT_SELECT_TRACE(("%s ---> \n", __PRETTY_FUNCTION__));
    dsError_t ret = dsERR_NONE;

    if(CBFunc == NULL){
        ret = dsERR_INVALID_PARAM;
    }
    if(ret == dsERR_NONE) {
        /* Register The call Back */
        _halCBComposite = CBFunc;
    }

    return ret;
}

#endif //HAS_COMPOSITE_IN_SUPPORT

#endif /* HAS_HDMI_IN_SUPPORT  */
