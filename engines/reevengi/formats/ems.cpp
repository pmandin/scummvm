/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/debug.h"
#include "common/md5.h"
#include "common/stream.h"
#include "common/substream.h"

#include "engines/reevengi/formats/ems.h"

namespace Reevengi {

/*--- Types ---*/

typedef struct {
	const char *md5sum;
	uint8 model, fType;	/* em0<model>.emd, fType=0 for EMD, 1 for TIM */
	int32 length;
	uint32 offset;	/* Found offset */
} ems_item_t;

ems_item_t emsItems[]={
	/* Leon */
	{"8479ebef2e5e49489ece15227620f814", 0x10, 0, 146892, 0},
	{"4385f25501af1b41eb87df27ac515e26", 0x10, 1, 66592, 0},
	{"0594f2f8e99daf0fe1d4c33ff296404e", 0x11, 0, 149468, 0},
	{"1bd30c3c9d2d34b538f71fe0daac039a", 0x11, 1, 66592, 0},
	{"3c2cbc1ddfeae4dc9809ca6ff7593a3d", 0x12, 0, 146268, 0},
	{"478d50899cab520e569f1b4790acc74d", 0x12, 1, 133152, 0},
	{"57160c3b92a876daec05b09e040ec894", 0x13, 0, 146732, 0},
	{"6b60ee48d12032a6154b5bf8b54f9b15", 0x13, 1, 66592, 0},
	{"8e7024afb96cbc7a2359dbe2b5a750aa", 0x15, 0, 146604, 0},
	{"54b016b6078faf327f7940d54f35bb53", 0x15, 1, 66592, 0},
	{"38cb59dfbb944fdaeb3f329782011f6a", 0x16, 0, 146012, 0},
	{"5cdf267b2e7aaca4150bfa890cc98848", 0x16, 1, 66592, 0},
	{"d83098df3592f75997762b1d009aeb66", 0x17, 0, 151192, 0},
	{"e87820d85d8c14526f2a7594c77d83e7", 0x17, 1, 66592, 0},
	{"7ea5c9e93671c06cde44660b188f9486", 0x18, 0, 140804, 0},
	{"eded3183637af0bdff389fd47c0871f6", 0x18, 1, 66592, 0},
	{"7ea5c9e93671c06cde44660b188f9486", 0x1e, 0, 140804, 0},
	{"81da2b41c60639ff7bca00d15385ca16", 0x1e, 1, 133152, 0},
	{"7ea5c9e93671c06cde44660b188f9486", 0x1f, 0, 140804, 0},
	{"81da2b41c60639ff7bca00d15385ca16", 0x1f, 1, 133152, 0},
	{"a9ca56311527a0f0780b8f790e521f1b", 0x20, 0, 121144, 0},
	{"e989eff61ae9451875d7bb811d8f84ab", 0x20, 1, 66592, 0},
	{"bc370bac2306e3b716db6c6c4f3077cc", 0x21, 0, 32296, 0},
	{"9dace0a69e32b1a4ef7638a5847bc235", 0x21, 1, 33312, 0},
	{"09aa186fd0d17cd256a4cce6980a021e", 0x22, 0, 174964, 0},
	{"cb66446c89e7de18fc8954d8cf46b90e", 0x22, 1, 66592, 0},
	{"9673f75fb713ef5947232b6c32b37242", 0x23, 0, 189208, 0},
	{"37dd53a15bae3a104fb35c7589276c1c", 0x23, 1, 133152, 0},
	{"7b4ff1861a79e4bc1341aee310755101", 0x24, 0, 129712, 0},
	{"672431dfacf710b6b5b1fdb5b88bcf24", 0x24, 1, 66592, 0},
	{"8b91976d410b964b87580ff384a8901a", 0x25, 0, 53448, 0},
	{"8ef6d0faef12f3f4f30e3f23b0f76457", 0x25, 1, 66592, 0},
	{"97949726ee43871544c00e3455ba1e1d", 0x26, 0, 6676, 0},
	{"6ea27dfbb8388f5fd7d1e150eeebfc30", 0x26, 1, 33312, 0},
	{"6f50669cfec2e783dddab5e2e59b8ece", 0x27, 0, 39632, 0},
	{"c6c2d650d18a58b94c2f75d544806bf0", 0x27, 1, 33312, 0},
	{"08fe2e5c3c96a9922e0ac339e119faec", 0x28, 0, 188172, 0},
	{"8fc09baefbf3cfbd3e38a5492ce45df9", 0x28, 1, 133152, 0},
	{"a3c290ffa3cfd425ee939ab0c826378f", 0x29, 0, 12560, 0},
	{"12061e2481721831d15ff784dc8eea8a", 0x29, 1, 33312, 0},
	{"0cc92755c016333f69e0069eb413ce15", 0x2a, 0, 138372, 0},
	{"b93c27aa658957ff7ff1f047453d0cb8", 0x2a, 1, 66592, 0},
	{"40b9b0cc398b6ff8573d95ac31fbe7aa", 0x2b, 0, 99872, 0},
	{"f8013ea530c5b1b54e97c60f728a2a87", 0x2b, 1, 139896, 0},
	{"3dceb40786c3b340ee1ccfbea181ee2c", 0x2c, 0, 99872, 0},
	{"af3a30ec501813db390fb2954c55c4ae", 0x2c, 1, 15964, 0},
	{"39d093569618311805c03ce9a1ef67e7", 0x2d, 0, 24608, 0},
	{"c965e1faa886bde5f873a4d92da0b35b", 0x2d, 1, 33312, 0},
	{"da1a245c7a3e11fdbe228d099ca10893", 0x2e, 0, 216780, 0},
	{"597393acf844348b6ae01232c3d0e9a6", 0x2e, 1, 66592, 0},
	{"762f6f048607c3eebf1280714f55e06f", 0x2f, 0, 5496, 0},
	{"75e52e00a5f6a8d1c00963fd8a155a35", 0x2f, 1, 33312, 0},
	{"aa67358d9f8ecc7e4588c262b8d287ec", 0x30, 0, 152188, 0},
	{"e203e27ca42c6929d6e783c53fe36b07", 0x30, 1, 99872, 0},
	{"9eaf491b7196a6f64e00dbb98fab8d1f", 0x31, 0, 193124, 0},
	{"abf89ee4772dc4b0be5f61d840f2338f", 0x31, 1, 99872, 0},
	{"043ae92f189c8270fdb59a049a0c5e43", 0x33, 0, 204968, 0},
	{"a03ef4b1a217bb620ed723cdbc2f4456", 0x33, 1, 99872, 0},
	{"034cb7b8b67df59c38fda499309fe9a5", 0x34, 0, 273812, 0},
	{"67d5f6322485677a7e43c8ac2cac1800", 0x34, 1, 99872, 0},
	{"3a49b091f8aded3c93540856349f5c5d", 0x36, 0, 112340, 0},
	{"274087f2c77f1363efd1cb2a841211e9", 0x36, 1, 132640, 0},
	{"22756bc25cfda5ce4233e2028fa5941a", 0x37, 0, 59376, 0},
	{"e3c118c118921ffb90480f493a6c3e64", 0x37, 1, 33312, 0},
	{"ec318c6e6f94dedb2187cd81a45a0637", 0x38, 0, 6268, 0},
	{"58b1e69b04cbcf9cdeecc6ef63075b13", 0x38, 1, 33312, 0},
	{"da1a245c7a3e11fdbe228d099ca10893", 0x39, 0, 216780, 0},
	{"50edab0ce1d66682b2c5600ce592f67a", 0x39, 1, 66592, 0},
	{"c084ef69626b557fcb6324f45f40f5fd", 0x3a, 0, 43612, 0},
	{"e086b31a3f3a0eaaea7daeb24952e6ec", 0x3a, 1, 66592, 0},
	{"75189ca3adf7e8b01be9f00810ffc985", 0x3b, 0, 3480, 0},
	{"d51a0fddd0d785b7b4918da7615e4619", 0x3b, 1, 33312, 0},
	{"dd592c77fb2eadd6259046930682ed4f", 0x3e, 0, 55312, 0},
	{"e0fd7b02739b8f2ba7b363cd27868ef0", 0x3e, 1, 33312, 0},
	{"bd3ca4f4c45a1b38db7c306744579ae5", 0x3f, 0, 25972, 0},
	{"34be3c0203aed4a7f9bb504b7ab95976", 0x3f, 1, 33312, 0},
	{"c68e845ffb355b5dbdb0220f1d457718", 0x40, 0, 54856, 0},
	{"e064a33abaa0ea2e037f2df3c8684586", 0x40, 1, 99872, 0},
	{"90e56a51ee2c250b5c8552efaa627a1e", 0x41, 0, 77264, 0},
	{"50c153de82d122a1b44e10db267ff015", 0x41, 1, 66592, 0},
	{"7da29a58770816df6d7d1dc31f6a63ac", 0x42, 0, 53032, 0},
	{"9845a7b6df4cbc7471a404144d373b03", 0x42, 1, 99872, 0},
	{"90e56a51ee2c250b5c8552efaa627a1e", 0x43, 0, 77264, 0},
	{"77a7d106562cb9530eabb8f722f18732", 0x43, 1, 99872, 0},
	{"dac46b9373ae893ac862da275abda750", 0x44, 0, 75320, 0},
	{"1e9adf1aa2301634d4846f1010b9e4f8", 0x44, 1, 99872, 0},
	{"b56695f236d17706abdcdbf18318cdf4", 0x45, 0, 73980, 0},
	{"01651b5abb5efe846a078f59ae048688", 0x45, 1, 66592, 0},
	{"dac46b9373ae893ac862da275abda750", 0x46, 0, 75320, 0},
	{"c3886b32d5dddbc7a73bca3ef3b2f3df", 0x46, 1, 99872, 0},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", 0x47, 0, 51252, 0},
	{"cbf238857adba90a47f364c48fcddaf2", 0x47, 1, 99872, 0},
	{"63dd2c1b864c3d9a98e49e278df6a992", 0x48, 0, 30876, 0},
	{"bccefccabaaf5839e7e19d413db6a9f1", 0x48, 1, 99872, 0},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", 0x49, 0, 51252, 0},
	{"f556ba26e51e97c26af0e7d3e919ee16", 0x49, 1, 99872, 0},
	{"8e9c72a9d3733bcd0ed257808b85bf9d", 0x4a, 0, 160568, 0},
	{"4d0a6328f194e4f9386200f1582b905a", 0x4a, 1, 99872, 0},
	{"f0c3ca390cc4be5579c5320b809b7299", 0x4b, 0, 18316, 0},
	{"374a746bf9ece6544e3d6fe234a9e698", 0x4b, 1, 66592, 0},
	{"710c76fcdd808ca2dc1d0f2f3aee04a7", 0x4c, 1, 66592, 0},
	{"7f1cc178056133ae506fecc721f02e6c", 0x4f, 0, 74508, 0},
	{"a8b1f204345315c69062a6836891cb59", 0x4f, 1, 66592, 0},
	{"e3f108230e364c8eb3730500bb70b2e7", 0x50, 0, 74968, 0},
	{"97836541381842f855415f271661e214", 0x50, 1, 99872, 0},
	{"eb11c964a87eb2ed8009a026098e406a", 0x51, 0, 76516, 0},
	{"7ac74ba3ea23345671347930431fda6c", 0x51, 1, 66592, 0},
	{"e3f108230e364c8eb3730500bb70b2e7", 0x54, 0, 74968, 0},
	{"e69a5156b1834285d0a0c3e61abae6b7", 0x54, 1, 66592, 0},
	{"9217988693cf44925fb1d0aaf8993c21", 0x55, 0, 76340, 0},
	{"7b6d631b69d71c73adaffd05a5510a7d", 0x55, 1, 66592, 0},
	{"5489924c864d3e10d6a5f9048af4b393", 0x58, 0, 74992, 0},
	{"e4fea702c120d38842d998fbfcede435", 0x58, 1, 66592, 0},
	{"6525392db19f11575f3ac684f4005bac", 0x59, 0, 74844, 0},
	{"62a20d18355fdfe6a84a5de70fa97533", 0x59, 1, 99872, 0},
	{"5ed409a2f1d755dd5117912d6148a71a", 0x5a, 0, 78064, 0},
	{"8dc47a09016872406d7c9a6426aee2e4", 0x5a, 1, 99872, 0},
	//{"b5e0f6d1e1f00b3dd93d938045849ef6", 0x13a, 1, 66592, 0},
	/* Claire */
	{"fbbeea85c53cb0b52a155ae54915feab", 0x10, 0, 147926, 0},
	{"4385f25501af1b41eb87df27ac515e26", 0x10, 1, 66592, 0},
	{"281187a3081824c12fbcf73aeb759df9", 0x11, 0, 149872, 0},
	{"1bd30c3c9d2d34b538f71fe0daac039a", 0x11, 1, 66592, 0},
	{"601059eb87629150b4bf666419eed0a0", 0x12, 0, 146672, 0},
	{"478d50899cab520e569f1b4790acc74d", 0x12, 1, 133152, 0},
	{"14e19d90eba7586f51d6641530a522f0", 0x13, 0, 147136, 0},
	{"6b60ee48d12032a6154b5bf8b54f9b15", 0x13, 1, 66592, 0},
	{"4a77d3cec668dfc1a1d5ab22afa2d797", 0x15, 0, 147008, 0},
	{"54b016b6078faf327f7940d54f35bb53", 0x15, 1, 66592, 0},
	{"c9a5ba480c01de3ce99de0ee591b0a9d", 0x16, 0, 146416, 0},
	{"5cdf267b2e7aaca4150bfa890cc98848", 0x16, 1, 66592, 0},
	{"4eb367f155f864ee82ee742b450ec43c", 0x17, 0, 151596, 0},
	{"e87820d85d8c14526f2a7594c77d83e7", 0x17, 1, 66592, 0},
	{"f157bd9bf9f7d917d8d15ed7601bcbfc", 0x18, 0, 141208, 0},
	{"eded3183637af0bdff389fd47c0871f6", 0x18, 1, 66592, 0},
	{"f157bd9bf9f7d917d8d15ed7601bcbfc", 0x1e, 0, 141208, 0},
	{"81da2b41c60639ff7bca00d15385ca16", 0x1e, 1, 133152, 0},
	{"f157bd9bf9f7d917d8d15ed7601bcbfc", 0x1f, 0, 141208, 0},
	{"81da2b41c60639ff7bca00d15385ca16", 0x1f, 1, 133152, 0},
	{"b95b5b435552e86c3b35da3036f12b5c", 0x20, 0, 121144, 0},
	{"e989eff61ae9451875d7bb811d8f84ab", 0x20, 1, 66592, 0},
	{"356c4f4855b871398274922d8794f1aa", 0x21, 0, 32716, 0},
	{"9dace0a69e32b1a4ef7638a5847bc235", 0x21, 1, 33312, 0},
	{"bf168891667fcf313a46c177b4bc5cad", 0x22, 0, 174960, 0},
	{"cb66446c89e7de18fc8954d8cf46b90e", 0x22, 1, 66592, 0},
	{"05e49a8710100e53b4765ed867e252f9", 0x23, 0, 189208, 0},
	{"37dd53a15bae3a104fb35c7589276c1c", 0x23, 1, 133152, 0},
	{"7b4ff1861a79e4bc1341aee310755101", 0x24, 0, 129712, 0},
	{"672431dfacf710b6b5b1fdb5b88bcf24", 0x24, 1, 66592, 0},
	{"8b91976d410b964b87580ff384a8901a", 0x25, 0, 53448, 0},
	{"8ef6d0faef12f3f4f30e3f23b0f76457", 0x25, 1, 66592, 0},
	{"fcd60f4d49d46b86ed6b3ae116a81594", 0x26, 0, 7096, 0},
	{"6ea27dfbb8388f5fd7d1e150eeebfc30", 0x26, 1, 33312, 0},
	{"71b1caa84ddbca135fb14a0437c1042e", 0x27, 0, 39632, 0},
	{"c6c2d650d18a58b94c2f75d544806bf0", 0x27, 1, 33312, 0},
	{"ba0fb709c16dda6444097af318c1ff39", 0x28, 0, 188172, 0},
	{"8fc09baefbf3cfbd3e38a5492ce45df9", 0x28, 1, 133152, 0},
	{"d7c505ceee8a4d782a592ac8099c8634", 0x29, 0, 10456, 0},
	{"12061e2481721831d15ff784dc8eea8a", 0x29, 1, 33312, 0},
	{"b53e26dd014e48026d3e424653d865ab", 0x2a, 0, 138372, 0},
	{"b93c27aa658957ff7ff1f047453d0cb8", 0x2a, 1, 66592, 0},
	{"a896bb6f7ff1592ae8057e664b4d60d8", 0x2b, 0, 144516, 0},
	{"f8013ea530c5b1b54e97c60f728a2a87", 0x2b, 1, 99872, 0},
	{"37a2e00c405e30da738a3bcf3807df02", 0x2c, 0, 16384, 0},
	{"af3a30ec501813db390fb2954c55c4ae", 0x2c, 1, 33312, 0},
	{"7e03d04e46fcf21f5dae9954e612a41c", 0x2d, 0, 24608, 0},
	{"c965e1faa886bde5f873a4d92da0b35b", 0x2d, 1, 33312, 0},
	{"7cae35510f0da78dcb1a4aa81416dd3c", 0x2e, 0, 216780, 0},
	{"597393acf844348b6ae01232c3d0e9a6", 0x2e, 1, 66592, 0},
	{"762f6f048607c3eebf1280714f55e06f", 0x2f, 0, 5496, 0},
	{"75e52e00a5f6a8d1c00963fd8a155a35", 0x2f, 1, 33312, 0},
	{"f51a209ae70951c7ca351e75b6321fb5", 0x30, 0, 152188, 0},
	{"e203e27ca42c6929d6e783c53fe36b07", 0x30, 1, 99872, 0},
	{"e01ad6106c9c38c064e9560d8b7cd282", 0x31, 0, 193124, 0},
	{"abf89ee4772dc4b0be5f61d840f2338f", 0x31, 1, 99872, 0},
	{"990fb36ecf6a3ee6649080cbf109475e", 0x33, 0, 204968, 0},
	{"a03ef4b1a217bb620ed723cdbc2f4456", 0x33, 1, 99872, 0},
	{"1f0a78e4d7aa767afb994fbf59f4f21d", 0x34, 0, 273812, 0},
	{"67d5f6322485677a7e43c8ac2cac1800", 0x34, 1, 99872, 0},
	{"3a49b091f8aded3c93540856349f5c5d", 0x36, 0, 112340, 0},
	{"274087f2c77f1363efd1cb2a841211e9", 0x36, 1, 132640, 0},
	{"9ca54ae9d3019f39f3c3986e37c0c6fb", 0x37, 0, 59376, 0},
	{"e3c118c118921ffb90480f493a6c3e64", 0x37, 1, 33312, 0},
	{"ec318c6e6f94dedb2187cd81a45a0637", 0x38, 0, 6268, 0},
	{"58b1e69b04cbcf9cdeecc6ef63075b13", 0x38, 1, 33312, 0},
	{"7cae35510f0da78dcb1a4aa81416dd3c", 0x39, 0, 216780, 0},
	{"50edab0ce1d66682b2c5600ce592f67a", 0x39, 1, 66592, 0},
	{"527b4578a9ce18b17cbe21577d221c93", 0x3a, 0, 44032, 0},
	{"e086b31a3f3a0eaaea7daeb24952e6ec", 0x3a, 1, 66592, 0},
	{"75189ca3adf7e8b01be9f00810ffc985", 0x3b, 0, 3480, 0},
	{"d51a0fddd0d785b7b4918da7615e4619", 0x3b, 1, 33312, 0},
	{"dd592c77fb2eadd6259046930682ed4f", 0x3e, 0, 55312, 0},
	{"e0fd7b02739b8f2ba7b363cd27868ef0", 0x3e, 1, 33312, 0},
	{"bd3ca4f4c45a1b38db7c306744579ae5", 0x3f, 0, 25972, 0},
	{"34be3c0203aed4a7f9bb504b7ab95976", 0x3f, 1, 33312, 0},
	{"c68e845ffb355b5dbdb0220f1d457718", 0x40, 0, 54856, 0},
	{"e064a33abaa0ea2e037f2df3c8684586", 0x40, 1, 99872, 0},
	{"2743183673067f15b56a431e7834a688", 0x41, 0, 79608, 0},
	{"50c153de82d122a1b44e10db267ff015", 0x41, 1, 66592, 0},
	{"b6ee62b485b3218aadb93a86bb17c049", 0x42, 0, 50688, 0},
	{"9845a7b6df4cbc7471a404144d373b03", 0x42, 1, 99872, 0},
	{"2743183673067f15b56a431e7834a688", 0x43, 0, 79608, 0},
	{"77a7d106562cb9530eabb8f722f18732", 0x43, 1, 99872, 0},
	{"dac46b9373ae893ac862da275abda750", 0x44, 0, 75320, 0},
	{"1e9adf1aa2301634d4846f1010b9e4f8", 0x44, 1, 99872, 0},
	{"b56695f236d17706abdcdbf18318cdf4", 0x45, 0, 73980, 0},
	{"01651b5abb5efe846a078f59ae048688", 0x45, 1, 66592, 0},
	{"dac46b9373ae893ac862da275abda750", 0x46, 0, 75320, 0},
	{"c3886b32d5dddbc7a73bca3ef3b2f3df", 0x46, 1, 99872, 0},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", 0x47, 0, 51252, 0},
	{"cbf238857adba90a47f364c48fcddaf2", 0x47, 1, 99872, 0},
	{"a8cff2b11a22f4e964ef03b51e8d51ff", 0x48, 0, 31244, 0},
	{"bccefccabaaf5839e7e19d413db6a9f1", 0x48, 1, 99872, 0},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", 0x49, 0, 51252, 0},
	{"f556ba26e51e97c26af0e7d3e919ee16", 0x49, 1, 99872, 0},
	{"29377fdfc788152e1e9fdc88fe446a10", 0x4a, 0, 160972, 0},
	{"4d0a6328f194e4f9386200f1582b905a", 0x4a, 1, 99872, 0},
	{"f0c3ca390cc4be5579c5320b809b7299", 0x4b, 0, 18316, 0},
	{"374a746bf9ece6544e3d6fe234a9e698", 0x4b, 1, 66592, 0},
	{"710c76fcdd808ca2dc1d0f2f3aee04a7", 0x4c, 1, 66592, 0},
	{"7f1cc178056133ae506fecc721f02e6c", 0x4f, 0, 74508, 0},
	{"a8b1f204345315c69062a6836891cb59", 0x4f, 1, 66592, 0},
	{"e3f108230e364c8eb3730500bb70b2e7", 0x50, 0, 74968, 0},
	{"97836541381842f855415f271661e214", 0x50, 1, 99872, 0},
	{"eb11c964a87eb2ed8009a026098e406a", 0x51, 0, 76516, 0},
	{"7ac74ba3ea23345671347930431fda6c", 0x51, 1, 66592, 0},
	{"e3f108230e364c8eb3730500bb70b2e7", 0x54, 0, 74968, 0},
	{"e69a5156b1834285d0a0c3e61abae6b7", 0x54, 1, 66592, 0},
	{"9217988693cf44925fb1d0aaf8993c21", 0x55, 0, 76340, 0},
	{"7b6d631b69d71c73adaffd05a5510a7d", 0x55, 1, 66592, 0},
	{"5489924c864d3e10d6a5f9048af4b393", 0x58, 0, 74992, 0},
	{"e4fea702c120d38842d998fbfcede435", 0x58, 1, 66592, 0},
	{"6525392db19f11575f3ac684f4005bac", 0x59, 0, 76844, 0},
	{"62a20d18355fdfe6a84a5de70fa97533", 0x59, 1, 99872, 0},
	{"5ed409a2f1d755dd5117912d6148a71a", 0x5a, 0, 78064, 0},
	{"8dc47a09016872406d7c9a6426aee2e4", 0x5a, 1, 99872, 0}
};

EmsArchive::EmsArchive(Common::SeekableReadStream *stream): _stream(stream) {
	_stream->seek(0);
}

EmsArchive::~EmsArchive() {
	_stream = nullptr;
}

/*void EmsArchive::scanArchive(void) {
	for (uint32 i=0; i<sizeof(emsItems)/sizeof(ems_item_t); i++) {
		if (emsItems[i].length==0)
			continue;

		debug(3, "re2:ems: Searching for %s...", emsItems[i].filename);
		for (uint32 startOffset=0; startOffset<_stream->size() - emsItems[i].length; startOffset+=0x800) {
			Common::SeekableSubReadStream *subStream = new Common::SeekableSubReadStream(_stream,
				startOffset, startOffset + emsItems[i].length);

			if (computeStreamMD5AsString(*subStream).equalsIgnoreCase(emsItems[i].md5sum)) {
				debug(3, "re2:ems: Found %s at offset 0x%04x", emsItems[i].filename, startOffset);
				break;
			}
		}
	}
}*/

Common::SeekableReadStream *EmsArchive::createReadStreamForMember(int numModel, int fileType) const {
	if (!_stream)
		return nullptr;

	debug(3, "re2:ems: Searching for em0%02x.%s...", numModel, (fileType==0 ? "emd" : "tim"));

	for (uint32 i=0; i<sizeof(emsItems)/sizeof(ems_item_t); i++) {
		if ((emsItems[i].length==0) || (emsItems[i].fType!=fileType) || (emsItems[i].model!=numModel))
			continue;

		/* Already found ? */
		if (emsItems[i].offset != 0) {
			Common::SeekableSubReadStream *subStream = new Common::SeekableSubReadStream(_stream,
				emsItems[i].offset, emsItems[i].offset + emsItems[i].length);

			return subStream;
		}

		for (uint32 startOffset=0; startOffset<_stream->size() - emsItems[i].length; startOffset+=0x800) {
			Common::SeekableSubReadStream *subStream = new Common::SeekableSubReadStream(_stream,
				startOffset, startOffset + emsItems[i].length);

			if (computeStreamMD5AsString(*subStream).equalsIgnoreCase(emsItems[i].md5sum)) {
				debug(3, "re2:ems:  Found at offset 0x%04x", startOffset);

				emsItems[i].offset = startOffset;
				return subStream;
			}

			delete subStream;
		}
	}

	debug(3, "re2:ems: Not found");
	return nullptr;
}

} // End of namespace Reevengi
