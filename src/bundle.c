#include <stdio.h>
#include <stdlib.h>

struct Resource {
	char *fileName;
	size_t offset;
	size_t size;
};

struct Resource resources[] = {
	{ .fileName = "resources/models/obj/alex_layer.obj" },
	{ .fileName = "resources/models/obj/alex_skin.obj" },
	{ .fileName = "resources/models/obj/osage-chan-lagtrain.png" },
};
size_t resources_count = sizeof(resources)/sizeof(*resources);

int
main(void)
{
	size_t bundle_size = 1;
	char *bundle = malloc(bundle_size);
	size_t offset = 0;

	if (!bundle) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < resources_count; i += 1) {
		FILE *f = fopen(resources[i].fileName, "rb");
		if (!f) {
			perror("fopen");
			free(bundle);
			return EXIT_FAILURE;
		}

		fseek(f, 0, SEEK_END);
		size_t size = (size_t)ftell(f);
		fseek(f, 0, SEEK_SET);
		resources[i].size = size;

		char *new = realloc(bundle, bundle_size + size + 1);
		if (!new) {
			perror("realloc");
			free(bundle);
			exit(EXIT_FAILURE);
		}
		bundle = new;
		bundle_size += size + 1;

		size_t count = fread(&bundle[offset], 1, size, f);
		if (count != size) {
			fprintf(stderr, "fread() failed: %zu\n", count);
			free(bundle);
			exit(EXIT_FAILURE);
		}

		resources[i].offset = offset;
		offset += size + 1;
		bundle[offset - 1] = '\0';

		fclose(f);
	}
	bundle[bundle_size - 1] = '\0';

	FILE *f = fopen("src/bundle.h", "w");
	if (!f) {
		perror("fopen");
		free(bundle);
		return EXIT_FAILURE;
	}

	fprintf(f,
		"struct Resource {\n"
		"\tchar *fileName;\n"
		"\tsize_t offset;\n"
		"\tsize_t size;\n"
		"};\n");

	fprintf(f, "struct Resource resources[] = {\n");
	for (size_t i = 0; i < resources_count; i += 1) {
		struct Resource r = resources[i];
		fprintf(f, "\t{ .fileName = \"%s\", .offset = %zu, .size = %zu },\n",
			r.fileName, r.offset, r.size);
	}
	fprintf(f,
		"};\n"
		"const size_t resources_count = %zu;\n",
		resources_count);

	const size_t rowsz = 12;
	fprintf(f, "unsigned char bundle[] = {\n");

	for (size_t row = 0; row < bundle_size/rowsz; row += 1) {
		fputc('\t', f);
		for (size_t col = 0; col < rowsz; col += 1) {
			size_t i = row*rowsz + col;
			fprintf(f, "0x%02X, ", (unsigned char)bundle[i]);
		}
		fputc('\n', f);
	}

	if (bundle_size%rowsz) {
		fputc('\t', f);
		for (size_t i = (bundle_size/rowsz*rowsz); i < bundle_size; i += 1) {
			fprintf(f, "0x%02X, ", (unsigned char)bundle[i]);
		}
		fputc('\n', f);
	}

	fprintf(f, "};\n");

	free(bundle);

	return EXIT_SUCCESS;
}
