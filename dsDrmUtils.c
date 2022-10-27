#include <string.h>
#include <fcntl.h>
#include <dsTypes.h>
#include "dsDrmUtils.h"
#include <pthread.h>

static pthread_mutex_t drmFD_lock = PTHREAD_MUTEX_INITIALIZER;

int openDefaultDRMDevice()
{
    int drmFD = -1;
    pthread_mutex_lock(&drmFD_lock);
    if (drmFD < 0) {
        drmFD = open(DEFUALT_DRM_DEVICE, O_RDWR | O_CLOEXEC);
        // Re-check if open successfully or not
        if (drmFD < 0) {
            printf("cannot open %s\n", DEFUALT_DRM_DEVICE);
        }else{
            drmDropMaster(drmFD);
        }
        //printf("%s:%d:open drmFD = %d\n", __func__, __LINE__,drmFD);
    }
    pthread_mutex_unlock(&drmFD_lock);
    return drmFD;
}

void closeDefaultDRMDevice(int drmFD)
{
    pthread_mutex_lock(&drmFD_lock);
    if (drmFD >= 0) {
        close(drmFD);
        //printf("%s:%d:close drmFD = %d\n", __func__, __LINE__,drmFD);
        drmFD = -1;
    }
    pthread_mutex_unlock(&drmFD_lock);
}

int getDefaultDRMResolution (drmModeConnector *conn, drmConnectorModes *drmResolution)
{
    int ret = 1;
    printf(" DSDRM  getDefaultDRMResolution \n");
    if(!strcmp(conn->modes[0].name,"720x480i")) {
        *drmResolution = drmMode_480i;
    }
    else if(!strcmp(conn->modes[0].name,"720x480")) {
        *drmResolution = drmMode_480p;
    }
    else if(!strcmp(conn->modes[0].name,"1280x720")) {
        *drmResolution = drmMode_720p;
    }
    else if(!strcmp(conn->modes[0].name,"1920x1080i")) {
        *drmResolution = drmMode_1080i;
    }
    else if(!strcmp(conn->modes[0].name,"1920x1080")) {
        if(conn->modes[0].vrefresh == 60) {
            *drmResolution = drmMode_1080p;
        }
        else {
            printf(" DSDRM: (%s) Couldn't recognize resolution\n",__func__);
            ret = 0;
        }
    }
    else if(!strcmp(conn->modes[0].name,"3840x2160")) {
        if(conn->modes[0].vrefresh == 24) {
            *drmResolution = drmMode_3840x2160p24;
        }
        else if(conn->modes[0].vrefresh == 25) {
            *drmResolution = drmMode_3840x2160p25;
        }
        else if(conn->modes[0].vrefresh == 30) {
            *drmResolution = drmMode_3840x2160p30;
        }
        else if(conn->modes[0].vrefresh == 50) {
            *drmResolution = drmMode_3840x2160p50;
        }
        else if(conn->modes[0].vrefresh == 60) {
            *drmResolution = drmMode_3840x2160p60;
        }
        else {
            printf(" DSDRM: (%s) Couldn't recognize resolution\n",__func__);
            ret = 0;
        }
    }
    else if(!strcmp(conn->modes[0].name,"4096x2160")) {
        if(conn->modes[0].vrefresh == 24) {
            *drmResolution = drmMode_4096x2160p24;
        }
        else if(conn->modes[0].vrefresh == 25) {
            *drmResolution = drmMode_4096x2160p25;
        }
        else if(conn->modes[0].vrefresh == 30) {
            *drmResolution = drmMode_4096x2160p30;
        }
        else if(conn->modes[0].vrefresh == 50) {
            *drmResolution = drmMode_4096x2160p50;
        }
        else if(conn->modes[0].vrefresh == 60) {
            *drmResolution = drmMode_4096x2160p60;
        }
        else {
            printf(" DSDRM: (%s) Couldn't recognize resolution\n",__func__);
            ret = 0;
        }
    }
    else {
        printf(" DSDRM: (%s) Couldn't recognize resolution\n",__func__);
        ret = 0;
    }

    return ret;
}


