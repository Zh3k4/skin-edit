#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "raylib.h"
#include "rlgl.h"

#ifndef VERSION
#	define VERSION "dev"
#endif

int
update_model_with_png(char const *const fp, Model *m, Texture2D *t)
{
	if (!IsFileExtension(fp, ".png")) return 0;

	UnloadTexture(*t);
	*t = LoadTexture(fp);
	m->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *t;
	return 1;
}

int
update_skin(char *dst, Model *model, Texture2D *texture)
{
	FilePathList files = LoadDroppedFiles();
	if (!update_model_with_png(files.paths[0], model, texture)) return 0;
	dst = strncpy(dst, files.paths[0], PATH_MAX);
	UnloadDroppedFiles(files);
	return 1;
}

int
main(void)
{
	Camera camera = { 0 };
	camera.position = (Vector3){ -1.0f, 2.0f, -1.0f };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 90.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	char skinfile[PATH_MAX + 1];
	sprintf(skinfile, "resources/models/obj/osage-chan-lagtrain.png");
	long old_time = GetFileModTime(skinfile);
	int queue_update = 0;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(400, 600, "SkinView " VERSION);

	Texture2D texture = LoadTexture(skinfile);
	Model model = LoadModel("resources/models/obj/alex.obj");
	model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

	Vector3 position = { 0.0f, 0.0f, 0.0f };

	DisableCursor();
	int cursor = 0;

	SetTargetFPS(60);

	/* Main loop */
	while (!WindowShouldClose()) {

	/* Update */
	if (IsFileDropped()) {
		update_skin(skinfile, &model, &texture);
		old_time = GetFileModTime(skinfile);
	}

	if (queue_update) {
		update_model_with_png(skinfile, &model, &texture);
		old_time = GetFileModTime(skinfile);
		queue_update = 0;
	}
	long new_time = GetFileModTime(skinfile);
	if (new_time != old_time) {
		queue_update = 1;
	}

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

	} /* End Main Loop */

	UnloadTexture(texture);
	UnloadModel(model);

	return EXIT_SUCCESS;
}
