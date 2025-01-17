#include <assert.h>
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
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"
#include "bundle.h"

#if defined(_WIN32) && !defined(PATH_MAX)
#define PATH_MAX 255
#endif

#define VERSION "0.6.0"

enum model {
	MODEL_HEAD = 0,
	MODEL_BODY,
	MODEL_LARM,
	MODEL_LLEG,
	MODEL_RARM,
	MODEL_RLEG,
	MODEL_LAYER_HEAD,
	MODEL_LAYER_BODY,
	MODEL_LAYER_LARM,
	MODEL_LAYER_LLEG,
	MODEL_LAYER_RARM,
	MODEL_LAYER_RLEG,
	MODEL_COUNT
};

struct button {
	Rectangle rec;
	bool active;
};

char *models_paths[] = {
	[MODEL_HEAD]       = "resources/models/obj/alex/skin/head.obj",
	[MODEL_BODY]       = "resources/models/obj/alex/skin/body.obj",
	[MODEL_LARM]       = "resources/models/obj/alex/skin/left_arm.obj",
	[MODEL_LLEG]       = "resources/models/obj/alex/skin/left_leg.obj",
	[MODEL_RARM]       = "resources/models/obj/alex/skin/right_arm.obj",
	[MODEL_RLEG]       = "resources/models/obj/alex/skin/right_leg.obj",
	[MODEL_LAYER_HEAD] = "resources/models/obj/alex/layer/head.obj",
	[MODEL_LAYER_BODY] = "resources/models/obj/alex/layer/body.obj",
	[MODEL_LAYER_LARM] = "resources/models/obj/alex/layer/left_arm.obj",
	[MODEL_LAYER_LLEG] = "resources/models/obj/alex/layer/left_leg.obj",
	[MODEL_LAYER_RARM] = "resources/models/obj/alex/layer/right_arm.obj",
	[MODEL_LAYER_RLEG] = "resources/models/obj/alex/layer/right_leg.obj",
};
static_assert(MODEL_COUNT == sizeof(models_paths)/sizeof(*models_paths),
	"Model count is wrong");

char *
lft(const char *fileName)
{
	for (size_t i = 0; i < resources_count; i += 1) {
		if (strcmp(fileName, resources[i].fileName) != 0)
			continue;

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
		if (strcmp(f, resources[i].fileName) == 0)
			return (unsigned char *)b;
	}
	return NULL;
}

size_t
get_bundle_size(char *f)
{
	for (size_t i = 0; i < resources_count; i += 1) {
		if (strcmp(f, resources[i].fileName) == 0)
			return resources[i].size;
	}
	return 0;
}

