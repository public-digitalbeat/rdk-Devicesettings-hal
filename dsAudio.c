#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dsVideoPort.h>

#include "dsAudio.h"
#include "dsError.h"
#include "dsUtl.h"
#include "dsS905D.h"
#include <ctype.h>
#include <math.h>
#include "audio_if.h"

#include <tinyalsa/asoundlib.h>
#include "dsSysfsUtils.h"
#include "volume-ctl-clnt.h"

#define FILE_NAME_SIZE   64

#define AUDIO_MAX_dB 0
#define AUDIO_MIN_dB -60
#define AUDIO_OPTIMAL_LEVEL 0
#define RESET		-1
#define CODEC_PCM 0
#define CODEC_AC3 2
#define CODEC_EAC3 4

#define HDMI_SPDIF_RAW_PCM 0
#define SPDIF_RAW_AC3_EAC3 1
#define HDMI_RAW_AC3_EAC3 2

#define AUDIO_COM_DATA "audio config"
#define POSITION_NUMBER 14
#define MUTE_DATA_LENGTH 3

#define DEFAULT_DB 0
#define DEFAULT_GAIN 1

enum _devSysFiles_t {
    MULTISTREAM_DECODE_FILE_INDEX,
    HDMI_MUTE_SYS_FILE,
    DIGITAL_RAW_SYS_FILE,
    DIGITAL_CODEC_SYS_FILE,
    VOLUME_SYS_FILE,
    /* add new file index before here */
    DEV_SYS_FILES
};

typedef struct _APortHandle_t {
        dsAudioPortType_t m_aType;
        int8_t m_index;
        bool m_IsEnabled;
} APortHandle_t;

static devsysfs_file_map_t sys_files[DEV_SYS_FILES] = { \
                           { MULTISTREAM_DECODE_FILE_INDEX, PARAM_FILE, AMAUDIO_SYS_PATH, "dolby_enable", O_RDONLY },
                           { HDMI_MUTE_SYS_FILE,     SYS_FILE, HDMI_SYS_PATH, "config", O_RDWR },
                           { DIGITAL_CODEC_SYS_FILE,   PARAM_FILE, AUDIODSP_SYS_PATH, "digital_codec", O_RDWR },
                           { DIGITAL_RAW_SYS_FILE,   PARAM_FILE, AUDIODSP_SYS_PATH, "digital_raw", O_RDWR },
			   { VOLUME_SYS_FILE, PARAM_FILE, VOLUME_SYS_PATH, "volume", O_RDWR },
                         };



/*********** Dolby MS12 Runtime Parameters ********************/

#define SPACE \
    " "
#define MS12_RUNTIME_CMD \
    "ms12_runtime="
#define DAP_DISABLE_SURROUND_DECODER_CMD \
    "-dap_surround_decoder_enable 0"
#define DAP_ENABLE_SURROUND_DECODER_CMD \
    "-dap_surround_decoder_enable 1"
#define DAP_BOOST_TREBLE \
    "-dap_graphic_eq 1,2,8000,16000,500,500"
#define DAP_BASS_ENHANCER \
    "-dap_bass_enhancer 1,"
#define DAP_BASS_ENHANCER_DEFAULT_ON \
    "-dap_bass_enhancer 1,192,200,16"
#define DAP_BASS_ENHANCER_OFF \
    "-dap_bass_enhancer 0"
#define DAP_VOLUME_LEVELER_MAX \
    "-dap_leveler 1 10"
#define DAP_SURROUND_VIRTUALIZER \
    "-dap_surround_virtualizer 1,"
#define DAP_SURROUND_VIRTUALIZER_OFF \
    "-dap_surround_virtualizer 0"
#define DAP_MI_STEERING_ON \
    "-dap_mi_steering 1"
#define DAP_MI_STEERING_OFF \
    "-dap_mi_steering 0"
#define DAP_POST_GAIN \
    "-dap_gains "
#define DAP_VOLUME_LEVELLER \
    "-dap_leveler 1,"
#define DAP_VOLUME_LEVELLER_OFF \
    "-dap_leveler 1,0"
#define DAP_VOLUME_LEVELLER_AUTO \
    "-dap_leveler 2,0"
#define DAP_DRC_LINE \
    "-dap_drc 0 -b 100 -c 100"
#define DAP_DRC_RF \
    "-dap_drc 1 -b 0 -c 0"
#define DAP_DIALOGUE_ENHANCER_OFF \
    "-dap_dialogue_enhancer 0"
#define DAP_DIALOGUE_ENHANCER \
    "-dap_dialogue_enhancer 1,"
#define DAP_INTELLIGENT_EQ_OFF \
    "-dap_ieq 0"
#define DAP_INTELLIGENT_EQ_OPEN \
    "-dap_ieq 1,10,16,65,136,467,634,841,1098,1812,2302,3663,4598,5756,7194,8976,11186,13927,17326,117,133,141,149,175,185,200,236,228,213,182,132,110,68,-27,-240"
#define DAP_INTELLIGENT_EQ_RICH \
    "-dap_ieq 1,10,16,65,136,467,634,841,1098,1812,2302,3663,4598,5756,7194,8976,11186,13927,17326,67,95,168,201,189,242,221,192,168,139,102,57,35,9,-55,-235"
#define DAP_INTELLIGENT_EQ_FOCUSED \
    "-dap_ieq 1,10,16,65,136,467,634,841,1098,1812,2302,3663,4598,5756,7194,8976,11186,13927,17326,-419,-112,113,160,165,80,79,98,64,70,44,-71,-33,-109,-238,-411"
#define DAP_GRAPHIC_EQ_OFF \
    "-dap_graphic_eq 0"
#define DAP_GRAPHIC_EQ_OPEN \
    "-dap_graphic_eq 1,20,65,136,223,332,467,634,841,1098,1416,1812,2302,2909,3663,4598,5756,7194,8976,11186,13927,17326,117,133,188,176,141,149,175,185,185,200,236,228,213,182,132,110,68,-27,-240"
#define DAP_GRAPHIC_EQ_RICH \
    "-dap_graphic_eq 1,20,65,136,223,332,467,634,841,1098,1416,1812,2302,2909,3663,4598,5756,7194,8976,11186,13927,17326,67,95,172,163,168,201,189,242,196,221,192,186,168,139,102,57,35,9,-55,-235"
#define DAP_GRAPHIC_EQ_FOCUSED \
    "-dap_graphic_eq 1,20,65,136,223,332,467,634,841,1098,1416,1812,2302,2909,3663,4598,5756,7194,8976,11186,13927,17326,-419,-112,75,116,113,160,165,80,61,79,98,121,64,70,44,-71,-33,-109,-238,-411"
#define DRC_LINE \
    "-drc 0 -bs 100 -cs 100"
#define DRC_RF \
    "-drc 1 -bs 0 -cs 0"
#define MS12_ATMOS_LOCK_ON \
    "-atmos_lock 1"
#define MS12_ATMOS_LOCK_OFF \
    "-atmos_lock 0"

static bool audioCompressionEnabled = false;
static int  audioCompressionLevel = 0;
static bool dolbyVolumeEnabled = false;
static bool LEConfigEnabled = false;
static int dialogueEnhancementLevel = 0;
static int volumeLeveller = 0;
static bool surroundDecoderEnabled = false;
static int drcMode = 0;
static int bassBoost = 0;
static int surroundVirtualizerLevel = 0;
static bool miSteeringEnabled = false;
static int dialogEnhancerLevel = 0;
static float speakerGain = 0;
static int intelligentEQMode = 0;
static uint32_t audioDelay = 0;
static int graphicEQMode = 0;
static int ms12Profile = 0;

/************ End of Dolby MS12 params ***************/

/********** Tinymix Controls *********/

#define DEFAULT_AML_SOUND_CARD 0
#define MASTER_VOL "AMP Master Volume"

const char * MASTER_MUTE = "SPK mute";
const char * AUDIO_SPDIF_MUTE = "Audio spdif mute";
const char * AUDIO_HDMI_OUT_MUTE = "Audio hdmi-out mute";

