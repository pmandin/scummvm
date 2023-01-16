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

#include "tetraedge/tetraedge.h"

#include "tetraedge/te/te_free_move_zone.h"
#include "tetraedge/te/micropather.h"
#include "tetraedge/te/te_renderer.h"
#include "tetraedge/te/te_ray_intersection.h"

namespace Tetraedge {

/*static*/
//TeIntrusivePtr<TeCamera> TeFreeMoveZone::_globalCamera;

class TeFreeMoveZoneGraph : micropather::Graph {
	friend class TeFreeMoveZone;
	TeVector2s32 _size;
	Common::Array<char> _flags;
	float _bordersDistance;
	TeFreeMoveZone *_owner;

	// These don't match ScummVM naming convention but are needed to match MicroPather API.
	virtual float LeastCostEstimate(void * stateStart, void *stateEnd);
	virtual void AdjacentCost(void *state, Common::Array<micropather::StateCost> *adjacent);
	virtual void PrintStateInfo(void *state);

	int flag(const TeVector2s32 &loc);
	void setSize(const TeVector2s32 &size);

	void deserialize(Common::ReadStream &stream);
	void serialize(Common::WriteStream &stream) const;

	float costForPoint(TeVector2s32 pt) {
		int flg = flag(pt);
		if (flg == 1)
			return FLT_MAX;
		if (flg == 2)
			return _bordersDistance;
		return 1.0;
	}
};


TeFreeMoveZone::TeFreeMoveZone() : _actzones(nullptr), _blockers(nullptr), _rectBlockers(nullptr),
_transformedVerticiesDirty(true), _bordersDirty(true), _pickMeshDirty(true), _projectedPointsDirty(true),
_loadedFromBin(false), _gridWorldY(0.0), _gridOffsetSomething(5.0f, 5.0f), _gridDirty(true)
{
	_graph = new TeFreeMoveZoneGraph();
	_graph->_bordersDistance = 2048.0f;
	_graph->_owner = this;
	_micropather = new micropather::MicroPather(_graph);
}

TeFreeMoveZone::~TeFreeMoveZone() {
	if (_camera) {
		_camera->onViewportChangedSignal().remove(this, &TeFreeMoveZone::onViewportChanged);
	}
	delete _micropather;
}

float TeFreeMoveZone::bordersDistance() const {
	return _graph->_bordersDistance;
}

TeVector2s32 TeFreeMoveZone::aStarResolution() const {
	TeVector2f32 diff = (_someGridVec2 - _someGridVec1);
	TeVector2s32 retval = TeVector2s32(diff / _gridOffsetSomething) + TeVector2s32(1, 1);
	if (retval._x > 2000)
		retval._x = 200;
	if (retval._y > 2000)
		retval._y = 200;
	return retval;
}

void TeFreeMoveZone::buildAStar() {
	preUpdateGrid();
	const TeVector2s32 graphSize = aStarResolution();
	_graph->setSize(graphSize);

	// Original checks these inside the loop below, seems like a waste as they never change?
	if (graphSize._x == 0 || graphSize._y == 0)
		return;

	if (!_loadedFromBin) {
		for (int x = 0; x < graphSize._x; x++) {
			for (int y = 0; y < graphSize._y; y++) {
				byte blockerIntersection = hasBlockerIntersection(TeVector2s32(x, y));
				if (blockerIntersection == 1) {
					_graph->_flags[_graph->_size._x * y + x] = 1;
				} else {
					if (!hasCellBorderIntersection(TeVector2s32(x, y))) {
						const float gridOffX = _gridOffsetSomething.getX();
						const float gridOffY = _gridOffsetSomething.getY();
						TeVector3f32 vout;
						float fout;
						TeVector3f32 gridPt(x * gridOffX + _someGridVec1.getX() + gridOffX * 0.5,
										1000000.0,
										gridOffY * 0.5 + y * gridOffY + _someGridVec1.getY());
						bool doesIntersect = intersect(gridPt, TeVector3f32(0.0, -1.0, 0.0), vout, fout, true, nullptr);
						if (!doesIntersect)
							doesIntersect = intersect(gridPt, TeVector3f32(0.0, 1.0, 0.0), vout, fout, true, nullptr);

						if (!doesIntersect)
							_graph->_flags[graphSize._x * y + x] = 1;
						else if (blockerIntersection == 2)
							_graph->_flags[graphSize._x * y + x] = 2;
						else
							_graph->_flags[graphSize._x * y + x] = 0;
					} else {
					_graph->_flags[graphSize._x * y + x] = 2;
					}
				}
			}
		}
	} else {
		// Loaded from bin..
		error("TODO: Implement TeFreeMoveZone::buildAStar for loaded from bin case");
	}
}

void TeFreeMoveZone::calcGridMatrix() {
	error("TODO: Implement TeFreeMoveZone::calcGridMatrix");
}

void TeFreeMoveZone::clear() {
	setNbTriangles(0);
	_pickMeshDirty = true;
	_projectedPointsDirty = true;
	_transformedVerticies.clear();
	_borders.clear();
	// TODO: Clear some other TeVector2f32 list here (field_0x178)
	_gridDirty = true;
	_graph->_flags.clear();
	_graph->_size = TeVector2s32(0, 0);
	_micropather->Reset();
}

Common::Array<TeVector3f32> TeFreeMoveZone::collisions(const TeVector3f32 &v1, const TeVector3f32 &v2) {
	updatePickMesh();
	updateProjectedPoints();
	error("TODO: Implement TeFreeMoveZone::collisions");
}

TeVector3f32 TeFreeMoveZone::correctCharacterPosition(const TeVector3f32 &pos, bool *flagout, bool intersectFlag) {
	float f = 0.0;
	TeVector3f32 intersectPoint;
	TeVector3f32 testPos(pos.x(), 0, pos.z());
	if (!intersect(testPos, TeVector3f32(0, -1, 0), intersectPoint, f, intersectFlag, nullptr)) {
		if (!intersect(testPos, TeVector3f32(0, 1, 0), intersectPoint, f, intersectFlag, nullptr)) {
			if (*flagout)
				*flagout = false;
			return pos;
		}
	}
	if (flagout)
		*flagout = true;
	return intersectPoint;
}

TeIntrusivePtr<TeBezierCurve> TeFreeMoveZone::curve(const TeVector3f32 &startpt, const TeVector2s32 &clickPt, float param_5, bool findMeshFlag) {
	updateGrid(false);
	Common::Array<TePickMesh2 *> meshes;
	TeVector3f32 newend;
	meshes.push_back(this);

	TePickMesh2 *nearest = findNearestMesh(_camera, clickPt, meshes, &newend, findMeshFlag);
	if (!nearest)
		return TeIntrusivePtr<TeBezierCurve>();

	return curve(startpt, newend);
}

TeIntrusivePtr<TeBezierCurve> TeFreeMoveZone::curve(const TeVector3f32 &startpt, const TeVector3f32 &endpt) {
	updateGrid(false);
	const TeVector2s32 projectedStart = projectOnAStarGrid(startpt);
	const TeVector2s32 projectedEnd = projectOnAStarGrid(endpt);
	const int xsize = _graph->_size._x;
	float cost = 0;
	// Passing an int to void*, yuck? but it's what the original does..
	Common::Array<void *> path;
	int pathResult = _micropather->Solve((void *)(xsize * projectedStart._y + projectedStart._x), (void *)(xsize * projectedEnd._y + projectedEnd._x), &path, &cost);

	TeIntrusivePtr<TeBezierCurve> retval;

	if (pathResult == micropather::MicroPather::SOLVED || pathResult == micropather::MicroPather::START_END_SAME) {
		Common::Array<TeVector2s32> points;
		points.resize(path.size() + 2);

		int i = 1;
		for (auto pathpt : path) {
			// each path point is an array offset
			int offset = static_cast<int>(reinterpret_cast<size_t>(pathpt));
			points[i] = TeVector2s32(offset % xsize, offset / xsize);
			i++;
		}

		Common::Array<TeVector3f32> pts3d;
		for (auto &pt : points) {
			pts3d.push_back(transformAStarGridInWorldSpace(pt));
		}

		pts3d.front() = startpt;
		pts3d.back() = endpt;
		removeInsignificantPoints(pts3d);
		retval = new TeBezierCurve();
		retval->setControlPoints(pts3d);
	} else {
		Common::Array<TeVector3f32> points;
		points.push_back(startpt);
		points.push_back(endpt);
		retval = new TeBezierCurve();
		retval->setControlPoints(points);
	}

	return retval;
}

/*static*/
void TeFreeMoveZone::deserialize(Common::ReadStream &stream, TeFreeMoveZone &dest, Common::Array<TeBlocker> *blockers,
			Common::Array<TeRectBlocker> *rectblockers, Common::Array<TeActZone> *actzones) {
	dest.clear();
	TePickMesh2::deserialize(stream, dest);
	TeVector2f32::deserialize(stream, dest._gridOffsetSomething);
	dest._transformedVerticiesDirty = (stream.readByte() != 0);
	dest._bordersDirty = (stream.readByte() != 0);
	dest._pickMeshDirty = (stream.readByte() != 0);
	dest._projectedPointsDirty = (stream.readByte() != 0);
	dest._gridDirty = (stream.readByte() != 0);

	Te3DObject2::deserializeVectorArray(stream, dest._freeMoveZoneVerticies);
	Te3DObject2::deserializeUintArray(stream, dest._pickMesh);
	Te3DObject2::deserializeVectorArray(stream, dest._transformedVerticies);
	Te3DObject2::deserializeUintArray(stream, dest._borders);

	TeOBP::deserialize(stream, dest._obp);

	TeVector2f32::deserialize(stream, dest._someGridVec1);
	TeVector2f32::deserialize(stream, dest._someGridVec2);
	dest._gridWorldY = stream.readFloatLE();

	dest._graph->deserialize(stream);
	if (dest.name().contains("19000")) {
		dest._gridOffsetSomething = TeVector2f32(2.0, 2.0);
		dest._gridDirty = true;
	}
	dest._blockers = blockers;
	dest._rectBlockers = rectblockers;
	dest._actzones = actzones;
}

void TeFreeMoveZone::draw() {
	if (!worldVisible())
		return;

	TeRenderer *renderer = g_engine->getRenderer();
	renderer->enableWireFrame();
	TePickMesh2::draw();
	Common::SharedPtr<TeMesh> mesh(TeMesh::makeInstance());
	mesh->setConf(_borders.size(), _borders.size(), TeMesh::MeshMode_Lines, 0, 0);
	for (uint i = 0; i < _borders.size(); i++) {
		mesh->setIndex(i, i);
		mesh->setVertex(i, verticies()[_borders[i]]);
	}

	const TeColor prevColor = renderer->currentColor();
	renderer->pushMatrix();
	renderer->multiplyMatrix(worldTransformationMatrix());
	renderer->setCurrentColor(TeColor(0, 0x80, 0xff, 0xff));
	mesh->draw();
	renderer->popMatrix();
	renderer->setCurrentColor(prevColor);

	// TODO: do a bunch of other drawing stuff here.

	renderer->disableWireFrame();
}

TeVector3f32 TeFreeMoveZone::findNearestPointOnBorder(const TeVector2f32 &pt) {
	error("TODO: Implement TeFreeMoveZone::findNearestPointOnBorder");
}

static int segmentIntersection(const TeVector2f32 &s1start, const TeVector2f32 &s1end,
						const TeVector2f32 &s2start, const TeVector2f32 &s2end,
						TeVector2f32 *sout, float *fout1, float *fout2) {
	TeVector2f32 s1len = s1end - s1start;
	TeVector2f32 s2len = s2end - s2start;
	float squarelen = s1len.getX() * s2len.getX() + s1len.getY() * s2len.getY();
	int result = 0;
	if (squarelen != 0) {
		result = 1;
		float intersection1 = -((s1len.getY() * s1start.getX() +
						(s1len.getX() * s2start.getY() - s1len.getX() * s1start.getY())) -
						 s1len.getY() * s2start.getX()) / squarelen;
		if (intersection1 >= 0.0f && intersection1 <= 1.0f) {
			float intersection2 = -((s2len.getY() * s2start.getY() +
						(s2len.getX() * s1start.getX() - s2len.getX() * s2start.getX())) -
						 s2len.getY() * s1start.getY()) / squarelen;
			if (intersection2 >= 0.0f && intersection2 <= 1.0f) {
				result = 2;
				if (sout || fout1 || fout2) {
					// Seems like these are always null?
					error("TODO: implement output in segmentIntersection");
				}
			}
		}
	}
	return result;
}


byte TeFreeMoveZone::hasBlockerIntersection(const TeVector2s32 &pt) {
	TeVector2f32 borders[4];

	const float gridOffsetX = _gridOffsetSomething.getX();
	const float gridOffsetY = _gridOffsetSomething.getX();
	borders[0] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX(),
							  pt._y * gridOffsetY + _someGridVec1.getY());
	borders[1] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX() + gridOffsetX,
							  pt._y * gridOffsetY + _someGridVec1.getY());
	borders[2] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX(),
							  pt._y * gridOffsetY + _someGridVec1.getY() + gridOffsetY);
	borders[3] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX() + gridOffsetX,
							  pt._y * gridOffsetY + _someGridVec1.getY() + gridOffsetY);

	for (uint i = 0; i < _blockers->size(); i++) {
		const TeBlocker &blocker = (*_blockers)[i];

		if (blocker._s != name())
			continue;

		for (uint b = 0; b < 4; b++) {
			int si = segmentIntersection(borders[b], borders[(b + 1) % 4], blocker._pts[0],
										 blocker._pts[1], nullptr, nullptr, nullptr);
			if (si == 2)
				return 2;
		}

		TeVector2f32 borderVec = ((borders[0] + borders[3]) / 2.0) - blocker._pts[0];
		TeVector2f32 blockerVec = blocker._pts[1] - blocker._pts[0];
		float dotVal = borderVec.dotProduct(blockerVec.getNormalized());
		float crosVal = borderVec.crossProduct(blockerVec);
		if ((crosVal < 0.0) && (0.0 <= dotVal)) {
			if (dotVal < blockerVec.length())
				return 1;
		}
	}
	return 0;
}

