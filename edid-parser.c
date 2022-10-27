/*if not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
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

#define MODULE_PREFIX "edid"
//#include "dslogger.h"

#include "edid-parser.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define UNUSED(_x_) _x_ = _x_

//#define printf(...)

//namespace edid_parser {

enum EDID_EXTENSION_TAG_BLOCK_MAP
{
    LCD_TIMING = 0x01,
    ADDITIONAL_TIMING = 0x02,
    EDID20_EXTENSION = 0x20,
    COLOR_INFORMATION_TYPE = 0x30,
    DVI_FEATURE_DATA = 0x40,
    TOUCH_SCREEN_DATA = 0x50,
    EXTENSION_TAG_BLOCK_MAP = 0xF0,
    MANUFACTURER_EXTENSION = 0xFF
};

static void parse_std_timing(unsigned char* bytes, edid_data_t* data_ptr) {
    int idx = 0;
    // two 1 means empty block
    if (bytes[idx] == 1 && bytes[idx + 1] == 1) return;
    int h = ((bytes[idx]) + 31) * 8;
    int v = 0;
    switch ((bytes[idx + 1] & 0xC0) >> 6) {
        case 0: v = (h * 10) / 16; break;
        case 1: v = (h * 3) / 4; break;
        case 2: v = (h * 4) / 5; break;
        case 3: v = (h * 9) / 16; break;
        default: return;
    }
    int r = (bytes[idx + 1] & 0x3F) + 60;
    printf("STD %dx%d@%d\n", h, v, r);

    UNUSED(h);
    UNUSED(v);
    UNUSED(r);
}

void parse_monitor_descriptor(unsigned char* bytes, edid_data_t* data_ptr) {
  // Monitor Descriptor Block
  if ((bytes[0] == 0) && (bytes[1] == 0) && (bytes[2] == 0)) {
      //  First Monitor Descriptor Block
      if (bytes[3] == 0xFC) {
          memset(data_ptr->monitor_name, '\0', sizeof(data_ptr->monitor_name));
          for (int i = 5; (i < (5 + 14)) && (bytes[i] != '\n') && isprint(bytes[i]); i++) {
              data_ptr->monitor_name[i - 5] = bytes[i];
          }
          printf("Monitor name:'%s'\n", data_ptr->monitor_name);
      }
  }
}
static void parse_dtd(unsigned char* bytes, edid_data_t* data_ptr, char native) {
    int idx = 0;
    // Monitor descriptor block, not detailed timing descriptor.
    if (bytes[idx] == 0 && bytes[idx + 1] == 0) {
      parse_monitor_descriptor(bytes, data_ptr);
    }
    else {
       // pixel clock, if any of bytes is 0 means std is invalid
        if (bytes[idx] == 0 || bytes[idx + 1] == 0) return;
        idx += 2;
        // horizonal active
        idx += 1;
        // horizonal blanking
        idx += 1;
        // horizonal active : horizontal blanking
        int h = ((bytes[idx] & 0xF0) << 4) | bytes[idx - 2];
        idx += 1;
        // vertical active
        idx += 1;
        // vertical blanking
        idx += 1;
        // vertical active : vertical blanking
        int v = ((bytes[idx] & 0xF0) << 4) | bytes[idx - 2];
        idx += 1;
        // horizontal sync offset
        idx += 1;
        // horizontal sync pulse width
        idx += 1;
        // vertical sync offset : vertical sunc pulse width
        idx += 1;
        // horizontal sync offset, horizontal sync pulse width, vertical sync offset, vertical sync pulse width
        idx += 1;
        // horizontal image size
        idx += 1;
        // vertical image size
        idx += 1;
        // horizontal and vertical image size
        idx += 1;
        // horizontal border
        idx += 1;
        // vertical border
        idx += 1;
        idx += 1;
        // flags
        char p = (bytes[idx] & 0x80) ? 0 : 1;
        idx += 1;

        printf("DTD, %dx%d@%c, native: %d (%s found already)\n", h, v, (p ? 'p' : 'i'), native, (data_ptr->res.native == EDID_NOT_NATIVE ? "not" : ""));

        if (native) {
            data_ptr->res.progressive = p ? EDID_PROGRESSIVE : EDID_INTERLACED;
            data_ptr->res.width = h;
            data_ptr->res.height = v * ((data_ptr->res.progressive == EDID_INTERLACED) ? 2 : 1);
            data_ptr->res.native = EDID_NATIVE;
        }
    }
}

typedef struct {
    int w;
    int h;
    int p;
    int r;
    int ar_h;
    int ar_v;
} edid_vic_t;

static edid_vic_t vic[108] = {
    {640, 480, 1, 60, 4, 3},
    {720, 480, 1, 60, 4, 3},
    {720, 480, 1, 60, 16, 9},
    {1280, 720, 1, 60, 16, 9},
    {1920, 1080, 0, 60, 16, 9},
    {1440, 480, 0, 60, 4, 3},
    {1440, 480, 0, 60, 16, 9},
    {1440, 240, 0, 60, 4, 3},
    {1440, 240, 1, 60, 16, 9},
    {2880, 480, 0, 60, 4, 3},
    {2880, 480, 0, 60, 16, 9},
    {2880, 240, 1, 60, 4, 3},
    {2880, 240, 1, 60, 16, 9},
    {1440, 480, 1, 60, 4, 3},
    {2880, 576, 0, 50, 4, 3},
    {2880, 576, 0, 50, 16, 9},
    {2880, 288, 1, 50, 4, 3},
    {2880, 288, 1, 50, 16, 9},
    {1440, 576, 1, 50, 4, 3},
    {1440, 576, 1, 50, 16, 9},
    {1920, 1080, 1, 50, 16, 9},
    {1920, 1080, 1, 24, 16, 9},
    {1920, 1080, 1, 25, 16, 9},
    {1920, 1080, 1, 30, 16, 9},
    {2880, 480, 1, 60,  4, 3},
    {2880, 480, 1, 60,  16, 9},
    {2880, 576, 1, 50, 4, 3},
    {2880, 576, 1, 50, 16, 9},
    {1920, 1080, 0, 50, 16, 9},
    {1920, 1080, 0, 100, 16, 9},
    {1280, 720, 1, 100, 16, 9},
    {720, 576, 1, 100, 4, 3},
    {720, 576, 1, 100, 16, 9},
    {1440, 576, 0, 100, 4, 3},
    {1440, 576, 0, 100, 16, 9},
    {1920, 1080, 0, 120, 16, 9},
    {1280, 720, 1, 120, 16, 9},
    {720, 480, 1,120, 4, 3},
    {720, 480, 1, 120, 16, 9},
    {1440, 480, 0, 120,4, 3},
    {1440, 480, 0, 120,16, 9},
    {720, 576, 1, 200, 4, 3},
    {720, 576, 1, 200, 16, 9},
    {1440, 576, 0, 200, 4, 3},
    {720, 576, 0, 200, 16, 9},
    {720, 480, 1, 240, 4, 3},
    {720, 480, 1, 240, 16, 9},
    {720, 480, 0, 240, 4, 3},
    {720, 480, 0, 240, 16, 9},
    {1280, 720, 1, 24, 16, 9},
    {1280, 720, 1, 25, 16, 9},
    {1280, 720, 1, 30, 16, 9},
    {1920, 1080, 1, 120, 16, 9},
    {1920, 1080, 1, 100, 16, 9},
    {1280, 720, 1, 24, 64, 27},
    {1280, 720, 1, 25, 64, 27},
    {1280, 720, 1, 30, 64, 27},
    {1280, 720, 1, 30, 64, 27},
    {1280, 720, 1, 50, 64, 27},
    {1280, 720, 1, 60, 64, 27},
    {1280, 720, 1, 100, 64, 27},
    {1280, 720, 1, 120, 64, 27},
    {1920, 1080, 1, 24, 64, 27},
    {1920, 1080, 1, 25, 64, 27},
    {1920, 1080, 1, 30, 64, 27},
    {1920, 1080, 1, 50, 64, 27},
    {1920, 1080, 1, 60, 64, 27},
    {1920, 1080, 1, 100, 64, 27},
    {1920, 1080, 1, 120, 64, 27},
    {1680, 720, 1, 24, 64, 27},
    {1680, 720, 1, 25, 64, 27},
    {1680, 720, 1, 30, 64, 27},
    {1680, 720, 1, 50, 64, 27},
    {1680, 720, 1, 60, 64, 27},
    {1680, 720, 1, 100,64, 27},
    {1680, 720, 1, 120, 64, 27},
    {2560, 1080, 1, 24, 64, 27},
    {2560, 1080, 1, 25, 64, 27},
    {2560, 1080, 1, 30, 64, 27},
    {2560, 1080, 1, 50, 64, 27},
    {2560, 1080, 1, 60, 64, 27},
    {2560, 1080, 1, 100, 64, 27},
    {2560, 1080, 1, 120, 64, 27},
    {3840, 2160, 1, 24, 16, 9},
    {3840, 2160, 1, 25, 16, 9},
    {3840, 2160, 1, 30, 16, 9},
    {3840, 2160, 1, 50, 16, 9},
    {3840, 2160, 1, 60, 16, 9},
    {4096, 2160, 1, 24, 256, 135},
    {4096, 2160, 1, 25, 256, 135},
    {4096, 2160, 1, 30, 256, 135},
    {4096, 2160, 1, 50,256, 135},
    {4096, 2160, 1, 60, 256, 135},
    {3840, 2160, 1, 24, 64, 27},
    {3840, 2160, 1, 25, 64, 27},
    {3840, 2160, 1, 30, 64, 27},
    {3840, 2160, 1, 50, 64, 27},
    {3840, 2160, 1, 60, 64, 27},
};

static void parse_ext_video(const unsigned char* bytes, edid_res_t* edid_res, const unsigned int len, const unsigned int native_cnt) {
    /* only if native resolution was not detected yet */
    if (edid_res->native == EDID_NOT_NATIVE) {
        int found_native = 0;
        unsigned int idx;
        unsigned int res;

        for (idx = 0; idx < len; idx++) {
            res = bytes[idx];
            printf("EXT RES byte 0x%x (num: %d) %s\n", res, (((res >= 129) && (res <= 192)) ? (res & 0x7F) : res), (((res >= 129) && (res <= 192)) ? "native" : ""));
            /* consider only native codes (CTA-861.F.pdf page 81) */
            if ((res >= 129) && (res <= 192)) {
                /* it is number of resolution in table, not index, so -1 */
                res = (res & 0x7F) - 1;
                found_native = 1;
            }
            /* When the first DTD and SVD do not match and the total number of
             * DTDs defining Native Video Formats in the whole EDID is zero
             * (see Table 41, byte 3, lower 4 bits), the first SVD shall take
             * precedence. (CTA-861.F.pdf page 75)*/
            else if ((native_cnt == 0) && (idx == 0)) {
                res = res - 1;
                found_native = 1;
            }
            if (found_native) {
                if (res < sizeof(vic) / sizeof(edid_vic_t)) {
                    edid_res->width = vic[res].w;
                    edid_res->height = vic[res].h;
                    edid_res->refresh = vic[res].r;
                    edid_res->progressive = vic[res].p ? EDID_PROGRESSIVE : EDID_INTERLACED;
                    edid_res->native = EDID_NATIVE;
                    printf("EXT RES native found: %dx%d%c@%d\n", vic[res].w, vic[res].h, vic[res].p ? 'p' : 'i', vic[res].r);
                    return;
                }
            }
        }
    }
}