int tinymix_get_value_percent(struct mixer *mixer, const char *control, int *val )
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_ctl_values;
    int num_values = 0;
    int percent = 0;

    if (isdigit(control[0])) {
        ctl = mixer_get_ctl(mixer, atoi(control));
    }
    else {
        ctl = mixer_get_ctl_by_name(mixer, control);
    }

    if (!ctl) {
        fprintf(stderr, "Invalid mixer control\n");
        return -1;
    }

    type = mixer_ctl_get_type(ctl);

    if ((type != MIXER_CTL_TYPE_INT) && (type != MIXER_CTL_TYPE_BOOL)) {
        fprintf(stderr, "Invalid mixer control. not an integer\n");
        return -1;
    }
    num_values = mixer_ctl_get_num_values(ctl);
    if (num_values > 1) {
        fprintf(stderr, "mixer control. can not support int array\n");
        return -1;

    }
    if (type == MIXER_CTL_TYPE_INT) {
        percent = mixer_ctl_get_percent(ctl,0);
    }
    else if (type == MIXER_CTL_TYPE_BOOL) {
        percent = mixer_ctl_get_value(ctl,0);
    }
    else {
        fprintf(stderr, "mixer control. can not get percent\n");
        return -1;
    }

    return percent;

}
int tinymix_set_value_percent(struct mixer *mixer, const char *control, int percent )
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_ctl_values;
    int num_values = 0;

    if (isdigit(control[0])) {
        ctl = mixer_get_ctl(mixer, atoi(control));
    }
    else {
        ctl = mixer_get_ctl_by_name(mixer, control);
    }

    if (!ctl) {
        fprintf(stderr, "Invalid mixer control\n");
        return -1;
    }

    type = mixer_ctl_get_type(ctl);

    if ((type != MIXER_CTL_TYPE_INT) && (type != MIXER_CTL_TYPE_BOOL)) {
        fprintf(stderr, "Invalid mixer control. not an integer\n");
        return -1;
    }
    num_values = mixer_ctl_get_num_values(ctl);
    if (num_values > 1) {
        fprintf(stderr, "mixer control. can not support int array\n");
        return -1;
    }

    if (type == MIXER_CTL_TYPE_INT) {
        percent = mixer_ctl_set_percent(ctl,0,percent);
    }
    else if (type == MIXER_CTL_TYPE_BOOL) {
        percent = mixer_ctl_set_value(ctl,0,percent);
    }
    else {
        fprintf(stderr, "mixer control. can not set percent\n");
        return -1;
    }

    return 0;

}
float tinymix_get_volume(){
    struct mixer *mixer;
    int volume, percent;

    mixer = mixer_open(DEFAULT_AML_SOUND_CARD);

    percent=tinymix_get_value_percent(mixer, MASTER_VOL, &volume);
   /* {
        fprintf(stderr, "Invalid volume\n");
        volume = -1;
    }*/
    mixer_close(mixer);
    return float(percent);
}

int tinymix_set_volume(int volume) {
    struct mixer *mixer;
    int ret=0;

    mixer = mixer_open(DEFAULT_AML_SOUND_CARD);
    ret=tinymix_set_value_percent(mixer,MASTER_VOL,volume);
    mixer_close(mixer);
    return ret;
}

bool tinymix_get_mute(const char *command) {
    struct mixer *mixer;
    int mute=0,percent=0;

    mixer = mixer_open(DEFAULT_AML_SOUND_CARD);

    percent=tinymix_get_value_percent(mixer, command, &mute);
   /* {
        fprintf(stderr, "Invalid mute\n");
    }*/

    mixer_close(mixer);

    if (percent==0)
        return false;
    else
        return true;
}

int tinymix_set_mute(bool mute, const char *command) {
    struct mixer *mixer;
    int mute_percent=0;
    int ret=0;

    if (mute)
        mute_percent=100;
    else
        mute_percent=0;

    mixer = mixer_open(DEFAULT_AML_SOUND_CARD);
    ret=tinymix_set_value_percent(mixer, command, mute_percent);

    mixer_close(mixer);
    return ret;
}






/*********** Tinymix controls end ******/

//Volume Controls
//Required for fixing no audio issue in Dolby enabled boards
static bool speakerMute = false;

static dsAudioStereoMode_t audioModeSPDIF = dsAUDIO_STEREO_UNKNOWN;
static dsAudioStereoMode_t audioModeNonSPDIF = dsAUDIO_STEREO_UNKNOWN;

#define VALIDATE_TYPE_AND_INDEX(t, idx) { \
    switch ( t ) { \
    case dsAUDIOPORT_TYPE_HDMI: \
        if ( HDMI_MAX <= idx ) \
           return dsERR_GENERAL; \
    break; \
    case dsAUDIOPORT_TYPE_SPDIF: \
        if ( SPDIF_MAX <= idx ) \
           return dsERR_GENERAL; \
    break; \
    case dsAUDIOPORT_TYPE_SPEAKER: \
        if ( SPEAKER_PORT_MAX <= idx ) \
           return dsERR_GENERAL; \
    break; \
    default: \
           return dsERR_GENERAL; \
    break; \
    } \
}

#define IS_PORT_INIT() \
       if ( ! isAudioPortInitialized ) \
           return dsERR_INVALID_STATE;

static bool isAudioPortInitialized = false;


static APortHandle_t _handles[dsAUDIOPORT_TYPE_MAX][MAX_PORTS];


static const devsysfs_file_map_t* get_sysfs ( const uint8_t type, const uint8_t findex ) {
    int i;
    switch (type) {
    case dsAUDIOPORT_TYPE_HDMI:
    case dsAUDIOPORT_TYPE_SPDIF:
        for ( i = 0; DEV_SYS_FILES > i; i++) {
            if ( sys_files[i].m_index == findex )
                return &sys_files[i];
        }
    break;
    default:
    break;
    }
    return NULL;
}
#define IS_FILE_INDEX_VALID(i) (i < DEV_SYS_FILES)
static void get_sysfs_filename(const uint8_t type, const uint8_t port, const uint8_t findex, char* sysfs) {
    if ( NULL == sysfs || ! IS_FILE_INDEX_VALID ( findex ) )
        return;
    const devsysfs_file_map_t* sysdev = get_sysfs ( type, findex );
    if ( NULL == sysdev ) {
        printf("DSHAL: AudioPort: Unable to get the sysfs file\n");
        return;
    }
    switch(sysdev->m_fileType) {
    case SYS_FILE:
        sprintf(sysfs, "%s%d/%s", sysdev->m_path, port, sysdev->m_file);
        break;
    case DEV_FILE:
        sprintf(sysfs, "%s/%s%d", sysdev->m_path, sysdev->m_file, port);
        break;
    case PARAM_FILE:
        sprintf(sysfs, "%s/%s", sysdev->m_path, sysdev->m_file);
    break;
    default:
    break;
    }
}

static bool isValidAopHandle(int handle) {
	uint8_t type, i;
	for ( type = 0; dsAUDIOPORT_TYPE_MAX > type; type ++) {
		for ( i = 0; MAX_PORTS > i; i ++) {
			if ( (int)&_handles[type][i] == handle )
				return true;
		}
	}
	return false;

}

/**
 * @print Values in dB(decibels)
 * This function is used to return dB values
 * @param [in] dB for Min/Max dB of audio port
 */

static void print_dB(long dB)
{
	if (dB < 0) {
		printf("-%lidB\n", -dB % 100);
	} else {
		printf("%lidB\n", dB % 100);
	}
}

float db_volume_convert(float volume)
{
        if(volume >= 0.0)
                return 20.0 * log10(volume);
        else
                return 0.0;
}


static void InitHDMI(APortHandle_t* HDMIPortList) {
	if(NULL == HDMIPortList)
		return;

	uint8_t index;
	APortHandle_t* hport  = NULL;

	for(index = 0, hport = HDMIPortList; HDMI_MAX > index; index ++, hport++) {
		// initialize each hport's fields
		hport->m_aType     = dsAUDIOPORT_TYPE_HDMI;
		hport->m_index     = index;
		hport->m_IsEnabled = false;
	}
}

static void InitSPDIF(APortHandle_t* SPDIFPortList) {
        if(NULL == SPDIFPortList)
                return;

        uint8_t index;
        APortHandle_t* sport  = NULL;

        for(index = 0, sport = SPDIFPortList; SPDIF_MAX > index; index ++, sport++) {
                // initialize each sport's fields
                sport->m_aType     = dsAUDIOPORT_TYPE_SPDIF;
                sport->m_index     = index;
                sport->m_IsEnabled = false;
        }
}

static void InitSPEAKER(APortHandle_t* SpeakerPortList) {
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
        if(NULL == SpeakerPortList)
                return;

        uint8_t index;
        APortHandle_t* sport  = NULL;
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

        for(index = 0, sport = SpeakerPortList; SPEAKER_PORT_MAX > index; index ++, sport++) {
                // initialize each sport's fields
                sport->m_aType     = dsAUDIOPORT_TYPE_SPEAKER;
                sport->m_index     = index;
                sport->m_IsEnabled = false;
        }
}



/**
 * @brief Initialize underlying Audio Output Port module
 *
 * This function must initialize all the audio specific output ports.
 *
 * @param None
 * @return dsError_t Error Code.
 */

dsError_t dsAudioPortInit()
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;
	uint8_t type;

	if ( ! isAudioPortInitialized ) {
		for(type = dsAUDIOPORT_TYPE_ID_LR; dsAUDIOPORT_TYPE_MAX > type; type++) {
			switch ( type ) {
				case dsAUDIOPORT_TYPE_HDMI:
					InitHDMI ( _handles[type] );
					break;

				case dsAUDIOPORT_TYPE_SPDIF:
					InitSPDIF ( _handles[type] );
					break;

                case dsAUDIOPORT_TYPE_SPEAKER:
                    InitSPEAKER ( _handles[type] );
                    break;

				default:
					break;
			}
		}

		if ( dsERR_NONE == ret ) {
			isAudioPortInitialized = true;
			printf("DSHAL: AudioPort: initialized successfully!\n");
		}
	}
	return ret;
}