bool TeFreeMoveZone::hasCellBorderIntersection(const TeVector2s32 &pt) {
	TeVector2f32 borders[4];

	const float gridOffsetX = _gridOffsetSomething.getX();
	const float gridOffsetY = _gridOffsetSomething.getX();
	borders[0] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX(),
							  pt._y * gridOffsetY + _someGridVec1.getY());
	borders[1] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX() + gridOffsetX,
							  pt._y * gridOffsetY + _someGridVec1.getY());
	borders[2] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX(),
							  pt._y * gridOffsetY + _someGridVec1.getY() + gridOffsetY);
	borders[3] = TeVector2f32(pt._x * gridOffsetX + _someGridVec1.getX() + gridOffsetX,
							  pt._y * gridOffsetY + _someGridVec1.getY() + gridOffsetY);

	int iresult = 0;
	for (uint border = 0; border < _borders.size() / 2; border++) {
		TeVector2f32 v1;
		TeVector2f32 v2;
		uint off1 = _pickMesh[_borders[border * 2]];
		uint off2 = _pickMesh[_borders[border * 2 + 1]];
		if (!_loadedFromBin) {
			v1 = TeVector2f32(_transformedVerticies[off1].x(), _transformedVerticies[off1].z());
			v2 = TeVector2f32(_transformedVerticies[off2].x(), _transformedVerticies[off2].z());
		} else {
			TeMatrix4x4 gridInverse = _gridMatrix;
			gridInverse.inverse();
			const TeVector3f32 v1_inv = gridInverse * _freeMoveZoneVerticies[off1];
			const TeVector3f32 v2_inv = gridInverse * _freeMoveZoneVerticies[off2];
			v1 = TeVector2f32(v1_inv.x(), v1_inv.z());
			v2 = TeVector2f32(v2_inv.x(), v2_inv.z());
		}
		iresult = segmentIntersection(borders[0], borders[1], v1, v2, nullptr, nullptr, nullptr);
		if (iresult == 2) break;
		iresult = segmentIntersection(borders[1], borders[2], v1, v2, nullptr, nullptr, nullptr);
		if (iresult == 2) break;
		iresult = segmentIntersection(borders[2], borders[3], v1, v2, nullptr, nullptr, nullptr);
		if (iresult == 2) break;
		iresult = segmentIntersection(borders[3], borders[0], v1, v2, nullptr, nullptr, nullptr);
		if (iresult == 2) break;
	}
	return iresult == 2;
}

