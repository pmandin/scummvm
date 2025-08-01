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

#include "ultima/nuvie/core/tile_manager.h"
#include "ultima/nuvie/core/map.h"
#include "ultima/nuvie/core/obj_manager.h"
#include "ultima/nuvie/misc/u6_misc.h"
#include "ultima/nuvie/files/tmx_map.h"

namespace Ultima {
namespace Nuvie {

TMXMap::TMXMap(TileManager *tm, Map *m, ObjManager *om) : tile_manager(tm),
		map(m), obj_manager(om), mapdata(nullptr) {
}

TMXMap::~TMXMap() {

}

bool TMXMap::exportTmxMapFiles(const Common::Path &dir, nuvie_game_t type) {
	savedir = dir;
	savename = get_game_tag(type);
	Common::Path filename;
	build_path(savedir, savename + "_tileset.bmp", filename);



	tile_manager->exportTilesetToBmpFile(filename);

	for (uint8 i = 0; i < 6; i++) {
		writeRoofTileset(i);
		exportMapLevel(i);
	}

	return true;
}

void TMXMap::writeRoofTileset(uint8 level) {
	if (map->get_roof_data(level) == nullptr) {
		return;
	}

	Common::Path filename = map->getRoofTilesetFilename();
	Common::Path destFilename;
	build_path(savedir, savename + "_roof_tileset.bmp", destFilename);
	NuvieIOFileRead read;
	NuvieIOFileWrite write;
	read.open(filename);
	write.open(destFilename);
	unsigned char *buf = read.readAll();
	write.writeBuf(buf, read.get_size());
	write.close();
	read.close();
	free(buf);
}

void TMXMap::writeLayer(NuvieIOFileWrite *tmx, uint16 sideLength, Std::string layerName,
		uint16 gidOffset, uint16 bitsPerTile, const unsigned char *data) {
	Std::string slen = sint32ToString((sint32)sideLength);
	Std::string header = " <layer name=\"" + layerName + "\" width=\"" + slen + "\" height=\""
	                     + slen + "\">\n";
	header += "  <data encoding=\"csv\">\n";

	tmx->writeBuf((const unsigned char *)header.c_str(), header.size());

	uint16 mx, my;
	for (my = 0; my < sideLength; my++) {
		for (mx = 0; mx < sideLength; mx++) {
			uint16 gid = 0;
			if (bitsPerTile == 8) { //base map is uint8
				gid = (uint16)data[my * sideLength + mx] + 1 + gidOffset;
			} else { //everything else is uint16
				gid = ((const uint16 *)data)[my * sideLength + mx] + 1 + gidOffset;
			}
			// 'nnnn\0'
			Common::String temp = Common::String::format("%d", gid);
			tmx->writeBuf((const unsigned char *)temp.c_str(), temp.size());
			if (mx < sideLength - 1 || my < sideLength - 1) { //don't write comma after last element in the array.
				tmx->write1(',');
			}
		}
		tmx->write1('\n');
	}

	Std::string footer = "  </data>\n";
	footer += " </layer>\n";

	tmx->writeBuf((const unsigned char *)footer.c_str(), footer.size());
}

void TMXMap::writeObjectLayer(NuvieIOFileWrite *tmx, uint8 level) {
	Std::string xml = "<objectgroup name=\"Object Layer\">\n";
	tmx->writeBuf((const unsigned char *)xml.c_str(), xml.size());

	writeObjects(tmx, level, true, false);
	writeObjects(tmx, level, false, false);
	writeObjects(tmx, level, false, true);

	xml = "</objectgroup>\n";
	tmx->writeBuf((const unsigned char *)xml.c_str(), xml.size());
}

bool TMXMap::canDrawTile(Tile *t, bool forceLower, bool toptile) {
	if (forceLower == false && (t->flags3 & 0x4) && toptile == false) //don't display force lower tiles.
		return false;

	if (forceLower == true && !(t->flags3 & 0x4))
		return false;

	if ((toptile && !t->toptile) || (!toptile && t->toptile))
		return false;

	return true;
}

Std::string TMXMap::writeObjectTile(Obj *obj, Std::string nameSuffix, uint16 tile_num, uint16 x, uint16 y, bool forceLower, bool toptile) {
	Tile *t = tile_manager->get_tile(tile_num);

	if (canDrawTile(t, forceLower, toptile)) {
		return "  <object name=\"" + encode_xml_entity(Std::string(obj_manager->get_obj_name(obj))) + nameSuffix + "\" gid=\"" + sint32ToString(tile_num + 1) + "\" x=\"" + sint32ToString(x * 16) + "\" y=\"" + sint32ToString((y + 1) * 16) + "\" width=\"16\" height=\"16\"/>\n";
	}

	return Std::string();
}

void TMXMap::writeObjects(NuvieIOFileWrite *tmx, uint8 level, bool forceLower, bool toptiles) {
	uint16 width = map->get_width(level);

	for (uint16 y = 0; y < width; y++) {
		for (uint16 x = 0; x < width; x++) {
			U6LList *list = obj_manager->get_obj_list(x, y, level);
			if (list) {
				for (U6Link *link = list->start(); link != nullptr; link = link->next) {
					Obj *obj = (Obj *)link->data;
					Tile *t = tile_manager->get_original_tile(obj_manager->get_obj_tile_num(obj->obj_n) + obj->frame_n);
					Std::string s;
					if (canDrawTile(t, forceLower, toptiles)) {
						s = "  <object name=\"" + encode_xml_entity(Std::string(obj_manager->get_obj_name(obj))) + "\" gid=\"" + sint32ToString(obj_manager->get_obj_tile_num(obj->obj_n) + obj->frame_n + 1) + "\" x=\"" + sint32ToString((x) * 16) + "\" y=\"" + sint32ToString((y + 1) * 16) + "\" width=\"16\" height=\"16\">\n";
						s += "    <properties>\n";
						s += "       <property name=\"obj_n\" value=\"" + sint32ToString(obj->obj_n) + "\"/>\n";
						s += "       <property name=\"frame_n\" value=\"" + sint32ToString(obj->frame_n) + "\"/>\n";
						s += "       <property name=\"qty\" value=\"" + sint32ToString(obj->qty) + "\"/>\n";
						s += "       <property name=\"quality\" value=\"" + sint32ToString(obj->quality) + "\"/>\n";
						s += "       <property name=\"status\" value=\"" + sint32ToString(obj->status) + "\"/>\n";
						s += "       <property name=\"toptile\" value=\"" + boolToString(t->toptile) + "\"/>\n";
						s += "    </properties>\n";
						s += "  </object>\n";
					}
					if (t->dbl_width) {
						s += writeObjectTile(obj, " -x", t->tile_num - 1, x - 1, y, forceLower, toptiles);
					}
					if (t->dbl_height) {
						uint16 tile_num = t->tile_num - 1;
						if (t->dbl_width) {
							tile_num--;
						}
						s += writeObjectTile(obj, " -y", tile_num, x, y - 1, forceLower, toptiles);
					}
					if (t->dbl_width && t->dbl_height) {
						s += writeObjectTile(obj, " -x,-y", t->tile_num - 3, x - 1, y - 1, forceLower, toptiles);
					}
					tmx->writeBuf((const unsigned char *)s.c_str(), s.size());
				}
			}
		}
	}
}

bool TMXMap::exportMapLevel(uint8 level) {
	NuvieIOFileWrite tmx;
	uint16 width = map->get_width(level);
	mapdata = map->get_map_data(level);
	Common::String level_string = Common::String::format("%d", level); // 'nn\0'
	Common::Path filename;
	build_path(savedir, savename + "_" + Std::string(level_string.c_str()) + ".tmx", filename);

	tmx.open(filename);
	Std::string swidth = sint32ToString((sint32)width);
	Std::string header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	header +=
	    "<map version=\"1.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\""
	    + swidth + "\" height=\"" + swidth
	    + "\" tilewidth=\"16\" tileheight=\"16\">\n";
	header +=
	    " <tileset firstgid=\"1\" name=\"tileset\" tilewidth=\"16\" tileheight=\"16\">\n";
	header += "  <image source=\"" + savename
	          + "_tileset.bmp\" trans=\"00dffc\" width=\"512\" height=\"1024\"/>\n";
	header += " </tileset>\n";

	if (map->get_roof_data(level) != nullptr) {
		header +=
		    " <tileset firstgid=\"2048\" name=\"roof_tileset\" tilewidth=\"16\" tileheight=\"16\">\n";
		header += "  <image source=\"" + savename + "_roof_tileset.bmp\" trans=\"0070fc\" width=\"80\" height=\"3264\"/>\n";
		header += " </tileset>\n";
	}

	tmx.writeBuf((const unsigned char *)header.c_str(), header.size());

	writeLayer(&tmx, width, "BaseLayer", 0, 8, mapdata);

	writeObjectLayer(&tmx, level);

	if (map->get_roof_data(level) != nullptr) {
		writeLayer(&tmx, width, "RoofLayer", 2047, 16, (const unsigned char *)map->get_roof_data(level));
	}

	Std::string footer = "</map>\n";


	tmx.writeBuf((const unsigned char *)footer.c_str(), footer.size());



	tmx.close();

	return true;
}

Std::string TMXMap::sint32ToString(sint32 value) {
	char buf[12];
	snprintf(buf, sizeof(buf), "%d", value);
	return Std::string(buf);
}

Std::string TMXMap::boolToString(bool value) {
	return value ? Std::string("true") : Std::string("false");
}

} // End of namespace Nuvie
} // End of namespace Ultima
