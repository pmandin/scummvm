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

#ifndef TETRAEDGE_GAME_IN_GAME_SCENE_H
#define TETRAEDGE_GAME_IN_GAME_SCENE_H

#include "common/array.h"
#include "common/str.h"
#include "common/hashmap.h"

#include "tetraedge/game/object3d.h"
#include "tetraedge/game/billboard.h"

#include "tetraedge/te/te_act_zone.h"
#include "tetraedge/te/te_bezier_curve.h"
#include "tetraedge/te/te_free_move_zone.h"
#include "tetraedge/te/te_scene.h"
#include "tetraedge/te/te_light.h"
#include "tetraedge/te/te_lua_gui.h"
#include "tetraedge/te/te_pick_mesh2.h"

namespace Tetraedge {

class Character;
class CharactersShadow;
class TeLayout;

class InGameScene : public TeScene {
public:
	InGameScene();

	struct AnimObject {
		bool onFinished();

		Common::String _name;
		TeSpriteLayout *_layout;
	};

	struct Callback {
		float _f;
		Common::String _name;
	};

	struct SoundStep {
		Common::String _stepSound1;
		Common::String _stepSound2;
	};

	struct AnchorZone {
		Common::String _name;
		bool _activated;
		TeVector3f32 _loc;
		float _radius;
	};

	struct Object {
		TeIntrusivePtr<TeModel> _model;
		Common::String _name;
	};

	struct TeMarker {
		Common::String _name;
		Common::String _val;
	};

	struct Dummy {
		Common::String _name;
		TeVector3f32 _position;
		TeQuaternion _rotation;
		TeVector3f32 _scale;
	};

	void activateAnchorZone(const Common::String &name, bool val);
	void addAnchorZone(const Common::String &s1, const Common::String &name, float radius);
	void addBlockingObject(const Common::String &obj) {
		_blockingObjects.push_back(obj);
	}
	void addCallbackAnimation2D(const Common::String &param_1, const Common::String &param_2, float param_3);
	bool addMarker(const Common::String &name, const Common::String &imgPath, float x, float y, const Common::String &locType, const Common::String &markerVal);
	static float angularDistance(float a1, float a2);
	bool aroundAnchorZone(const AnchorZone *zone);
	TeLayout *background();
	Billboard *billboard(const Common::String &name);
	TeVector2f32 boundLayerSize();
	bool changeBackground(const Common::String &name);
	Character *character(const Common::String &name);
	virtual void close() override;
	// Original has a typo, "converPathToMesh", corrected.
	void convertPathToMesh(TeFreeMoveZone *zone);
	TeIntrusivePtr<TeBezierCurve> curve(const Common::String &curveName);
	void deleteAllCallback();
	void deleteCallback(const Common::String &key, const Common::String &name, float f);
	void deleteMarker(const Common::String &markerName);
	// Original just calls these "deserialize" but that's a fairly vague name
	// so renamed to be more meaningful.
	void deserializeCam(Common::ReadStream &stream, TeIntrusivePtr<TeCamera> &cam);
	void deserializeModel(Common::ReadStream &stream, TeIntrusivePtr<TeModel> &model, TePickMesh2 *pickmesh);
	virtual void draw() override;
	void drawPath();
	Dummy dummy(const Common::String &name);
	bool findKate();
	const TeMarker *findMarker(const Common::String &name);
	const TeMarker *findMarkerByInt(const Common::String &name);
	SoundStep findSoundStep(const Common::String &name);
	void freeGeometry();
	void freeSceneObjects();
	Common::Path getActZoneFileName() const;
	Common::Path getBlockersFileName() const;
	Common::Path getLightsFileName() const;
	float getHeadHorizontalRotation(Character *cter, const TeVector3f32 &vec);
	float getHeadVerticalRotation(Character *cter, const TeVector3f32 &vec);
	Common::String imagePathMarker(const Common::String &name);
	void initScroll();
	bool isMarker(const Common::String &name);
	bool isObjectBlocking(const Common::String &name);
	bool isVisibleMarker(const Common::String &name);
	TeVector2f32 layerSize();

	virtual bool load(const Common::Path &path) override;
	void loadBackground(const Common::Path &path);
	bool loadBillboard(const Common::String &name);
	void loadBlockers();
	bool loadCharacter(const Common::String &name);
	void loadInteractions(const Common::Path &path);
	bool loadLights(const Common::Path &path);
	void loadMarkers(const Common::Path &path);
	bool loadObject(const Common::String &oname);
	bool loadObjectMaterials(const Common::String &name);
	bool loadObjectMaterials(const Common::String &path, const Common::String &name);
	bool loadPlayerCharacter(const Common::String &cname);