static void parse_vendor_specific_video_data_block(unsigned char* bytes, edid_data_t* data_ptr)
{
	unsigned char *dat = bytes;
	unsigned char pos = 0;
	unsigned int ieeeoui = 0;
	unsigned char length = 0;
	unsigned char dv_ver = 0;
	unsigned char dv_length = 0;

	length = bytes[pos] & 0x1f;
	pos++;

	if (dat[pos] != 1) {
		printf("VSVDB: parsing fail [%02x]\n", dat[pos]);
		return;
	}

	pos++;
	ieeeoui = dat[pos++];
	ieeeoui += dat[pos++] << 8;
	ieeeoui += dat[pos++] << 16;
	printf("VSVDB: ieeeoui=0x%x, len=%u\n", ieeeoui, length);

	/* HDR10+ */
	if (ieeeoui == HDR10_PLUS_IEEE_OUI) {
		data_ptr->hdr_capabilities |= HDR_standard_HDR10_PLUS;
		printf("HDR10+ VSVDB: application ver %d, \n", dat[pos] & 0x3);
		return;
	} else if (ieeeoui != DV_IEEE_OUI)
		return;

	/* Dovi */
	dv_ver = (dat[pos] >> 5) & 0x7;
	dv_length = length;
	if (dv_ver > 2) {
		printf("DV VSVDB: wrong dv ver %d\n", dv_ver);
		return;
	}

	data_ptr->dv_capabilities = 0;
	if (dv_ver == 0) {
		if (dv_length == 0x19) {
			data_ptr->dv_capabilities |= DolbyVision_Sink_led;
			if (dat[pos] & 0x1)
				data_ptr->dv_capabilities |= DolbyVision_Native;
			if ((dat[pos] >> 1) & 0x1)
				data_ptr->dv_capabilities |= DolbyVision_4K60;
		} else {
			printf("DV VSVDB: wrong v0 len %d\n", dv_length);
			return;
		}
	}

	if (dv_ver == 1) {
		if (dv_length == 0x0B) {
			data_ptr->dv_capabilities |= DolbyVision_Sink_led;
			if (dat[pos] & 0x1)
				data_ptr->dv_capabilities |= DolbyVision_Native;
			if ((dat[pos] >> 1) & 0x1)
				data_ptr->dv_capabilities |= DolbyVision_4K60;
			pos++;
			/* sup_global_dimming and tmaxLUM */
			pos++;
			/* dv->colorimetry and tminLUM */
			pos++;
			if ((dat[pos] & 0x3) == 1)
				data_ptr->dv_capabilities |= DolbyVision_Source_led;
		} else if (dv_length == 0x0E) {
			data_ptr->dv_capabilities |= DolbyVision_Sink_led;
			if (dat[pos] & 0x1)
				data_ptr->dv_capabilities |= DolbyVision_Native;
			if ((dat[pos] >> 1) & 0x1)
				data_ptr->dv_capabilities |= DolbyVision_4K60;
		} else {
			printf("DV VSVDB: wrong v1 len %d\n", dv_length);
			return;
		}
	}
	if (dv_ver == 2) {
		if (dv_length >= 0x0B) {
			data_ptr->dv_capabilities |= DolbyVision_4K60;
			if (dat[pos] & 0x1)
				data_ptr->dv_capabilities |= DolbyVision_Native;
			pos++;
			/* sup_global_dimming, backlt_min_luma, tminPQ */
			pos++;
			/* Interface */
			if ((dat[pos] & 0x3) <= 1) {
				data_ptr->dv_capabilities |= DolbyVision_Source_led;
			} else {
				data_ptr->dv_capabilities |= DolbyVision_Sink_led;
				data_ptr->dv_capabilities |= DolbyVision_Source_led;
			}
			pos++;
			if (((dat[pos] & 0x1) << 1) | (dat[pos + 1] & 0x1))
				data_ptr->dv_capabilities |= DolbyVision_444_DeepColor;

		} else {
			printf("DV VSVDB: wrong v2 len %d\n", dv_length);
			return;
		}
	}
	data_ptr->hdr_capabilities |= HDR_standard_DolbyVision;
	printf("DV VSVDB: ver=%d, len=%d, sup_sinkled=%d, low_latency=%d, sup_4k60=%d, sup_native=%d, sup_444_deepcolor=%d\n",
		dv_ver, dv_length,
		data_ptr->dv_capabilities & DolbyVision_Sink_led,
		data_ptr->dv_capabilities & DolbyVision_Source_led,
		data_ptr->dv_capabilities & DolbyVision_4K60,
		data_ptr->dv_capabilities & DolbyVision_Native,
		data_ptr->dv_capabilities & DolbyVision_444_DeepColor);
}

