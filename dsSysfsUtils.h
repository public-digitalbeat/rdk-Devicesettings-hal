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

#ifndef _DS_SYSFS_UTILS_H
#define _DS_SYSFS_UTILS_H

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "dsTypes.h"
#include <stdio.h>
#include <stdarg.h>
#include <linux/version.h>

#define DEBUG_ENABLE
#ifdef DEBUG_ENABLE
#define DBG(fmt, args...)   fprintf(stderr, "[DSHAL_DBG] func %s line %d " fmt, __func__, __LINE__, ##args)
#define ERR(fmt, args...)   fprintf(stderr, "[DSHAL_ERR] func %s line %d " fmt, __func__, __LINE__, ##args)
#else
#define DBG(fmt, args...)
#define ERR(fmt, args...)
#endif /* DEBUG_ENABLE */

#ifdef AML_STB
#define SYSFS_HDMITX_HDCPMODE               "/sys/class/amhdmitx/amhdmitx0/hdcp_mode"
#define SYSFS_HDMITX_HDCPVERSION            "/sys/class/amhdmitx/amhdmitx0/hdcp_ver"
#define SYSFS_DRM_CARD0_HDMISTATUS          "/sys/class/drm/card0-HDMI-A-1/status"
#define SYSFS_HDMITX_ATTR                   "/sys/class/amhdmitx/amhdmitx0/attr"
#define SYSFS_HDMITX_HDR_STATUS             "/sys/class/amhdmitx/amhdmitx0/hdmi_hdr_status"
#define SYSFS_HDMITX_SINK_AUDIO_CAPABILITY  "/sys/class/amhdmitx/amhdmitx0/aud_cap"
#define SYSFS_HDMITX_EDID_PARSED            "/sys/class/amhdmitx/amhdmitx0/edid"
#define SYSFS_HDMITX_DEBG                   "/sys/class/amhdmitx/amhdmitx0/debug"


#define SYSFS_DOLBYVISION_SUPPORT_INFO     "/sys/class/amdolby_vision/support_info"
#define SYSMODULE_DRM_HDMITX_DOLBYVISION_ENABLE     "/sys/module/aml_drm/parameters/crtc_dv_enable"


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
#define SYSCLASS_HDMITX_DOLBYVISION_DEBUG       "/sys/class/amdolby_vision/debug"
#define SYSMODULE_HDMITX_DOLBYVISION_DEBUG      "/sys/module/aml_media/parameters/debug_dolby"
#define SYSMODULE_HDMITX_DOLBYVISION_POLICY     "/sys/module/aml_media/parameters/dolby_vision_policy"

#ifndef MS_DELAY_HOTPLUG_CB
#define MS_DELAY_HOTPLUG_CB 100 /* Delay in millisec added between hotplug udev event to call back invocation. */
#endif

#else
#define SYSMODULE_HDMITX_DOLBYVISION_DEBUG      "/sys/module/amdolby_vision/parameters/debug_dolby"
#define SYSMODULE_HDMITX_DOLBYVISION_POLICY     "/sys/module/amdolby_vision/parameters/dolby_vision_policy"
#endif

#define SYSCLASS_HDMITX_SINK_DOLBYVISION_CAPABILITY "/sys/class/amhdmitx/amhdmitx0/dv_cap"
#define DOLBY_VISION_KERNEL_MODULE_NAME "dovi" /* Assuming DolbyVision Kernel Module name will have this string. */
#endif /* AML_STB */

int amsysfs_set_sysfs_str(const char *path, const char *val);
int amsysfs_get_sysfs_str(const char *path, char *valstr, int size);
int amsysfs_set_sysfs_int(const char *path, int val);
int amsysfs_get_sysfs_int(const char *path);

char *strnstr(const char *s, const char *find, size_t slen);

#endif