int getSupportedDRMResolutions(drmModeConnector *conn, drmConnectorModes *drmResolution)
{
    int ret = 1;
    int copy_index = 0;
    int isDuplicate = 0;
    printf(" DSDRM  %s \n",__func__);
    for (int i =0; i < conn->count_modes; i++) {
        isDuplicate=0;
        for(int j = 0; j < i; j++){
            if(!strcmp(conn->modes[j].name,conn->modes[i].name) && (conn->modes[j].vrefresh == conn->modes[i].vrefresh)){
                isDuplicate = 1;
                break;
            }
        }
        if(isDuplicate==1)
            continue;
        if(!strcmp(conn->modes[i].name,"720x480i") || !strcmp(conn->modes[i].name,"480i60hz")) {
            drmResolution[copy_index] = drmMode_480i;
        }
        else if(!strcmp(conn->modes[i].name,"720x480") || !strcmp(conn->modes[i].name,"480p60hz")) {
            drmResolution[copy_index] = drmMode_480p;
        }
        else if(!strcmp(conn->modes[i].name,"720x576i") || !strcmp(conn->modes[i].name,"576i50hz")) {
            drmResolution[copy_index] = drmMode_576i;
        }
        else if(!strcmp(conn->modes[i].name,"720x576") || !strcmp(conn->modes[i].name,"576p50hz")) {
            drmResolution[copy_index] = drmMode_576p;
        }
        else if(!strcmp(conn->modes[i].name,"1280x720") || strstr(conn->modes[i].name,"720p")) {
            if(conn->modes[i].vrefresh == 60) {
                drmResolution[copy_index] = drmMode_720p;
            }
            if(conn->modes[i].vrefresh == 24) {
                drmResolution[copy_index] = drmMode_720p24;
            }
            if(conn->modes[i].vrefresh == 25) {
                drmResolution[copy_index] = drmMode_720p25;
            }
            if(conn->modes[i].vrefresh == 30) {
                drmResolution[copy_index] = drmMode_720p30;
            }
            if(conn->modes[i].vrefresh == 50) {
                drmResolution[copy_index] = drmMode_720p50;
            }
        }
        else if(!strcmp(conn->modes[i].name, "1280x1024") || strstr(conn->modes[i].name, "1024p")) {
            drmResolution[copy_index] = drmMode_1024p;
        }
        else if(!strcmp(conn->modes[i].name, "1920x1080i") || strstr(conn->modes[i].name, "1080i")) {
            if(conn->modes[i].vrefresh == 60) {
                drmResolution[copy_index] = drmMode_1080i;
            }
            if(conn->modes[i].vrefresh == 50) {
                drmResolution[copy_index] = drmMode_1080i50;
            }
        }
        else if(!strcmp(conn->modes[i].name, "1920x1080") || strstr(conn->modes[i].name, "1080p")) {
            if(conn->modes[i].vrefresh == 60) {
                drmResolution[copy_index] = drmMode_1080p;
            }
            else if(conn->modes[i].vrefresh == 24) {
                drmResolution[copy_index] = drmMode_1080p24;
            }
            else if(conn->modes[i].vrefresh == 25) {
                drmResolution[copy_index] = drmMode_1080p25;
            }
            else if(conn->modes[i].vrefresh == 30) {
                drmResolution[copy_index] = drmMode_1080p30;
            }
            else if(conn->modes[i].vrefresh == 50) {
                drmResolution[copy_index] = drmMode_1080p50;
            }
            else {
                //drmResolution[copy_index] = drmMode_Unknown;
                continue;
            }

        }
        else if(!strcmp(conn->modes[i].name, "3840x2160") || strstr(conn->modes[i].name, "2160p")) {
            if(conn->modes[i].vrefresh == 24) {
                drmResolution[copy_index] = drmMode_3840x2160p24;
            }
            else if(conn->modes[i].vrefresh == 25) {
                drmResolution[copy_index] = drmMode_3840x2160p25;
            }
            else if(conn->modes[i].vrefresh == 30) {
                drmResolution[copy_index] = drmMode_3840x2160p30;
            }
            else if(conn->modes[i].vrefresh == 50) {
                drmResolution[copy_index] = drmMode_3840x2160p50;
            }
            else if(conn->modes[i].vrefresh == 60) {
                drmResolution[copy_index] = drmMode_3840x2160p60;
            }
            else {
                //drmResolution[copy_index] = drmMode_Unknown;
                continue;
            }
        }
        else if(!strcmp(conn->modes[i].name,"4096x2160")) {
            if(conn->modes[i].vrefresh == 24) {
                drmResolution[copy_index] = drmMode_4096x2160p24;
            }
            else if(conn->modes[i].vrefresh == 25) {
                drmResolution[copy_index] = drmMode_4096x2160p25;
            }
            else if(conn->modes[i].vrefresh == 30) {
                drmResolution[copy_index] = drmMode_4096x2160p30;
            }
            else if(conn->modes[i].vrefresh == 50) {
                drmResolution[copy_index] = drmMode_4096x2160p50;
            }
            else if(conn->modes[i].vrefresh == 60) {
                drmResolution[copy_index] = drmMode_4096x2160p60;
            }
            else {
                //drmResolution[copy_index] = drmMode_Unknown;
                continue;
            }
        }
        else{
            continue;
        }
        copy_index++;
    }
    return ret;
}

