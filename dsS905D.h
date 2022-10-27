/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file dsS905D.h
 *
 * @brief Device Settings sysfs file mapping.
 *
 * This file defines a structure to map sysfs / dev / parameter files with open flags.
 *
 * @par Document
 * Document reference.
 *
 * @par Open Issues (in no particular order)
 * -# None
 *
 * @par Assumptions
 * -# None
 *
 * @par Abbreviations
 * -# None
 *
 * @par Implementation Notes
 * -# None
 *
 */
#ifndef _DS_S905D_H
#define _DS_S905D_H

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "dsTypes.h"

/**
 * This enumeration defines the HDMI port indices
 */
typedef enum _HDMIPort_t{
    HDMI0,
    HDMI_MAX,
}HDMIPort_t;

typedef enum _FB_t {
    FB0,
    FB1,
    AML_FB_MAX
}FB_t;

/**
 * This enumeration defines the SPDIF port indices
 */
typedef enum _SPDIFPort_t{
    SPDIF0,
    SPDIF_MAX,
}SPDIFPort_t;

/**
 * This enumeration defines the SPEAKER port indices
 */
typedef enum _SPEAKERPort_t{
    SPEAKER_PORT_0,
    SPEAKER_PORT_MAX,
}SPEAKERPort_t;


/**
 * This enumeration defines the type of sysfs file used in API's implementation.
 */
typedef enum _sysFileType_t {
    SYS_FILE,
    DEV_FILE,
    PARAM_FILE
}sysFileType_t;

typedef struct _kResolutionMap_t {
    const char* m_rdkResName;
    const char* m_boardResName;
}ResolutionMap_t;

/**
 * @brief defines sysfs, dev and parameter file mapping with index and open flags
 *
 * This structure defines the sysfs file path, name and open flags mapped with index.
 *
 */
typedef struct _devsys_file_map_t {
    uint8_t       m_index;
    sysFileType_t m_fileType;
    const char*         m_path;
    const char*         m_file;
    int           m_openFlags;
}devsysfs_file_map_t;

struct SuppResolutionlist
{
        int vic;
        const char *res_name;
};

#define MAX_PORTS       HDMI_MAX

#define ARRAY_SIZE(t)      (sizeof(t) / sizeof(t[0]))

#define IS_VIDEO_PORT_TYPE_SUPPORT(t)      ( dsVIDEOPORT_TYPE_HDMI == (t) )

#define IS_AUDIO_PORT_TYPE_SUPPORT(t)      ( dsAUDIOPORT_TYPE_HDMI == (t) || dsAUDIOPORT_TYPE_SPDIF == (t) || dsAUDIOPORT_TYPE_SPEAKER == (t))

#define HDMI_SYS_PATH      "/sys/class/amhdmitx/amhdmitx"
#define VIDEO_SYS_PATH     "/sys/class/video"
#define DISPLAY_SYS_PATH   "/sys/class/display"
#define HDMI_PARAM_PATH    "/sys/module/hdmitx20/parameters"

#define AMAUDIO_SYS_PATH   "/sys/class/amaudio"
#define AUDIODSP_SYS_PATH  "/sys/class/audiodsp"
#define VOLUME_SYS_PATH    "/sys/class/aml_i2s"

void findBoardResolution (const char* rdkResname, char* out_boardResName) ;
void findRDKResolution ( const char* boardResName, char* out_rdkResName ) ;
dsVideoPortResolution_t* getResolutionRef (const char* name) ;

#endif