static void parse_colorimetry_block(unsigned char* bytes, edid_data_t* data_ptr) {
    data_ptr->colorimetry_info = ((uint32_t)bytes[2] << 8) | ((uint32_t)bytes[3]);
    printf("colorimetry info:0x%x\n", data_ptr->colorimetry_info);
}

static void parse_extended_db(uint8_t* bytes, edid_data_t* data_ptr) {
    const uint8_t extended_code = bytes[1];
    printf("parse_extended_db extended_code=%d\n", extended_code);

    // 0x1 - "Vendor Specific Video Data block"
    if (extended_code == 1) {
      parse_vendor_specific_video_data_block(bytes, data_ptr);
    }
    // 0x5 - "Colorimetry Data block"
    else if (extended_code == 5) {
      parse_colorimetry_block(bytes, data_ptr);
    }
    // 0x6 - "HDR Static Metadata Data Block" (from CTA-861-G_FINAL_revised_2017.pdf)
    else if (extended_code == 6) {

        uint8_t eotfByte = bytes[2];
        data_ptr->hdr_capabilities = 0;

        if (eotfByte & 0x01) {
            data_ptr->hdr_capabilities |= HDR_standard_SDR;
        }

        if (eotfByte & 0x02) {
            data_ptr->hdr_capabilities |= HDR_standard_Traditional_HDR;
        }

        // BHDM_EDID_HdrDbEotfSupport_eSMPTESt2084 == NEXUS_VideoEotf_eHdr10 / NEXUS_VideoEotf_eSmpteSt2084 == dsHDRSTANDARD_HDR10
        if (eotfByte & 0x04) {
            data_ptr->hdr_capabilities |= HDR_standard_HDR10;
        }
        // BHDM_EDID_HdrDbEotfSupport_eFuture == BAVC_HDMI_DRM_EOTF_eHLG/BAVC_HDMI_DRM_EOTF_eFuture == NEXUS_VideoEotf_eHlg/NEXUS_VideoEotf_eAribStdB67 == dsHDRSTANDARD_HLG
        if (eotfByte & 0x08) {
            data_ptr->hdr_capabilities |= HDR_standard_HLG;
        }
    } else {
        printf("Extended DB - extended code %d not supported\n", extended_code);
    }
}

