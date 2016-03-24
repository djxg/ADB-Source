/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "utils.h"
#include "usb_vendors.h"

#include <stdio.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include "windows.h"
#  include "shlobj.h"
#else
#  include <unistd.h>
#  include <sys/stat.h>
#endif

#include "sysdeps.h"
#include "adb.h"

#define ANDROID_PATH            ".android"
#define ANDROID_ADB_INI         "adb_usb.ini"

#define TRACE_TAG               TRACE_USB

// Google's USB Vendor ID
#define VENDOR_ID_GOOGLE        0x18d1
// HTC's USB Vendor ID
#define VENDOR_ID_HTC           0x0bb4
// Samsung's USB Vendor ID
#define VENDOR_ID_SAMSUNG       0x04e8
// Motorola's USB Vendor ID
#define VENDOR_ID_MOTOROLA      0x22b8
// LG's USB Vendor ID
#define VENDOR_ID_LGE           0x1004
// Huawei's USB Vendor ID
#define VENDOR_ID_HUAWEI        0x12D1
// Acer's USB Vendor ID
#define VENDOR_ID_ACER          0x0502
// Sony Ericsson's USB Vendor ID
#define VENDOR_ID_SONY_ERICSSON 0x0FCE
// Foxconn's USB Vendor ID
#define VENDOR_ID_FOXCONN       0x0489
// Dell's USB Vendor ID
#define VENDOR_ID_DELL          0x413c
// Nvidia's USB Vendor ID
#define VENDOR_ID_NVIDIA        0x0955
// Garmin-Asus's USB Vendor ID
#define VENDOR_ID_GARMIN_ASUS   0x091E
// Sharp's USB Vendor ID
#define VENDOR_ID_SHARP         0x04dd
// ZTE's USB Vendor ID
#define VENDOR_ID_ZTE           0x19D2
// Kyocera's USB Vendor ID
#define VENDOR_ID_KYOCERA       0x0482
// Pantech's USB Vendor ID
#define VENDOR_ID_PANTECH       0x10A9
// Qualcomm's USB Vendor ID
#define VENDOR_ID_QUALCOMM      0x05c6
// On-The-Go-Video's USB Vendor ID
#define VENDOR_ID_OTGV          0x2257
// NEC's USB Vendor ID
#define VENDOR_ID_NEC           0x0409
// Panasonic Mobile Communication's USB Vendor ID
#define VENDOR_ID_PMC           0x04DA
// Toshiba's USB Vendor ID
#define VENDOR_ID_TOSHIBA       0x0930
// SK Telesys's USB Vendor ID
#define VENDOR_ID_SK_TELESYS    0x1F53
// KT Tech's USB Vendor ID
#define VENDOR_ID_KT_TECH       0x2116
// Asus's USB Vendor ID
#define VENDOR_ID_ASUS          0x0b05
// Philips's USB Vendor ID
#define VENDOR_ID_PHILIPS       0x0471
// Texas Instruments's USB Vendor ID
#define VENDOR_ID_TI            0x0451


/** built-in vendor list */
int builtInVendorIds[] = {
   VENDOR_ID_GOOGLE,
   VENDOR_ID_HTC,
    VENDOR_ID_SAMSUNG,
    VENDOR_ID_MOTOROLA,
    VENDOR_ID_LGE,
    VENDOR_ID_HUAWEI,
    VENDOR_ID_ACER,
    VENDOR_ID_SONY_ERICSSON,
    VENDOR_ID_FOXCONN,
    VENDOR_ID_DELL,
    VENDOR_ID_NVIDIA,
    VENDOR_ID_GARMIN_ASUS,
    VENDOR_ID_SHARP,
    VENDOR_ID_ZTE,
    VENDOR_ID_KYOCERA,
    VENDOR_ID_PANTECH,
    VENDOR_ID_QUALCOMM,
    VENDOR_ID_OTGV,
    VENDOR_ID_NEC,
    VENDOR_ID_PMC,
    VENDOR_ID_TOSHIBA,
    VENDOR_ID_SK_TELESYS,
    VENDOR_ID_KT_TECH,
    VENDOR_ID_ASUS,
    VENDOR_ID_PHILIPS,
    VENDOR_ID_TI,
};

#define BUILT_IN_VENDOR_COUNT    (sizeof(builtInVendorIds)/sizeof(builtInVendorIds[0]))

/* max number of supported vendor ids (built-in + 3rd party). increase as needed */
#define VENDOR_COUNT_MAX         128

