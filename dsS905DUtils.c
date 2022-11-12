#include "dsS905D.h"
#include "dsVideoResolutionSettings.h"
#include <stdio.h>
#include <string.h>

static const ResolutionMap_t kResolutionMap [] = \
           {
                     {"480p",         "480p60hz"},
                     {"576p50",       "576p50hz"},
                     {"720p24",       "720p24hz"},
                     {"720p25",       "720p25hz"},
                     {"720p30",       "720p30hz"},
                     {"720p50",       "720p50hz"},
                     {"720p60",       "720p60hz"},
                     {"1080p24",      "1080p24hz"},
                     {"1080p25",      "1080p25hz"},
                     {"1080p30",      "1080p30hz"},
                     {"1080p50",      "1080p50hz"},
                     {"1080p60",      "1080p60hz"},
                     {"1080i50",      "1080i50hz"},
                     {"1080i60",      "1080i60hz"},
                     {"2160p24",      "2160p24hz"},
                     {"2160p25",      "2160p25hz"},
                     {"2160p30",      "2160p30hz"},
                     {"2160p50",      "2160p50hz"},
                     {"2160p60",      "2160p60hz"},
                     {"2160p50hz420", "2160p50hz420"},
                     {"2160p60hz420", "2160p60hz420"},
                     {"smpte24hz",    "smpte24hz"},
                     {"smpte25hz",    "smpte25hz"},
                     {"smpte30hz",    "smpte30hz"},
                     {"smpte50hz",    "smpte50hz"},
                     {"smpte60hz",    "smpte60hz"},
                     {"smpte50hz420", "smpte50hz420"},
                     {"smpte60hz420", "smpte60hz420"},
                     {"1080p",        "1080p60hz"},
                     {"2160p",        "2160p60hz"},
           };


dsVideoPortResolution_t* getResolutionRef (const char* name) {
    uint8_t i;
    for( i = 0; ARRAY_SIZE(kResolutions) > i; i++) {
        if ( 0 == strcmp ( name, kResolutions[i].name ) ) {
            return &kResolutions[i];
	}
    }
    return NULL;
}

void findRDKResolution ( const char* boardResName, char* out_rdkResName ) {
    uint8_t i;
    for ( i = 0; ARRAY_SIZE ( kResolutionMap ) > i; i ++ ) {
        if ( 0 == strcmp ( boardResName, kResolutionMap[i].m_boardResName ) )  {
            if ( out_rdkResName )
                strcpy ( out_rdkResName, kResolutionMap[i].m_rdkResName );
        }
    }
}

void findBoardResolution (const char* rdkResname, char* out_boardResName) {
    uint8_t i;
    for( i = 0; ARRAY_SIZE( kResolutionMap ) > i; i++) {
        if ( 0 == strcmp ( rdkResname, kResolutionMap[i].m_rdkResName ) ) {
            if ( out_boardResName )
                strcpy ( out_boardResName, kResolutionMap[i].m_boardResName );
        }
    }
}