/**
 * @brief To get the audio port handle
 *
 * This function will get the handle for the type of Audio port requested. Must return
 * dsERR_OPERATION_NOT_SUPPORTED when the unavailable Audio port is requested.
 *
 * @param [in] type    Type of Audio Output Port (HDMI, SPDIF)
 * @param [in] index   Index of Audio output port (0, 1, ...)
 * @param [out] *handle Pointer to get the handle
 * @return dsError_t Error code.
 */

dsError_t  dsGetAudioPort(dsAudioPortType_t type, int index, int *handle)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;

	if (NULL == handle || (0 > index && MAX_PORTS <= index) || !dsAudioType_isValid(type)) {
		ret = dsERR_INVALID_PARAM;
	}

	else if ( !IS_AUDIO_PORT_TYPE_SUPPORT( type ) ) {
		ret = dsERR_OPERATION_NOT_SUPPORTED;
	}
	if (dsERR_NONE == ret) {
		IS_PORT_INIT();

		VALIDATE_TYPE_AND_INDEX ( type, index );

		*handle = (int)&_handles[type][index];
	}
	else {
	}
	return ret;
}

static int8_t get_codec_type ( const uint8_t type, const uint8_t port ) {
	char sysfs[FILE_NAME_SIZE] = {0};

	get_sysfs_filename ( type, port, DIGITAL_CODEC_SYS_FILE, sysfs );
	return amsysfs_get_sysfs_int ( sysfs );
}

/**
 * @brief Get the encoding type used in the Audio Output
 *
 * This function will get the type of encoding used in a given output audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *encoding   Type of Encoding used
 * @return dsError_t Error code.
 */

dsError_t dsGetAudioEncoding(int handle, dsAudioEncoding_t *encoding)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;
	if ( !isValidAopHandle (handle) || encoding == NULL){
		ret = dsERR_INVALID_PARAM;

	}

	if ( dsERR_NONE == ret ) {
		APortHandle_t *aPHandle = (APortHandle_t*)handle;

		VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );

		switch(aPHandle->m_aType){
			case dsAUDIOPORT_TYPE_HDMI:
			case dsAUDIOPORT_TYPE_SPDIF:
				uint8_t result;
				result = get_codec_type ( aPHandle->m_aType, aPHandle->m_index );
				if ( result == CODEC_PCM){
					*encoding = dsAUDIO_ENC_PCM;
				}
				else if ( result == CODEC_AC3 ){
					*encoding = dsAUDIO_ENC_AC3;
				}
				else if ( result == CODEC_EAC3 ){
					*encoding = dsAUDIO_ENC_EAC3;
				}
				else {
					ret  = dsERR_GENERAL;
				}
				break;

			default:
				break;
		}
	}
	return ret;
}

/**
 * @brief To get the graphic equalizer mode
 *
 * This function will get the graphic EQ mode used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *mode  Graphic EQ mode
 * @return dsError_t Error code.
 */

//TODO: To be defined properly
dsError_t  dsGetGraphicEqualizerMode(int handle, int *mode)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || mode == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *mode = graphicEQMode;
    }

    return ret;
}

/**
 * @brief To get the MS12 audio profile
 *
 * This function will get the MS12 audio profile
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] *profile   profile used
 * @return dsError_t Error code.
 */

dsError_t  dsGetMS12Profile(int handle, int *profile)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *profile = ms12Profile;
    }

    return ret;
}

/**
 * @brief To get the Dolby volume leveller
 *
 * This function will get the Volume leveller value used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *level  Volume Leveller value
 * @return dsError_t Error code.
 */

dsError_t  dsGetVolumeLeveller(int handle, int* level)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || level == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *level = volumeLeveller;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}

/**
 * @brief To get the audio Bass
 *
 * This function will get the Bass used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *boost      Bass Enhancer boost value
 * @return dsError_t Error code.
 */

dsError_t  dsGetBassEnhancer(int handle, int *boost)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || boost == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *boost = bassBoost;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}

/**
 * @brief To get the audio Surround Decoder
 *
 * This function will get enable/disable status of surround decoder
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *enabled  Surround Decoder enable/disable
 * @return dsError_t Error code.
 */

dsError_t  dsIsSurroundDecoderEnabled(int handle, bool *enabled)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || enabled == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *enabled = surroundDecoderEnabled;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}

/**
 * @brief To get the DRC Mode
 *
 * This function will get the Dynamic Range Control used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *mode     line/RF mode
 * @return dsError_t Error code.
 */

dsError_t  dsGetDRCMode(int handle, int *mode)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || mode == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) /*&& (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)*/) {
        *mode = drcMode;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}

/**
 * @brief To get the audio Surround Virtualizer level
 *
 * This function will get the Surround Virtualizer level used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *boost    Surround virtualizer level
 * @return dsError_t Error code.
 */

dsError_t  dsGetSurroundVirtualizer(int handle, int* boost)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || boost == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *boost = surroundVirtualizerLevel;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}

/**
 * @brief To get the audio Media intelligent Steering
 *
 * This function will get the Media Intelligent Steerinf
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [out] *enabled      enable/disable MI Steering
 * @return dsError_t Error code.
 */

dsError_t  dsGetMISteering(int handle, bool *enabled)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || enabled == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *enabled = miSteeringEnabled;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}


/**
 * @brief To get the stereo mode used
 *
 * This function will get the stereo mode used in a given audio port.
 *
 * @param [in] handle        Handle for the Output Audio port
 * @param [out] *stereoMode  Type of stereo mode used
 * @return dsError_t Error code.
 */

dsError_t dsGetStereoMode(int handle, dsAudioStereoMode_t *stereoMode)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;
    if ( !isValidAopHandle (handle) || stereoMode == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPDIF)) {
        *stereoMode = audioModeSPDIF;
        printf("%s: Stereo Mode: %d\n",__FUNCTION__,*stereoMode);
    } else {
        *stereoMode = audioModeNonSPDIF;
    }

    return ret;
}

dsError_t dsGetPersistedStereoMode (int handle, dsAudioStereoMode_t *stereoMode)
{
        return dsERR_NONE;
}

dsError_t dsGetStereoAuto (int handle, int *autoMode)
{
        return dsERR_NONE;
}

/**
 * @brief Get the audio gain
 *
 * This function will get the Gain in a given audio output port
 *
 * @param [in] handle  Handle for the Output Audio port
 * @param [out] *gain  Gain value in a given audio port
 * @return dsError_t Error code.
 */

dsError_t dsGetAudioGain (int handle, float *gain)
{
    #ifndef AML_STB
        dsError_t ret = dsERR_NONE;

        if( ! isValidAopHandle(handle) || gain == NULL) {
                ret = dsERR_INVALID_PARAM;
        }

        APortHandle_t *aPHandle = (APortHandle_t*)handle;

        if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
            *gain = speakerGain;
        }
        return ret;
        #else
        return dsERR_OPERATION_NOT_SUPPORTED;
        #endif
}

/**
 * @brief Get the current DB of a audio port
 *
 * This function will get the current DB value in a given Audio port
 *
 * @param [in] handle  Handle for the Output Audio port
 * @param [out] *db    DB value in a given Audio port
 * @return dsError_t Error code.
 */

dsError_t dsGetAudioDB (int handle, float *db)
{
        dsError_t ret = dsERR_NONE;

        if( ! isValidAopHandle(handle) || db == NULL) {
                ret = dsERR_INVALID_PARAM;
        }

        if ( dsERR_NONE == ret ) {
            *db = 0;
        }

        return ret;
}

static int8_t get_level ( const uint8_t index, char* bufData, const uint16_t size ) {
    char sysfs[FILE_NAME_SIZE] = {0};
    get_sysfs_filename ( dsAUDIOPORT_TYPE_HDMI, index, VOLUME_SYS_FILE, sysfs );
    return amsysfs_get_sysfs_str ( sysfs, bufData, size );
}

static int get_master_volume(float * level){
    int ret = 0;
    audio_hw_device_t *device;
    float vol = 0.0;

    ret = audio_hw_load_interface(&device);
    if (ret) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return ret;
    }

    ret = device->init_check(device);
    if (ret) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return -1;
    }

    ret = device->get_master_volume(device, &vol);
    if(ret){
        fprintf(stderr, "set_master_volume failed\n");
    }

    audio_hw_unload_interface(device);

    *level = vol*100.0;

    return ret;
}

/**
 * @brief To get the audio level
 *
 * This function is used to get the audio level in a given output audio port
 *
 * @param [in] handle   Handle for the Output Audio port
 * @param [out] *level  The audio level in a given port
 * @return dsError_t Error code.
 */

dsError_t dsGetAudioLevel (int handle, float *level)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;
    int err = 0;

    if( ! isValidAopHandle(handle) || level == NULL){
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );
    switch(aPHandle->m_aType){
        case dsAUDIOPORT_TYPE_HDMI:
        case dsAUDIOPORT_TYPE_SPDIF:
            err = get_master_volume(level);
            if(err){
                fprintf(stderr, "get_master_volume failed\n");
                ret = dsERR_GENERAL;
            }
            break;
        case dsAUDIOPORT_TYPE_SPEAKER:
            *level = tinymix_get_volume();
            break;
        default:
            break;
    }
	return ret;
}

