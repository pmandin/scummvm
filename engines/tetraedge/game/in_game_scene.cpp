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

#include "common/file.h"
#include "common/path.h"
#include "common/textconsole.h"

#include "tetraedge/tetraedge.h"
#include "tetraedge/game/application.h"
#include "tetraedge/game/billboard.h"
#include "tetraedge/game/game.h"
#include "tetraedge/game/in_game_scene.h"
#include "tetraedge/game/character.h"
#include "tetraedge/game/characters_shadow.h"
#include "tetraedge/game/object3d.h"
#include "tetraedge/game/scene_lights_xml_parser.h"

#include "tetraedge/te/te_bezier_curve.h"
#include "tetraedge/te/te_camera.h"
#include "tetraedge/te/te_core.h"
#include "tetraedge/te/te_free_move_zone.h"
#include "tetraedge/te/te_light.h"
#include "tetraedge/te/te_renderer.h"
#include "tetraedge/te/te_lua_script.h"
#include "tetraedge/te/te_lua_thread.h"

//#define TETRAEDGE_DEBUG_PATHFINDING
//#define TETRAEDGE_DEBUG_LIGHTS

namespace Tetraedge {

InGameScene::InGameScene() : _character(nullptr), _charactersShadow(nullptr),
_shadowLightNo(-1), _waitTime(-1.0f), _shadowColor(0, 0, 0, 0x80), _shadowFov(20.0f),
_shadowFarPlane(1000), _shadowNearPlane(1)
 {
}

void InGameScene::activateAnchorZone(const Common::String &name, bool val) {
	for (AnchorZone *zone : _anchorZones) {
		if (zone->_name == name)
			zone->_activated = val;
	}
}

void InGameScene::addAnchorZone(const Common::String &s1, const Common::String &name, float radius) {
	for (AnchorZone *zone : _anchorZones) {
		if (zone->_name == name) {
			zone->_radius = radius;
			return;
		}
	}

	currentCamera()->apply();
	AnchorZone *zone = new AnchorZone();
	zone->_name = name;
	zone->_radius = radius;
	zone->_activated = true;

	if (s1.contains("Int")) {
		TeButtonLayout *btn = hitObjectGui().buttonLayout(name);
		TeVector3f32 pos = btn->position();
		pos.x() += g_engine->getDefaultScreenWidth() / 2.0f;
		pos.y() += g_engine->getDefaultScreenHeight() / 2.0f;
		zone->_loc = currentCamera()->worldTransformationMatrix() * currentCamera()->transformPoint2Dto3D(pos);
	} else {
		if (s1.contains("Dummy")) {
			Dummy d = dummy(name);
			zone->_loc = d._position;
		}
	}
}

bool InGameScene::addMarker(const Common::String &markerName, const Common::String &imgPath, float x, float y, const Common::String &locType, const Common::String &markerVal) {
	const TeMarker *marker = findMarker(markerName);
	if (!marker) {
		Game *game = g_engine->getGame();
		TeSpriteLayout *markerSprite = new TeSpriteLayout();
		// Note: game checks paths here but seems to just use the original?
		markerSprite->setName(markerName);
		markerSprite->setAnchor(TeVector3f32(0.0f, 0.0f, 0.0f));
		markerSprite->load(imgPath);
		markerSprite->setSizeType(TeILayout::RELATIVE_TO_PARENT);
		markerSprite->setPositionType(TeILayout::RELATIVE_TO_PARENT);
		TeVector3f32 newPos;
		if (locType == "PERCENT") {
			Application *app = g_engine->getApplication();
			TeVector3f32 frontLayoutSize = app->frontLayout().userSize();
			newPos.x() = frontLayoutSize.x() * (x / 100.0);
			newPos.y() = frontLayoutSize.y() * (y / 100.0);
		} else {
			newPos.x() = x / g_engine->getDefaultScreenWidth();
			newPos.y() = y / g_engine->getDefaultScreenHeight();
		}
		markerSprite->setPosition(newPos);

		const TeVector3f32 winSize = g_engine->getApplication()->getMainWindow().size();
		if (g_engine->getCore()->fileFlagSystemFlag("definition") == "SD") {
			markerSprite->setSize(TeVector3f32(0.07f, (4.0f / ((winSize.y() / winSize.x()) * 4.0f)) * 0.07f, 0.0));
		} else {
			markerSprite->setSize(TeVector3f32(0.04f, (4.0f / ((winSize.y() / winSize.x()) * 4.0f)) * 0.04f, 0.0));
		}
		markerSprite->setVisible(game->markersVisible());
		markerSprite->_tiledSurfacePtr->_frameAnim.setLoopCount(-1);
		markerSprite->play();

		TeMarker newMarker;
		newMarker._name = markerName;
		newMarker._val = markerVal;
		_markers.push_back(newMarker);
		TeLayout *bg = game->forGui().layout("background");
		if (bg)
			bg->addChild(markerSprite);
		_sprites.push_back(markerSprite);
	} else  {
		setImagePathMarker(markerName, imgPath);
	}
	return true;
}

/*static*/
float InGameScene::angularDistance(float a1, float a2) {
	float result = a2 - a1;
	if (result >= -M_PI && result > M_PI) {
		result = result + -(M_PI * 2);
	} else {
		result = result + (M_PI * 2);
	}
	return result;
}

bool InGameScene::aroundAnchorZone(const AnchorZone *zone) {
	if (!zone->_activated)
		return false;
	const TeVector3f32 charpos = _character->_model->position();

	float xoff = charpos.x() - zone->_loc.x();
	float zoff = charpos.z() - zone->_loc.z();
	return sqrt(xoff * xoff + zoff * zoff) <= zone->_radius;
}

TeLayout *InGameScene::background() {
	return _bgGui.layout("background");
}

Billboard *InGameScene::billboard(const Common::String &name) {
	for (Billboard *billboard : _billboards) {
		if (billboard->model()->name() == name)
			return billboard;
	}
	return nullptr;
}

bool InGameScene::changeBackground(const Common::String &name) {
	if (Common::File::exists(name)) {
		_bgGui.spriteLayoutChecked("root")->load(name);
		return true;
	}
	return false;
}

Character *InGameScene::character(const Common::String &name) {
	if (_character && _character->_model->name() == name)
		return _character;

	for (Character *c : _characters) {
		if (c->_model->name() == name)
			return c;
	}

	// WORKAROUND: Didn't find char, try again with case insensitive
	// for "OScar" typo in scenes/ValTrain/19000.
	for (Character *c : _characters) {
		if (c->_model->name().compareToIgnoreCase(name) == 0)
			return c;
	}

	return nullptr;
}

void InGameScene::close() {
	reset();
	_loadedPath = "";
	TeScene::close();
	freeGeometry();
	if (_character && _character->_model && !findKate()) {
		models().push_back(_character->_model);
		models().push_back(_character->_shadowModel[0]);
		models().push_back(_character->_shadowModel[1]);
	}
	_objects.clear();
	for (TeFreeMoveZone *zone : _freeMoveZones)
		delete zone;
	_freeMoveZones.clear();
	_hitObjects.clear();
	for (TePickMesh2 *mesh : _clickMeshes)
		delete mesh;
	_clickMeshes.clear();
	_bezierCurves.clear();
	_dummies.clear();
	freeSceneObjects();
}

void InGameScene::convertPathToMesh(TeFreeMoveZone *zone) {
	TeIntrusivePtr<TeModel> model = new TeModel();
	model->meshes().clear();
	model->meshes().push_back(Common::SharedPtr<TeMesh>(TeMesh::makeInstance()));
	model->setName("shadowReceiving");
	model->setPosition(zone->position());
	model->setRotation(zone->rotation());
	model->setScale(zone->scale());
	unsigned long nverticies = zone->verticies().size();
	model->meshes()[0]->setConf(nverticies, nverticies, TeMesh::MeshMode_Triangles, 0, 0);
	for (uint i = 0; i < nverticies; i++) {
		model->meshes()[0]->setIndex(i, i);
		model->meshes()[0]->setVertex(i, zone->verticies()[i]);
		model->meshes()[0]->setNormal(i, TeVector3f32(0, 0, 1));
	}
	_zoneModels.push_back(model);
}

TeIntrusivePtr<TeBezierCurve> InGameScene::curve(const Common::String &curveName) {
	for (TeIntrusivePtr<TeBezierCurve> &c : _bezierCurves) {
		if (c->name() == curveName)
			return c;
	}
	return TeIntrusivePtr<TeBezierCurve>();
}

void InGameScene::deleteAllCallback() {
	for (auto &pair : _callbacks) {
		for (auto *cb : pair._value) {
			delete cb;
		}
		pair._value.clear();
	}
	_callbacks.clear();
}

void InGameScene::deleteMarker(const Common::String &markerName) {
	if (!isMarker(markerName))
		return;

	for (uint i = 0; i < _markers.size(); i++) {
		if (_markers[i]._name == markerName) {
			_markers.remove_at(i);
			break;
		}
	}

	Game *game = g_engine->getGame();
	TeLayout *bg = game->forGui().layout("background");
	if (!bg)
		return;
	for (Te3DObject2 *child : bg->childList()) {
		if (child->name() == markerName) {
			bg->removeChild(child);
			break;
		}
	}
}

void InGameScene::deserializeCam(Common::ReadStream &stream, TeIntrusivePtr<TeCamera> &cam) {
	cam->setProjMatrixType(2);
	cam->viewport(0, 0, _viewportSize.getX(), _viewportSize.getY());
	// load name/position/rotation/scale
	Te3DObject2::deserialize(stream, *cam);
	cam->setFov(stream.readFloatLE());
	cam->setPerspectiveVal(stream.readFloatLE());
	// Original loads the second val then ignores it and sets 3000.
	cam->setOrthoPlanes(stream.readFloatLE(), 3000.0);
	stream.readFloatLE();
}

void InGameScene::deserializeModel(Common::ReadStream &stream, TeIntrusivePtr<TeModel> &model, TePickMesh2 *pickmesh) {
	TeVector3f32 vec;
	TeVector2f32 vec2;
	TeQuaternion rot;
	TeColor col;
	Common::SharedPtr<TeMesh> mesh(TeMesh::makeInstance());

	assert(pickmesh);

	TeVector3f32::deserialize(stream, vec);
	model->setPosition(vec);
	pickmesh->setPosition(vec);
	TeQuaternion::deserialize(stream, rot);
	model->setRotation(rot);
	pickmesh->setRotation(rot);
	TeVector3f32::deserialize(stream, vec);
	model->setScale(vec);
	pickmesh->setScale(vec);
	uint32 indexcount = stream.readUint32LE();
	uint32 vertexcount = stream.readUint32LE();

	if (indexcount > 100000 || vertexcount > 100000)
		error("InGameScene::deserializeModel: Unxpected counts %d %d", indexcount, vertexcount);

	mesh->setConf(vertexcount, indexcount, TeMesh::MeshMode_Triangles, 0, 0);
	for (uint i = 0; i < indexcount; i++)
		mesh->setIndex(i, stream.readUint32LE());

	for (uint i = 0; i < vertexcount; i++) {
		TeVector3f32::deserialize(stream, vec);
		mesh->setVertex(i, vec);
	}
	for (uint i = 0; i < vertexcount; i++) {
		TeVector3f32::deserialize(stream, vec);
		mesh->setNormal(i, vec);
	}
	for (uint i = 0; i < vertexcount; i++) {
		TeVector2f32::deserialize(stream, vec2);
		mesh->setTextureUV(i, vec2);
	}
	for (uint i = 0; i < vertexcount; i++) {
		col.deserialize(stream);
		mesh->setColor(i, col);
	}

	pickmesh->setNbTriangles(indexcount / 3);
	for (uint i = 0; i < indexcount; i++) {
		vec = mesh->vertex(mesh->index(i));
		pickmesh->verticies()[i] = vec;
	}
	model->addMesh(mesh);
}

void InGameScene::draw() {
	TeScene::draw();

	if (currentCameraIndex() >= (int)cameras().size())
		return;

	currentCamera()->apply();

#ifdef TETRAEDGE_DEBUG_PATHFINDING
	if (_character && _character->curve()) {
		_character->curve()->setVisible(true);
		_character->curve()->draw();
	}

	for (TeFreeMoveZone *zone : _freeMoveZones) {
		zone->setVisible(true);
		zone->draw();
	}

	for (TePickMesh2 *mesh : _clickMeshes) {
		mesh->setVisible(true);
		mesh->draw();
	}
#endif

	g_engine->getRenderer()->updateGlobalLight();
	for (uint i = 0; i < _lights.size(); i++)
		_lights[i]->update(i);

	TeCamera::restore();
}

void InGameScene::drawPath() {
	if (currentCameraIndex() >= (int)cameras().size())
		return;

	currentCamera()->apply();
	g_engine->getRenderer()->disableZBuffer();

	for (uint i = 0; i < _freeMoveZones.size(); i++)
		_freeMoveZones[i]->draw();

	g_engine->getRenderer()->enableZBuffer();
}

InGameScene::Dummy InGameScene::dummy(const Common::String &name) {
	for (const Dummy &dummy : _dummies) {
		if (dummy._name == name)
			return dummy;
	}
	return Dummy();
}

bool InGameScene::findKate() {
	for (auto &m : models()) {
		if (m->name() == "Kate")
			return true;
	}
	return false;
}

const InGameScene::TeMarker *InGameScene::findMarker(const Common::String &name) {
	for (const TeMarker &marker : _markers) {
		if (marker._name == name)
			return &marker;
	}
	return nullptr;
}

const InGameScene::TeMarker *InGameScene::findMarkerByInt(const Common::String &val) {
	for (const TeMarker &marker : _markers) {
		if (marker._val == val)
			return &marker;
	}
	return nullptr;
}

InGameScene::SoundStep InGameScene::findSoundStep(const Common::String &name) {
	for (const auto &step : _soundSteps) {
		if (step._key == name)
			return step._value;
	}
	return SoundStep();
}

void InGameScene::freeGeometry() {
	_loadedPath.set("");

	for (TeFreeMoveZone *zone : _freeMoveZones)
		delete zone;
	_freeMoveZones.clear();
	_bezierCurves.clear();
	_dummies.clear();
	cameras().clear();
	_zoneModels.clear();
	if (_charactersShadow) {
		delete _charactersShadow;
		_charactersShadow = nullptr;
	}
}

void InGameScene::freeSceneObjects() {
	if (_character) {
		_character->setCharLookingAt(nullptr);
		_character->deleteAllCallback();
	}
	if (_characters.size() == 1) {
		_characters[0]->deleteAllCallback();
	}

	Game *game = g_engine->getGame();
	game->unloadCharacters();

	_characters.clear();

	for (Object3D *obj : _object3Ds) {
		obj->deleteLater();
	}
	_object3Ds.clear();

	for (Billboard *bb : _billboards) {
		bb->deleteLater();
	}
	_billboards.clear();

	for (TeSpriteLayout *sprite : _sprites) {
		sprite->deleteLater();
	}
	_sprites.clear();

	deleteAllCallback();
	_markers.clear();

	for (InGameScene::AnchorZone *zone : _anchorZones) {
		delete zone;
	}
	_anchorZones.clear();
}

float InGameScene::getHeadHorizontalRotation(Character *cter, const TeVector3f32 &vec) {
	TeVector3f32 pos = vec - cter->_model->position();
	TeVector3f32 zvec = TeVector3f32(0, 0, 1.0f);
	zvec.rotate(cter->_model->rotation());
	float angle = atan2f(-pos.x(), pos.z()) - atan2f(-zvec.x(), zvec.z());
	if (angle < -M_PI)
		angle += (float)(M_PI * 2);
	else if (angle > M_PI)
		angle -= (float)(M_PI * 2);
	return angle;
}

float InGameScene::getHeadVerticalRotation(Character *cter, const TeVector3f32 &vec) {
	TeVector3f32 modelPos = cter->_model->position();
	TeVector3f32 headWorldTrans = cter->_model->worldTransformationMatrix() * cter->lastHeadBoneTrans();
	modelPos.y() = headWorldTrans.y();
	TeVector3f32 offsetPos = vec - modelPos;
	currentCamera()->apply();
	float angle = atan2f(offsetPos.y(), TeVector2f32(offsetPos.x(), offsetPos.z()).length());
	return angle;
}

Common::String InGameScene::imagePathMarker(const Common::String &name) {
	if (!isMarker(name))
		return Common::String();
	Game *game = g_engine->getGame();
	TeLayout *bg = game->forGui().layoutChecked("background");
	for (Te3DObject2 *child : bg->childList()) {
		TeSpriteLayout *spritelayout = dynamic_cast<TeSpriteLayout *>(child);
		if (spritelayout && spritelayout->name() == name) {
			return spritelayout->_tiledSurfacePtr->path().toString();
		}
	}
	return Common::String();
}

void InGameScene::initScroll() {
	_someScrollVector = TeVector2f32(0.5f, 0.0f);
}

bool InGameScene::isMarker(const Common::String &name) {
	for (const TeMarker &marker : _markers) {
		if (marker._name == name)
			return true;
	}
	return false;
}

bool InGameScene::isObjectBlocking(const Common::String &name) {
	for (const Common::String &b: _blockingObjects) {
		if (name == b)
			return true;
	}
	return false;
}

bool InGameScene::load(const Common::Path &path) {
	_actZones.clear();
	Common::File actzonefile;
	if (actzonefile.open(getActZoneFileName())) {
		if (Te3DObject2::loadAndCheckFourCC(actzonefile, "0TCA")) {
			uint32 count = actzonefile.readUint32LE();
			if (count > 1000000)
				error("Improbable number of actzones %d", count);
			_actZones.resize(count);
			for (uint i = 0; i < _actZones.size(); i++) {
				_actZones[i].s1 = Te3DObject2::deserializeString(actzonefile);
				_actZones[i].s2 = Te3DObject2::deserializeString(actzonefile);
				for (int j = 0; j < 4; j++)
					TeVector2f32::deserialize(actzonefile, _actZones[i].points[j]);
				_actZones[i].flag1 = (actzonefile.readByte() != 0);
				_actZones[i].flag2 = true;
			}
		}
	}
	if (!_lights.empty()) {
		g_engine->getRenderer()->disableAllLights();
		for (uint i = 0; i < _lights.size(); i++) {
			_lights[i]->disable(i);
		}
		_lights.clear();
	}
	_shadowLightNo = -1;

	const Common::Path lightspath = getLightsFileName();
	if (Common::File::exists(lightspath))
		loadLights(lightspath);

	if (!Common::File::exists(path))
		return false;

	close();
	_loadedPath = path;
	Common::File scenefile;
	if (!scenefile.open(path))
		return false;

	uint32 ncameras = scenefile.readUint32LE();
	if (ncameras > 1024)
		error("Improbable number of cameras %d", ncameras);
	for (uint i = 0; i < ncameras; i++) {
		TeIntrusivePtr<TeCamera> cam = new TeCamera();
		deserializeCam(scenefile, cam);
		cameras().push_back(cam);
	}

	uint32 nobjects = scenefile.readUint32LE();
	if (nobjects > 1024)
		error("Improbable number of objects %d", nobjects);
	for (uint i = 0; i < nobjects; i++) {
		TeIntrusivePtr<TeModel> model = new TeModel();
		const Common::String modelname = Te3DObject2::deserializeString(scenefile);
		model->setName(modelname);
		const Common::String objname = Te3DObject2::deserializeString(scenefile);
		TePickMesh2 *pickmesh = new TePickMesh2();
		deserializeModel(scenefile, model, pickmesh);
		if (modelname.contains("Clic")) {
			//debug("Loaded clickMesh %s", modelname.c_str());
			_hitObjects.push_back(model);
			model->setVisible(false);
			model->setColor(TeColor(0, 0xff, 0, 0xff));
			models().push_back(model);
			pickmesh->setName(modelname);
			_clickMeshes.push_back(pickmesh);
		} else {
			delete pickmesh;
			if (modelname.substr(0, 2) != "ZB") {
				if (objname.empty()) {
					debug("[InGameScene::load] Unknown type of object named : %s", modelname.c_str());
				} else {
					InGameScene::Object obj;
					obj._name = objname;
					obj._model = model;
					_objects.push_back(obj);
					model->setVisible(false);
					models().push_back(model);
				}
			}
		}
	}

	uint32 nfreemovezones = scenefile.readUint32LE();
	if (nfreemovezones > 1024)
		error("Improbable number of free move zones %d", nfreemovezones);
	for (uint i = 0; i < nfreemovezones; i++) {
		TeFreeMoveZone *zone = new TeFreeMoveZone();
		TeFreeMoveZone::deserialize(scenefile, *zone, &_blockers, &_rectBlockers, &_actZones);
		_freeMoveZones.push_back(zone);
		zone->setVisible(false);
	}

	uint32 ncurves = scenefile.readUint32LE();
	if (ncurves > 1024)
		error("Improbable number of curves %d", ncurves);
	for (uint i = 0; i < ncurves; i++) {
		TeIntrusivePtr<TeBezierCurve> curve = new TeBezierCurve();
		TeBezierCurve::deserialize(scenefile, *curve);
		curve->setVisible(true);
		_bezierCurves.push_back(curve);
	}

	uint32 ndummies = scenefile.readUint32LE();
	if (ndummies > 1024)
		error("Improbable number of dummies %d", ndummies);
	for (uint i = 0; i < ndummies; i++) {
		InGameScene::Dummy dummy;
		TeVector3f32 vec;
		TeQuaternion rot;
		dummy._name = Te3DObject2::deserializeString(scenefile);
		TeVector3f32::deserialize(scenefile, vec);
		dummy._position = vec;
		TeQuaternion::deserialize(scenefile, rot);
		dummy._rotation = rot;
		TeVector3f32::deserialize(scenefile, vec);
		dummy._scale = vec;
		_dummies.push_back(dummy);
	}

	for (TeFreeMoveZone *zone : _freeMoveZones) {
		convertPathToMesh(zone);
	}
	_charactersShadow = CharactersShadow::makeInstance();
	_charactersShadow->create(this);
	onMainWindowSizeChanged();

	return true;
}

bool InGameScene::loadCharacter(const Common::String &name) {
	Character *c = character(name);
	if (!c) {
		c = new Character();
		if (!c->loadModel(name, false)) {
			delete c;
			return false;
		}
		models().push_back(c->_model);
		models().push_back(c->_shadowModel[0]);
		models().push_back(c->_shadowModel[1]);
		_characters.push_back(c);
	}
	c->_model->setVisible(true);
	return true;
}

bool InGameScene::loadLights(const Common::Path &path) {
	SceneLightsXmlParser parser;

	parser.setLightArray(&_lights);

	if (!parser.loadFile(path.toString()))
		error("InGameScene::loadLights: Can't load %s", path.toString().c_str());
	if (!parser.parse())
		error("InGameScene::loadLights: Can't parse %s", path.toString().c_str());

	_shadowColor = parser.getShadowColor();
	_shadowLightNo = parser.getShadowLightNo();
	_shadowFarPlane = parser.getShadowFarPlane();
	_shadowNearPlane = parser.getShadowNearPlane();
	_shadowFov = parser.getShadowFov();

	g_engine->getRenderer()->enableAllLights();
	for (uint i = 0; i < _lights.size(); i++) {
		_lights[i]->enable(i);
	}

#ifdef TETRAEDGE_DEBUG_LIGHTS
	debug("--- Scene lights ---");
	debug("Shadow: %s no:%d far:%.02f near:%.02f fov:%.02f", _shadowColor.dump().c_str(), _shadowLightNo, _shadowFarPlane, _shadowNearPlane, _shadowFov);
	debug("Global: %s", TeLight::globalAmbient().dump().c_str());
	for (uint i = 0; i < _lights.size(); i++) {
		debug("%s", _lights[i].dump().c_str());
	}
	debug("---  end lights  ---");
#endif

	return true;
}

void InGameScene::loadMarkers(const Common::Path &path) {
	_markerGui.load(path);
	TeLayout *bg = _bgGui.layoutChecked("background");
	TeSpriteLayout *root = Game::findSpriteLayoutByName(bg, "root");
	bg->setRatioMode(TeILayout::RATIO_MODE_NONE);
	root->addChild(bg);
}

bool InGameScene::loadObject(const Common::String &name) {
	Object3D *obj = object3D(name);
	if (!obj) {
		obj = new Object3D();
		if (!obj->loadModel(name)) {
			warning("InGameScene::loadObject: Loading %s failed", name.c_str());
			delete obj;
			return false;
		}
		models().push_back(obj->model());
		_object3Ds.push_back(obj);
	}
	obj->model()->setVisible(true);
	return true;
}

bool InGameScene::loadObjectMaterials(const Common::String &name) {
	TeImage img;
	bool retval = false;
	for (auto &obj : _objects) {
		if (obj._name.empty())
			continue;

		Common::Path mpath = _loadedPath.getParent().join(name).join(obj._name + ".png");
		if (img.load(mpath)) {
			Te3DTexture *tex = Te3DTexture::makeInstance();
			tex->load(img);
			obj._model->meshes()[0]->defaultMaterial(tex);
			retval = true;
		}
	}
	return retval;
}

bool InGameScene::loadObjectMaterials(const Common::String &path, const Common::String &name) {
	error("TODO: InGameScene::loadObjectMaterials(%s, %s)", path.c_str(), name.c_str());
}

bool InGameScene::loadPlayerCharacter(const Common::String &name) {
	if (_character == nullptr) {
		_character = new Character();
		if (!_character->loadModel(name, true)) {
			_playerCharacterModel.release();
			return false;
		}

		_playerCharacterModel = _character->_model;

		if (!findKate()) {
			models().push_back(_character->_model);
			models().push_back(_character->_shadowModel[0]);
			models().push_back(_character->_shadowModel[1]);
		}
	}

	_character->_model->setVisible(true);
	return true;
}

static Common::Path _sceneFileNameBase() {
	Game *game = g_engine->getGame();
	Common::Path retval("scenes");
	retval.joinInPlace(game->currentZone());
	retval.joinInPlace(game->currentScene());
	return retval;
}

Common::Path InGameScene::getLightsFileName() const {
	return _sceneFileNameBase().joinInPlace("lights.xml");
}

Common::Path InGameScene::getActZoneFileName() const {
	return _sceneFileNameBase().joinInPlace("actions.bin");
}

Common::Path InGameScene::getBlockersFileName() const {
	return _sceneFileNameBase().joinInPlace("blockers.bin");
}

void InGameScene::loadBlockers() {
	_blockers.clear();
	_rectBlockers.clear();
	const Common::Path blockersPath = getBlockersFileName();
	if (!Common::File::exists(blockersPath))
		return;

	Common::File blockersfile;
	if (!blockersfile.open(blockersPath)) {
		warning("Couldn't open blockers file %s.", blockersPath.toString().c_str());
		return;
	}

	bool hasHeader = Te3DObject2::loadAndCheckFourCC(blockersfile, "BLK0");
	if (!hasHeader)
		blockersfile.seek(0);

	uint32 nblockers = blockersfile.readUint32LE();
	if (nblockers > 1024)
		error("Improbable number of blockers %d", nblockers);
	_blockers.resize(nblockers);
	for (uint i = 0; i < nblockers; i++) {
		_blockers[i]._s = Te3DObject2::deserializeString(blockersfile);
		TeVector2f32::deserialize(blockersfile, _blockers[i]._pts[0]);
		TeVector2f32::deserialize(blockersfile, _blockers[i]._pts[1]);
		_blockers[i]._enabled = true;
	}

	if (hasHeader) {
		uint32 nrectblockers = blockersfile.readUint32LE();
		if (nrectblockers > 1024)
			error("Improbable number of rectblockers %d", nrectblockers);
		_rectBlockers.resize(nrectblockers);
		for (uint i = 0; i < nrectblockers; i++) {
			_rectBlockers[i]._s = Te3DObject2::deserializeString(blockersfile);
			for (uint j = 0; j < 4l; j++) {
				TeVector2f32::deserialize(blockersfile, _rectBlockers[i]._pts[j]);
			}
			_rectBlockers[i]._enabled = true;
		}
	}
}

void InGameScene::loadBackground(const Common::Path &path) {
	_bgGui.load(path);
	TeLayout *bg = _bgGui.layout("background");
	TeLayout *root = _bgGui.layout("root");
	bg->setRatioMode(TeILayout::RATIO_MODE_NONE);
	root->setRatioMode(TeILayout::RATIO_MODE_NONE);
	TeCamera *wincam = g_engine->getApplication()->mainWindowCamera();
	bg->disableAutoZ();
	bg->setZPosition(wincam->orthoNearPlane());

	for (auto layoutEntry : _bgGui.spriteLayouts()) {
		AnimObject *animobj = new AnimObject();
		animobj->_name = layoutEntry._key;
		animobj->_layout = layoutEntry._value;
		animobj->_layout->_tiledSurfacePtr->_frameAnim.onFinished().add(animobj, &AnimObject::onFinished);
		if (animobj->_name != "root")
			animobj->_layout->setVisible(false);
		_animObjects.push_back(animobj);
	}
}

bool InGameScene::loadBillboard(const Common::String &name) {
	Billboard *b = billboard(name);
	if (b)
		return true;
	b = new Billboard();
	if (b->load(name)) {
		_billboards.push_back(b);
		return true;
	} else {
		delete b;
		return false;
	}
}

void InGameScene::loadInteractions(const Common::Path &path) {
	_hitObjectGui.load(path);
	TeLayout *bgbackground = _bgGui.layoutChecked("background");
	Game *game = g_engine->getGame();
	TeSpriteLayout *root = game->findSpriteLayoutByName(bgbackground, "root");
	TeLayout *background = _hitObjectGui.layoutChecked("background");
	for (auto *child : background->childList()) {
		TeButtonLayout *btn = dynamic_cast<TeButtonLayout *>(child);
		if (btn)
			btn->setDoubleValidationProtectionEnabled(false);
	}
	background->setRatioMode(TeILayout::RATIO_MODE_NONE);
	root->addChild(background);
}

void InGameScene::moveCharacterTo(const Common::String &charName, const Common::String &curveName, float curveOffset, float curveEnd) {
	Character *c = character(charName);
	if (c != nullptr && c != _character) {
		Game *game = g_engine->getGame();
		if (!game->_movePlayerCharacterDisabled) {
			c->setCurveStartLocation(c->characterSettings()._cutSceneCurveDemiPosition);
			TeIntrusivePtr<TeBezierCurve> crve = curve(curveName);
			if (!curveName.empty() && !crve)
				warning("moveCharacterTo: curve %s not found", curveName.c_str());
			c->placeOnCurve(crve);
			c->setCurveOffset(curveOffset);
			const Common::String walkStartAnim = c->walkAnim(Character::WalkPart_Start);
			if (walkStartAnim.empty()) {
				c->setAnimation(c->walkAnim(Character::WalkPart_Loop), true);
			} else {
				c->setAnimation(c->walkAnim(Character::WalkPart_Start), false);
			}
			c->walkTo(curveEnd, false);
		}
	}
}

void InGameScene::onMainWindowSizeChanged() {
	TeCamera *mainWinCam = g_engine->getApplication()->mainWindowCamera();
	_viewportSize = mainWinCam->viewportSize();
	Common::Array<TeIntrusivePtr<TeCamera>> &cams = cameras();
	for (uint i = 0; i < cams.size(); i++) {
		cams[i]->viewport(0, 0, _viewportSize.getX(), _viewportSize.getY());
	}
}

Object3D *InGameScene::object3D(const Common::String &name) {
	for (Object3D *obj : _object3Ds) {
		if (obj->model()->name() == name)
			return obj;
	}
	return nullptr;
}

TeFreeMoveZone *InGameScene::pathZone(const Common::String &name) {
	for (TeFreeMoveZone *zone: _freeMoveZones) {
		if (zone->name() == name)
			return zone;
	}
	return nullptr;
}

void InGameScene::reset() {
	if (_character)
		_character->setFreeMoveZone(nullptr);
	freeSceneObjects();
	_bgGui.unload();
	unloadSpriteLayouts();
	_markerGui.unload();
	_hitObjectGui.unload();
}

TeLight *InGameScene::shadowLight() {
	if (_shadowLightNo == -1) {
		return nullptr;
	}
	return _lights[_shadowLightNo].get();
}

void InGameScene::setImagePathMarker(const Common::String &markerName, const Common::String &path) {
	if (!isMarker(markerName))
		return;

	Game *game = g_engine->getGame();
	TeLayout *bg = game->forGui().layoutChecked("background");

	for (Te3DObject2 *child : bg->childList()) {
		if (child->name() == markerName) {
			TeSpriteLayout *sprite = dynamic_cast<TeSpriteLayout *>(child);
			if (sprite) {
				sprite->load(path);
				sprite->_tiledSurfacePtr->_frameAnim.setLoopCount(-1);
				sprite->play();
			}
		}
	}
}

void InGameScene::setPositionCharacter(const Common::String &charName, const Common::String &freeMoveZoneName, const TeVector3f32 &position) {
	Character *c = character(charName);
	if (!c) {
		warning("[SetCharacterPosition] Character not found %s", charName.c_str());
	} else if (c == _character && c->positionFlag()) {
		c->setFreeMoveZoneName(freeMoveZoneName);
		c->setPositionCharacter(position);
		c->setPositionFlag(false);
		c->setNeedsSomeUpdate(true);
	} else {
		c->stop();
		TeFreeMoveZone *zone = pathZone(freeMoveZoneName);
		if (!zone) {
			warning("[SetCharacterPosition] PathZone not found %s", freeMoveZoneName.c_str());
			for (TeFreeMoveZone *z : _freeMoveZones)
				warning("zone: %s", z->name().c_str());
			return;
		}
		TeIntrusivePtr<TeCamera> cam = currentCamera();
		zone->setCamera(cam, false);
		c->setFreeMoveZone(zone);
		SoundStep step = findSoundStep(freeMoveZoneName);
		c->setStepSound(step._stepSound1, step._stepSound2);
		bool correctFlag = true;
		const TeVector3f32 corrected = zone->correctCharacterPosition(position, &correctFlag, true);
		c->_model->setPosition(corrected);
		if (!correctFlag)
			warning("[SetCharacterPosition] Warning : The character is not above the ground %s", charName.c_str());
	}
}

void InGameScene::setStep(const Common::String &scene, const Common::String &step1, const Common::String &step2) {
	SoundStep ss;
	ss._stepSound1 = step1;
	ss._stepSound2 = step2;
	_soundSteps[scene] = ss;
}

void InGameScene::setVisibleMarker(const Common::String &markerName, bool val) {
	if (!isMarker(markerName))
		return;

	Game *game = g_engine->getGame();
	TeLayout *bg = game->forGui().layout("background");
	if (!bg)
		return;

	for (Te3DObject2 *child : bg->childList()) {
		if (child->name() == markerName) {
			child->setVisible(val);
			break;
		}
	}
}

void InGameScene::unloadCharacter(const Common::String &name) {
	if (_character && _character->_model->name() == name) {
		_character->removeAnim();
		_character->deleteAnim();
		_character->deleteAllCallback();
		if (_character->_model->anim())
		_character->_model->anim()->stop(); // TODO: added this
		_character->setFreeMoveZone(nullptr); // TODO: added this
		// TODO: deleteLater() something here..
		_character = nullptr;
	}
	for (uint i = 0; i < _characters.size(); i++) {
		Character *c = _characters[i];
		if (c && c->_model->name() == name) {
			c->removeAnim();
			c->deleteAnim();
			c->deleteAllCallback();
			// TODO: deleteLater() something here..
			if (c->_model->anim())
				c->_model->anim()->stop(); // TODO: added this
			c->setFreeMoveZone(nullptr); // TODO: added this
			_characters.remove_at(i);
			break;
		}
	}
}

void InGameScene::unloadObject(const Common::String &name) {
	for (uint i = 0; i < _object3Ds.size(); i++) {
		if (_object3Ds[i]->model()->name() == name) {
			// Remove from the scene models.
			for (uint j = 0; j < models().size(); j++) {
				if (models()[j] == _object3Ds[i]->model())	{
					models().remove_at(j);
					break;
				}
			}
			_object3Ds[i]->deleteLater();
			_object3Ds.remove_at(i);
			break;
		}
	}
}

void InGameScene::unloadSpriteLayouts() {
	for (auto *animobj : _animObjects) {
		delete animobj;
	}
	_animObjects.clear();
}

void InGameScene::update() {
	Game *game = g_engine->getGame();
	if (_bgGui.loaded()) {
		_bgGui.layoutChecked("background")->setZPosition(0.0f);
	}
	if (_character) {
		_character->setHasAnchor(false);
		for (AnchorZone *zone : _anchorZones) {
			if (aroundAnchorZone(zone)) {
				TeVector2f32 headRot(getHeadHorizontalRotation(_character, zone->_loc),
					getHeadVerticalRotation(_character, zone->_loc));
				if (headRot.getX() * 180.0 / M_PI > 90.0 || headRot.getY() * 180.0 / M_PI > 45.0) {
					_character->setHasAnchor(false);
					_character->setLastHeadRotation(_character->headRotation());
				} else {
					_character->setHeadRotation(headRot);
					_character->setHasAnchor(true);
				}
			}
		}
		if (_character->charLookingAt()) {
			TeVector3f32 targetpos = _character->charLookingAt()->_model->position();
			if (_character->lookingAtTallThing())
				targetpos.y() += 17;
			TeVector2f32 headRot(getHeadHorizontalRotation(_character, targetpos),
					getHeadVerticalRotation(_character, targetpos));
			float hangle = headRot.getX() * 180.0f / M_PI;
			if (hangle > 90.0f)
				headRot.setX((float)M_PI_2);
			else if (hangle < -90.0f)
				headRot.setX((float)-M_PI_2);
			_character->setHeadRotation(headRot);
			_character->setHasAnchor(true);
		}
	}
	for (Character *c : _characters) {
		if (c->charLookingAt()) {
			TeVector3f32 targetpos = c->charLookingAt()->_model->position();
			if (c->lookingAtTallThing())
				targetpos.y() += 17;
			TeVector2f32 headRot(getHeadHorizontalRotation(c, targetpos),
					getHeadVerticalRotation(c, targetpos));
			float hangle = headRot.getX() * 180.0f / M_PI;
			if (hangle > 90)
				headRot.setX((float)M_PI_2);
			else if (hangle < -90)
				headRot.setX((float)-M_PI_2);
			c->setHeadRotation(headRot);
			c->setHasAnchor(true);
		}
	}

	TeLuaGUI::StringMap<TeSpriteLayout *> &sprites = bgGui().spriteLayouts();
	for (auto &sprite : sprites) {
		if (_callbacks.contains(sprite._key)) {
			error("TODO: handle sprite callback in InGameScene::update");
		}
	}

	TeScene::update();

	float waitTime = _waitTimeTimer.timeFromLastTimeElapsed();
	if (_waitTime != -1.0 && waitTime > _waitTime) {
		_waitTime = -1.0;
		_waitTimeTimer.stop();
		bool resumed = false;
		for (uint i = 0; i < game->yieldedCallbacks().size(); i++) {
			Game::YieldedCallback &yc = game->yieldedCallbacks()[i];
			if (yc._luaFnName == "OnWaitFinished") {
				TeLuaThread *thread = yc._luaThread;
				game->yieldedCallbacks().remove_at(i);
				thread->resume();
				resumed = true;
			}
		}
		if (!resumed)
			game->luaScript().execute("OnWaitFinished");
	}

	for (Object3D *obj : _object3Ds) {
		// TODO: update object3ds if they are translating or rotating.
		if (obj->_translateTime >= 0) {
			float time = MIN((float)(obj->_translateTimer.getTimeFromStart() / 1000000.0), obj->_translateTime);
			TeVector3f32 trans = obj->_translateStart + (obj->_translateAmount * (time / obj->_translateTime));
			obj->model()->setPosition(trans);
		}
		if (obj->_rotateTime >= 0) {
			// Never actually used in the game.
			error("TODO: handle _rotateTime > 0 in InGameScene::update");
		}
	}
}


bool InGameScene::AnimObject::onFinished() {
	Game *game = g_engine->getGame();
	for (uint i = 0; i < game->yieldedCallbacks().size(); i++) {
		Game::YieldedCallback &yc = game->yieldedCallbacks()[i];
		if (yc._luaFnName == "OnFinishedAnim" && yc._luaParam == _name) {
			TeLuaThread *thread = yc._luaThread;
			game->yieldedCallbacks().remove_at(i);
			if (thread) {
				thread->resume();
				return false;
			}
			break;
		}
	}
	game->luaScript().execute("OnFinishedAnim", _name);
	return false;
}

} // end namespace Tetraedge
