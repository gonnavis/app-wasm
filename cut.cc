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
    numOutPositions[0] = 0;
    for (size_t i = 0; i < positions.size(); i++) {
      outPositions[numOutPositions[0]] = positions[numOutPositions[0]++];
      outPositions[numOutPositions[0]] = positions[numOutPositions[0]++];
      outPositions[numOutPositions[0]] = positions[numOutPositions[0]++];
    }

    const std::vector<Point3> &upperIndices = upper.facets();
    memcpy(outFaces, faces, faces.size()*3*sizeof(face[0]));
    numOutFaces[0] = faces.size()*3;
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