/**
 * @brief To get the max db that the audio port can support
 *
 * This function is used to get the max db of a given ouput audio port
 *
 * @param [in] handle   Handle for the output audio port
 * @param [out] *maxDb  The max db in a given audio port
 * @return dsError_t Error code.
 */

dsError_t dsGetAudioMaxDB (int handle, float *maxDb)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;

	if( ! isValidAopHandle(handle)) {
		ret = dsERR_INVALID_PARAM;
	}
	if ( dsERR_NONE == ret ) {
		*maxDb = AUDIO_MAX_dB;
	}

	return ret;
}

/**
 * @brief To get the minimum db that the audio port can support
 *
 * This function is used to get the minimum db of a given ouput audio port
 *
 * @param [in] handle   Handle for the Output Audio port
 * @param [out] *minDb  The minimum db in a given audio port
 * @return dsError_t Error code.
 */

dsError_t dsGetAudioMinDB (int handle, float *minDb)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;

	if( ! isValidAopHandle(handle)) {
		ret = dsERR_INVALID_PARAM;
	}
	if ( dsERR_NONE == ret ) {
		*minDb = AUDIO_MIN_dB;
	}

	return ret;
}

/**
 * @brief To get the optimum audio level
 *
 * This function will be used to get the optimal audio level for a given audio port
 *
 * @param [in] handle          Handle for the Output Audio port
 * @param [out] *optimalLevel  The minimum db in a given audio port
 * @return dsError_t Error code.
 */

dsError_t dsGetAudioOptimalLevel (int handle, float *optimalLevel)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;
	if( ! isValidAopHandle(handle)) {
		ret = dsERR_INVALID_PARAM;
	}
	if ( dsERR_NONE == ret ) {
		*optimalLevel = AUDIO_OPTIMAL_LEVEL;
	}

	return ret;
}

/**
 * @brief To find whether the audio is loop thro'
 *
 * This function is used to check whether the given audio port is configured for loop thro'
 *
 * @param [in] handle      Handle for the Output Audio port
 * @param [out] *loopThru  True when output is loop thro'
 * @return dsError_t Error code.
 */

dsError_t dsIsAudioLoopThru (int handle, bool *loopThru)
{
	return dsERR_NONE;
}

/**
 * @brief To find the audio mute status
 *
 * This function is used to check whether the audio is muted or not
 *
 * @param [in] handle   Handle for the Output Audio port
 * @param [out] *muted  True when output is muted
 * @return dsError_t Error code.
 */
static int8_t hdmi_get_mute ( const uint8_t index, char* bufData, const uint16_t size ) {
    char sysfs[FILE_NAME_SIZE] = {0};
    get_sysfs_filename ( dsAUDIOPORT_TYPE_HDMI, index, HDMI_MUTE_SYS_FILE, sysfs );
    return amsysfs_get_sysfs_str ( sysfs, bufData, size );
}

static int get_master_mute(bool *mute){
    int ret = 0;
    audio_hw_device_t *device;

    ret = audio_hw_load_interface(&device);
    if (ret) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return ret;
    }

    ret = device->init_check(device);
    if (ret) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return -1;
    }

    ret = device->get_master_mute(device, mute);
    if(ret){
        fprintf(stderr, "set_master_volume failed\n");
    }

    audio_hw_unload_interface(device);

return ret;
}

dsError_t dsIsAudioMute (int handle, bool *muted)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;
    int err = 0;
    if(!isValidAopHandle(handle) || NULL == muted){
        ret = dsERR_INVALID_PARAM;
    }

    if ( dsERR_NONE == ret ) {
        APortHandle_t *aPHandle = (APortHandle_t*)handle;
        VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );

        switch(aPHandle->m_aType){
        case dsAUDIOPORT_TYPE_HDMI:
            *muted = tinymix_get_mute(AUDIO_HDMI_OUT_MUTE);
            break;

        case dsAUDIOPORT_TYPE_SPDIF:
            *muted = tinymix_get_mute(AUDIO_SPDIF_MUTE);
            break;

        case dsAUDIOPORT_TYPE_SPEAKER:
            *muted = tinymix_get_mute(MASTER_MUTE);
            break;

        default:
            break;
        }
    }
    return ret;
}


dsError_t  dsIsAudioPortEnabled(int handle, bool *enabled)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    bool muted = false;
    dsError_t ret = dsIsAudioMute(handle, &muted);
    *enabled = !muted;
    return ret;
}


dsError_t  dsEnableAudioPort(int handle, bool enabled)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;

    if ( ! isValidAopHandle(handle)) {
        ret = dsERR_INVALID_PARAM;
    }

    printf("%s: enable: %d\n",__FUNCTION__,enabled);
    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) ) {
        ret = dsSetAudioMute ( handle, !enabled);
    }

    return ret;
}

/**
 * @brief This function is used to enable or disable Loudness Equivalence feature.
 *
 * @param[in] handle      Handle of the Audio port.
 * @param[in] enable     Flag to control the LE features
 *                         (@a true to enable, @a false to disable)
 * @return Device Settings error code
 * @retval dsERR_NONE If API executed successfully.
 * @retval dsERR_GENERAL General failure.
 */
dsError_t  dsEnableLEConfig(int handle, const bool enable)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if ( ! isValidAopHandle(handle) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        if (enable) {
            err = device->set_parameters(device,
                    MS12_RUNTIME_CMD
                    DAP_VOLUME_LEVELLER_AUTO);
            LEConfigEnabled = true;
        } else {
            char volumeLeveller_param[50];
            memset( volumeLeveller_param, 0, sizeof(volumeLeveller_param) );
            strcat(volumeLeveller_param, MS12_RUNTIME_CMD);
            strcat(volumeLeveller_param, DAP_VOLUME_LEVELLER);
            char value[10];
            sprintf(value, "%d", volumeLeveller);
            strcat(volumeLeveller_param, value);
            err = device->set_parameters(device, volumeLeveller_param);
            LEConfigEnabled = false;
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            LEConfigEnabled = false;
            ret = dsERR_GENERAL;
        } else {
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, enable);
        }
    } else if (aPHandle->m_aType != dsAUDIOPORT_TYPE_SPEAKER){
        printf("%s: [Port: %d] Dolby MS12 parameter set not supported \n", __FUNCTION__,aPHandle->m_aType );
        ret = dsERR_INVALID_PARAM;
    }

    audio_hw_unload_interface(device);

    return ret;
}

/**
 * @brief To Get LE configuration
 *
 * This function is used to Get LE features
 *
 * @param [in] handle   Handle for the Output Audio port
 * @param [in] *enable  true if LE is enabled else False
 * @return dsError_t Error code.
 */
dsError_t dsGetLEConfig(int handle, bool *enable)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if ( ! isValidAopHandle(handle) || enable == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *enable = LEConfigEnabled;
    } else if (aPHandle->m_aType != dsAUDIOPORT_TYPE_SPEAKER){
        printf("%s: [Port: %d] Dolby MS12 parameter set not supported \n", __FUNCTION__,aPHandle->m_aType );
        ret = dsERR_INVALID_PARAM;
    }

    return ret;
}

/**
 * @brief To set encoding type for audio port
 *
 * This function will set the encoding type to specified audio port
 *
 * @param [in] handle     Handle for the Output Audio port
 * @param [in] encoding   Type of Encoding to be used
 * @return dsError_t Error code.
 */
static int8_t digital_raw_set (const uint8_t type, const uint8_t port, const uint8_t value ) {
    char sysfs[FILE_NAME_SIZE] = {0};

    get_sysfs_filename(type, port, DIGITAL_RAW_SYS_FILE, sysfs);
    return (int8_t)amsysfs_set_sysfs_int ( sysfs, value );
}

static int8_t digital_codec_set ( const uint8_t type, const uint8_t port, const uint8_t value ) {
    char sysfs[FILE_NAME_SIZE] = {0};

    get_sysfs_filename(type, port, DIGITAL_CODEC_SYS_FILE, sysfs);
    return (int8_t)amsysfs_set_sysfs_int ( sysfs, value );
}