bool
update_model_with_png(char const *const fp, Model *m, Texture2D *t)
{
	if (!IsFileExtension(fp, ".png"))
		return 0;

	UnloadTexture(*t);
	*t = LoadTexture(fp);
	for (size_t i = 0; i < MODEL_COUNT; i++) {
		m[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *t;
	}
	return 1;
}

bool
update_skin(char *dst, Model *models, Texture2D *texture)
{
	FilePathList files = LoadDroppedFiles();
	if (!update_model_with_png(files.paths[0], models, texture))
		return 0;
	dst = strncpy(dst, files.paths[0], PATH_MAX);
	UnloadDroppedFiles(files);
	return 1;
}

void
buttons_update(struct button *button)
{
	const int w = GetScreenWidth();
	const int h = GetScreenHeight();
	const float min = (float)(w < h ? w : h);
	const float UI_PIXEL = min * 0.01f;

	button[MODEL_HEAD].rec = (Rectangle){
		UI_PIXEL * 4, 0,
		UI_PIXEL * 8, UI_PIXEL * 8,
	};

	button[MODEL_BODY].rec = (Rectangle){
		UI_PIXEL * 4, UI_PIXEL * 8,
		UI_PIXEL * 8, UI_PIXEL * 12,
	};

	button[MODEL_RARM].rec = (Rectangle){
		0, UI_PIXEL * 8,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	button[MODEL_LARM].rec = (Rectangle){
		UI_PIXEL * 12, UI_PIXEL * 8,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	button[MODEL_RLEG].rec = (Rectangle){
		UI_PIXEL * 4, UI_PIXEL * 20,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	button[MODEL_LLEG].rec = (Rectangle){
		UI_PIXEL * 8, UI_PIXEL * 20,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	const float pad = UI_PIXEL * (16 + 1);

	button[MODEL_LAYER_HEAD].rec = (Rectangle){
		pad + UI_PIXEL * 4, 0,
		UI_PIXEL * 8, UI_PIXEL * 8,
	};

	button[MODEL_LAYER_BODY].rec = (Rectangle){
		pad + UI_PIXEL * 4, UI_PIXEL * 8,
		UI_PIXEL * 8, UI_PIXEL * 12,
	};

	/* position of arms/legs are swapped */
	button[MODEL_LAYER_RARM].rec = (Rectangle){
		pad, UI_PIXEL * 8,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	button[MODEL_LAYER_LARM].rec = (Rectangle){
		pad + UI_PIXEL * 12, UI_PIXEL * 8,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	button[MODEL_LAYER_RLEG].rec = (Rectangle){
		pad + UI_PIXEL * 4, UI_PIXEL * 20,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	button[MODEL_LAYER_LLEG].rec = (Rectangle){
		pad + UI_PIXEL * 8, UI_PIXEL * 20,
		UI_PIXEL * 4, UI_PIXEL * 12,
	};

	for (size_t i = 0; i < MODEL_COUNT; i++) {
		button[i].rec.x += UI_PIXEL;
		button[i].rec.y += (float)h - UI_PIXEL * 33;
	}
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
			if (errno == ENOENT)
				fprintf(stderr, "File does not exist!\n");
			else
				fprintf(stderr, "Could not check file %s: %s!\n", argv[1], strerror(errno));

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
	Model models[MODEL_COUNT] = {0};
	for (size_t i = 0; i < MODEL_COUNT; i++) {
		models[i] = LoadModel(models_paths[i]);
		models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
	}
	SetLoadFileTextCallback(NULL);

	Vector3 position = { 0.0f, 0.0f, 0.0f };

	int savedCursorPos[2] = { GetMouseX(), GetMouseY() };

	struct button button[MODEL_COUNT] = {0};

	SetTargetFPS(60);

	/* Main loop */
	while (!WindowShouldClose()) {

	/* Update */
	buttons_update(button);

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
	if (new_time != old_time)
		queue_update = 1;

	bool rmb_down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
	bool cur_hidden = IsCursorHidden();
	if (rmb_down && !cur_hidden) {
		savedCursorPos[0] = GetMouseX();
		savedCursorPos[1] = GetMouseY();
		DisableCursor();
	} else if (!rmb_down && cur_hidden) {
		EnableCursor();
		SetMousePosition(savedCursorPos[0], savedCursorPos[1]);
	}

	if (cur_hidden) {
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
		float zoom = GetMouseWheelMoveV().y * 0.5f;
		float distance = Vector3Distance(camera.position, camera.target);
		distance += zoom;

		if (distance > 0.5f && distance < 5)
			CameraMoveToTarget(&camera, zoom);
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		Vector2 pos = GetMousePosition();
		for (size_t i = 0; i < MODEL_COUNT; i++) {
			if (CheckCollisionPointRec(pos, button[i].rec)) {
				button[i].active = !button[i].active;
				break;
			}
		}
	}

	/* Draw */
	BeginDrawing();
	ClearBackground(BLACK);

	BeginMode3D(camera);
	for (size_t i = 0; i < MODEL_COUNT; i++) {
		if (!button[i].active)
			DrawModel(models[i], position, 1.0f, WHITE);
	}
	EndMode3D();

	for (size_t i = 0; i < MODEL_COUNT; i++) {
		DrawRectangleRec(button[i].rec,
			(button[i].active ? DARKGRAY : WHITE));
	}

	EndDrawing();

	} /* End Main Loop */

	UnloadTexture(texture);
	UnloadImage(image);
	for (size_t i = 0; i < MODEL_COUNT; i++) {
		UnloadModel(models[i]);
	}

	return EXIT_SUCCESS;
}
