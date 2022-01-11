#include "cut.h"

//using namespace Slic3r;

void cut(
  float *positions,
  unsigned int numPositions,
  unsigned int *faces,
  unsigned int numFaces,
  float *position,
  float *quaternion,
  float *scale,
  float *outPositions,
  unsigned int *numOutPositions,
  unsigned int *outFaces,
  unsigned int *numOutFaces
) {

  {
    // bak
    float positionBaks[72]; // todo: do not hard code, fix problem in app / physx.js / w.doCut.
    for(int i=0;i<numPositions;i++){
      positionBaks[i] = positions[i];
    }
    float faceBaks[36]; // todo: do not hard code, fix problem in app / physx.js / w.doCut.
    for(int i=0;i<numFaces;i++){
      faceBaks[i] = faces[i];
    }

    // position
    numOutPositions[0] = numPositions;
    for(int i=0;i<numPositions;i++){
      outPositions[i] = positionBaks[i];
    }

    // face
    numOutFaces[0] = numFaces;
    for(int i=0;i<numFaces;i++){
      outFaces[i] = faceBaks[i];
    }
  }
  {
    numOutPositions[1] = 0;
    numOutFaces[1] = 0;
  }

  /* csgjs_plane plane;
  plane.normal = csgjs_vector(0, 1, 0);
  plane.w = 0.5;

  csgjs_polygon polygon;

  std::vector<csgjs_polygon> coplanarFront;
  std::vector<csgjs_polygon> coplanarBack;
  std::vector<csgjs_polygon> front;
  std::vector<csgjs_polygon> back;
  plane.splitPolygon(polygon, coplanarFront, coplanarBack, front, back); */
}