int vendorIds[VENDOR_COUNT_MAX];
unsigned vendorIdCount = 0;

int get_adb_usb_ini(char* buff, size_t len);

void usb_vendors_init(void)
{
	int len;
	int dbgOut = 0;
	char buf[MAX_PATH];
	char folder[MAX_PATH];
	char iniPath[MAX_PATH];

    if (VENDOR_COUNT_MAX < BUILT_IN_VENDOR_COUNT) {
        fprintf(stderr, "VENDOR_COUNT_MAX not big enough for built-in vendor list.\n");
        exit(2);
    }

    /* add the built-in vendors at the beginning of the array */
    memcpy(vendorIds, builtInVendorIds, sizeof(builtInVendorIds));

    /* default array size is the number of built-in vendors */
    vendorIdCount = BUILT_IN_VENDOR_COUNT;

    if (VENDOR_COUNT_MAX == BUILT_IN_VENDOR_COUNT)
        return;

	//__asm int 3
   
    if (get_adb_usb_ini(folder, sizeof(folder)) == 0) {
		sprintf(iniPath, "%sadbUsb.ini", folder);
        FILE * f = fopen(iniPath, "rt");

        if (f != NULL) {
			// 是否为调试模式
			while (fgets(buf, sizeof(buf), f) != NULL) {
				if (buf[0] != '#')
					continue;

				if (strncmp(buf, "#dbgOut", 7) == 0) {
					dbgOut = 1;
					break;
				}
			}

            /* The vendor id file is pretty basic. 1 vendor id per line.
               Lines starting with # are comments */
			fseek(f, 0L, SEEK_SET);
            while (fgets(buf, sizeof(buf), f) != NULL) {
                if (buf[0] == '#')
                    continue;

                long value = strtol(buf, NULL, 0);
                if (errno == EINVAL || errno == ERANGE || value > INT_MAX || value < 0) {
                    fprintf(stderr, "Invalid content in %s. Quitting.\n", ANDROID_ADB_INI);
                    exit(2);
                }

                vendorIds[vendorIdCount++] = (int)value;

                /* make sure we don't go beyond the array */
                if (vendorIdCount == VENDOR_COUNT_MAX) {
                    break;
                }
            }
        }
    }

	// 查看输出
	if (dbgOut) {
		FILE *dbgFile = NULL;
		sprintf(iniPath, "%sUser\\vids.ini", folder);
		dbgFile = fopen(iniPath, "wt");

		for (int i = 0; i < vendorIdCount; i++) {
			len = sprintf(buf, "0x%04x\r\n", vendorIds[i]);
			fwrite(buf, 1, len, dbgFile);
		}
		fclose(dbgFile);
	}
}

/* Utils methods */

/* builds the path to the adb vendor id file. returns 0 if success */
int build_path(char* buff, size_t len, const char* format, const char* home)
{
    if (sprintf(buff, format, home, ANDROID_PATH, ANDROID_ADB_INI) >= (signed)len) {
        return 1;
    }

    return 0;
}

int get_current_directory(char*buff, size_t len)
{
	int i;
	char *last = NULL;
	char path[MAX_PATH];

	memset(path, 0x00, sizeof(path));

	if (!GetModuleFileNameA(NULL, path, sizeof(path) / sizeof(path[0])))
		return -1;

	// 去除程序名称
	// c:\a\b\c.exe
	if ((last = strrchr(path, '/')) || (last = strrchr(path, '\\'))) {
		*(last + 1) = 0;
		if (strlen(path) + 10 + 1> len)
			return -1;

		strcpy(buff, path);
		return 0;
	}
	return -1;
}

/* fills buff with the path to the adb vendor id file. returns 0 if success */
int get_adb_usb_ini(char* buff, size_t len)
{
#ifdef _WIN32
	return get_current_directory(buff, len);
	/*int ret;
    const char* home = getenv("ANDROID_SDK_HOME");
    if (home != NULL) {
        return build_path(buff, len, "%s\\%s\\%s", home);
    } else {
        char path[MAX_PATH];
        SHGetFolderPath( NULL, CSIDL_PROFILE, NULL, 0, path);
        ret = build_path(buff, len, "%s\\%s\\%s", path);
		dbg_out1("ini path : %s", buff);
		return ret;
    }*/
#else
    const char* home = getenv("HOME");
    if (home == NULL)
        home = "/tmp";

    return build_path(buff, len, "%s/%s/%s", home);
#endif
}