static void parse_vendodr_specific_block(unsigned char* bytes, edid_data_t* data_ptr) {
    data_ptr->physical_address_a = bytes[4]  >> 4;
    data_ptr->physical_address_b = bytes[4] & 0x0F;
    data_ptr->physical_address_c = bytes[3]  >> 4;
    data_ptr->physical_address_d = bytes[5] & 0x0F;
    printf("Vendor specific bolck, physical adress a:0x%x b:0x%x c:0x%x d:0x%x\n",
          data_ptr->physical_address_a,
          data_ptr->physical_address_b,
          data_ptr->physical_address_c,
          data_ptr->physical_address_d);
}

static void parse_ext_timing(unsigned char* bytes, edid_data_t* data_ptr) {
    int idx = 0;
    // dtd start
    printf("TimingExtension version: %d\n", bytes[-1]);
    int dtd_start = bytes[idx] - 2;
    idx += 1;
    // extension dtds number (0x0F), underscan (0x80), basic audio (0x40), ycbcr444 (0x20), ycbcr422 (0x10), native formats (0x07)
    int native_cnt = bytes[idx] & 0x07;
    printf("Native cnt: %d\n", native_cnt);
    idx += 1;
    if (dtd_start != 2) {
        int end = dtd_start;
        edid_res_t ext_video_res;

        ext_video_res.native = EDID_NOT_NATIVE;
        while (idx < end) {
            int tag = (bytes[idx] & 0xE0) >> 5;
            int len = bytes[idx] & 0x1F;
            printf("parse_ext_timing: extension tag=%d len=%d\n", tag, len);

            switch (tag) {
                // reserved
                case 0: break;
                // audio
                case 1: break;
                // video
                case 2: parse_ext_video(&bytes[idx + 1], &ext_video_res, len, native_cnt); break;
                // vendor specific
                case 3: parse_vendodr_specific_block(&bytes[idx], data_ptr); break;
                // speaker
                case 4: break;
                // vesa dtc data block
                case 5: break;
                // reserved
                case 6: break;
                // 'Use Extended Tag'
                case 7: parse_extended_db(&bytes[idx], data_ptr); break;
                // default - unsupported
                default: printf("Unsupported extension tag: 0x%X\n", tag);
            }
            idx += len + 1;
        }
        /* if there is native resoluton defined in Video Data Block we will
         * prioritize that over DTD native - important for example for monitors
         * where real native resolution could be quite exotic */
        if (ext_video_res.native != EDID_NOT_NATIVE) {
            data_ptr->res.width = ext_video_res.width;
            data_ptr->res.height = ext_video_res.height;
            data_ptr->res.refresh = ext_video_res.refresh;
            data_ptr->res.progressive = ext_video_res.progressive;
            data_ptr->res.native = ext_video_res.native;
        }
    }
    /* At the moment this implementation parses only 1 (first) native resolution,
     * as DTDs are as well in frist EDID block it could be that native resolution
     * is already parsed, and we should not overwrite it */
    if (data_ptr->res.native == EDID_NOT_NATIVE) {
        idx = dtd_start;
        while (idx < dtd_start + (4 * 18)) {
            parse_dtd(&bytes[idx], data_ptr, (data_ptr->res.native == EDID_NOT_NATIVE));
            idx += 18;
        }
    }
}