dsError_t dsSetAudioEncoding (int handle, dsAudioEncoding_t encoding)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;
	if ( !isValidAopHandle (handle) || dsAUDIO_ENC_NONE == encoding )
		ret = dsERR_INVALID_PARAM;

	if ( dsERR_NONE == ret ) {
		APortHandle_t *aPHandle = (APortHandle_t*)handle;
		uint8_t rvalue, cvalue, rcvalue;
		VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );

		switch(aPHandle->m_aType){
			case dsAUDIOPORT_TYPE_HDMI:
				if ( encoding == dsAUDIO_ENC_PCM ){
					rcvalue = HDMI_SPDIF_RAW_PCM;
					if( 0 != digital_raw_set ( aPHandle->m_aType, aPHandle->m_index, rcvalue) ){
						ret = dsERR_GENERAL;}
					if( 0 != digital_codec_set ( aPHandle->m_aType, aPHandle->m_index, rcvalue) ){
						ret = dsERR_GENERAL;}
				}
				else if ( encoding == dsAUDIO_ENC_AC3 ){
					rvalue = HDMI_RAW_AC3_EAC3;
					cvalue = CODEC_AC3;
					if( 0 != digital_raw_set ( aPHandle->m_aType, aPHandle->m_index, rvalue) ){
						ret = dsERR_GENERAL;}
					if( 0 != digital_codec_set ( aPHandle->m_aType, aPHandle->m_index, cvalue) ){
						ret = dsERR_GENERAL;}
				}
				else if ( encoding == dsAUDIO_ENC_EAC3 ){
					rvalue = HDMI_RAW_AC3_EAC3;
					cvalue = CODEC_EAC3;
					if( 0 != digital_raw_set ( aPHandle->m_aType, aPHandle->m_index, rvalue) ){
						ret = dsERR_GENERAL;}
					if( 0 != digital_codec_set ( aPHandle->m_aType, aPHandle->m_index, cvalue) ){
						ret = dsERR_GENERAL;}

				}
				else{
					ret = dsERR_NONE;
				}
				break;

			case dsAUDIOPORT_TYPE_SPDIF:
				if ( encoding == dsAUDIO_ENC_PCM ){
					rcvalue = HDMI_SPDIF_RAW_PCM;
					if( 0 != digital_raw_set ( aPHandle->m_aType, aPHandle->m_index, rcvalue) ){
						ret = dsERR_GENERAL;}
					if( 0 != digital_codec_set ( aPHandle->m_aType, aPHandle->m_index, rcvalue) ){
						ret = dsERR_GENERAL;}
				}
				else if ( encoding == dsAUDIO_ENC_AC3 ){
					rvalue = SPDIF_RAW_AC3_EAC3;
					cvalue = CODEC_AC3;
					if( 0 != digital_raw_set ( aPHandle->m_aType, aPHandle->m_index, rvalue) ){
						ret = dsERR_GENERAL;}
					if( 0 != digital_codec_set ( aPHandle->m_aType, aPHandle->m_index, cvalue) ){
						ret = dsERR_GENERAL;}
				}
				else if ( encoding == dsAUDIO_ENC_EAC3 ){
					rvalue = SPDIF_RAW_AC3_EAC3;
					cvalue = CODEC_EAC3;
					if( 0 != digital_raw_set ( aPHandle->m_aType,aPHandle->m_index, rvalue) ){
						ret = dsERR_GENERAL;}
					if( 0 != digital_codec_set ( aPHandle->m_aType, aPHandle->m_index, cvalue) ){
						ret = dsERR_GENERAL;}

				}
				else{
					ret = dsERR_NONE;
				}
				break;

			default:
				break;

		}
	}

		return ret;
}
/**
 * @brief To set the graphic equalizer mode
 *
 * This function will set the graphic EQ mode used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] mode  Graphic EQ mode
 * @return dsError_t Error code.
 */

//TODO: To be defined properly
dsError_t  dsSetGraphicEqualizerMode(int handle, int mode)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || (mode < 0) || (mode > 3)) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {

        if(mode == 0) {
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_GRAPHIC_EQ_OFF);
        }
        if(mode == 1){
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_GRAPHIC_EQ_OPEN);
        }
        if(mode == 2){
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_GRAPHIC_EQ_RICH);
        }
        if(mode == 3){
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_GRAPHIC_EQ_FOCUSED);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
           graphicEQMode = mode;
           printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, graphicEQMode);
        }
    }

    audio_hw_unload_interface(device);

    return ret;

}

/**
 * @brief To set the MS12 audio profile
 *
 * This function will set the MS12 audio profile
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] profile   profile to be used
 * @return dsError_t Error code.
 */

dsError_t  dsSetMS12Profile(int handle, int profile)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        ms12Profile = profile;
    }

    return ret;
}

/**
 * @brief To set the Dolby volume leveller
 *
 * This function will set the Volume leveller value used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] level  Volume Leveller value
 * @return dsError_t Error code.
 */

dsError_t  dsSetVolumeLeveller(int handle, int level)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || (level < 0) || (level > 10)) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {

        char volumeLeveller_param[50];
        memset( volumeLeveller_param, 0, sizeof(volumeLeveller_param) );
        strcat(volumeLeveller_param, MS12_RUNTIME_CMD);
        strcat(volumeLeveller_param, DAP_VOLUME_LEVELLER);
        char value[10];
        sprintf(value, "%d", level);
        strcat(volumeLeveller_param, value);

        if(level) {
            err = device->set_parameters(device,volumeLeveller_param);
        }
        else {
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_VOLUME_LEVELLER_OFF);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
           volumeLeveller = level;
           LEConfigEnabled = false;
           printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, volumeLeveller);
        }
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
}

/**
 * @brief To set the audio Bass
 *
 * This function will set the Bass used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] boost   Bass Enhancer boost value
 * @return dsError_t Error code.
 */

dsError_t  dsSetBassEnhancer(int handle, int boost)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || (boost < 0) || (boost > 100) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    int aml_boost = (int)((float)boost * 3.84f); //TODO:appox conversion
    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        char bassBoost_param[100];
        memset( bassBoost_param, 0, sizeof(bassBoost_param) );
        strcat(bassBoost_param, MS12_RUNTIME_CMD);
        strcat(bassBoost_param, DAP_BASS_ENHANCER);
        char value[20];
//TODO: Cut off frequency = 200hz, width = 16;
//To be finalised after Dolby Tuning
        sprintf(value, "%d,200,16", aml_boost);
        strcat(bassBoost_param, value);

        if(boost) {
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_BASS_ENHANCER);
        }
        else{
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_BASS_ENHANCER_OFF);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
            bassBoost = boost;
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, bassBoost);
        }
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
}

/**
 * @brief To set the audio Surround Decoder
 *
 * This function will enable/disable surround decoder
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] enabled  Surround Decoder enable/disable
 * @return dsError_t Error code.
 */

dsError_t  dsEnableSurroundDecoder(int handle, bool enabled)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        if(enabled) {
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_ENABLE_SURROUND_DECODER_CMD);
        }
        else{
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_DISABLE_SURROUND_DECODER_CMD);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
            surroundDecoderEnabled = enabled;
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, surroundDecoderEnabled);
        }
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
}

/**
 * @brief To set the DRC Mode
 *
 * This function will set the Dynamic Range Control used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] mode     line/RF mode
 * @return dsError_t Error code.
 */

dsError_t  dsSetDRCMode(int handle, int mode)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || (mode < 0) || (mode > 1)) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;
    audio_hw_device_t *device;

    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) /*&& (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)*/) {
        if(mode) {
            err = device->set_parameters(device, MS12_RUNTIME_CMD DAP_DRC_RF SPACE DRC_RF);
        }
        else{
            err = device->set_parameters(device, MS12_RUNTIME_CMD DAP_DRC_LINE SPACE DRC_LINE);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
            drcMode = mode;
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, drcMode);
        }
    } else {
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
}

/**
 * @brief To set the audio Surround Virtualizer level
 *
 * This function will set the Surround Virtualizer level used in a given audio port
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] boost    Surround virtualizer level
 * @return dsError_t Error code.
 */

dsError_t  dsSetSurroundVirtualizer(int handle, int boost)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || (boost < 0) || (boost > 96)) {
        ret = dsERR_INVALID_PARAM;
    }
    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        char surround_param[50];
        memset( surround_param, 0, sizeof(surround_param) );
        strcat(surround_param, MS12_RUNTIME_CMD);
        strcat(surround_param, DAP_SURROUND_VIRTUALIZER);
        char value[10];
        sprintf(value, "%d", boost);
        strcat(surround_param, value);

        if(boost) {
            err = device->set_parameters(device,surround_param);
        }
        else{
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_SURROUND_VIRTUALIZER_OFF);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
            surroundVirtualizerLevel = boost;
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, surroundVirtualizerLevel);
        }
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
}

/**
 * @brief To set the audio Media intelligent Steering
 *
 * This function will set the Media Intelligent Steerinf
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] enabled      enable/disable MI Steering
 * @return dsError_t Error code.
 */

dsError_t  dsSetMISteering(int handle, bool enabled)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        if(enabled) {
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_MI_STEERING_ON);
        }
        else{
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_MI_STEERING_OFF);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
            miSteeringEnabled = enabled;
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, miSteeringEnabled);
	    }
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
}

#define DIGITAL_MODE_PCM    0
#define DIGITAL_MODE_DD     4
#define DIGITAL_MODE_AUTO   5
#define DIGITAL_MODE_BYPASS 6

#define DIGITAL_MODE_CMD "hdmi_format="

/**
 * @brief Set the stereo mode to be used
 *
 * This function will set the stereo mode to be used in a given audio port.
 *
 * @param [in] handle      Handle for the Output Audio port
 * @param [in] stereoMode  Type of stereo mode to be used
 * @return dsError_t Error code.
 */

