/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "engines/advancedDetector.h"

#include "engines/reevengi/reevengi.h"
#include "engines/reevengi/detection.h"
#include "engines/reevengi/re1/re1.h"
#include "engines/reevengi/re2/re2.h"
#include "engines/reevengi/re3/re3.h"

namespace Reevengi {

static const PlainGameDescriptor reevengiGames[] = {
	{ "re1", "Resident Evil" },
	{ "re2", "Resident Evil 2" },
	{ "re3", "Resident Evil 3" },
	{ nullptr, nullptr }
};

static const char *directoryGlobs[] = {
	/* RE1 */
	"horr",
	"usa", "ger", "jpn", "fra",
	"data",
	/* RE2 */
	"pl0", "pl1", "regist",
	"zmovie",
	0
};

#define REEVENGI_ENTRY(gameid, flags, lang, platform, filename, extra, gametype)  \
	{                                                                      \
		{                                                                  \
			gameid,                                                        \
			extra,                                                         \
			{                                                              \
				{ filename, 0, nullptr, AD_NO_SIZE },                              \
				{ nullptr, 0, nullptr, AD_NO_SIZE },                               \
			},                                                             \
			lang,                                                          \
			platform,                                                      \
			ADGF_UNSTABLE | flags,                                        \
			GUIO_NONE                                                      \
		},                                                                 \
		gametype                                                           \
	},

static const ReevengiGameDescription gameDescriptions[] = {
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformWindows, "horr/usa/data/capcom.ptc", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::DE_DEU, Common::kPlatformWindows, "horr/ger/data/capcom.ptc", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformWindows, "horr/jpn/data/capcom.ptc", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::FR_FRA, Common::kPlatformWindows, "horr/fra/data/capcom.ptc", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformWindows, "usa/data/capcom.ptc", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::DE_DEU, Common::kPlatformWindows, "ger/data/capcom.ptc", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformWindows, "jpn/data/capcom.ptc", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::FR_FRA, Common::kPlatformWindows, "fra/data/capcom.ptc", "", RType_RE1)

	REEVENGI_ENTRY("re1", ADGF_DEMO, Common::JA_JPN, Common::kPlatformPSX, "slpm_800.27", "Trial", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_DEMO, Common::JA_JPN, Common::kPlatformPSX, "ntsc.exe", "1.0", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_002.00", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::FR_FRA, Common::kPlatformPSX, "sles_002.27", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::DE_DEU, Common::kPlatformPSX, "sles_002.28", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_009.69", "Director's Cut", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::FR_FRA, Common::kPlatformPSX, "sles_009.70", "Director's Cut", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::DE_DEU, Common::kPlatformPSX, "sles_009.71", "Director's Cut", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slpm_867.70", "5th Anniversary LE", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_002.22", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_009.98", "Director's Cut", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_015.12", "Director's Cut Dual Shock", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_001.70", "", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_005.51", "Director's Cut", RType_RE1)
	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_007.47", "Director's Cut Dual Shock", RType_RE1)

	REEVENGI_ENTRY("re1", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformSaturn, "bu_load.prg", "", RType_RE1)

	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_ANY, Common::kPlatformWindows, "pl0/zmovie/r108l.bin", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_ANY, Common::kPlatformWindows, "pl1/zmovie/r108l.bin", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_ANY, Common::kPlatformWindows, "regist/leonp.exe", "Preview", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_ANY, Common::kPlatformWindows, "regist/leonu.exe", "Preview", RType_RE2_LEON)

	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_GRB, Common::kPlatformPSX, "sced_003.60", "Preview", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_GRB, Common::kPlatformPSX, "sced_008.27", "Preview", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_GRB, Common::kPlatformPSX, "sled_009.77", "Preview", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::JA_JPN, Common::kPlatformPSX, "slps_009.99", "Trial Edition", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_USA, Common::kPlatformPSX, "slus_900.09", "Preview", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_GRB, Common::kPlatformPSX, "sced_011.14", "Preview", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_DEMO, Common::EN_GRB, Common::kPlatformPSX, "psx.exe", "Beta v2", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_009.72", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::FR_FRA, Common::kPlatformPSX, "sles_009.73", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::DE_DEU, Common::kPlatformPSX, "sles_009.74", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::IT_ITA, Common::kPlatformPSX, "sles_009.75", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::ES_ESP, Common::kPlatformPSX, "sles_009.76", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_009.72", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_012.22", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_015.10", "Leon Dual Shock", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_004.21", "Leon", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_007.48", "Leon Dual Shock", RType_RE2_LEON)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_109.72", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::FR_FRA, Common::kPlatformPSX, "sles_109.73", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::DE_DEU, Common::kPlatformPSX, "sles_109.74", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::IT_ITA, Common::kPlatformPSX, "sles_109.75", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::ES_ESP, Common::kPlatformPSX, "sles_109.76", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_109.72", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_012.23", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_015.11", "Claire Dual Shock", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_005.92", "Claire", RType_RE2_CLAIRE)
	REEVENGI_ENTRY("re2", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_007.56", "Claire Dual Shock", RType_RE2_CLAIRE)

	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::EN_ANY, Common::kPlatformWindows, "rofs2.dat", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_DEMO, Common::EN_ANY, Common::kPlatformWindows, "rofs1.dat", "Preview", RType_RE3)

	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_025.28", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_025.29", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::FR_FRA, Common::kPlatformPSX, "sles_025.30", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::DE_DEU, Common::kPlatformPSX, "sles_025.31", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::ES_ESP, Common::kPlatformPSX, "sles_025.32", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::IT_ITA, Common::kPlatformPSX, "sles_025.33", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::EN_GRB, Common::kPlatformPSX, "sles_026.98", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::JA_JPN, Common::kPlatformPSX, "slps_023.00", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_NO_FLAGS, Common::EN_USA, Common::kPlatformPSX, "slus_009.23", "", RType_RE3)
	REEVENGI_ENTRY("re3", ADGF_DEMO, Common::EN_USA, Common::kPlatformPSX, "slus_900.64", "Trial", RType_RE3)

	{AD_TABLE_END_MARKER, RType_None}
};

class ReevengiMetaEngineDetection : public AdvancedMetaEngineDetection<ReevengiGameDescription> {
public:
	ReevengiMetaEngineDetection() : AdvancedMetaEngineDetection(gameDescriptions, reevengiGames) {
		_maxScanDepth = 4;
		_directoryGlobs = directoryGlobs;
		_flags = kADFlagMatchFullPaths;
		_guiOptions = GUIO_NOMIDI;
	}

	const char *getName() const override {
		return "reevengi";
	}

	const char *getEngineName() const override {
		return "Reevengi";
	}

	const char *getOriginalCopyright() const override {
		return "(C) Capcom";
	}
};

} // End of namespace Reevengi

REGISTER_PLUGIN_STATIC(REEVENGI_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, Reevengi::ReevengiMetaEngineDetection);
