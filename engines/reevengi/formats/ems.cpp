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
	const char *filename;
	int32 length;
} ems_item_t;

ems_item_t emsItems[]={
	/* Leon */
	{"8479ebef2e5e49489ece15227620f814", "em010.emd", 146892},
	{"4385f25501af1b41eb87df27ac515e26", "em010.tim", 66592},
	{"0594f2f8e99daf0fe1d4c33ff296404e", "em011.emd", 149468},
	{"1bd30c3c9d2d34b538f71fe0daac039a", "em011.tim", 66592},
	{"3c2cbc1ddfeae4dc9809ca6ff7593a3d", "em012.emd", 146268},
	{"478d50899cab520e569f1b4790acc74d", "em012.tim", 133152},
	{"57160c3b92a876daec05b09e040ec894", "em013.emd", 146732},
	{"6b60ee48d12032a6154b5bf8b54f9b15", "em013.tim", 66592},
	{"8e7024afb96cbc7a2359dbe2b5a750aa", "em015.emd", 146604},
	{"54b016b6078faf327f7940d54f35bb53", "em015.tim", 66592},
	{"38cb59dfbb944fdaeb3f329782011f6a", "em016.emd", 146012},
	{"5cdf267b2e7aaca4150bfa890cc98848", "em016.tim", 66592},
	{"d83098df3592f75997762b1d009aeb66", "em017.emd", 151192},
	{"e87820d85d8c14526f2a7594c77d83e7", "em017.tim", 66592},
	{"7ea5c9e93671c06cde44660b188f9486", "em018.emd", 140804},
	{"eded3183637af0bdff389fd47c0871f6", "em018.tim", 66592},
	{"7ea5c9e93671c06cde44660b188f9486", "em01e.emd", 140804},
	{"81da2b41c60639ff7bca00d15385ca16", "em01e.tim", 133152},
	{"7ea5c9e93671c06cde44660b188f9486", "em01f.emd", 140804},
	{"81da2b41c60639ff7bca00d15385ca16", "em01f.tim", 133152},
	{"a9ca56311527a0f0780b8f790e521f1b", "em020.emd", 121144},
	{"e989eff61ae9451875d7bb811d8f84ab", "em020.tim", 66592},
	{"bc370bac2306e3b716db6c6c4f3077cc", "em021.emd", 32296},
	{"9dace0a69e32b1a4ef7638a5847bc235", "em021.tim", 33312},
	{"09aa186fd0d17cd256a4cce6980a021e", "em022.emd", 174964},
	{"cb66446c89e7de18fc8954d8cf46b90e", "em022.tim", 66592},
	{"9673f75fb713ef5947232b6c32b37242", "em023.emd", 189208},
	{"37dd53a15bae3a104fb35c7589276c1c", "em023.tim", 133152},
	{"7b4ff1861a79e4bc1341aee310755101", "em024.emd", 129712},
	{"672431dfacf710b6b5b1fdb5b88bcf24", "em024.tim", 66592},
	{"8b91976d410b964b87580ff384a8901a", "em025.emd", 53448},
	{"8ef6d0faef12f3f4f30e3f23b0f76457", "em025.tim", 66592},
	{"97949726ee43871544c00e3455ba1e1d", "em026.emd", 6676},
	{"6ea27dfbb8388f5fd7d1e150eeebfc30", "em026.tim", 33312},
	{"6f50669cfec2e783dddab5e2e59b8ece", "em027.emd", 39632},
	{"c6c2d650d18a58b94c2f75d544806bf0", "em027.tim", 33312},
	{"08fe2e5c3c96a9922e0ac339e119faec", "em028.emd", 188172},
	{"8fc09baefbf3cfbd3e38a5492ce45df9", "em028.tim", 133152},
	{"a3c290ffa3cfd425ee939ab0c826378f", "em029.emd", 12560},
	{"12061e2481721831d15ff784dc8eea8a", "em029.tim", 33312},
	{"0cc92755c016333f69e0069eb413ce15", "em02a.emd", 138372},
	{"b93c27aa658957ff7ff1f047453d0cb8", "em02a.tim", 66592},
	{"40b9b0cc398b6ff8573d95ac31fbe7aa", "em02b.emd", 99872},
	{"f8013ea530c5b1b54e97c60f728a2a87", "em02b.tim", 139896},
	{"3dceb40786c3b340ee1ccfbea181ee2c", "em02c.emd", 99872},
	{"af3a30ec501813db390fb2954c55c4ae", "em02c.tim", 15964},
	{"39d093569618311805c03ce9a1ef67e7", "em02d.emd", 24608},
	{"c965e1faa886bde5f873a4d92da0b35b", "em02d.tim", 33312},
	{"da1a245c7a3e11fdbe228d099ca10893", "em02e.emd", 216780},
	{"597393acf844348b6ae01232c3d0e9a6", "em02e.tim", 66592},
	{"762f6f048607c3eebf1280714f55e06f", "em02f.emd", 5496},
	{"75e52e00a5f6a8d1c00963fd8a155a35", "em02f.tim", 33312},
	{"aa67358d9f8ecc7e4588c262b8d287ec", "em030.emd", 152188},
	{"e203e27ca42c6929d6e783c53fe36b07", "em030.tim", 99872},
	{"9eaf491b7196a6f64e00dbb98fab8d1f", "em031.emd", 193124},
	{"abf89ee4772dc4b0be5f61d840f2338f", "em031.tim", 99872},
	{"043ae92f189c8270fdb59a049a0c5e43", "em033.emd", 204968},
	{"a03ef4b1a217bb620ed723cdbc2f4456", "em033.tim", 99872},
	{"034cb7b8b67df59c38fda499309fe9a5", "em034.emd", 273812},
	{"67d5f6322485677a7e43c8ac2cac1800", "em034.tim", 99872},
	{"3a49b091f8aded3c93540856349f5c5d", "em036.emd", 112340},
	{"274087f2c77f1363efd1cb2a841211e9", "em036.tim", 132640},
	{"22756bc25cfda5ce4233e2028fa5941a", "em037.emd", 59376},
	{"e3c118c118921ffb90480f493a6c3e64", "em037.tim", 33312},
	{"ec318c6e6f94dedb2187cd81a45a0637", "em038.emd", 6268},
	{"58b1e69b04cbcf9cdeecc6ef63075b13", "em038.tim", 33312},
	{"da1a245c7a3e11fdbe228d099ca10893", "em039.emd", 216780},
	{"50edab0ce1d66682b2c5600ce592f67a", "em039.tim", 66592},
	{"c084ef69626b557fcb6324f45f40f5fd", "em03a.emd", 43612},
	{"e086b31a3f3a0eaaea7daeb24952e6ec", "em03a.tim", 66592},
	{"75189ca3adf7e8b01be9f00810ffc985", "em03b.emd", 3480},
	{"d51a0fddd0d785b7b4918da7615e4619", "em03b.tim", 33312},
	{"dd592c77fb2eadd6259046930682ed4f", "em03e.emd", 55312},
	{"e0fd7b02739b8f2ba7b363cd27868ef0", "em03e.tim", 33312},
	{"bd3ca4f4c45a1b38db7c306744579ae5", "em03f.emd", 25972},
	{"34be3c0203aed4a7f9bb504b7ab95976", "em03f.tim", 33312},
	{"c68e845ffb355b5dbdb0220f1d457718", "em040.emd", 54856},
	{"e064a33abaa0ea2e037f2df3c8684586", "em040.tim", 99872},
	{"90e56a51ee2c250b5c8552efaa627a1e", "em041.emd", 77264},
	{"50c153de82d122a1b44e10db267ff015", "em041.tim", 66592},
	{"7da29a58770816df6d7d1dc31f6a63ac", "em042.emd", 53032},
	{"9845a7b6df4cbc7471a404144d373b03", "em042.tim", 99872},
	{"90e56a51ee2c250b5c8552efaa627a1e", "em043.emd", 77264},
	{"77a7d106562cb9530eabb8f722f18732", "em043.tim", 99872},
	{"dac46b9373ae893ac862da275abda750", "em044.emd", 75320},
	{"1e9adf1aa2301634d4846f1010b9e4f8", "em044.tim", 99872},
	{"b56695f236d17706abdcdbf18318cdf4", "em045.emd", 73980},
	{"01651b5abb5efe846a078f59ae048688", "em045.tim", 66592},
	{"dac46b9373ae893ac862da275abda750", "em046.emd", 75320},
	{"c3886b32d5dddbc7a73bca3ef3b2f3df", "em046.tim", 99872},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", "em047.emd", 51252},
	{"cbf238857adba90a47f364c48fcddaf2", "em047.tim", 99872},
	{"63dd2c1b864c3d9a98e49e278df6a992", "em048.emd", 30876},
	{"bccefccabaaf5839e7e19d413db6a9f1", "em048.tim", 99872},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", "em049.emd", 51252},
	{"f556ba26e51e97c26af0e7d3e919ee16", "em049.tim", 99872},
	{"8e9c72a9d3733bcd0ed257808b85bf9d", "em04a.emd", 160568},
	{"4d0a6328f194e4f9386200f1582b905a", "em04a.tim", 99872},
	{"f0c3ca390cc4be5579c5320b809b7299", "em04b.emd", 18316},
	{"374a746bf9ece6544e3d6fe234a9e698", "em04b.tim", 66592},
	{"710c76fcdd808ca2dc1d0f2f3aee04a7", "em04c.tim", 66592},
	{"7f1cc178056133ae506fecc721f02e6c", "em04f.emd", 74508},
	{"a8b1f204345315c69062a6836891cb59", "em04f.tim", 66592},
	{"e3f108230e364c8eb3730500bb70b2e7", "em050.emd", 74968},
	{"97836541381842f855415f271661e214", "em050.tim", 99872},
	{"eb11c964a87eb2ed8009a026098e406a", "em051.emd", 76516},
	{"7ac74ba3ea23345671347930431fda6c", "em051.tim", 66592},
	{"e3f108230e364c8eb3730500bb70b2e7", "em054.emd", 74968},
	{"e69a5156b1834285d0a0c3e61abae6b7", "em054.tim", 66592},
	{"9217988693cf44925fb1d0aaf8993c21", "em055.emd", 76340},
	{"7b6d631b69d71c73adaffd05a5510a7d", "em055.tim", 66592},
	{"5489924c864d3e10d6a5f9048af4b393", "em058.emd", 74992},
	{"e4fea702c120d38842d998fbfcede435", "em058.tim", 66592},
	{"6525392db19f11575f3ac684f4005bac", "em059.emd", 74844},
	{"62a20d18355fdfe6a84a5de70fa97533", "em059.tim", 99872},
	{"5ed409a2f1d755dd5117912d6148a71a", "em05a.emd", 78064},
	{"8dc47a09016872406d7c9a6426aee2e4", "em05a.tim", 99872},
	{"b5e0f6d1e1f00b3dd93d938045849ef6", "em13a.tim", 66592},
	/* Claire */
	{"fbbeea85c53cb0b52a155ae54915feab", "em110.emd", 147926},
	{"4385f25501af1b41eb87df27ac515e26", "em110.tim", 66592},
	{"281187a3081824c12fbcf73aeb759df9", "em111.emd", 149872},
	{"1bd30c3c9d2d34b538f71fe0daac039a", "em111.tim", 66592},
	{"601059eb87629150b4bf666419eed0a0", "em112.emd", 146672},
	{"478d50899cab520e569f1b4790acc74d", "em112.tim", 133152},
	{"14e19d90eba7586f51d6641530a522f0", "em113.emd", 147136},
	{"6b60ee48d12032a6154b5bf8b54f9b15", "em113.tim", 66592},
	{"4a77d3cec668dfc1a1d5ab22afa2d797", "em115.emd", 147008},
	{"54b016b6078faf327f7940d54f35bb53", "em115.tim", 66592},
	{"c9a5ba480c01de3ce99de0ee591b0a9d", "em116.emd", 146416},
	{"5cdf267b2e7aaca4150bfa890cc98848", "em116.tim", 66592},
	{"4eb367f155f864ee82ee742b450ec43c", "em117.emd", 151596},
	{"e87820d85d8c14526f2a7594c77d83e7", "em117.tim", 66592},
	{"f157bd9bf9f7d917d8d15ed7601bcbfc", "em118.emd", 141208},
	{"eded3183637af0bdff389fd47c0871f6", "em118.tim", 66592},
	{"f157bd9bf9f7d917d8d15ed7601bcbfc", "em11e.emd", 141208},
	{"81da2b41c60639ff7bca00d15385ca16", "em11e.tim", 133152},
	{"f157bd9bf9f7d917d8d15ed7601bcbfc", "em11f.emd", 141208},
	{"81da2b41c60639ff7bca00d15385ca16", "em11f.tim", 133152},
	{"b95b5b435552e86c3b35da3036f12b5c", "em120.emd", 121144},
	{"e989eff61ae9451875d7bb811d8f84ab", "em120.tim", 66592},
	{"356c4f4855b871398274922d8794f1aa", "em121.emd", 32716},
	{"9dace0a69e32b1a4ef7638a5847bc235", "em121.tim", 33312},
	{"bf168891667fcf313a46c177b4bc5cad", "em122.emd", 174960},
	{"cb66446c89e7de18fc8954d8cf46b90e", "em122.tim", 66592},
	{"05e49a8710100e53b4765ed867e252f9", "em123.emd", 189208},
	{"37dd53a15bae3a104fb35c7589276c1c", "em123.tim", 133152},
	{"7b4ff1861a79e4bc1341aee310755101", "em124.emd", 129712},
	{"672431dfacf710b6b5b1fdb5b88bcf24", "em124.tim", 66592},
	{"8b91976d410b964b87580ff384a8901a", "em125.emd", 53448},
	{"8ef6d0faef12f3f4f30e3f23b0f76457", "em125.tim", 66592},
	{"fcd60f4d49d46b86ed6b3ae116a81594", "em126.emd", 7096},
	{"6ea27dfbb8388f5fd7d1e150eeebfc30", "em126.tim", 33312},
	{"71b1caa84ddbca135fb14a0437c1042e", "em127.emd", 39632},
	{"c6c2d650d18a58b94c2f75d544806bf0", "em127.tim", 33312},
	{"ba0fb709c16dda6444097af318c1ff39", "em128.emd", 188172},
	{"8fc09baefbf3cfbd3e38a5492ce45df9", "em128.tim", 133152},
	{"d7c505ceee8a4d782a592ac8099c8634", "em129.emd", 10456},
	{"12061e2481721831d15ff784dc8eea8a", "em129.tim", 33312},
	{"b53e26dd014e48026d3e424653d865ab", "em12a.emd", 138372},
	{"b93c27aa658957ff7ff1f047453d0cb8", "em12a.tim", 66592},
	{"a896bb6f7ff1592ae8057e664b4d60d8", "em12b.emd", 144516},
	{"f8013ea530c5b1b54e97c60f728a2a87", "em12b.tim", 99872},
	{"37a2e00c405e30da738a3bcf3807df02", "em12c.emd", 16384},
	{"af3a30ec501813db390fb2954c55c4ae", "em12c.tim", 33312},
	{"7e03d04e46fcf21f5dae9954e612a41c", "em12d.emd", 24608},
	{"c965e1faa886bde5f873a4d92da0b35b", "em12d.tim", 33312},
	{"7cae35510f0da78dcb1a4aa81416dd3c", "em12e.emd", 216780},
	{"597393acf844348b6ae01232c3d0e9a6", "em12e.tim", 66592},
	{"762f6f048607c3eebf1280714f55e06f", "em12f.emd", 5496},
	{"75e52e00a5f6a8d1c00963fd8a155a35", "em12f.tim", 33312},
	{"f51a209ae70951c7ca351e75b6321fb5", "em130.emd", 152188},
	{"e203e27ca42c6929d6e783c53fe36b07", "em130.tim", 99872},
	{"e01ad6106c9c38c064e9560d8b7cd282", "em131.emd", 193124},
	{"abf89ee4772dc4b0be5f61d840f2338f", "em131.tim", 99872},
	{"990fb36ecf6a3ee6649080cbf109475e", "em133.emd", 204968},
	{"a03ef4b1a217bb620ed723cdbc2f4456", "em133.tim", 99872},
	{"1f0a78e4d7aa767afb994fbf59f4f21d", "em134.emd", 273812},
	{"67d5f6322485677a7e43c8ac2cac1800", "em134.tim", 99872},
	{"3a49b091f8aded3c93540856349f5c5d", "em136.emd", 112340},
	{"274087f2c77f1363efd1cb2a841211e9", "em136.tim", 132640},
	{"9ca54ae9d3019f39f3c3986e37c0c6fb", "em137.emd", 59376},
	{"e3c118c118921ffb90480f493a6c3e64", "em137.tim", 33312},
	{"ec318c6e6f94dedb2187cd81a45a0637", "em138.emd", 6268},
	{"58b1e69b04cbcf9cdeecc6ef63075b13", "em138.tim", 33312},
	{"7cae35510f0da78dcb1a4aa81416dd3c", "em139.emd", 216780},
	{"50edab0ce1d66682b2c5600ce592f67a", "em139.tim", 66592},
	{"527b4578a9ce18b17cbe21577d221c93", "em13a.emd", 44032},
	{"e086b31a3f3a0eaaea7daeb24952e6ec", "em13a.tim", 66592},
	{"75189ca3adf7e8b01be9f00810ffc985", "em13b.emd", 3480},
	{"d51a0fddd0d785b7b4918da7615e4619", "em13b.tim", 33312},
	{"dd592c77fb2eadd6259046930682ed4f", "em13e.emd", 55312},
	{"e0fd7b02739b8f2ba7b363cd27868ef0", "em13e.tim", 33312},
	{"bd3ca4f4c45a1b38db7c306744579ae5", "em13f.emd", 25972},
	{"34be3c0203aed4a7f9bb504b7ab95976", "em13f.tim", 33312},
	{"c68e845ffb355b5dbdb0220f1d457718", "em140.emd", 54856},
	{"e064a33abaa0ea2e037f2df3c8684586", "em140.tim", 99872},
	{"2743183673067f15b56a431e7834a688", "em141.emd", 79608},
	{"50c153de82d122a1b44e10db267ff015", "em141.tim", 66592},
	{"b6ee62b485b3218aadb93a86bb17c049", "em142.emd", 50688},
	{"9845a7b6df4cbc7471a404144d373b03", "em142.tim", 99872},
	{"2743183673067f15b56a431e7834a688", "em143.emd", 79608},
	{"77a7d106562cb9530eabb8f722f18732", "em143.tim", 99872},
	{"dac46b9373ae893ac862da275abda750", "em144.emd", 75320},
	{"1e9adf1aa2301634d4846f1010b9e4f8", "em144.tim", 99872},
	{"b56695f236d17706abdcdbf18318cdf4", "em145.emd", 73980},
	{"01651b5abb5efe846a078f59ae048688", "em145.tim", 66592},
	{"dac46b9373ae893ac862da275abda750", "em146.emd", 75320},
	{"c3886b32d5dddbc7a73bca3ef3b2f3df", "em146.tim", 99872},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", "em147.emd", 51252},
	{"cbf238857adba90a47f364c48fcddaf2", "em147.tim", 99872},
	{"a8cff2b11a22f4e964ef03b51e8d51ff", "em148.emd", 31244},
	{"bccefccabaaf5839e7e19d413db6a9f1", "em148.tim", 99872},
	{"88ca733bfe6ee5aacda66e6ecf47dfe1", "em149.emd", 51252},
	{"f556ba26e51e97c26af0e7d3e919ee16", "em149.tim", 99872},
	{"29377fdfc788152e1e9fdc88fe446a10", "em14a.emd", 160972},
	{"4d0a6328f194e4f9386200f1582b905a", "em14a.tim", 99872},
	{"f0c3ca390cc4be5579c5320b809b7299", "em14b.emd", 18316},
	{"374a746bf9ece6544e3d6fe234a9e698", "em14b.tim", 66592},
	{"710c76fcdd808ca2dc1d0f2f3aee04a7", "em14c.tim", 66592},
	{"7f1cc178056133ae506fecc721f02e6c", "em14f.emd", 74508},
	{"a8b1f204345315c69062a6836891cb59", "em14f.tim", 66592},
	{"e3f108230e364c8eb3730500bb70b2e7", "em150.emd", 74968},
	{"97836541381842f855415f271661e214", "em150.tim", 99872},
	{"eb11c964a87eb2ed8009a026098e406a", "em151.emd", 76516},
	{"7ac74ba3ea23345671347930431fda6c", "em151.tim", 66592},
	{"e3f108230e364c8eb3730500bb70b2e7", "em154.emd", 74968},
	{"e69a5156b1834285d0a0c3e61abae6b7", "em154.tim", 66592},
	{"9217988693cf44925fb1d0aaf8993c21", "em155.emd", 76340},
	{"7b6d631b69d71c73adaffd05a5510a7d", "em155.tim", 66592},
	{"5489924c864d3e10d6a5f9048af4b393", "em158.emd", 74992},
	{"e4fea702c120d38842d998fbfcede435", "em158.tim", 66592},
	{"6525392db19f11575f3ac684f4005bac", "em159.emd", 76844},
	{"62a20d18355fdfe6a84a5de70fa97533", "em159.tim", 99872},
	{"5ed409a2f1d755dd5117912d6148a71a", "em15a.emd", 78064},
	{"8dc47a09016872406d7c9a6426aee2e4", "em15a.tim", 99872}
};

EmsArchive::EmsArchive(Common::SeekableReadStream *stream): _stream(stream),
	_numFiles(0) {

	_stream->seek(0);
	//scanArchive();
}

EmsArchive::~EmsArchive() {
	_stream = nullptr;
}

void EmsArchive::scanArchive(void) {
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
}

Common::SeekableReadStream *EmsArchive::createReadStreamForMember(int numFile) const {
/*	int32 fileOffset = 0x800, fileLength;*/

	if (!_stream || (numFile >= _numFiles))
		return nullptr;
/*
	_stream->seek(8);
	fileLength = _stream->readUint32LE();
	_stream->skip(4);

	for (int i=0; i<numFile; i++) {
		fileOffset += fileLength;
		fileOffset |= 0x7ff;
		fileOffset++;

		fileLength = _stream->readUint32LE();
		_stream->skip(4);
	}

	Common::SeekableSubReadStream *subStream = new Common::SeekableSubReadStream(_stream,
		fileOffset, fileOffset + fileLength);

	return subStream;*/

	return nullptr;
}

} // End of namespace Reevengi
