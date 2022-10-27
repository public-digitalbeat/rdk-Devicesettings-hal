#ifndef _DS_VIDEOOUTPUTPORTSETTINGS_H_
#define _DS_VIDEOOUTPUTPORTSETTINGS_H_

#include "dsTypes.h"
#include "dsUtl.h"
#include "dsVideoResolutionSettings.h"


#ifdef __cplusplus
extern "C" {
#endif

namespace {
/*
 * Setup the supported configurations here.
 */
static const dsVideoPortType_t kSupportedPortTypes[] = { dsVIDEOPORT_TYPE_HDMI };

static const dsVideoPortTypeConfig_t kConfigs[]= {
		{
		/*.typeId = */					dsVIDEOPORT_TYPE_HDMI,
		/*.name = */ 					"HDMI",
		/*.dtcpSupported = */			false,
		/*.hdcpSupported = */			true,
		/*.restrictedResollution = */	-1,
		/*.numSupportedResolutions = */ dsUTL_DIM(kResolutions), // 0 means "Info available at runtime"
		/*.supportedResolutons = */     kResolutions,
		},
};

static const dsVideoPortPortConfig_t kPorts[] = {
		{
		/*.typeId = */ 					{dsVIDEOPORT_TYPE_HDMI, 0},
		/*.connectedAOP */              {dsAUDIOPORT_TYPE_SPEAKER, 0},
		/*.defaultResolution = */		"2160p60"
		},
};

}

#ifdef __cplusplus
}
#endif


#endif /* VIDEOOUTPUTPORTSETTINGS_H_ */
