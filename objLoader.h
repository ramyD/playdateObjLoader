#ifndef objLoader_h
#define objLoader_h

#include "pd_api.h"
#include "3dmath.h"
#include "scene.h"

typedef struct {
	int v1;
	int v2;
	int v3;
} Triangle;

typedef struct {
	int vCount;
	int tCount;
	Point3D* vectors;
	Triangle* triangles;
} ObjModel;

Shape3D* MakeShapeFromObj (PlaydateAPI* playdate, const char* filename);

#endif /* objLoader_h */