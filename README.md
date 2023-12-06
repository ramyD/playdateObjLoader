# playdateObjLoader
an obj loader for the playdate

simply use `Shape3D* MakeShapeFromObj (PlaydateAPI* playdate, const char* filename);` to load an Obj file you placed in your Source folder.

Obj files are expected to be authored as triangles. There is no current quad support.

An example implementation:

```
#include <stdio.h>
#include <stdlib.h>
#include "pd_api.h"
#include "scene.h"
#include "3dmath.h"
#include "objLoader.h"

#ifdef _WINDLL
__declspec(dllexport)
#endif

Scene3D* scene;

int eventHandler (PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
  switch (event) {
    case kEventInit:
      mini3d_setRealloc(playdate->system->realloc);
      playdate->display->setRefreshRate(0);

      scene = m3d_malloc(sizeof(Scene3D));
      Scene3D_init(scene);
      Scene3D_setCamera(scene, Point3DMake(0.f, 0.f, -4.f), Point3DMake(0.f, 0.f, 0.f), 1.0f, Vector3DMake(0.f, 1.f, 0.f));
      Scene3D_setGlobalLight(scene, Vector3DMake(0.2f, 0.8f, 0.4f));
      
      Scene3DNode* root = Scene3D_getRootNode(scene);
      Scene3DNode* child1 = Scene3DNode_newChild(root);
      
      Shape3D* objShape = MakeShapeFromObj(playdate, "ico.obj");
      
      Scene3DNode_addShape(child1, objShape);

      playdate->system->setUpdateCallback(Update, playdate);
      break;
  }

  return 0;
}

int Update (void* ud) {
  PlaydateAPI* playdate = ud;

  uint8_t* frame = playdate->graphics->getFrame();
  Scene3D_draw(scene, frame, LCD_ROWSIZE);
  playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);

  return 1;
}
```