static void parse_est_timing(unsigned char b1, unsigned char b2, unsigned char b3, edid_data_t* data_ptr) {
    int width = 0;
    int height = 0;
    int refresh = 0;
    char progressive = 1;
    if ((b1 & 0x01) == 0x01) {
        width = 800;
        height = 600;
        refresh = 60;
    }
    if ((b1 & 0x02) == 0x02) {
        width = 800;
        height = 600;
        refresh = 56;
    }
    if ((b1 & 0x04) == 0x04) {
        width = 640;
        height = 480;
        refresh = 75;
    }
    if ((b1 & 0x08) == 0x08) {
        width = 640;
        height = 480;
        refresh = 72;
    }
    if ((b1 & 0x10) == 0x10) {
        width = 640;
        height = 480;
        refresh = 67;
    }
    if ((b1 & 0x20) == 0x20) {
        width = 640;
        height = 480;
        refresh = 60;
    }
    if ((b1 & 0x40) == 0x40) {
        width = 720;
        height = 400;
        refresh = 88;
    }
    if ((b1 & 0x80) == 0x80) {
        width = 720;
        height = 400;
        refresh = 70;
    }
    if ((b2 & 0x01) == 0x01) {
        width = 1280;
        height = 1024;
        refresh = 75;
    }
    if ((b2 & 0x02) == 0x02) {
        width = 1024;
        height = 768;
        refresh = 75;
    }
    if ((b2 & 0x04) == 0x04) {
        width = 1024;
        height = 768;
        refresh = 70;
    }
    if ((b2 & 0x08) == 0x08) {
        width = 1024;
        height = 768;
        refresh = 60;
    }
    if ((b2 & 0x10) == 0x10) {
        width = 1024;
        height = 768;
        refresh = 87;
        progressive = 0;
    }
    if ((b2 & 0x20) == 0x20) {
        width = 832;
        height = 624;
        refresh = 75;
    }
    if ((b2 & 0x40) == 0x40) {
        width = 800;
        height = 600;
        refresh = 75;
    }
    if ((b2 & 0x80) == 0x80) {
        width = 800;
        height = 600;
        refresh = 72;
    }
    if ((b3 & 0x80) == 0x80) {
        width = 1152;
        height = 870;
        refresh = 75;
    }

    printf("EST %dx%d%c@%d\n", width, height, progressive ? 'p' : 'i', refresh);

    UNUSED(width);
    UNUSED(height);
    UNUSED(progressive);
    UNUSED(refresh);
}