dsError_t dsSetStereoMode (int handle, dsAudioStereoMode_t mode)
{
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;

    char cmd[50];
    int aml_mode;
    if( ! isValidAopHandle(handle) ){
            ret = dsERR_INVALID_PARAM;
    }

    switch(mode) {
        case dsAUDIO_STEREO_STEREO:
	    aml_mode = DIGITAL_MODE_PCM;
            break;

        case dsAUDIO_STEREO_SURROUND:
	    aml_mode = DIGITAL_MODE_AUTO;
            break;

        case dsAUDIO_STEREO_PASSTHRU:
	    aml_mode = DIGITAL_MODE_BYPASS;
            break;

        case dsAUDIO_STEREO_DD:
            aml_mode = DIGITAL_MODE_AUTO;
            break;

        case dsAUDIO_STEREO_DDPLUS:
            aml_mode = DIGITAL_MODE_AUTO;
            break;

        default:
            aml_mode = DIGITAL_MODE_AUTO;
            ret = dsERR_INVALID_PARAM;
            break;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;
    memset( cmd, 0, sizeof(cmd) );
    snprintf(cmd, sizeof(cmd), "%s%d", DIGITAL_MODE_CMD, aml_mode);

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }
    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if(dsERR_NONE == ret) {
        if (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPDIF) {
            audioModeSPDIF = mode;
        } else {
            err = device->set_parameters(device,cmd);
            if (err) {
                printf("%s: failed: %d\n", __FUNCTION__, ret);
                ret = dsERR_GENERAL;
            } else {
                audioModeNonSPDIF = mode;
                printf("%s: successful, mode : %d\n", __FUNCTION__, mode);
            }
        }
    }
    audio_hw_unload_interface(device);
    return ret;
}

dsError_t dsSetStereoAuto (int handle, int autoMode)
{
        return dsERR_NONE;
}

/**
 * @brief Set the audio gain
 *
 * This function will set the Gain in a given audio output port
 *
 * @param [in] handle  Handle for the output audio port
 * @param [in] gain    Gain value for a given audio port
 * @return dsError_t Error code.
 */

dsError_t dsSetAudioGain (int handle, float gain)
{
    #ifndef AML_STB
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || (gain < -2080) || (gain > 480)) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {

        char postGain_param[50];
        memset( postGain_param, 0, sizeof(postGain_param) );
        strcat(postGain_param, MS12_RUNTIME_CMD);
        strcat(postGain_param, DAP_POST_GAIN);
        char value[10];
        snprintf(value, sizeof(value), "%f", gain);
        strcat(postGain_param, value);

        err = device->set_parameters(device,postGain_param);

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
           speakerGain = gain;
           printf("%s: Dolby MS12 parameter set : %f\n", __FUNCTION__, speakerGain);
        }
    }

    audio_hw_unload_interface(device);

    return ret;
    #else
    return dsERR_OPERATION_NOT_SUPPORTED;
    #endif
}

/**
 * @brief To set the audio DB value
 *
 * This function will set the Db value to be used in a given audio port
 *
 * @param [in] handle  Handle for the Output Audio port
 * @param [in] db      The DB value to be used
 * @return dsError_t Error code.
 */

dsError_t dsSetAudioDB (int handle, float db)
{
        dsError_t ret = dsERR_NONE;

        if( ! isValidAopHandle(handle) ) {
                ret = dsERR_INVALID_PARAM;
        }

        return ret;
}

static int8_t set_level ( const uint8_t index, float Audiolevel ) {
        char sysfs[FILE_NAME_SIZE] = {0};

        get_sysfs_filename ( dsAUDIOPORT_TYPE_HDMI, index, VOLUME_SYS_FILE, sysfs );
        return (int8_t)amsysfs_set_sysfs_int ( sysfs, (int) Audiolevel );
}

static int set_master_volume(float level){
    int ret = 0;
    audio_hw_device_t *device;
    float vol = level/100.0;

    ret = audio_hw_load_interface(&device);
    if (ret) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return ret;
    }

    ret = device->init_check(device);
    if (ret) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return -1;
    }

    ret = device->set_master_volume(device, vol);
    if (ret){
        fprintf(stderr, "set_master_volume failed\n");
    }

    audio_hw_unload_interface(device);

	volume_ctl_set_vol((int)level);
    return ret;
}

static int set_master_mute(bool mute){
    int ret = 0;
    audio_hw_device_t *device;

    ret = audio_hw_load_interface(&device);
    if (ret) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return ret;
    }

    ret = device->init_check(device);
    if (ret) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return -1;
    }

    ret = device->set_master_mute(device, mute);
    if (ret){
        fprintf(stderr, "set_master_volume failed\n");
    }

    audio_hw_unload_interface(device);

    if (mute) {
	    volume_ctl_mute();
    }
    else {
        float cvol;
        get_master_volume(&cvol);
        volume_ctl_set_vol((int)cvol);
    }
    return ret;
}


static struct audio_port_config source_;
static struct audio_port_config sink_;
static audio_patch_handle_t patch_h_;


/**
 * @brief Set the audio level for a port
 *
 * This function will set the audio level to be used in a given audio port
 *
 * @param [in] handle   Handle for the output audio port
 * @param [in] level    The audio level for a given port
 * @return dsError_t Error code.
 */

dsError_t dsSetAudioLevel (int handle, float level)
{
	printf("Inside %s :%d, level : %f\n",__FUNCTION__,__LINE__, level);
	dsError_t ret = dsERR_NONE;

	if( ! isValidAopHandle(handle)) {
		ret = dsERR_INVALID_PARAM;
	}

	if(level < 0 || level > 100) {
		ret = dsERR_INVALID_PARAM;
	}

//        float post_gain = 0;
        audio_hw_device_t *device;
        int err = 0;

	if ( dsERR_NONE == ret ) {
                APortHandle_t *aPHandle = (APortHandle_t*)handle;

                VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );
                switch(aPHandle->m_aType){
                        case dsAUDIOPORT_TYPE_HDMI:
                        case dsAUDIOPORT_TYPE_SPDIF:
                            err = set_master_volume(level);
                            if (err){
                                fprintf(stderr, "set volume failed\n");
                                ret = dsERR_GENERAL;
                            }
                        break;

                        case dsAUDIOPORT_TYPE_SPEAKER:
                           //AMP 1
                            err = tinymix_set_volume(int(level));
                            if(err != 0) {
                                printf("%s: Volume Set failed\n",__FUNCTION__);
                                return dsERR_GENERAL;
                            }

                           break;

                        default:
                                break;
                }
        }
        return ret;
}

/**
 * @brief set the audio port to loop thro'
 *
 * This function will set the audio port to do loop thro'
 *
 * @param [in] handle    Handle for the Output Audio port
 * @param [in] loopThru  True to set output to loop thro'
 * @return dsError_t Error code.
 */

dsError_t dsEnableLoopThru(int handle, bool loopThru)
{
        return dsERR_OPERATION_NOT_SUPPORTED;
}

static int8_t hdmi_set_mute ( const uint8_t index, const bool mute ) {
	char sysfs[FILE_NAME_SIZE] = {0};

	get_sysfs_filename ( dsAUDIOPORT_TYPE_HDMI, index, HDMI_MUTE_SYS_FILE, sysfs );
	return (int8_t)amsysfs_set_sysfs_str ( sysfs, mute ? "audio_off" : "audio_on" );
}


/**
 * @brief Set the audio to mute
 *
 * This function will set the audio to mute
 *
 * @param [in] handle  Handle for the Output Audio port
 * @param [in] mute    True to mute the audio
 * @return dsError_t Error code.
 */

dsError_t dsSetAudioMute(int handle, bool mute)
{
    printf("Inside %s :%d,mute %d\n",__FUNCTION__,__LINE__, mute);
    dsError_t ret = dsERR_NONE;
    if (!isValidAopHandle(handle)) {
        ret = dsERR_INVALID_PARAM;
    }

//        float post_gain = 0;

    if (dsERR_NONE == ret) {
        APortHandle_t *aPHandle = (APortHandle_t*)handle;

        VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );

        switch (aPHandle->m_aType) {
            case dsAUDIOPORT_TYPE_HDMI:
                if (tinymix_set_mute(mute,AUDIO_HDMI_OUT_MUTE)) {
                    printf("%s: Speaker Mute failed\ for HDMIn", __FUNCTION__);
                }
                break;

            case dsAUDIOPORT_TYPE_SPDIF:
                if (tinymix_set_mute(mute,AUDIO_SPDIF_MUTE) )
                {
                    printf("%s: Speaker Mute failed for SPDIF\n", __FUNCTION__);
                }
                break;

            case dsAUDIOPORT_TYPE_SPEAKER:
                if (tinymix_set_mute(mute,MASTER_MUTE) )
                {
                    printf("%s: Speaker Mute failed for Speaker\n", __FUNCTION__);
                }
                break;

            default:
                break;
        }
    }
    return ret;
}

static bool is_dolby_ms_enabled ( const uint8_t type, const uint8_t port ) {
    char sysfs[FILE_NAME_SIZE] = {0};

    get_sysfs_filename ( type, port, MULTISTREAM_DECODE_FILE_INDEX, sysfs );
    return amsysfs_get_sysfs_int ( sysfs );
}

