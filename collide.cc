// #include "DirectXMath.h"
/* #include "PhysX/physx/include/geometry/PxMeshQuery.h"
#include "geometry/PxCapsuleGeometry.h"
#include "foundation/PxTransform.h"
 */
#include "collide.h"
#include "subparcel.h"

using namespace physx;

// const float subparcelRadius = std::sqrt(((float)SUBPARCEL_SIZE/2.0f)*((float)SUBPARCEL_SIZE/2.0f)*3.0f);

Physicer::Physicer() {
  gAllocator = new PxDefaultAllocator();
  gErrorCallback = new PxDefaultErrorCallback();
  gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *gAllocator, *gErrorCallback);
  PxTolerancesScale tolerancesScale;
  physics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, tolerancesScale);
  PxCookingParams cookingParams(tolerancesScale);
  // cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
  cooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, cookingParams);
}

PhysicsGeometry::PhysicsGeometry(unsigned int meshId, PxTriangleMesh *triangleMesh, PxGeometry *meshGeom, Vec position, Quat quaternion, Sphere boundingSphere) :
  meshId(meshId), triangleMesh(triangleMesh), meshGeom(meshGeom), position(position), quaternion(quaternion), boundingSphere(boundingSphere) {}
PhysicsGeometry::~PhysicsGeometry() {
  delete meshGeom;
  if (triangleMesh) {
    triangleMesh->release();
  }
}

/* void doInitPhysx() {
  gAllocator = new PxDefaultAllocator();
  gErrorCallback = new PxDefaultErrorCallback();
  gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *gAllocator, *gErrorCallback);
  PxTolerancesScale tolerancesScale;
  physics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, tolerancesScale);
  PxCookingParams cookingParams(tolerancesScale);
  // cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
  cooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, cookingParams);
} */

std::shared_ptr<PhysicsGeometry> doMakeBakedGeometry(Physicer *physicer, unsigned int meshId, PxDefaultMemoryOutputStream *writeStream, float *meshPosition, float *meshQuaternion) {
  PxDefaultMemoryInputData readBuffer((PxU8 *)writeStream->getData(), writeStream->getSize());
  PxTriangleMesh *triangleMesh = physicer->physics->createTriangleMesh(readBuffer);
  delete writeStream;

  PxTriangleMeshGeometry *meshGeom = new PxTriangleMeshGeometry(triangleMesh);
  Sphere boundingSphere(meshPosition[0], meshPosition[1], meshPosition[2], slabRadius);
  std::shared_ptr<PhysicsGeometry> geometrySpec(new PhysicsGeometry(meshId, triangleMesh, meshGeom, Vec(), Quat(), boundingSphere));

  {
    std::lock_guard<std::mutex> lock(physicer->gPhysicsMutex);
    physicer->geometrySpecs.insert(geometrySpec);
  }

  return std::move(geometrySpec);
}

