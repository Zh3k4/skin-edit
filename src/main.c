#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "raylib.h"
#include "rcamera.h"
#include "rlgl.h"

#include "bundle.h"

#ifndef VERSION
#	define VERSION "dev"
#endif

char *
lft(const char *fileName)
{
	for (size_t i = 0; i < resources_count; i += 1) {
		if (!strcmp(fileName, resources[i].fileName)) {
			char *res = ((char *)&bundle) + resources[i].offset;
			size_t size = resources[i].size;

			char *buf = calloc(size + 1, sizeof(*buf));
			if (!buf) return NULL;
			memcpy(buf, res, size);

			return buf;
		}
	}
	fprintf(stderr, "REACHED WHERE SHOULD NOT HAVE\n");
	exit(1);
}

const unsigned char *
get_bundle(char *f)
{
	for (size_t i = 0; i < resources_count; i += 1) {
		char *b = &((char *)bundle)[resources[i].offset];
		if (!strcmp(f, resources[i].fileName)) return (unsigned char *)b;
	}
	return NULL;
}

size_t
get_bundle_size(char *f)
{
	for (size_t i = 0; i < resources_count; i += 1) {
		if (!strcmp(f, resources[i].fileName)) return resources[i].size;
	}
	return 0;
}

_Bool
update_model_with_png(char const *const fp, Model *m[2], Texture2D *t)
{
	if (!IsFileExtension(fp, ".png")) return 0;

	UnloadTexture(*t);
	*t = LoadTexture(fp);
	m[0]->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *t;
	m[1]->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *t;
	return 1;
}

_Bool
update_skin(char *dst, Model *models[2], Texture2D *texture)
{
	FilePathList files = LoadDroppedFiles();
	if (!update_model_with_png(files.paths[0], models, texture)) return 0;
	dst = strncpy(dst, files.paths[0], PATH_MAX);
	UnloadDroppedFiles(files);
	return 1;
}

int
main(int argc, char **argv)
{
	Camera camera = { 0 };
	camera.position = (Vector3){ -1.0f, 2.0f, -1.0f };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 90.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	char skinfile[PATH_MAX + 1];
	strcpy(skinfile, "resources/models/obj/osage-chan-lagtrain.png");
	long old_time = GetFileModTime(skinfile);
	int queue_update = 0;

	struct stat statbuf;
	if (argc > 1) {
		if (stat(argv[1], &statbuf) < 0) {
			if (errno == ENOENT) fprintf(stderr, "File does not exist!\n");
			else fprintf(stderr, "Could not check file %s: %s!\n", argv[1], strerror(errno));

			return EXIT_FAILURE;
		}

		size_t len = strnlen(argv[1], PATH_MAX);
		if (len >= PATH_MAX) {
			fprintf(stderr, "Filename too long!\n");
			return EXIT_FAILURE;
		}

		strcpy(skinfile, argv[1]);
		skinfile[len] = '\0';
	}

	SetTraceLogLevel(LOG_WARNING);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(400, 600, "SkinView " VERSION);

	Image image = LoadImageFromMemory(".png", get_bundle(skinfile),
			(int)get_bundle_size(skinfile));
	Texture2D texture = LoadTextureFromImage(image);
	SetLoadFileTextCallback(lft);
	Model skin = LoadModel("resources/models/obj/alex_skin.obj");
	Model layer = LoadModel("resources/models/obj/alex_layer.obj");
	SetLoadFileTextCallback(NULL);
	skin.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
	layer.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

	Vector3 position = { 0.0f, 0.0f, 0.0f };

	DisableCursor();
	int cursor = 0;

	SetTargetFPS(60);

	/* Main loop */
	while (!WindowShouldClose()) {

	/* Update */
	if (IsFileDropped()) {
		Model *models[2] = { &skin, &layer };
		update_skin(skinfile, models, &texture);
		old_time = GetFileModTime(skinfile);
	}

	if (queue_update) {
		Model *models[2] = { &skin, &layer };
		update_model_with_png(skinfile, models, &texture);
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

		const Vector2 d = GetMouseDelta();
		float sens = 0.05f * GetFrameTime();
		CameraYaw(&camera, -d.x * sens, true);
		CameraPitch(&camera, -d.y * sens, true, true, false);

		if (IsKeyDown(KEY_LEFT_SHIFT)) {
			camera.position.y -= 1.0f/16.0f;
			camera.target.y -= 1.0f/16.0f;
		}
		if (IsKeyDown(KEY_SPACE)) {
			camera.position.y += 1.0f/16.0f;
			camera.target.y += 1.0f/16.0f;
		}
	} else if (!cursor) {
		cursor = !cursor;
		EnableCursor();
	}

	/* Draw */
	BeginDrawing();
	ClearBackground(BLACK);

	BeginMode3D(camera);
	DrawModel(skin, position, 1.0f, WHITE);
	DrawModel(layer, position, 1.0f, WHITE);
	EndMode3D();

	EndDrawing();

	} /* End Main Loop */

	UnloadTexture(texture);
	UnloadImage(image);
	UnloadModel(skin);
	UnloadModel(layer);

	return EXIT_SUCCESS;
}