TeActZone *TeFreeMoveZone::isInZone(const TeVector3f32 &pt) {
	error("TODO: Implement TeFreeMoveZone::isInZone");
}

bool TeFreeMoveZone::onViewportChanged() {
	_projectedPointsDirty = true;
	return false;
}

void TeFreeMoveZone::preUpdateGrid() {
	updateTransformedVertices();
	updatePickMesh();
	updateBorders();
	if (_loadedFromBin) {
		calcGridMatrix();
	}

	TeMatrix4x4 gridInverse = _gridMatrix;
	gridInverse.inverse();

	TeVector3f32 newVec;
	if (_transformedVerticies.empty() || _pickMesh.empty()) {
		debug("[TeFreeMoveZone::buildAStar] %s have no mesh or is entierly occluded", name().c_str());
	} else {
		if (!_loadedFromBin) {
			newVec = _transformedVerticies[_pickMesh[0]];
		} else {
			newVec = gridInverse * _freeMoveZoneVerticies[_pickMesh[0]];
		}
		_someGridVec1.setX(newVec.x());
		_someGridVec1.setY(newVec.z());

		_gridWorldY = newVec.y();
	}
	for (uint i = 0; i < _pickMesh.size(); i++) {
		uint vertNo = _pickMesh[_pickMesh[i]];

		if (!_loadedFromBin)
			newVec = _transformedVerticies[vertNo];
		else
			newVec = gridInverse * _freeMoveZoneVerticies[vertNo];

		if (_someGridVec1.getX() <= newVec.x()) {
			if (_someGridVec2.getX() < newVec.x())
				_someGridVec2.setX(newVec.x());
		} else {
			_someGridVec1.setX(newVec.x());
		}

		if (_someGridVec1.getY() <= newVec.z()) {
			if (_someGridVec2.getY() < newVec.z())
				_someGridVec2.setY(newVec.z());
		} else {
			_someGridVec1.setY(newVec.z());
		}

		if (newVec.y() < _gridWorldY)
			_gridWorldY = newVec.y();
	}

	if (!_loadedFromBin) {
		if (!name().contains("19000"))
			_gridOffsetSomething = TeVector2f32(5.0f, 5.0f);
		else
			_gridOffsetSomething = TeVector2f32(2.0f, 2.0f);
	} else {
		const TeVector2f32 gridVecDiff = _someGridVec2 - _someGridVec1;
		float minSide = MIN(gridVecDiff.getX(), gridVecDiff.getY()) / 20.0f;
		_gridOffsetSomething.setX(minSide);
		_gridOffsetSomething.setY(minSide);

		error("FIXME: Finish preUpdateGrid for loaded-from-bin case.");
		/*
		// what's this field?
		if (_field_0x414.x != 0.0)
			_gridOffsetSomething = _field_0x414;
		*/
	}

	TeMatrix4x4 worldTrans = worldTransformationMatrix();
	worldTrans.inverse();
	_inverseWorldTransform = worldTrans;
}

