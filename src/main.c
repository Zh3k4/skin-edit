#include <errno.h>
#include <limits.h>
#include <stdbool.h>
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

#if defined(_WIN32)
#define PATH_MAX MAX_PATH
#endif

#define VERSION "0.4.1"

char *models_paths[] = {
	"resources/models/obj/alex/skin/body.obj",
	"resources/models/obj/alex/skin/head.obj",
	"resources/models/obj/alex/skin/left_arm.obj",
	"resources/models/obj/alex/skin/left_leg.obj",
	"resources/models/obj/alex/skin/right_arm.obj",
	"resources/models/obj/alex/skin/right_leg.obj",
	"resources/models/obj/alex/layer/body.obj",
	"resources/models/obj/alex/layer/head.obj",
	"resources/models/obj/alex/layer/left_arm.obj",
	"resources/models/obj/alex/layer/left_leg.obj",
	"resources/models/obj/alex/layer/right_arm.obj",
	"resources/models/obj/alex/layer/right_leg.obj",
};
enum {
	models_count = sizeof(models_paths)/sizeof(*models_paths),
};

char *
lft(const char *fileName)
{
	for (size_t i = 0; i < resources_count; i += 1) {
		if (strcmp(fileName, resources[i].fileName)) {
			continue;
		}

		char *res = ((char *)&bundle) + resources[i].offset;
		size_t size = resources[i].size;

		char *buf = calloc(size + 1, sizeof(*buf));
		memcpy(buf, res, size);

		return buf;
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

bool
update_model_with_png(char const *const fp, Model *m, Texture2D *t)
{
	if (!IsFileExtension(fp, ".png")) return 0;

	UnloadTexture(*t);
	*t = LoadTexture(fp);
	for (size_t i = 0; i < models_count; i++) {
		m[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *t;
	}
	return 1;
}

bool
update_skin(char *dst, Model *models, Texture2D *texture)
{
	FilePathList files = LoadDroppedFiles();
	if (!update_model_with_png(files.paths[0], models, texture)) return 0;
	dst = strncpy(dst, files.paths[0], PATH_MAX);
	UnloadDroppedFiles(files);
	return 1;
}

#ifdef _MSC_VER
#define main WinMain
#endif
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
	Model models[models_count] = {0};
	for (size_t i = 0; i < models_count; i++) {
		models[i] = LoadModel(models_paths[i]);
		models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
	}
	SetLoadFileTextCallback(NULL);

	Vector3 position = { 0.0f, 0.0f, 0.0f };

	int savedCursorPos[2] = { GetMouseX(), GetMouseY() };
	bool cursor = 0;
	DisableCursor();

	SetTargetFPS(60);

	/* Main loop */
	while (!WindowShouldClose()) {

	/* Update */
	if (IsFileDropped()) {
		update_skin(skinfile, models, &texture);
		old_time = GetFileModTime(skinfile);
	}

	if (queue_update) {
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
			savedCursorPos[0] = GetMouseX();
			savedCursorPos[1] = GetMouseY();
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
		SetMousePosition(savedCursorPos[0], savedCursorPos[1]);
	}

	/* Draw */
	BeginDrawing();
	ClearBackground(BLACK);

	BeginMode3D(camera);
	for (size_t i = 0; i < models_count; i++) {
		DrawModel(models[i], position, 1.0f, WHITE);
	}
	EndMode3D();

	EndDrawing();

	} /* End Main Loop */

	UnloadTexture(texture);
	UnloadImage(image);
	for (size_t i = 0; i < models_count; i++) {
		UnloadModel(models[i]);
	}

	return EXIT_SUCCESS;
}
