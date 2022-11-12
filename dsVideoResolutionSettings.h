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
		{   /*480p*/
			/*.name = */					"480p",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_720x480,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_4x3,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_59dot94,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*576p50*/
			/*.name = */					"576p50",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_720x576,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_4x3,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_50,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*720p24*/
			/*.name = */					"720p24",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1280x720,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_24,
			/*.interlaced = */				_PROGRESSIVE,
		},
        {   /*720p25  */
			/*.name = */					"720p25",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1280x720,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_25,
			/*.interlaced = */				_PROGRESSIVE,
		},
        {   /*720p30  */
			/*.name = */					"720p30",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1280x720,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_30,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*720p50  */
			/*.name = */					"720p50",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1280x720,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_50,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*720p60 */
			/*.name = */					"720p60",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1280x720,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_59dot94,
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
		{   /*1080p25*/
			/*.name = */					"1080p25",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_25,
			/*.interlaced = */				_PROGRESSIVE,
		},
		{   /*1080p30*/
			/*.name = */					"1080p30",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_30,
			/*.interlaced = */				_PROGRESSIVE,
		},
        {   /*1080p50*/
            /*.name = */                    "1080p50",
            /*.pixelResolution = */         dsVIDEO_PIXELRES_1920x1080,
            /*.aspectRatio = */             dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */        dsVIDEO_SSMODE_2D,
            /*.frameRate = */               dsVIDEO_FRAMERATE_50,
            /*.interlaced = */              _PROGRESSIVE,
        },
        {   /*1080p60 - Default - AutoSelect */
            /*.name = */					"1080p60",
            /*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
            /*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
            /*.frameRate = */				dsVIDEO_FRAMERATE_60,
            /*.interlaced = */				_PROGRESSIVE,
        },
		{   /*1080i50*/
			/*.name = */					"1080i50",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_50,
			/*.interlaced = */				_INTERLACED,
		},
		{   /*1080i60*/
			/*.name = */					"1080i60",
			/*.pixelResolution = */			dsVIDEO_PIXELRES_1920x1080,
			/*.aspectRatio = */				dsVIDEO_ASPECT_RATIO_16x9,
			/*.stereoscopicMode = */		dsVIDEO_SSMODE_2D,
			/*.frameRate = */				dsVIDEO_FRAMERATE_60,
			/*.interlaced = */				_INTERLACED,
		},
        {   /*2160p24*/
            /*.name = */                            "2160p24",
            /*.pixelResolution = */                 dsVIDEO_PIXELRES_3840x2160,
            /*.aspectRatio = */                     dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
            /*.frameRate = */                       dsVIDEO_FRAMERATE_24,
            /*.interlaced = */                      _PROGRESSIVE,
        },
        {   /*2160p25*/
            /*.name = */                            "2160p25",
            /*.pixelResolution = */                 dsVIDEO_PIXELRES_3840x2160,
            /*.aspectRatio = */                     dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
            /*.frameRate = */                       dsVIDEO_FRAMERATE_25,
            /*.interlaced = */                      _PROGRESSIVE,
        },
        {   /*2160p30*/
            /*.name = */                             "2160p30",
            /*.pixelResolution = */                 dsVIDEO_PIXELRES_3840x2160,
            /*.aspectRatio = */                     dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
            /*.frameRate = */                       dsVIDEO_FRAMERATE_30,
            /*.interlaced = */                      _PROGRESSIVE,
        },
        {   /*2160p50*/
            /*.name = */                            "2160p50",
            /*.pixelResolution = */                 dsVIDEO_PIXELRES_3840x2160,
            /*.aspectRatio = */                     dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
            /*.frameRate = */                       dsVIDEO_FRAMERATE_50,
            /*.interlaced = */                      _PROGRESSIVE,
        },
        {   /*2160p60*/
            /*.name = */                            "2160p60",
            /*.pixelResolution = */                 dsVIDEO_PIXELRES_3840x2160,
            /*.aspectRatio = */                     dsVIDEO_ASPECT_RATIO_16x9,
            /*.stereoscopicMode = */                dsVIDEO_SSMODE_2D,
            /*.frameRate = */                       dsVIDEO_FRAMERATE_59dot94,
            /*.interlaced = */                      _PROGRESSIVE,
        },
};

static const int kDefaultResIndex = 11; //Pick one resolution from kResolutions[] as default

#ifdef __cplusplus
}
}
#endif

#endif /* VIDEORESOLUTIONSETTINGS_H_ */