int getCurrentDRMResolution (int drmFD, drmModeRes *res, drmModeConnector *conn, drmConnectorModes *drmResolution)
{
    int ret = 1;
    drmModeEncoder* enc = NULL;
    drmModeCrtc* crtc = NULL;
    for( int i= 0; i < res->count_encoders; i++ )
    {
        enc = drmModeGetEncoder(drmFD, res->encoders[i]);
        if ( enc )
        {
            if ( enc->encoder_id == conn->encoder_id ) // encoder for connector
            {
                break;
            }
            drmModeFreeEncoder(enc);
            enc = 0;
            ret = 0;
        }
    }

    if ( enc ) {
        crtc= drmModeGetCrtc(drmFD, enc->crtc_id);   //CRTC for connector
        if ( crtc && crtc->mode_valid ) {
            if(!strcmp(crtc->mode.name,"720x480i") || !strcmp(crtc->mode.name,"480i60hz")) {
                *drmResolution = drmMode_480i;
            }
            else if(!strcmp(crtc->mode.name,"720x480") || !strcmp(crtc->mode.name,"480p60hz")) {
                *drmResolution = drmMode_480p;
            }
            else if(!strcmp(crtc->mode.name,"1280x720") || strstr(crtc->mode.name,"720p")) {
                *drmResolution = drmMode_720p;
            }
            else if(!strcmp(crtc->mode.name,"1920x1080i") || strstr(crtc->mode.name,"1080i")) {
                *drmResolution = drmMode_1080i;
            }
            else if(!strcmp(crtc->mode.name,"1920x1080") || strstr(crtc->mode.name,"1080p")) {
                if(crtc->mode.vrefresh == 60) {
                    *drmResolution = drmMode_1080p;
                }
                else {
                    printf(" DSDRM: (%s) line %d Couldn't recognize resolution %s\n",__func__, __LINE__, crtc->mode.name);
                    ret = 0;
                }
            }
            else if(!strcmp(crtc->mode.name,"3840x2160") || strstr(crtc->mode.name,"2160p")) {
                if(crtc->mode.vrefresh == 24) {
                    *drmResolution = drmMode_3840x2160p24;
                }
                else if(crtc->mode.vrefresh == 25) {
                    *drmResolution = drmMode_3840x2160p25;
                }
                else if(crtc->mode.vrefresh == 30) {
                    *drmResolution = drmMode_3840x2160p30;
                }
                else if(crtc->mode.vrefresh == 50) {
                    *drmResolution = drmMode_3840x2160p50;
                }
                else if(crtc->mode.vrefresh == 60) {
                    *drmResolution = drmMode_3840x2160p60;
                }
                else {
                    printf(" DSDRM: (%s) line %d Couldn't recognize resolution %s\n",__func__, __LINE__, crtc->mode.name);
                    ret = 0;
                }
            }
            else if(!strcmp(crtc->mode.name,"4096x2160") || strstr(crtc->mode.name,"smpte")) {
                if(crtc->mode.vrefresh == 24) {
                    *drmResolution = drmMode_4096x2160p24;
                }
                else if(crtc->mode.vrefresh == 25) {
                    *drmResolution = drmMode_4096x2160p25;
                }
                else if(crtc->mode.vrefresh == 30) {
                    *drmResolution = drmMode_4096x2160p30;
                }
                else if(crtc->mode.vrefresh == 50) {
                    *drmResolution = drmMode_4096x2160p50;
                }
                else if(crtc->mode.vrefresh == 60) {
                    *drmResolution = drmMode_4096x2160p60;
                }
                else {
                    printf(" DSDRM: (%s) line %d Couldn't recognize resolution %s\n",__func__, __LINE__, crtc->mode.name);
                    ret = 0;
                }
            }
            else{
                printf(" DSDRM: (%s) line %d Couldn't recognize resolution %s\n",__func__, __LINE__, crtc->mode.name);
            }
            drmModeFreeCrtc(crtc);
            drmModeFreeEncoder(enc);
        }
    }else{
        fprintf(stderr, "DSDRM %s get enc failed\n", __FUNCTION__);
    }
    printf(" DSDRM  %s  Resolution read is : %d\n",__func__,*drmResolution);
    return ret;
}


