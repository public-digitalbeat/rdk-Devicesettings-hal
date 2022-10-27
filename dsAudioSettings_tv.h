#ifndef _DS_AUDIOOUTPUTPORTSETTINGS_H
#define _DS_AUDIOOUTPUTPORTSETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif


#include "dsUtl.h"
#include "dsTypes.h"

namespace  {


/*
 * Setup the supported configurations here.
 */
static const dsAudioPortType_t 			kSupportedPortTypes[] 				= { dsAUDIOPORT_TYPE_HDMI, dsAUDIOPORT_TYPE_SPDIF, dsAUDIOPORT_TYPE_SPEAKER };
static const dsAudioEncoding_t 		kSupportedHDMIEncodings[]			= { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3};
static const dsAudioCompression_t 	kSupportedHDMICompressions[] 		= { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
static const dsAudioStereoMode_t 	kSupportedHDMIStereoModes[] 		= { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, };
static const dsAudioEncoding_t 		kSupportedSPDIFEncodings[] 			= { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
static const dsAudioCompression_t 	kSupportedSPDIFCompressions[] 		= { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
static const dsAudioStereoMode_t 	kSupportedSPDIFStereoModes[] 		= { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, };
static const dsAudioEncoding_t          kSupportedSPEAKEREncodings[]                      = { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
static const dsAudioCompression_t       kSupportedSPEAKERCompressions[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
static const dsAudioStereoMode_t        kSupportedSPEAKERStereoModes[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, };

static const dsAudioTypeConfig_t 	kConfigs[]= {
		{
		/*.typeId = */					dsAUDIOPORT_TYPE_HDMI,
		/*.name = */					"HDMI", //HDMI
		/*.numSupportedCompressions = */dsUTL_DIM(kSupportedHDMICompressions),
		/*.compressions = */			kSupportedHDMICompressions,
		/*.numSupportedEncodings = */	dsUTL_DIM(kSupportedHDMIEncodings),
		/*.encodings = */				kSupportedHDMIEncodings,
		/*.numSupportedStereoModes = */	dsUTL_DIM(kSupportedHDMIStereoModes),
		/*.stereoModes = */				kSupportedHDMIStereoModes,
		},
		{
		/*.typeId = */					dsAUDIOPORT_TYPE_SPDIF,
		/*.name = */					"SPDIF", //SPDIF
		/*.numSupportedCompressions = */dsUTL_DIM(kSupportedSPDIFCompressions),
		/*.compressions = */			kSupportedSPDIFCompressions,
		/*.numSupportedEncodings = */	dsUTL_DIM(kSupportedSPDIFEncodings),
		/*.encodings = */				kSupportedSPDIFEncodings,
		/*.numSupportedStereoModes = */	dsUTL_DIM(kSupportedSPDIFStereoModes),
		/*.stereoModes = */				kSupportedSPDIFStereoModes,
		},
                {
                /*.typeId = */                                  dsAUDIOPORT_TYPE_SPEAKER,
                /*.name = */                                    "SPEAKER", //SPEAKER
                /*.numSupportedCompressions = */dsUTL_DIM(kSupportedSPEAKERCompressions),
                /*.compressions = */                    kSupportedSPEAKERCompressions,
                /*.numSupportedEncodings = */   dsUTL_DIM(kSupportedSPEAKEREncodings),
                /*.encodings = */                               kSupportedSPEAKEREncodings,
                /*.numSupportedStereoModes = */ dsUTL_DIM(kSupportedSPEAKERStereoModes),
                /*.stereoModes = */                             kSupportedSPEAKERStereoModes,
                }

};

static const dsVideoPortPortId_t connectedVOPs[dsAUDIOPORT_TYPE_MAX][dsVIDEOPORT_TYPE_MAX] = {
		{/*VOPs connected to LR Audio */

		},
		{/*VOPs connected to HDMI Audio */
				{dsVIDEOPORT_TYPE_HDMI, 0},
		},
		{/*VOPs connected to SPDIF Audio */
				{dsVIDEOPORT_TYPE_HDMI, 0},
		},
                {/*VOPs connected to SPEAKER Audio */
                                {dsVIDEOPORT_TYPE_HDMI, 0},
                },
};

static const dsAudioPortConfig_t kPorts[] = {
		{
		/*.typeId = */ 					{dsAUDIOPORT_TYPE_HDMI, 0},
		/*.connectedVOPs = */			connectedVOPs[dsAUDIOPORT_TYPE_HDMI],
		},
		{
		/*.typeId = */ 					{dsAUDIOPORT_TYPE_SPDIF, 0},
		/*.connectedVOPs = */			connectedVOPs[dsAUDIOPORT_TYPE_SPDIF],
		},
                {
                /*.typeId = */                                  {dsAUDIOPORT_TYPE_SPEAKER, 0},
                /*.connectedVOPs = */                   connectedVOPs[dsAUDIOPORT_TYPE_HDMI],
                },
};

}
#ifdef __cplusplus
}
#endif

#endif