TeVector2s32 TeFreeMoveZone::projectOnAStarGrid(const TeVector3f32 &pt) {
	TeVector2f32 otherpt;
	if (!_loadedFromBin) {
		otherpt = TeVector2f32(pt.x() - _someGridVec1.getX(), pt.z() - _someGridVec1.getY());
	} else {
		error("TODO: Implement TeFreeMoveZone::projectOnAStarGrid for _loadedFromBin");
	}
	TeVector2f32 projected = otherpt / _gridOffsetSomething;
	return TeVector2s32((int)projected.getX(), (int)projected.getY());
}

Common::Array<TeVector3f32> TeFreeMoveZone::removeInsignificantPoints(const Common::Array<TeVector3f32> &points) {
	if (points.size() < 2)
		return points;

	Common::Array<TeVector3f32> result;
	result.push_back(points[0]);

	if (points.size() > 2) {
		int point1 = 0;
		int point2 = 2;
		do {
			const TeVector2f32 pt1(points[point1].x(), points[point1].z());
			const TeVector2f32 pt2(points[point2].x(), points[point2].z());
			for (uint i = 0; i * 2 < _borders.size() / 2; i++) {
				const TeVector3f32 transpt3d1 = worldTransformationMatrix() * verticies()[_borders[i * 2]];
				const TeVector2f32 transpt1(transpt3d1.x(), transpt3d1.z());
				const TeVector3f32 transpt3d2 = worldTransformationMatrix() * verticies()[_borders[i * 2 + 1]];
				const TeVector2f32 transpt2(transpt3d2.x(), transpt3d2.z());
				if (segmentIntersection(pt1, pt2, transpt1, transpt2, nullptr, nullptr, nullptr) == 2)
					break;
			}
			point1 = point2 - 1;
			result.push_back(points[point1]);
			point2++;
		} while (point2 < (int)points.size());
	}

	if (result.back() != points[points.size() - 2]) {
		result.push_back(points[points.size() - 1]);
	} else {
		result.back() = points[points.size() - 1];
	}
	return result;
}