drmConnectorModes getDRMConnectorModeInfo(dsVideoResolution_t pixelRes, dsVideoFrameRate_t f_rate, bool interlaced)
{
    drmConnectorModes drmMode = drmMode_Unknown;

    switch(pixelRes) {
        case dsVIDEO_PIXELRES_720x480:
            if(interlaced)
                drmMode = drmMode_480i;
            else
                drmMode = drmMode_480p;
            break;

        case dsVIDEO_PIXELRES_720x576:
            drmMode = drmMode_Unknown;
            break;

        case dsVIDEO_PIXELRES_1280x720:
            switch(f_rate) {
                case dsVIDEO_FRAMERATE_24:
                case dsVIDEO_FRAMERATE_23dot98:
                    drmMode = drmMode_720p24;
                    break;

                case dsVIDEO_FRAMERATE_25:
                    drmMode = drmMode_720p25;
                    break;

                case dsVIDEO_FRAMERATE_29dot97:
                case dsVIDEO_FRAMERATE_30:
                    drmMode = drmMode_720p30;
                    break;

                case dsVIDEO_FRAMERATE_50:
                    drmMode = drmMode_720p50;
                    break;

                case dsVIDEO_FRAMERATE_59dot94:
                case dsVIDEO_FRAMERATE_60:
                    drmMode = drmMode_720p;
                    break;

                default:
                    drmMode = drmMode_Unknown;
                    break;
            }
            break;

        case dsVIDEO_PIXELRES_1920x1080:
            if(interlaced) {
                drmMode = drmMode_1080i;
            }
            else {
                switch(f_rate) {
                    case dsVIDEO_FRAMERATE_24:
                    case dsVIDEO_FRAMERATE_23dot98:
                        drmMode = drmMode_1080p24;
                        break;

                    case dsVIDEO_FRAMERATE_25:
                        drmMode = drmMode_1080p25;
                        break;

                    case dsVIDEO_FRAMERATE_29dot97:
                    case dsVIDEO_FRAMERATE_30:
                        drmMode = drmMode_1080p30;
                        break;

                    case dsVIDEO_FRAMERATE_50:
                        drmMode = drmMode_1080p50;
                        break;

                    case dsVIDEO_FRAMERATE_59dot94:
                    case dsVIDEO_FRAMERATE_60:
                        drmMode = drmMode_1080p;
                        break;

                    default:
                        drmMode = drmMode_Unknown;
                        break;
                }
            }
            break;

        case dsVIDEO_PIXELRES_3840x2160:
            switch(f_rate) {
                case dsVIDEO_FRAMERATE_24:
                case dsVIDEO_FRAMERATE_23dot98:
                    drmMode = drmMode_3840x2160p24;
                    break;

                case dsVIDEO_FRAMERATE_25:
                    drmMode = drmMode_3840x2160p25;
                    break;

                case dsVIDEO_FRAMERATE_29dot97:
                case dsVIDEO_FRAMERATE_30:
                    drmMode = drmMode_3840x2160p30;
                    break;

                case dsVIDEO_FRAMERATE_50:
                    drmMode = drmMode_3840x2160p50;
                    break;

                case dsVIDEO_FRAMERATE_59dot94:
                case dsVIDEO_FRAMERATE_60:
                    drmMode = drmMode_3840x2160p60;
                    break;

                default:
                    drmMode = drmMode_Unknown;
                    break;
            }
            break;

        case dsVIDEO_PIXELRES_4096x2160:
            switch(f_rate) {
                case dsVIDEO_FRAMERATE_24:
                case dsVIDEO_FRAMERATE_23dot98:
                    drmMode = drmMode_4096x2160p24;
                    break;

                case dsVIDEO_FRAMERATE_25:
                    drmMode = drmMode_4096x2160p25;
                    break;

                case dsVIDEO_FRAMERATE_29dot97:
                case dsVIDEO_FRAMERATE_30:
                    drmMode = drmMode_4096x2160p30;
                    break;

                case dsVIDEO_FRAMERATE_50:
                    drmMode = drmMode_4096x2160p50;
                    break;

                case dsVIDEO_FRAMERATE_59dot94:
                case dsVIDEO_FRAMERATE_60:
                    drmMode = drmMode_4096x2160p60;
                    break;

                default:
                    drmMode = drmMode_Unknown;
                    break;
            }
            break;

        default:
            drmMode = drmMode_Unknown;
            break;
    }

    return drmMode;
}