// bytes length needs to be (at least) 128
int block_checksum_ok(unsigned char* bytes) {
    // CTA-861-G_FINAL_revised_2017.pdf:
    //  Checksum byte = (256-(S%256)) %256
    //  Where:
    //      S is the sum of the first 127 bytes
    //      % is modulus operator
    uint32_t sum = 0;
    for (size_t idx=0; idx < 127; ++idx) {
        sum += bytes[idx];
    }
    return bytes[127] == (256 - (sum % 256)) % 256;
}

edid_status_e parse_extension_block(uint32_t base_idx, unsigned char* bytes, edid_data_t* data_ptr)
{
    int ext_tag = bytes[base_idx];
    // skiping revision number

    switch (ext_tag) {
        // LCD timing
        case LCD_TIMING: break;
        // Additional timing
        case ADDITIONAL_TIMING: parse_ext_timing(&bytes[base_idx + 2], data_ptr);break;
        // EDID 2.0 extension
        case EDID20_EXTENSION: break;
        // Color information type
        case COLOR_INFORMATION_TYPE: break;
        // DVI feature data
        case DVI_FEATURE_DATA: break;
        // Touch screen data
        case TOUCH_SCREEN_DATA: break;
        // Block Map
        case EXTENSION_TAG_BLOCK_MAP: break;
        // Manufacturer extension
        case MANUFACTURER_EXTENSION: break;
        default: printf("Unsupported tag: 0x%X\n", ext_tag);
    }

    return EDID_STATUS_OK;
}