void TeFreeMoveZone::setBordersDistance(float dist) {
	_graph->_bordersDistance = dist;
}

void TeFreeMoveZone::setCamera(TeIntrusivePtr<TeCamera> &cam, bool noRecalcProjPoints) {
	if (_camera) {
		_camera->onViewportChangedSignal().remove(this, &TeFreeMoveZone::onViewportChanged);
	}
	//_globalCamera = camera;  // Seems like this is never used?
	_camera = cam;
	cam->onViewportChangedSignal().add(this, &TeFreeMoveZone::onViewportChanged);
	if (!noRecalcProjPoints)
		_projectedPointsDirty = true;
}

void TeFreeMoveZone::setNbTriangles(uint len) {
	_freeMoveZoneVerticies.resize(len * 3);

	_gridDirty = true;
	_transformedVerticiesDirty = true;
	_bordersDirty = true;
	_pickMeshDirty = true;
	_projectedPointsDirty = true;
}

void TeFreeMoveZone::setPathFindingOccluder(const TeOBP &occluder) {
	_obp = occluder;
	_projectedPointsDirty = true;
	_bordersDirty = true;
	_gridDirty = true;
}

void TeFreeMoveZone::setVertex(uint offset, const TeVector3f32 &vertex) {
	_freeMoveZoneVerticies[offset] = vertex;

	_gridDirty = true;
	_transformedVerticiesDirty = true;
	_bordersDirty = true;
	_pickMeshDirty = true;
	_projectedPointsDirty = true;
}