std::shared_ptr<PhysicsGeometry> doMakeBoxGeometry(Physicer *physicer, unsigned int meshId, float *position, float *quaternion, float w, float h, float d) {
  Vec p(position[0], position[1], position[2]);
  Quat q(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
  Vec halfScale(w/2.0f, h/2.0f, d/2.0f);
  PxBoxGeometry *meshGeom = new PxBoxGeometry(halfScale.x, halfScale.y, halfScale.z);
  Sphere boundingSphere(0, 0, 0, halfScale.magnitude());
  std::shared_ptr<PhysicsGeometry> geometrySpec(new PhysicsGeometry(meshId, nullptr, meshGeom, p, q, boundingSphere));

  {
    std::lock_guard<std::mutex> lock(physicer->gPhysicsMutex);
    physicer->staticGeometrySpecs.insert(geometrySpec);
  }

 return std::move(geometrySpec);
}

std::shared_ptr<PhysicsGeometry> doMakeCapsuleGeometry(Physicer *physicer, unsigned int meshId, float *position, float *quaternion, float radius, float halfHeight) {
  Vec p(position[0], position[1], position[2]);
  Quat q(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
  PxCapsuleGeometry *meshGeom = new PxCapsuleGeometry(radius, halfHeight);
  Sphere boundingSphere(0, 0, 0, radius + halfHeight);
  std::shared_ptr<PhysicsGeometry> geometrySpec(new PhysicsGeometry(meshId, nullptr, meshGeom, p, q, boundingSphere));

  {
    std::lock_guard<std::mutex> lock(physicer->gPhysicsMutex);
    physicer->staticGeometrySpecs.insert(geometrySpec);
  }

  return std::move(geometrySpec);
}

PxDefaultMemoryOutputStream *doBakeGeometry(Physicer *physicer, float *positions, unsigned int *indices, unsigned int numPositions, unsigned int numIndices) {
  PxVec3 *verts = (PxVec3 *)positions;
  PxU32 nbVerts = numPositions/3;
  PxU32 *indices32 = (PxU32 *)indices;
  PxU32 triCount = numIndices/3;

  PxTriangleMeshDesc meshDesc;
  meshDesc.points.count           = nbVerts;
  meshDesc.points.stride          = sizeof(PxVec3);
  meshDesc.points.data            = verts;

  meshDesc.triangles.count        = triCount;
  meshDesc.triangles.stride       = 3*sizeof(PxU32);
  meshDesc.triangles.data         = indices32;

  PxDefaultMemoryOutputStream *writeBuffer = new PxDefaultMemoryOutputStream();
  bool status = physicer->cooking->cookTriangleMesh(meshDesc, *writeBuffer);
  if (status) {
    return writeBuffer;
  } else {
    delete writeBuffer;
    return nullptr;
  }
}

/* void doUnregisterGeometry(PhysicsGeometry * geometrySpec) {
  {
    std::lock_guard<std::mutex> lock(gPhysicsMutex);
    for (std::set<PhysicsGeometry *> *geometrySpecSet : geometrySpecSets) {
      geometrySpecSet->erase(geometrySpec);
    }
  }

  delete geometrySpec;
} */

void doRaycast(Physicer *physicer, float *origin, float *direction, float *meshPosition, float *meshQuaternion, unsigned int &hit, float *position, float *normal, float &distance, unsigned int &meshId, unsigned int &faceIndex) {
  std::set<std::shared_ptr<PhysicsGeometry>> &geometrySpecs = physicer->geometrySpecs;
  std::vector<std::set<std::shared_ptr<PhysicsGeometry>> *> &geometrySpecSets = physicer->geometrySpecSets;
  PxVec3 originVec{origin[0], origin[1], origin[2]};
  PxVec3 directionVec{direction[0], direction[1], direction[2]};
  Ray ray(Vec{origin[0], origin[1], origin[2]}, Vec{direction[0], direction[1], direction[2]});
  PxTransform meshPose(
    PxVec3{meshPosition[0], meshPosition[1], meshPosition[2]},
    PxQuat{meshQuaternion[0], meshQuaternion[1], meshQuaternion[2], meshQuaternion[3]}
  );
  Vec p(meshPosition[0], meshPosition[1], meshPosition[2]);
  Quat q(meshQuaternion[0], meshQuaternion[1], meshQuaternion[2], meshQuaternion[3]);

  PxRaycastHit hitInfo;
  constexpr float maxDist = 1000.0;
  const PxHitFlags hitFlags = PxHitFlag::eDEFAULT;
  constexpr PxU32 maxHits = 1;

  // {
    // std::lock_guard<std::mutex> lock(gPhysicsMutex);

    std::vector<std::shared_ptr<PhysicsGeometry>> sortedGeometrySpecs;
    sortedGeometrySpecs.reserve(geometrySpecs.size());
    for (std::set<std::shared_ptr<PhysicsGeometry>> *geometrySpecSet : geometrySpecSets) {
      for (const std::shared_ptr<PhysicsGeometry> &geometrySpec : *geometrySpecSet) {
        Sphere sphere(
          (geometrySpec->boundingSphere.center.clone().applyQuaternion(geometrySpec->quaternion) + geometrySpec->position)
            .applyQuaternion(q) + p,
          geometrySpec->boundingSphere.radius
        );
        if (ray.intersectsSphere(sphere)) {
          sortedGeometrySpecs.push_back(geometrySpec);
        }
      }
    }
    /* std::sort(sortedGeometrySpecs.begin(), sortedGeometrySpecs.end(), [](const std::pair<float, GeometrySpec *> &a, const std::pair<float, GeometrySpec *> &b) -> bool {
      return a.first < b.first;
    }); */

    hit = 0;
    for (const std::shared_ptr<PhysicsGeometry> &geometrySpec : sortedGeometrySpecs) {
      const unsigned int &meshIdData = geometrySpec->meshId;
      PxGeometry *meshGeom = geometrySpec->meshGeom;
      PxTransform meshPose2{
        PxVec3{geometrySpec->position.x, geometrySpec->position.y, geometrySpec->position.z},
        PxQuat{geometrySpec->quaternion.x, geometrySpec->quaternion.y, geometrySpec->quaternion.z, geometrySpec->quaternion.w}
      };
      PxTransform meshPose3 = meshPose * meshPose2;
      PxTransform meshPose4 = meshPose2 * meshPose;

      PxU32 hitCount = PxGeometryQuery::raycast(originVec, directionVec,
                                                *meshGeom,
                                                meshPose3,
                                                maxDist,
                                                hitFlags,
                                                maxHits, &hitInfo);

      if (hitCount > 0 && (!hit || hitInfo.distance < distance)) {
        hit = 1;
        position[0] = hitInfo.position.x;
        position[1] = hitInfo.position.y;
        position[2] = hitInfo.position.z;
        normal[0] = hitInfo.normal.x;
        normal[1] = hitInfo.normal.y;
        normal[2] = hitInfo.normal.z;
        distance = hitInfo.distance;
        meshId = meshIdData;
        faceIndex = hitInfo.faceIndex;
      }
    }
  // }
}

void doCollide(Physicer *physicer, float radius, float halfHeight, float *position, float *quaternion, float *meshPosition, float *meshQuaternion, unsigned int maxIter, unsigned int &hit, float *direction, unsigned int &grounded) {
  std::set<std::shared_ptr<PhysicsGeometry>> &staticGeometrySpecs = physicer->staticGeometrySpecs;
  std::vector<std::set<std::shared_ptr<PhysicsGeometry>> *> &geometrySpecSets = physicer->geometrySpecSets;
  PxCapsuleGeometry geom(radius, halfHeight);
  PxTransform geomPose(
    PxVec3{position[0], position[1], position[2]},
    PxQuat{quaternion[0], quaternion[1], quaternion[2], quaternion[3]}
  );
  PxTransform meshPose{
    PxVec3{meshPosition[0], meshPosition[1], meshPosition[2]},
    PxQuat{meshQuaternion[0], meshQuaternion[1], meshQuaternion[2], meshQuaternion[3]}
  };

  Vec p(meshPosition[0], meshPosition[1], meshPosition[2]);
  Quat q(meshQuaternion[0], meshQuaternion[1], meshQuaternion[2], meshQuaternion[3]);

  // {
    // std::lock_guard<std::mutex> lock(gPhysicsMutex);

    std::vector<std::tuple<bool, float, std::shared_ptr<PhysicsGeometry>>> sortedGeometrySpecs;
    unsigned int totalNumSpecs = 0;
    for (std::set<std::shared_ptr<PhysicsGeometry>> *geometrySpecSet : geometrySpecSets) {
      totalNumSpecs += geometrySpecSet->size();
    }
    sortedGeometrySpecs.reserve(totalNumSpecs);

    Vec offset(0, 0, 0);
    bool anyHadHit = false;
    bool anyHadGrounded = false;
    for (unsigned int i = 0; i < maxIter; i++) {
      Vec capsulePosition(geomPose.p.x, geomPose.p.y, geomPose.p.z);
      sortedGeometrySpecs.clear();

      for (std::set<std::shared_ptr<PhysicsGeometry>> *geometrySpecSet : geometrySpecSets) {
        for (const std::shared_ptr<PhysicsGeometry> &geometrySpec : *geometrySpecSet) {
          Vec spherePosition = (geometrySpec->boundingSphere.center.clone().applyQuaternion(geometrySpec->quaternion) + geometrySpec->position)
            .applyQuaternion(q) + p;
          float distance = spherePosition.distanceTo(capsulePosition);
          if (distance < (geometrySpec->boundingSphere.radius + halfHeight + radius)) {
            sortedGeometrySpecs.push_back(std::tuple<bool, float, std::shared_ptr<PhysicsGeometry>>(geometrySpecSet == &staticGeometrySpecs, distance, geometrySpec));
          }
        }
      }
      std::sort(sortedGeometrySpecs.begin(), sortedGeometrySpecs.end(), [](const std::tuple<bool, float, std::shared_ptr<PhysicsGeometry>> &a, const std::tuple<bool, float, std::shared_ptr<PhysicsGeometry>> &b) -> bool {
        const bool &aStatic = std::get<0>(a);
        const bool &bStatic = std::get<0>(b);
        if (aStatic != bStatic) {
          return aStatic > bStatic;
        } else {
          const float &aDistance = std::get<1>(a);
          const float &bDistance = std::get<1>(b);
          return aDistance < bDistance;
        }
      });

      bool hadHit = false;
      for (const std::tuple<bool, float, std::shared_ptr<PhysicsGeometry>> &t : sortedGeometrySpecs) {
        std::shared_ptr<PhysicsGeometry> geometrySpec = std::get<2>(t);
        PxGeometry *meshGeom = geometrySpec->meshGeom;
        PxTransform meshPose2{
          PxVec3{geometrySpec->position.x, geometrySpec->position.y, geometrySpec->position.z},
          PxQuat{geometrySpec->quaternion.x, geometrySpec->quaternion.y, geometrySpec->quaternion.z, geometrySpec->quaternion.w}
        };
        PxTransform meshPose3 = meshPose * meshPose2;

        PxVec3 directionVec;
        PxReal depthFloat;
        bool result = PxGeometryQuery::computePenetration(directionVec, depthFloat, geom, geomPose, *meshGeom, meshPose3);
        if (result) {
          anyHadHit = true;
          hadHit = true;
          offset += Vec(directionVec.x, directionVec.y, directionVec.z)*depthFloat;
          geomPose.p.x += directionVec.x*depthFloat;
          geomPose.p.y += directionVec.y*depthFloat;
          geomPose.p.z += directionVec.z*depthFloat;
          anyHadGrounded = anyHadGrounded || directionVec.y > 0;
          // break;
        }
      }
      if (hadHit) {
        continue;
      } else {
        break;
      }
    }
    if (anyHadHit) {
      hit = 1;
      direction[0] = offset.x;
      direction[1] = offset.y;
      direction[2] = offset.z;
      grounded = +anyHadGrounded;
    } else {
      hit = 0;
    }
  // }
}
int PEEK_FACE_INDICES[] = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,1,2,3,4,255,255,255,255,255,255,255,255,255,255,0,255,5,6,7,8,255,255,255,255,255,255,255,255,255,255,1,5,255,9,10,11,255,255,255,255,255,255,255,255,255,255,2,6,9,255,12,13,255,255,255,255,255,255,255,255,255,255,3,7,10,12,255,14,255,255,255,255,255,255,255,255,255,255,4,8,11,13,14,255};

/* GroupSet::GroupSet(int x, int y, int z, int index, const Sphere &boundingSphere, unsigned char *peeks, Group *groups, unsigned int numGroups) :
  x(x), y(y), z(z), index(index), boundingSphere(boundingSphere)
{
  memcpy(this->peeks, peeks, 15);
  this->groups.reserve(numGroups);
  for (unsigned int i = 0; i < numGroups; i++) {
    this->groups.push_back(groups[i]);
  }
} */
PeekDirection PEEK_DIRECTIONS[6] = {
  {{0, 0, 1}, {0, 0, 1}, PEEK_FACES::FRONT},
  {{0, 0, -1}, {0, 0, -1}, PEEK_FACES::BACK},
  {{-1, 0, 0}, {-1, 0, 0}, PEEK_FACES::LEFT},
  {{1, 0, 0}, {1, 0, 0}, PEEK_FACES::RIGHT},
  {{0, 1, 0}, {0, 1, 0}, PEEK_FACES::TOP},
  {{0, -1, 0}, {0, -1, 0}, PEEK_FACES::BOTTOM},
};

/* Culler::Culler() {
  groupSets.reserve(512);
}
CullResult::CullResult(unsigned int start, unsigned int count, unsigned int materialIndex) : start(start), count(count), materialIndex(materialIndex) {}
Culler *doMakeCuller() {
  return new Culler();
}
GroupSet *doRegisterGroupSet(Culler *culler, int x, int y, int z, float r, unsigned char *peeks, Group *groups, unsigned int numGroups) {
  GroupSet *groupSet = new GroupSet(
    x,
    y,
    z,
    getSubparcelIndex(x, y, z),
    Sphere(x*SUBPARCEL_SIZE + SUBPARCEL_SIZE/2, y*SUBPARCEL_SIZE + SUBPARCEL_SIZE/2, z*SUBPARCEL_SIZE + SUBPARCEL_SIZE/2, r),
    peeks,
    groups,
    numGroups
  );
  culler->groupSets.push_back(groupSet);
  return groupSet;
}
void doUnregisterGroupSet(Culler *culler, GroupSet *groupSet) {
  auto groupSetIter = std::find(culler->groupSets.begin(), culler->groupSets.end(), groupSet);
  culler->groupSets.erase(groupSetIter);
  delete groupSet;
}
void doCull(Culler *culler, float *positionData, float *matrixData, float slabRadius, CullResult *cullResults, unsigned int &numCullResults) {
  Vec position(positionData[0], positionData[1], positionData[2]);
  Frustum frustum;
  frustum.setFromMatrix(matrixData);
  std::vector<GroupSet *> &groupSets = culler->groupSets;

  // frustum cull
  std::vector<GroupSet *> frustumGroupSets;
  frustumGroupSets.reserve(groupSets.size());
  for (int i = 0; i < groupSets.size(); i++) {
    if (frustum.intersectsSphere(groupSets[i]->boundingSphere)) {
      frustumGroupSets.push_back(groupSets[i]);
    }
  }
  std::sort(frustumGroupSets.begin(), frustumGroupSets.end(), [&](GroupSet *a, GroupSet *b) -> bool {
    return a->boundingSphere.center.distanceTo(position) < b->boundingSphere.center.distanceTo(position);
  });

  // intialize queue
  std::deque<GroupSet *> queue;
  std::set<GroupSet *> seenQueue;
  for (int i = 0; i < frustumGroupSets.size(); i++) {
    GroupSet *groupSet = frustumGroupSets[i];
    if (groupSet->boundingSphere.center.distanceTo(position) < slabRadius*2.0f) {
      queue.push_back(groupSet);
      seenQueue.insert(groupSet);
    }
  }

  // run queue
  numCullResults = 0;
  while (queue.size() > 0) {
    GroupSet *groupSet = queue.front();
    queue.pop_front();

    for (const Group &group : groupSet->groups) {
      CullResult &cullResult = cullResults[numCullResults++];
      cullResult.start = group.start;
      cullResult.count = group.count;
      cullResult.materialIndex = group.materialIndex;
    }

    for (const PeekDirection &enterPeekDirection : PEEK_DIRECTIONS) {
      const Vec &enterNormal = enterPeekDirection.normal;
      const int *enterINormal = enterPeekDirection.inormal;
      const PEEK_FACES &enterFace = enterPeekDirection.face;
      const Vec direction = groupSet->boundingSphere.center
        + (enterNormal * (float)SUBPARCEL_SIZE/2.0f)
        - position;
      if (direction.dot(enterNormal) <= 0) {
        for (const PeekDirection &exitPeekDirection : PEEK_DIRECTIONS) {
          const Vec &exitNormal = exitPeekDirection.normal;
          const PEEK_FACES &exitFace = exitPeekDirection.face;
          const int *exitINormal = exitPeekDirection.inormal;
          const Vec direction = groupSet->boundingSphere.center
            + (exitNormal * (float)SUBPARCEL_SIZE/2.0f)
            - position;
          if (direction.dot(exitNormal) >= 0 && groupSet->peeks[PEEK_FACE_INDICES[(int)enterFace << 4 | (int)exitFace]]) {
            int index = getSubparcelIndex(groupSet->x + exitINormal[0], groupSet->y + exitINormal[1], groupSet->z + exitINormal[2]);
            auto nextGroupSetIter = std::find_if(frustumGroupSets.begin(), frustumGroupSets.end(), [&](GroupSet *groupSet) -> bool {
              return groupSet->index == index;
            });
            if (nextGroupSetIter != frustumGroupSets.end()) {
              GroupSet *nextGroupSet = *nextGroupSetIter;
              if (nextGroupSet != nullptr && seenQueue.find(nextGroupSet) == seenQueue.end()) {
                queue.push_back(nextGroupSet);
                seenQueue.insert(nextGroupSet);
              }
            }
          }
        }
      }
    }
  }
  std::sort(cullResults, cullResults + numCullResults, [&](const CullResult &a, const CullResult &b) -> bool {
    return a.materialIndex < b.materialIndex;
  });
} */
void doTickCull(Tracker *tracker, float *positionData, float *matrixData, CullResult *landCullResults, unsigned int &numLandCullResults, CullResult *vegetationCullResults, unsigned int &numVegetationCullResults) {
  Vec position(positionData[0], positionData[1], positionData[2]);
  Frustum frustum;
  frustum.setFromMatrix(matrixData);

  // frustum cull
  std::vector<std::shared_ptr<Subparcel>> frustumSubparcels;
  frustumSubparcels.reserve(tracker->subparcels.size());
  for (auto &iter : tracker->subparcels) {
    std::shared_ptr<Subparcel> &subparcel = iter.second;
    if (frustum.intersectsSphere(subparcel->boundingSphere)) {
      frustumSubparcels.push_back(subparcel);
    }
  }
  std::sort(frustumSubparcels.begin(), frustumSubparcels.end(), [&](std::shared_ptr<Subparcel> &a, std::shared_ptr<Subparcel> &b) -> bool {
    return a->boundingSphere.center.distanceTo(position) < b->boundingSphere.center.distanceTo(position);
  });

  // intialize queue
  std::deque<std::shared_ptr<Subparcel>> queue;
  std::set<std::shared_ptr<Subparcel>> seenQueue;
  for (int i = 0; i < frustumSubparcels.size(); i++) {
    std::shared_ptr<Subparcel> &subparcel = frustumSubparcels[i];
    if (subparcel->boundingSphere.center.distanceTo(position) < slabRadius*2.0f) {
      queue.push_back(subparcel);
      seenQueue.insert(subparcel);
    }
  }

  // run queue
  numLandCullResults = 0;
  numVegetationCullResults = 0;
  while (queue.size() > 0) {
    std::shared_ptr<Subparcel> &subparcel = queue.front();

    for (const Group &group : subparcel->landGroups) {
      CullResult &cullResult = landCullResults[numLandCullResults++];
      cullResult.start = group.start;
      cullResult.count = group.count;
      cullResult.materialIndex = group.materialIndex;
    }
    for (const Group &group : subparcel->vegetationGroups) {
      CullResult &cullResult = vegetationCullResults[numVegetationCullResults++];
      cullResult.start = group.start;
      cullResult.count = group.count;
      cullResult.materialIndex = group.materialIndex;
    }

    for (const PeekDirection &enterPeekDirection : PEEK_DIRECTIONS) {
      const Vec &enterNormal = enterPeekDirection.normal;
      const int *enterINormal = enterPeekDirection.inormal;
      const PEEK_FACES &enterFace = enterPeekDirection.face;
      const Vec direction = subparcel->boundingSphere.center
        + (enterNormal * (float)SUBPARCEL_SIZE/2.0f)
        - position;
      if (direction.dot(enterNormal) <= 0) {
        for (const PeekDirection &exitPeekDirection : PEEK_DIRECTIONS) {
          const Vec &exitNormal = exitPeekDirection.normal;
          const PEEK_FACES &exitFace = exitPeekDirection.face;
          const int *exitINormal = exitPeekDirection.inormal;
          const Vec direction = subparcel->boundingSphere.center
            + (exitNormal * (float)SUBPARCEL_SIZE/2.0f)
            - position;
          if (direction.dot(exitNormal) >= 0 && subparcel->peeks[PEEK_FACE_INDICES[(int)enterFace << 4 | (int)exitFace]]) {
            int index = getSubparcelIndex(subparcel->coord.x + exitINormal[0], subparcel->coord.y + exitINormal[1], subparcel->coord.z + exitINormal[2]);
            auto nextSubparcelIter = std::find_if(frustumSubparcels.begin(), frustumSubparcels.end(), [&](std::shared_ptr<Subparcel> &subparcel) -> bool {
              return subparcel->coord.index == index;
            });
            if (nextSubparcelIter != frustumSubparcels.end()) {
              std::shared_ptr<Subparcel> nextSubparcel = *nextSubparcelIter;
              if (nextSubparcel != nullptr && seenQueue.find(nextSubparcel) == seenQueue.end()) {
                queue.push_back(nextSubparcel);
                seenQueue.insert(nextSubparcel);
              }
            }
          }
        }
      }
    }

    queue.pop_front();
  }
  std::sort(landCullResults, landCullResults + numLandCullResults, [&](const CullResult &a, const CullResult &b) -> bool {
    return a.materialIndex < b.materialIndex;
  });
}