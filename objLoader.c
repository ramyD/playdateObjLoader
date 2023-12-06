#include <string.h>
#include "objLoader.h"

int GetVectorFromLine (PlaydateAPI* playdate, char* vectorLine, Point3D* point) {
	// set an offset so we start after the "v "
	size_t offset = 2;
	size_t len = strlen(vectorLine);
	float values[3];
	int index = 0;

	char* numbers;
	char* nextSpace;
	char* nextNumber = NULL;
	do {
		// apply the accumulated offset and find the end of this number
		numbers = vectorLine+offset;
		nextSpace = strstr(numbers, " ");

		// if we don't find a space we are probably at the end of the line
		if (nextSpace == NULL) {
			nextSpace = vectorLine + len;
		}
		if (nextSpace != NULL) {
			// isolate the next number
			long long numberLength = nextSpace - numbers;
			nextNumber = playdate->system->realloc(nextNumber, numberLength + sizeof(char));
			if (nextNumber == NULL) continue;
			memcpy(nextNumber, numbers, numberLength);
			nextNumber[numberLength] = '\0';

			offset += numberLength+1;
			//float value = atof(nextNumber);
			float value;
			int result = sscanf_s(nextNumber, "%f", &value);
			if (result != 1) value = 0;
			values[index++] = value;
		}
	} while (nextSpace != NULL && offset < len && index < 3);

	playdate->system->realloc(nextNumber, 0);

	if (index == 3) {
		point->x = values[0];
		point->y = values[1];
		point->z = values[2];
		return 0;
	}

	return -1;
}

int GetTriangleFromLine (PlaydateAPI* playdate, char* faceLine, Triangle* triangle) {
	// set an offset so we start after the "f "
	size_t offset = 2;
	size_t len = strlen(faceLine);
	int values[3];
	int index = 0;

	char* trios;
	char* nextSpace;
	char* nextTrio = NULL;
	char* firstDash;
	do {
		// apply the accumulated offset and find the end of this number
		trios = faceLine + offset;
		nextSpace = strstr(trios, " ");

		// if we don't find a space we are probably at the end of the line
		if (nextSpace == NULL) {
			nextSpace = faceLine + len;
		}
		if (nextSpace != NULL) {
			// isolate the next trio that looks like "1/2/4"
			long long trioLength = nextSpace - trios;
			nextTrio = playdate->system->realloc(nextTrio, trioLength + sizeof(char));
			if (nextTrio == NULL) continue;
			memcpy(nextTrio, trios, trioLength);
			nextTrio[trioLength] = '\0';
			
			// update the offset for the next trio
			offset += trioLength + 1;

			// find the first / in this trio, we only need the first number
			firstDash = strstr(nextTrio, "/");
			if (firstDash == NULL) continue;
			long long firstDashIndex = firstDash - nextTrio;
			nextTrio[firstDashIndex] = '\0';

			// nextTrio now only contains the first number
			int value = atoi(nextTrio);
			// Subtract 1 because obj files start their index at 1, not 0
			values[index++] = value-1;
		}
	} while (nextSpace != NULL && offset < len && index < 3);

	playdate->system->realloc(nextTrio, 0);

	if (index == 3) {
		triangle->v1 = values[0];
		triangle->v2 = values[1];
		triangle->v3 = values[2];
		return 0;
	}

	return -1;
}

int GetTotalVectorsInObjFile (char* objFile) {
	char* nextEntry;
	size_t cursor = 0;
	int totalVectors = 0;
	do {
		nextEntry = strstr(objFile + cursor, "v ");
		if (nextEntry != NULL) {
			totalVectors++;
			long long ePosition = nextEntry - (objFile + cursor);
			cursor += ePosition + 1;
		}
	} while (nextEntry != NULL);

	return totalVectors;
}

int GetTotalFacesInObjFile (char* objFile) {
	char* nextEntry;
	size_t cursor = 0;
	int totalFaces = 0;
	do {
		nextEntry = strstr(objFile + cursor, "f ");
		if (nextEntry != NULL) {
			totalFaces++;
			long long ePosition = nextEntry - (objFile + cursor);
			cursor += ePosition + 1;
		}
	} while (nextEntry != NULL);

	return totalFaces;
}