TeVector3f32 TeFreeMoveZone::transformAStarGridInWorldSpace(const TeVector2s32 &gridpt) {
	float offsety = (float)gridpt._y * _gridOffsetSomething.getY() + _someGridVec1.getY() +
				_gridOffsetSomething.getY() * 0.5;
	float offsetx = (float)gridpt._x * _gridOffsetSomething.getX() + _someGridVec1.getX() +
				_gridOffsetSomething.getX() * 0.5;
	if (!_loadedFromBin) {
		return TeVector3f32(offsetx, _gridWorldY, offsety);
	} else {
		TeVector3f32 result = _gridMatrix * TeVector3f32(offsetx, _gridWorldY, offsety);
		return worldTransformationMatrix() * result;
	}
}

float TeFreeMoveZone::transformHeightMin(float minval) {
	TeVector3f32 vec = worldTransformationMatrix() * TeVector3f32(_someGridVec1.getX(), minval, _someGridVec1.getY());
	return vec.y();
}

TeVector3f32 TeFreeMoveZone::transformVectorInWorldSpace(float x, float y) {
	TeVector3f32 vec = _gridMatrix * TeVector3f32(x, _gridWorldY, y);
	return worldTransformationMatrix() * vec;
}

void TeFreeMoveZone::updateBorders() {
	if (!_bordersDirty)
		return;

	updatePickMesh();

	if (_verticies.size() > 2) {
		for (uint triNo1 = 0; triNo1 < _verticies.size() / 3; triNo1++) {
			for (uint vecNo1 = 0; vecNo1 < 3; vecNo1++) {
				uint left1 = triNo1 * 3 + vecNo1;
				uint left2 = triNo1 * 3 + (vecNo1 == 2 ? 0 : vecNo1 + 1);
				const TeVector3f32 vleft1 = _verticies[left1];
				const TeVector3f32 vleft2 = _verticies[left2];

				bool skip = false;
				for (uint triNo2 = 0; triNo2 < _verticies.size() / 3; triNo2++) {
					if (skip)
						break;

					for (uint vecNo2 = 0; vecNo2 < 3; vecNo2++) {
						if (triNo2 == triNo1)
							continue;
						uint right1 = triNo2 * 3 + vecNo2;
						uint right2 = triNo2 * 3 + (vecNo2 == 2 ? 0 : vecNo2 + 1);
						TeVector3f32 vright1 = _verticies[right1];
						TeVector3f32 vright2 = _verticies[right2];
						if (vright1 == vleft1 && vright2 == vleft2 && vright1 == vleft2 && vright2 == vleft1) {
							skip = true;
							break;
						}
					}
				}
				if (!skip) {
					_borders.push_back(left1);
					_borders.push_back(left2);
				}
			}
		}
	}
	_bordersDirty = false;
}