dsError_t dsIsAudioMSDecode (int handle, bool *HasMS11Decode)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;

	if( ! isValidAopHandle(handle) || NULL == HasMS11Decode ) {
                ret = dsERR_INVALID_PARAM;
        }

	if ( dsERR_NONE == ret ) {
		*HasMS11Decode = false;
		APortHandle_t *aPHandle = (APortHandle_t*)handle;
		VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );

		switch(aPHandle->m_aType) {
			case dsAUDIOPORT_TYPE_HDMI:
			case dsAUDIOPORT_TYPE_SPDIF:
            case dsAUDIOPORT_TYPE_SPEAKER:
				*HasMS11Decode = is_dolby_ms_enabled ( aPHandle->m_aType, aPHandle->m_index);
				break;
			default:
				break;
		}
	}
	return ret;
}

dsError_t dsIsAudioMS12Decode (int handle, bool *HasMS12Decode)
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	dsError_t ret = dsERR_NONE;

	if ( ! isValidAopHandle(handle) || NULL == HasMS12Decode ) {
				ret = dsERR_INVALID_PARAM;
		}

	if ( dsERR_NONE == ret ) {
		*HasMS12Decode = false;
		APortHandle_t *aPHandle = (APortHandle_t*)handle;
		VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );

		switch (aPHandle->m_aType) {
			case dsAUDIOPORT_TYPE_HDMI:
			case dsAUDIOPORT_TYPE_SPDIF:
			case dsAUDIOPORT_TYPE_SPEAKER:
				*HasMS12Decode = is_dolby_ms_enabled ( aPHandle->m_aType, aPHandle->m_index);
				break;
			default:
				break;
		}
	}
	return ret;
}

static dsError_t deInitHDMI(APortHandle_t* hports) {
    uint8_t index;
    APortHandle_t*  hport   = NULL;

    // close all sysfs and dev files
    for(index = 0, hport = hports; HDMI_MAX > index; index ++, hport ++) {
        hport->m_index               = RESET;
        hport->m_aType               = dsAUDIOPORT_TYPE_MAX;
    }

	return dsERR_NONE;
}

static dsError_t deInitSPDIF(APortHandle_t* sports) {
    uint8_t index;
    APortHandle_t*  sport   = NULL;

    // close all sysfs and dev files
    for(index = 0, sport = sports; SPDIF_MAX > index; index ++, sport ++) {
        sport->m_index               = RESET;
        sport->m_aType               = dsAUDIOPORT_TYPE_MAX;
    }

	return dsERR_NONE;
}

static dsError_t deInitSPEAKER(APortHandle_t* sports) {
    uint8_t index;
    APortHandle_t*  sport   = NULL;

    // close all sysfs and dev files
    for(index = 0, sport = sports; SPEAKER_PORT_MAX > index; index ++, sport ++) {
        sport->m_index               = RESET;
        sport->m_aType               = dsAUDIOPORT_TYPE_MAX;
    }

        return dsERR_NONE;
}


/**
 * @brief Terminate the usage of audio output ports.
 *
 * This function will reset the data structs used within this module and release the audio port specific handles
 *
 * @param None
 * @return dsError_t Error code.
 */

dsError_t dsAudioPortTerm()
{
	printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
	uint8_t type;
	dsError_t ret = dsERR_NONE;

	for(type = dsAUDIOPORT_TYPE_ID_LR; dsAUDIOPORT_TYPE_MAX > type; type++) {
		switch ( type ) {
			case dsAUDIOPORT_TYPE_HDMI:
				deInitHDMI( _handles[type] );
				break;

			case dsAUDIOPORT_TYPE_SPDIF:
				deInitSPDIF( _handles[type] );
				break;

            case dsAUDIOPORT_TYPE_SPEAKER:
                deInitSPEAKER( _handles[type] );
                break;

			default:
				break;
		}
	}
	if ( dsERR_NONE == ret ) {
		isAudioPortInitialized = false;
		printf("DSHAL: AudioPort: deinitialized successfully\n");
	}
	else {
		printf("DSHAL: AudioPort: Failed to deinitialized!\r\n");
	}

	return ret;
}

#ifdef AML_STB
dsError_t dsGetSinkDeviceAtmosCapability(int handle, dsATMOSCapability_t *capability){
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;
    int err = 0;
    char buffer[256];
    int cnt = 0;
    if((!isValidAopHandle(handle)) || (capability == NULL)){
        ret = dsERR_INVALID_PARAM;
    }

    if ( dsERR_NONE == ret ) {
        APortHandle_t *aPHandle = (APortHandle_t*)handle;
        VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );

        FILE * fp = NULL;

        fp = fopen(SYSFS_HDMITX_SINK_AUDIO_CAPABILITY, "r");
        if (fp){
            char * str = NULL;
            cnt = 0;
            while(str = fgets(buffer, sizeof(buffer), fp)){
                int i = 0;
                //in the string read from file, contain '\0', should remove it
                while(str[i] != '\n'){
                    if(str[i] == '\0'){
                        str[i] = ' ';
                    }
                    i ++;
                    if (i > 256){
                        fprintf(stderr, "error!!! too much characters in %s\n", str);
                        break;
                    }
                }
                //fprintf(stderr, "str %s\n", str);
                if (strcasestr(str, "Dobly_Digital+")){
                    if (*capability != dsAUDIO_ATMOS_ATMOSMETADATA){
                        *capability = dsAUDIO_ATMOS_DDPLUSSTREAM;
                    }
                }
                if (strcasestr(str, "ATMOS")){
                    *capability = dsAUDIO_ATMOS_ATMOSMETADATA;
                }
                cnt ++;
                if (cnt > 64){
                    fprintf(stderr, "error!!! read 64 strings, perhaps too much strings in %s\n", SYSFS_HDMITX_SINK_AUDIO_CAPABILITY);
                    break;
                }
            }
            fclose(fp);
        }else{
            fprintf(stderr, "open file %s error %s\n", SYSFS_HDMITX_SINK_AUDIO_CAPABILITY, strerror(errno));
            ret = dsERR_GENERAL;
        }
    }

    return ret;
}

dsError_t  dsGetSurroundMode(int handle, int *surround){
    (void) handle;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);
    dsError_t ret = dsERR_NONE;
    int err = 0;
    char buffer[256];
    int cnt = 0;
    if (surround == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    *surround = dsSURROUNDMODE_NONE;

    if ( dsERR_NONE == ret ) {
        FILE * fp = NULL;

        fp = fopen(SYSFS_HDMITX_SINK_AUDIO_CAPABILITY, "r");
        if (fp){
            char * str = NULL;
            cnt = 0;
            while(str = fgets(buffer, sizeof(buffer), fp)){
                int i = 0;
                //in the string read from file, contain '\0', should remove it
                while(str[i] != '\n'){
                    if(str[i] == '\0'){
                        str[i] = ' ';
                    }
                    i ++;
                    if (i > 256){
                        fprintf(stderr, "error!!! too much characters in %s\n", str);
                        break;
                    }
                }
                //fprintf(stderr, "str %s\n", str);
                if (strcasestr(str, "Dobly_Digital+")) {
                    *surround = dsSURROUNDMODE_DDPLUS;
                }
                else if (strcasestr(str, "Dobly_Digital")) {
                    *surround = dsSURROUNDMODE_DD;
                }
                cnt ++;
                if (cnt > 64){
                    fprintf(stderr, "error!!! read 64 strings, perhaps too much strings in %s\n", SYSFS_HDMITX_SINK_AUDIO_CAPABILITY);
                    break;
                }
            }
            fclose(fp);
        }else{
            fprintf(stderr, "open file %s error %s\n", SYSFS_HDMITX_SINK_AUDIO_CAPABILITY, strerror(errno));
            ret = dsERR_GENERAL;
        }
    }

    return ret;
}
#endif

dsError_t  dsGetAudioCompression(int handle, int *compression){
    //*compression = 6;
    return dsERR_OPERATION_NOT_SUPPORTED;
}


dsError_t  dsSetAudioCompression(int handle, int compression){
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t  dsGetDolbyVolumeMode(int handle, bool *mode){
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t  dsSetDolbyVolumeMode(int handle, bool mode){
    return dsERR_OPERATION_NOT_SUPPORTED;
}

#define SPEAKER_DELAY_CMD "hal_param_speaker_delay_time_ms="
#define SPDIF_DELAY_CMD "hal_param_spdif_delay_time_ms="

dsError_t dsSetAudioDelay(int handle, const uint32_t audioDelayMs){
    printf("Inside %s :%d, audioDelayMs : %d\n",__FUNCTION__,__LINE__, audioDelayMs);
    dsError_t ret = dsERR_NONE;
    audio_hw_device_t *device;
    int err = 0;
    char cmd[256];
    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if(!isValidAopHandle(handle)) {
        ret = dsERR_INVALID_PARAM;
    }

    err = audio_hw_load_interface(&device);
    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return dsERR_GENERAL;
    }

    if ( dsERR_NONE == ret ) {
        VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );
        switch(aPHandle->m_aType){
            case dsAUDIOPORT_TYPE_SPDIF:
                snprintf(cmd, sizeof(cmd), "%s%d", SPDIF_DELAY_CMD, audioDelayMs);
                err = device->set_parameters(device, cmd);
                if (err){
                    fprintf(stderr, "set spdif delay failed, err %d\n", err);
                    ret = dsERR_GENERAL;
                }
                break;

            case dsAUDIOPORT_TYPE_SPEAKER:
                snprintf(cmd, sizeof(cmd), "%s%d", SPEAKER_DELAY_CMD, audioDelayMs);
                err = device->set_parameters(device, cmd);
                if (err){
                    fprintf(stderr, "set spdif delay failed, err %d\n", err);
                    ret = dsERR_GENERAL;
                }
                break;

            case dsAUDIOPORT_TYPE_HDMI:
                break;
            default:
                break;
        }
    }

    if (ret == dsERR_NONE){
        audioDelay = audioDelayMs;
        fprintf(stderr, "set audio delay %d ms on port %d successfully\n", audioDelay, aPHandle->m_aType);
    }
    audio_hw_unload_interface(device);
    return ret;
}