edid_status_e parse_extension_blocks(uint32_t extensions, unsigned char* bytes, size_t count, edid_data_t* data_ptr)
{
    if (count < 128 * (1 + extensions)) {
        printf("parse_extension_blocks: too short for extension count - count:%zu extensions:%u\n",count,extensions);
        return EDID_STATUS_INVALID_HEADER;
    }

    /*  CTA-861-G_FINAL_revised_2017.pdf: "Some devices have been incorrectly designed so that the block map is not counted in the
        extension count. Design of compliant devices should take compatibility with those non-compliant
        devices into consideration. For example, when a Source finds an extension count of 2, it may
        attempt to read 3 extensions on the chance that the Sink has incorrectly set its count"
    */
    if (count == 128 * (2 + extensions)) {
        // there is 1 more block (128 bytes) than would seem from extensions count
        // assume we are handling 'incorrectly designed' device & attempt to read 1 more extension
        printf("extensions:%d, but count: %zu - increase extension cnt\n", extensions, count);
        ++extensions;
    }

    // extension tag
    const int first_ext_tag = bytes[128];
    uint32_t extension_block_idx = 0;

    if (extensions > 1 ) {
        if (first_ext_tag != EXTENSION_TAG_BLOCK_MAP) {
            printf("Incorrect input, more than one extension: %d - but block 1 is not extension block map\n", extensions);
            return EDID_STATUS_NOT_SUPPORTED;
        } else {
            // skip block map; the extensions are tagged anyway
            extension_block_idx = 1;
        }
    }

    edid_status_e ret = EDID_STATUS_OK;

    for ( ; extension_block_idx < extensions; ++extension_block_idx) {
        const uint32_t base_idx = (1 + extension_block_idx) * 128;
        ret = parse_extension_block(base_idx, bytes, data_ptr);
        if (ret != EDID_STATUS_OK) break;
    }

    return ret;
}

#define SET_LETTER(x) ( isprint((x) + '@')  ? (x) + '@' : '\0')

static void parse_manufacturer_name(uint8_t b1, uint8_t b2, edid_data_t* data_ptr)
{
    data_ptr->manufacturer_name[0] = SET_LETTER((b1 & 0x7C) >> 2);
    data_ptr->manufacturer_name[1] = SET_LETTER(((b1 & 0x03) << 3) | ((b2 & 0xE0) >> 5));
    data_ptr->manufacturer_name[2] = SET_LETTER(b2 & 0x1F);
    data_ptr->manufacturer_name[3] = '\0';
    printf("Manufacturer name:'%s'\n", data_ptr->manufacturer_name);
}