void TeFreeMoveZone::updateGrid(bool force) {
	if (!force && !_gridDirty)
		return;
	_gridDirty = true;
	_updateTimer.stop();
	_updateTimer.start();
	buildAStar();
	_micropather->Reset();
	debug("[TeFreeMoveZone::updateGrid()] %s time : %.2f", name().c_str(), _updateTimer.getTimeFromStart() / 1000000.0);
	_gridDirty = false;
}

void TeFreeMoveZone::updatePickMesh() {
	if (!_pickMeshDirty)
		return;

	updateTransformedVertices();
	_pickMesh.clear();
	_pickMesh.reserve(_freeMoveZoneVerticies.size());
	int vecNo = 0;
	for (uint tri = 0; tri < _freeMoveZoneVerticies.size() / 3; tri++) {
		_pickMesh.push_back(vecNo);
		_pickMesh.push_back(vecNo + 1);
		_pickMesh.push_back(vecNo + 2);
		vecNo += 3;
}

	debug("[TeFreeMoveZone::updatePickMesh] %s nb triangles reduced from : %d to : %d", name().c_str(),
			 _freeMoveZoneVerticies.size() / 3, _pickMesh.size() / 3);

	TePickMesh2::setNbTriangles(_pickMesh.size() / 3);

	for (uint i = 0; i < _pickMesh.size(); i++) {
		_verticies[i] = _freeMoveZoneVerticies[_pickMesh[i]];
	}
	_bordersDirty = true;
	_pickMeshDirty = false;
	_projectedPointsDirty = true;
	_gridDirty = true;
}

void TeFreeMoveZone::updateProjectedPoints() {
	if (!_projectedPointsDirty)
		return;

	error("TODO: Implement TeFreeMoveZone::updateProjectedPoints");
}

void TeFreeMoveZone::updateTransformedVertices() {
	if (!_transformedVerticiesDirty)
		return;

	const TeMatrix4x4 worldTransform = worldTransformationMatrix();
	_transformedVerticies.resize(_freeMoveZoneVerticies.size());
	for (uint i = 0; i < _transformedVerticies.size(); i++) {
		_transformedVerticies[i] = worldTransform * _freeMoveZoneVerticies[i];
	}
	_transformedVerticiesDirty = false;
}

/*========*/

float TeFreeMoveZoneGraph::LeastCostEstimate(void *stateStart, void *stateEnd) {
	int startInt = static_cast<int>(reinterpret_cast<size_t>(stateStart));
	int endInt = static_cast<int>(reinterpret_cast<size_t>(stateEnd));
	int starty = startInt / _size._x;
	int endy = endInt / _size._x;
	TeVector2s32 start(startInt - starty * _size._x, starty);
	TeVector2s32 end(endInt - endy * _size._x, endy);
	return (end - start).squaredLength();
}

void TeFreeMoveZoneGraph::AdjacentCost(void *state, Common::Array<micropather::StateCost> *adjacent) {
	int stateInt = static_cast<int>(reinterpret_cast<size_t>(state));
	int stateY = stateInt / _size._x;
	const TeVector2s32 statept(stateInt - stateY * _size._x, stateY);

	micropather::StateCost cost;
	TeVector2s32 pt;

	pt = TeVector2s32(statept._x - 1, statept._y);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);

	pt = TeVector2s32(statept._x - 1, statept._y + 1);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);

	pt = TeVector2s32(statept._x, statept._y + 1);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);

	pt = TeVector2s32(statept._x + 1, statept._y + 1);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);

	pt = TeVector2s32(statept._x + 1, statept._y);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);

	pt = TeVector2s32(statept._x + 1, statept._y - 1);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);

	pt = TeVector2s32(statept._x, statept._y - 1);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);

	pt = TeVector2s32(statept._x - 1, statept._y - 1);
	cost.state = reinterpret_cast<void *>(_size._x * pt._y + pt._x);
	cost.cost = costForPoint(pt);
	adjacent->push_back(cost);
}

void TeFreeMoveZoneGraph::PrintStateInfo(void *state) {
	error("TODO: Implement TeFreeMoveZone::TeFreeMoveZoneGraph::PrintStateInfo");
}