ObjModel* LoadObj(PlaydateAPI* playdate, const char* objPath) {
	ObjModel* objModel = NULL;
	objModel = playdate->system->realloc(objModel, sizeof(ObjModel));
	if (objModel == NULL) return NULL;
	objModel->vectors = NULL;
	objModel->triangles = NULL;
	Point3D* vectors = NULL;
	Triangle* triangles = NULL;
	int vIndex = 0;
	int tIndex = 0;

	char* objFile = NULL;
	size_t cursor = 0;
	FileStat stat;
	int result = playdate->file->stat(objPath, &stat);

	if (result != -1) {
		SDFile* file = playdate->file->open(objPath, kFileRead);
		objFile = playdate->system->realloc(objFile, stat.size * sizeof(char));
		if (objFile != NULL) {
			int len = playdate->file->read(file, objFile, stat.size);
			playdate->file->close(file);
			//playdate->system->logToConsole("obj contents: %s", objFile);

			char* nextEntry;
			char* nextNewLine;
			char* relevantLine = NULL;

			// Allocate space for obj shape
			int totalVectors = GetTotalVectorsInObjFile(objFile);
			int totalFaces = GetTotalFacesInObjFile(objFile);
			vectors = playdate->system->realloc(vectors, totalVectors * sizeof(Point3D));
			triangles = playdate->system->realloc(triangles, totalFaces * sizeof(Triangle));

			if (vectors != NULL && triangles != NULL) {
				// Get Vectors
				do {
					// find next entry we want, a vertex in this case
					nextEntry = strstr(objFile + cursor, "v ");
					if (nextEntry != NULL) {
						long long ePosition = nextEntry - (objFile + cursor);
						//int vLen = len - ePosition;

						// find the end of the line
						nextNewLine = strstr(nextEntry, "\n");
						if (nextNewLine == NULL) continue;
						long long lineLength = nextNewLine - nextEntry;

						// use the bounds to isolate the relevant line
						relevantLine = playdate->system->realloc(relevantLine, lineLength + sizeof(char));
						if (relevantLine == NULL) continue;
						memcpy(relevantLine, nextEntry, lineLength);
						relevantLine[lineLength] = '\0';


						// parse the relevant line and extract the vector
						Point3D vector;
						result = GetVectorFromLine(playdate, relevantLine, &vector);
						if (result != -1) {
							// add the vector to our array
							vectors[vIndex++] = vector;
							playdate->system->logToConsole("obj contents: %s", relevantLine);

							// move the pointer further along the file
							cursor += ePosition + lineLength;
						}
					}
				} while (nextEntry != NULL);

				// Get Faces
				do {
					// find next entry we want, a face in this case
					nextEntry = strstr(objFile + cursor, "f ");
					if (nextEntry != NULL) {
						long long ePosition = nextEntry - (objFile + cursor);
						//int vLen = len - ePosition;

						// find the end of the line
						nextNewLine = strstr(nextEntry, "\n");
						if (nextNewLine == NULL) continue;
						long long lineLength = nextNewLine - nextEntry;

						// use the bounds to isolate the relevant line
						relevantLine = playdate->system->realloc(relevantLine, lineLength + sizeof(char));
						if (relevantLine == NULL) continue;
						memcpy(relevantLine, nextEntry, lineLength);
						relevantLine[lineLength] = '\0';

						// parse the relevant line and extract the triangle
						Triangle triangle;
						result = GetTriangleFromLine(playdate, relevantLine, &triangle);
						if (result != -1) {
							// add the vector to our array
							triangles[tIndex++] = triangle;
							playdate->system->logToConsole("obj contents: %s", relevantLine);

							// move the pointer further along the file
							cursor += lineLength + ePosition;
						}
					}
				} while (nextEntry != NULL);

				objModel->vCount = vIndex;
				objModel->tCount = tIndex;

				objModel->vectors = playdate->system->realloc(objModel->vectors, vIndex * sizeof(Point3D));
				if (objModel->vectors == NULL) return NULL;
				memcpy(objModel->vectors, vectors, vIndex * sizeof(Point3D));

				objModel->triangles = playdate->system->realloc(objModel->triangles, tIndex * sizeof(Triangle));
				if (objModel->triangles == NULL) return NULL;
				memcpy(objModel->triangles, triangles, tIndex * sizeof(Triangle));

				// free memory
				playdate->system->realloc(relevantLine, 0);
				playdate->system->realloc(vectors, 0);
				playdate->system->realloc(triangles, 0);
				playdate->system->realloc(objFile, 0);
			}
		}
	}
	return objModel;
}

Shape3D* BuildShape(ObjModel* objModel) {
	Shape3D* shape = m3d_malloc(sizeof(Shape3D));
	Shape3D_init(shape);
	Shape3D_retain(shape);

	for (int i = 0; i < objModel->tCount; i++) {
		Point3D* v1 = &objModel->vectors[objModel->triangles[i].v1];
		Point3D* v2 = &objModel->vectors[objModel->triangles[i].v2];
		Point3D* v3 = &objModel->vectors[objModel->triangles[i].v3];

		Shape3D_addFace(shape, v1, v2, v3, NULL, 0);
	}
	Shape3D_setClosed(shape, 1);
	return shape;
}

Shape3D* MakeShapeFromObj (PlaydateAPI* playdate, const char* filename) {
	ObjModel* objModel = LoadObj(playdate, filename);
	Shape3D* objShape = BuildShape(objModel);
	playdate->system->realloc(objModel->vectors, 0);
	playdate->system->realloc(objModel->triangles, 0);
	playdate->system->realloc(objModel, 0);
	return objShape;
}