static void parse_product_code(uint8_t b1, uint8_t b2, edid_data_t* data_ptr)
{
    data_ptr->product_code =  (((int32_t)b1 << 8) | (int32_t)b2);
    printf("Product code:%d\n", data_ptr->product_code);
}

static void parse_serial_number(uint8_t* serial_ptr, edid_data_t* data_ptr)
{
    data_ptr->serial_number = ((int32_t)serial_ptr[0] << 24) | ((int32_t)serial_ptr[1] << 16) | ((int32_t)serial_ptr[2] << 8) | ((int32_t)serial_ptr[3]);
    printf("Serial number:%d\n", data_ptr->serial_number);
}

static void parse_manufacture_week(uint8_t b1, edid_data_t* data_ptr)
{
    data_ptr->manufacture_week = b1;
    printf("Manufacture week:%d\n", data_ptr->manufacture_week);
}

static void parse_manufacture_year(uint8_t b1, edid_data_t* data_ptr)
{
    data_ptr->manufacture_year = b1;
    printf("Manufacture year:%d\n", data_ptr->manufacture_year);
}

static void parse_edid_version(uint8_t b1, uint8_t b2, edid_data_t* data_ptr)
{
    data_ptr->edid_version[0] = b1;
    data_ptr->edid_version[1] = b2;
    printf("EDID version:%d.%d\n", data_ptr->edid_version[0], data_ptr->edid_version[1]);
}

edid_status_e EDID_Verify(unsigned char* bytes, size_t count) {
    if (!bytes || count < 128) {
        return EDID_STATUS_INVALID_PARAMETER;
    }
    static const unsigned char header[8] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    if (memcmp(bytes, header, sizeof(header)) != 0) {
        printf("Incorrect input, header does not match: %02x %02x %02x %02x %02x %02x %02x %02x\n", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
        return EDID_STATUS_INVALID_HEADER;
    }
    return EDID_STATUS_OK;
}

edid_status_e EDID_Parse(unsigned char* bytes, size_t count, edid_data_t* data_ptr) {
    if (!data_ptr) {
        printf("Incorrect input, data_ptr null\n");
        return EDID_STATUS_INVALID_PARAMETER;
    }
    edid_status_e verify_status = EDID_Verify(bytes, count);
    if (verify_status != EDID_STATUS_OK) {
        return verify_status;
    }

    memset(data_ptr, 0, sizeof(edid_data_t));
    data_ptr->res.native = EDID_NOT_NATIVE;

    int idx = 0;

    idx += 8;
    // vendor / product ID
    parse_manufacturer_name(bytes[idx], bytes[idx + 1], data_ptr);
    parse_product_code(bytes[idx + 2], bytes[idx + 3], data_ptr);
    parse_serial_number(&bytes[idx + 4], data_ptr);
    parse_manufacture_week(bytes[idx + 8], data_ptr);
    parse_manufacture_year(bytes[idx + 9], data_ptr);
    idx += 10;
    // EDID version (version, revision)
    parse_edid_version(bytes[idx], bytes[idx + 1], data_ptr);
    idx += 2;
    // basics
    idx += 5;
    // colors
    idx += 10;
    // established timings
    parse_est_timing(bytes[idx], bytes[idx + 1], bytes[idx + 2], data_ptr);
    idx += 3;
    // standard timing ID
    for (int i = idx; i < idx + 16; i += 2) {
        parse_std_timing(&bytes[i], data_ptr);
    }
    idx += 16;
    // detailed timing descriptions
    for (int i = idx; i < idx + 72; i += 18) {
        parse_dtd(&bytes[i], data_ptr, i == idx);
    }
    idx += 72;
    // extension flag
    int extension = bytes[idx];
    idx += 2;

    edid_status_e ret = EDID_STATUS_OK;

    // parse extension blocks, if any
    if (extension > 0)
    {
        ret = parse_extension_blocks(extension,bytes,count,data_ptr);
    }
    return ret;
}


