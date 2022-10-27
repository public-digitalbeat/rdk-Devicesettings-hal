#ifndef _DS_VIDEORESOLUTIONSETTINGS_H_
#define _DS_VIDEORESOLUTIONSETTINGS_H_

#include "dsTypes.h"

#ifdef __cplusplus
extern "C" {
namespace {
#endif


#define  _INTERLACED true
#define _PROGRESSIVE false

#define dsVideoPortRESOLUTION_NUMMAX 32

/* List all supported resolutions here */

static dsVideoPortResolution_t kResolutions[] = {
		{   /*480i*/
			/*.name = */					"480i",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_720x480,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_4x3,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_29dot97,
			/*.interlaced = */				_INTERLACED,
		},
		{   /*480p*/
			/*.name = */					"480p",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_720x480,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_4x3,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_59dot94,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*576p*/
			/*.name = */					"576p50",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_720x576,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_4x3,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_50,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*720p - Default - AutoSelect */
			/*.name = */					"720p",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1280x720,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_59dot94,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*720p - Default - AutoSelect */
			/*.name = */					"720p50",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1280x720,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_50,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*1080p24*/
			/*.name = */					"1080p24",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_24,
			/*.interlaced = */				_PROGRESSIVE,
		},
                {   /*1080p60hz*/
                        /*.name = */                                    "1080p60",
                        /*.pixelResolution = */                 dsVIDEO_PIXELRES_1920x1080,
                        /*.aspectRatio = */                             dsVIDEO_ASPECT_RATIO_16x9,
                        /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
                        /*.frameRate = */                               dsVIDEO_FRAMERATE_60,
                        /*.interlaced = */                              _PROGRESSIVE,
                },
		{   /*1080p60*/
			/*.name = */					"1080p",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_59dot94,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*1080i*/
			/*.name = */					"1080i50hz",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_50,
			/*.interlaced = */				_INTERLACED,
		},
		{   /*1080p30*/
			/*.name = */					"1080i60hz",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_59dot94,
			/*.interlaced = */				_INTERLACED,
		},
        {   /*2160p30*/
            /*.name = */                                    "2160p30",
            /*.pixelResolution = */                 dsVIDEO_PIXELRES_3840x2160,
            /*.aspectRatio = */                     dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
            /*.frameRate = */                       dsVIDEO_FRAMERATE_30,
            /*.interlaced = */                      _PROGRESSIVE,
        },
        {   /*2160p60*/
            /*.name = */                                    "2160p60",
            /*.pixelResolution = */                 dsVIDEO_PIXELRES_3840x2160,
            /*.aspectRatio = */                     dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
            /*.frameRate = */                       dsVIDEO_FRAMERATE_59dot94,
            /*.interlaced = */                      _PROGRESSIVE,
        },
};

static const int kDefaultResIndex = 7; //Pick one resolution from kResolutions[] as default

#ifdef __cplusplus
}
}
#endif

#endif /* VIDEORESOLUTIONSETTINGS_H_ */