	void moveCharacterTo(const Common::String &charName, const Common::String &curveName, float curveOffset, float curveEnd);
	int object(const Common::String &oname);
	Object3D *object3D(const Common::String &oname);
	void onMainWindowSizeChanged();
	TeFreeMoveZone *pathZone(const Common::String &zname);
	TeVector3f32 positionMarker(const Common::String &mname);
	void removeBlockingObject(const Common::String &oname);

	void reset();
	void setImagePathMarker(const Common::String &markerName, const Common::String &path);
	void setPositionCharacter(const Common::String &charName, const Common::String &freeMoveZoneName, const TeVector3f32 &position);
	void setPositionMarker(const Common::String &name, const TeVector3f32 &vec);
	void setStep(const Common::String &scene, const Common::String &step1, const Common::String &step2);
	void setVisibleMarker(const Common::String &markerName, bool val);
	TeLight *shadowLight();
	bool showAllObjects(const Common::String &name);
	void unloadAllObjects();
	void unloadCharacter(const Common::String &name);
	void unloadObject(const Common::String &name);
	void unloadSpriteLayouts();
	void update() override;

	// Does nothing, but to keep calls from original..
	static void updateScroll() {};
	static void updateViewport() {};

	Character *_character;
	Common::Array<Character *> _characters;

	TeLuaGUI &bgGui() { return _bgGui; }
	TeLuaGUI &hitObjectGui() { return _hitObjectGui; }
	TeLuaGUI &markerGui() { return _markerGui; }

	Common::Array<TePickMesh2 *> &clickMeshes() { return _clickMeshes; }

	float shadowFarPlane() const { return _shadowFarPlane; }
	float shadowNearPlane() const { return _shadowNearPlane; }
	float shadowFov() const { return _shadowFov; }
	const TeColor &shadowColor() const { return _shadowColor; }
	int shadowLightNo() const { return _shadowLightNo; }
	CharactersShadow *charactersShadow() { return _charactersShadow; }

	TeIntrusivePtr<TeBezierCurve> curve() { return _curve; }
	void setCurve(TeIntrusivePtr<TeBezierCurve> &c) { _curve = c; }
	Common::Array<TeIntrusivePtr<TeModel>> &zoneModels() { return _zoneModels; }
	Common::Array<TeRectBlocker> &rectBlockers() { return _rectBlockers; }
	Common::Array<TeBlocker> &blockers() { return _blockers; }
	Common::Array<Object3D *> object3Ds() { return _object3Ds; }
	void setWaitTime(float usecs) { _waitTime = usecs; }
	TeTimer &waitTimeTimer() { return _waitTimeTimer; }
	Common::Array<Common::SharedPtr<TeLight>> &lights() { return _lights; }


private:
	int _shadowLightNo;
	CharactersShadow *_charactersShadow;
	TeColor _shadowColor;
	float _shadowFarPlane;
	float _shadowNearPlane;
	float _shadowFov;

	float _waitTime;
	TeTimer _waitTimeTimer;

	Common::Array<TeBlocker> _blockers;
	Common::Array<TeRectBlocker> _rectBlockers;
	Common::Array<TeActZone> _actZones;
	Common::Array<TeFreeMoveZone*> _freeMoveZones;
	Common::Array<TeMarker> _markers;
	Common::Array<AnchorZone *> _anchorZones;
	Common::Array<AnimObject *> _animObjects;
	Common::Array<Object3D *> _object3Ds;
	Common::Array<Billboard *> _billboards;
	Common::Array<TeSpriteLayout *> _sprites;
	Common::Array<TePickMesh2 *> _clickMeshes;

	Common::HashMap<Common::String, SoundStep> _soundSteps;
	Common::HashMap<Common::String, Common::Array<Callback*>> _callbacks;

	Common::Array<TeIntrusivePtr<TeModel>> _hitObjects;
	Common::Array<Object> _objects;
	Common::Array<TeIntrusivePtr<TeBezierCurve>> _bezierCurves;
	Common::Array<Dummy> _dummies;
	Common::Array<TeIntrusivePtr<TeModel>> _zoneModels;

	TeIntrusivePtr<TeModel> _playerCharacterModel;
	TeIntrusivePtr<TeBezierCurve> _curve;
	Common::Array<Common::String> _blockingObjects;
	TeLuaGUI _bgGui;
	TeLuaGUI _markerGui;
	TeLuaGUI _hitObjectGui;

	Common::Array<Common::SharedPtr<TeLight>> _lights;

	TeVector2f32 _someScrollVector;
	TeVector2f32 _viewportSize;

	Common::Path _loadedPath;
};

} // end namespace Tetraedge

#endif // TETRAEDGE_GAME_IN_GAME_SCENE_H
