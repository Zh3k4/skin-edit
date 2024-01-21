#include <stddef.h>

#include "raylib.h"
#include "rlgl.h"

#ifndef VERSION
#	define VERSION "dev"
#endif

int
update_skin(Model *model, Texture2D *texture)
{
	FilePathList files = LoadDroppedFiles();

	if (!IsFileExtension(files.paths[0], ".png")) return 0;

	UnloadTexture(*texture);
	*texture = LoadTexture(files.paths[0]);
	model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *texture;

	return 1;
}

int
main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(400, 600, "SkinView " VERSION);

	Camera camera = { 0 };
	camera.position = (Vector3){ -1.0f, 2.0f, -1.0f };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 90.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	Texture2D texture = LoadTexture("resources/models/obj/osage-chan-lagtrain.png");

	Model model = LoadModel("resources/models/obj/alex.obj");
	model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

	Vector3 position = { 0.0f, 0.0f, 0.0f };

	DisableCursor();
	int cursor = 0;

	SetTargetFPS(60);

	/* Main loop */
	while (!WindowShouldClose()) {

	/* Update */
	if (IsFileDropped()) update_skin(&model, &texture);

	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		if (cursor) {
			cursor = !cursor;
			DisableCursor();
		}
		UpdateCamera(&camera, CAMERA_THIRD_PERSON);
	} else if (!cursor) {
		cursor = !cursor;
		EnableCursor();
	}

	/* Draw */
	BeginDrawing();
	ClearBackground(BLACK);

	BeginMode3D(camera);
	DrawModel(model, position, 1.0f, WHITE);
	EndMode3D();

	EndDrawing();

	}

	UnloadTexture(texture);
	UnloadModel(model);

	return 0;
}