int TeFreeMoveZoneGraph::flag(const TeVector2s32 &loc) {
	if (loc._x < 0 || loc._x >= _size._x || loc._y < 0 || loc._y >= _size._y)
		return 1;
	return _flags[loc._y * _size._x + loc._x];
}

void TeFreeMoveZoneGraph::setSize(const TeVector2s32 &size) {
	_flags.clear();
	_size = size;
	_flags.resize(size._x * _size._y);
}

void TeFreeMoveZoneGraph::deserialize(Common::ReadStream &stream) {
	TeVector2s32::deserialize(stream, _size);
	uint32 flaglen = stream.readUint32LE();
	if (flaglen > 1000000 || (int)flaglen != _size._x * _size._y)
		error("TeFreeMoveZoneGraph: Flags unexpected size, expect %d got %d", _size._x * _size._y, flaglen);
	_flags.resize(flaglen);
	for (uint i = 0; i < flaglen; i++) {
		_flags[i] = stream.readByte();
	}
	_bordersDistance = stream.readFloatLE();
}

void TeFreeMoveZoneGraph::serialize(Common::WriteStream &stream) const {
	error("TODO: Implement TeFreeMoveZoneGraph::serialize");
}

/*static*/
TePickMesh2 *TeFreeMoveZone::findNearestMesh(TeIntrusivePtr<TeCamera> &camera, const TeVector2s32 &fromPt,
			Common::Array<TePickMesh2*> &pickMeshes, TeVector3f32 *outloc, bool lastHitFirst) {
	TeVector3f32 closestLoc;
	TePickMesh2 *nearestMesh = nullptr;
	if (!camera)
		return nullptr;
	float closestDist = camera->orthoFarPlane();
	Math::Ray camRay;
	for (uint i = 0; i < pickMeshes.size(); i++) {
		TePickMesh2 *mesh = pickMeshes[i];
		const TeMatrix4x4 meshWorldTransform = mesh->worldTransformationMatrix();
		if (lastHitFirst) {
			// Note: it seems like a bug in the original.. this never sets
			// the ray parameters?? It should still find the right triangle below.
			uint tricount = mesh->verticies().size() / 3;
			uint vert = mesh->lastTriangleHit() * 3;
			if (mesh->lastTriangleHit() >= tricount)
				vert = 0;
			const TeVector3f32 v1 = meshWorldTransform * mesh->verticies()[vert + 0];
			const TeVector3f32 v2 = meshWorldTransform * mesh->verticies()[vert + 1];
			const TeVector3f32 v3 = meshWorldTransform * mesh->verticies()[vert + 2];
			TeVector3f32 intersectLoc;
			float intersectDist;
			bool intResult = camRay.intersectTriangle(v1, v2, v3, intersectLoc, intersectDist);
			if (intResult && intersectDist < closestDist && intersectDist >= camera->orthoNearPlane())
				return mesh;
		}
		for (uint tri = 0; tri < mesh->verticies().size() / 3; tri++) {
			const TeVector3f32 v1 = meshWorldTransform * mesh->verticies()[tri * 3 + 0];
			const TeVector3f32 v2 = meshWorldTransform * mesh->verticies()[tri * 3 + 1];
			const TeVector3f32 v3 = meshWorldTransform * mesh->verticies()[tri * 3 + 2];
			camRay = camera->getRay(fromPt);
			TeVector3f32 intersectLoc;
			float intersectDist;
			bool intResult = camRay.intersectTriangle(v1, v2, v3, intersectLoc, intersectDist);
			/*debug("PickMesh2 %s intersect Ray(%s, %s) Triangle(%s, %s, %s) -> %s", mesh->name().c_str(),
					TeVector3f32(camRay.getOrigin()).dump().c_str(),
					TeVector3f32(camRay.getDirection()).dump().c_str(),
					v1.dump().c_str(), v2.dump().c_str(), v3.dump().c_str(),
					intResult ? "hit!" : "no hit");*/
			if (intResult && intersectDist < closestDist && intersectDist >= camera->orthoNearPlane()) {
				mesh->setLastTriangleHit(tri);
				closestLoc = intersectLoc;
				closestDist = intersectDist;
				nearestMesh = mesh;
				if (lastHitFirst)
					break;
			}
		}
	}
	if (outloc) {
		*outloc = closestLoc;
	}
	return nearestMesh;
}


} // end namespace Tetraedge