dsError_t dsGetAudioDelay(int handle, uint32_t *audioDelayMs){
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || audioDelayMs == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    if (dsERR_NONE == ret){
        *audioDelayMs = audioDelay;
    }

    return ret;
}

dsError_t dsGetAudioDelayOffset(int handle, uint32_t *audioDelayOffsetMs){
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if (!isValidAopHandle(handle) || audioDelayOffsetMs == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    if (dsERR_NONE == ret) {
        *audioDelayOffsetMs = audioDelay;
    }

    return ret;
}

dsError_t dsSetAudioDelayOffset(int handle, const uint32_t audioDelayOffsetMs){
    printf("Inside %s :%d, audioDelayOffsetMs : %d\n",__FUNCTION__,__LINE__, audioDelayOffsetMs);
    dsError_t ret = dsERR_NONE;
    audio_hw_device_t *device;
    int err = 0;
    char cmd[256];
    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if (!isValidAopHandle(handle)) {
        ret = dsERR_INVALID_PARAM;
    }

    err = audio_hw_load_interface(&device);
    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return dsERR_GENERAL;
    }

    if (dsERR_NONE == ret) {
        VALIDATE_TYPE_AND_INDEX ( aPHandle->m_aType, aPHandle->m_index );
        switch (aPHandle->m_aType) {
            case dsAUDIOPORT_TYPE_SPDIF:
                snprintf(cmd, sizeof(cmd), "%s%d", SPDIF_DELAY_CMD, audioDelayOffsetMs);
                err = device->set_parameters(device, cmd);
                if (err) {
                    fprintf(stderr, "set spdif delay failed, err %d\n", err);
                    ret = dsERR_GENERAL;
                }
                break;

            case dsAUDIOPORT_TYPE_SPEAKER:
                snprintf(cmd, sizeof(cmd), "%s%d", SPEAKER_DELAY_CMD, audioDelayOffsetMs);
                err = device->set_parameters(device, cmd);
                if (err) {
                    fprintf(stderr, "set spdif delay failed, err %d\n", err);
                    ret = dsERR_GENERAL;
                }
                break;

            case dsAUDIOPORT_TYPE_HDMI:
                break;
            default:
                break;
        }
    }

    if (ret == dsERR_NONE) {
        audioDelay = audioDelayOffsetMs;
        fprintf(stderr, "set audio delay %d ms on port %d successfully\n", audioDelay, aPHandle->m_aType);
    }
    audio_hw_unload_interface(device);
    return ret;
}

dsError_t  dsGetIntelligentEqualizerMode(int handle, int *mode){
    #ifndef AML_STB
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || mode == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ((dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)){
        *mode = intelligentEQMode;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
    #else
    return dsERR_OPERATION_NOT_SUPPORTED;
    #endif
}

dsError_t  dsSetIntelligentEqualizerMode(int handle, int mode){
    #ifndef AML_STB
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d, mode: %d\n",__FUNCTION__,__LINE__, mode);

    if( ! isValidAopHandle(handle) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

typedef enum {
    EQ_MODE_OFF,
    EQ_MODE_OPEN,
    EQ_MODE_RICH,
    EQ_MODE_FOCUSED
}eqMode;
    if ((dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        eqMode mode1 = (eqMode)mode;
        switch(mode1){
            case EQ_MODE_OFF:
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_INTELLIGENT_EQ_OFF);
                break;
            case EQ_MODE_OPEN:
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_INTELLIGENT_EQ_OPEN);
                break;
            case EQ_MODE_RICH:
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_INTELLIGENT_EQ_RICH);
                break;
            case EQ_MODE_FOCUSED:
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_INTELLIGENT_EQ_FOCUSED);
                break;
            default:
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_INTELLIGENT_EQ_OFF);
                break;
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
            intelligentEQMode = mode;
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, intelligentEQMode);
        }
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
    #else
    return dsERR_OPERATION_NOT_SUPPORTED;
    #endif
}

dsError_t  dsGetDialogEnhancement(int handle, int *level){
    #ifndef AML_STB
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) || level == NULL) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    if ( (dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        *level = dialogEnhancerLevel;
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
    #else
    return dsERR_OPERATION_NOT_SUPPORTED;
    #endif
}

dsError_t  dsSetDialogEnhancement(int handle, int level){
    #ifndef AML_STB
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if( ! isValidAopHandle(handle) ) {
        ret = dsERR_INVALID_PARAM;
    }

    APortHandle_t *aPHandle = (APortHandle_t*)handle;

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);

    if (err) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        ret = dsERR_GENERAL;
    }

    err = device->init_check(device);
    if (err) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        ret = dsERR_GENERAL;
    }

    if ((dsERR_NONE == ret) && (aPHandle->m_aType == dsAUDIOPORT_TYPE_SPEAKER)) {
        if(level > 0) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "%s%s%d", MS12_RUNTIME_CMD, DAP_DIALOGUE_ENHANCER, level);
            fprintf(stderr, "dialog enhancer command %s\n", cmd);
            err = device->set_parameters(device, cmd);
        }else{
            err = device->set_parameters(device,
                MS12_RUNTIME_CMD
                DAP_DIALOGUE_ENHANCER_OFF);
        }

        if (err) {
            printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
            ret = dsERR_GENERAL;
        }
        else {
            dialogEnhancerLevel = level;
            printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, dialogEnhancerLevel);
        }
    }else{
        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    audio_hw_unload_interface(device);

    return ret;
    #else
    return dsERR_OPERATION_NOT_SUPPORTED;
    #endif
}

const char* MS12_LIB_PATH = "/tmp/ds/0x4d_0x5331_0x32.so";

static inline bool exists (const char *filename) {
  struct stat buffer;
  return (stat (filename, &buffer) == 0);
}

dsError_t dsGetMS12Capabilities(int handle, int *capabilities)
{
    dsError_t ret = dsERR_NONE;

    //if MS12 lib is present, set all capabilities
    if (exists(MS12_LIB_PATH)) {
        *capabilities = dsMS12SUPPORT_DolbyVolume |
                        dsMS12SUPPORT_InteligentEqualizer |
                        dsMS12SUPPORT_DialogueEnhancer;

        ret = dsERR_NONE;
    }
    else {
        *capabilities = dsMS12SUPPORT_NONE;

        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}

dsError_t dsGetAudioCapabilities(int handle, int *capabilities)
{
    dsError_t ret = dsERR_NONE;

    //if MS12 lib is present, set all capabilities
    if (exists(MS12_LIB_PATH)) {
        *capabilities = dsAUDIOSUPPORT_ATMOS |
                        dsAUDIOSUPPORT_DD |
                        dsAUDIOSUPPORT_DDPLUS |
                        dsAUDIOSUPPORT_MS12;

        ret = dsERR_NONE;
    }
    else {
        *capabilities = dsAUDIOSUPPORT_NONE;

        ret = dsERR_OPERATION_NOT_SUPPORTED;
    }

    return ret;
}


/**
 * @brief Set the audio ATMOS outout mode
 *
 * This function will set the Audio Atmos output mode
 *
 * @param [in] handle        Handle for the Output Audio port
 * @param [in] enable set audio ATMOS output mode
 * @return dsError_t Error code.
 */
dsError_t dsSetAudioAtmosOutputMode(int handle, bool enable)
{
    dsError_t ret = dsERR_NONE;
    printf("Inside %s :%d\n",__FUNCTION__,__LINE__);

    if ( ! isValidAopHandle(handle)) {
        return dsERR_INVALID_PARAM;
    }

    audio_hw_device_t *device;
    int err = audio_hw_load_interface(&device);
    if (err) {
        printf("DS_HAL : %s: audio_hw_load_interface failed: %d\n", __func__, err);
        device = NULL;
        return dsERR_GENERAL;
    }

    err = device->init_check(device);

    if (err) {
        printf("DS_HAL: %s :device not inited, quit\n", __func__);
        audio_hw_unload_interface(device);
        device = NULL;
        return dsERR_GENERAL;
    }

    if (enable) {
        err = device->set_parameters(device,
            MS12_RUNTIME_CMD
            MS12_ATMOS_LOCK_ON);
    }
    else {
        err = device->set_parameters(device,
            MS12_RUNTIME_CMD
            MS12_ATMOS_LOCK_OFF);
    }

    if (err) {
        printf("%s: Dolby MS12 parameter set failed: %d\n", __FUNCTION__, ret);
        ret = dsERR_GENERAL;
    }
    else {
        printf("%s: Dolby MS12 parameter set : %d\n", __FUNCTION__, enable);
    }

    return ret;
}