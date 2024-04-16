#include <stdio.h>
#include <stdlib.h>

struct Resource {
	char *fileName;
	size_t offset;
	size_t size;
};

struct Resource res[] = {
	{ .fileName = "resources/models/obj/alex_layer.obj" },
	{ .fileName = "resources/models/obj/alex_skin.obj" },
	{ .fileName = "resources/models/obj/osage-chan-lagtrain.png" },
};

const size_t res_len = sizeof(res)/sizeof(res[0]);

int
main(void)
{
	size_t bundle_size = 1;
	char *bundle = malloc(bundle_size);
	size_t offset = 0;

	if (!bundle) return 1;

	for (size_t i = 0; i < res_len; i += 1) {
		FILE *f = fopen(res[i].fileName, "rb");
		if (!f) {
			free(bundle);
			return 2;
		}

		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		res[i].size = size;

		char *new = realloc(bundle, bundle_size + size + 1);
		if (!new) {
			fclose(f);
			free(bundle);
			return 3;
		}
		bundle = new;
		bundle_size += size + 1;

		size_t count = fread(&bundle[offset], 1, size, f);
		if (count != size) {
			fclose(f);
			free(bundle);
			return 4;
		}

		res[i].offset = offset;
		offset += size + 1;
		bundle[offset - 1] = '\0';

		fclose(f);
	}
	bundle[bundle_size - 1] = '\0';

	FILE *f = fopen("src/bundle.h", "w");
	if (!f) {
		free(bundle);
		return 5;
	}

	fprintf(f, "struct Resource {\n");
	fprintf(f, "\tchar *fileName;\n");
	fprintf(f, "\tsize_t offset;\n");
	fprintf(f, "\tsize_t size;\n");
	fprintf(f, "};\n");

	fprintf(f, "struct Resource resources[] = {\n");
	for (size_t i = 0; i < res_len; i += 1) {
		struct Resource r = res[i];
		fprintf(f, "\t{ .fileName = \"%s\", .offset = %zu, .size = %zu },\n", r.fileName, r.offset, r.size);
	}
	fprintf(f, "};\n");
	fprintf(f, "const size_t res_len = %zu;\n", res_len);

	const size_t rowsz = 12;
	fprintf(f, "unsigned char bundle[] = {\n");

	for (size_t row = 0; row < bundle_size/rowsz; row += 1) {
		fprintf(f, "\t");
		for (size_t col = 0; col < rowsz; col += 1) {
			size_t i = row*rowsz + col;
			fprintf(f, "0x%02X, ", (unsigned char)bundle[i]);
		}
		fprintf(f, "\n");
	}

	if (bundle_size%rowsz) {
		fprintf(f, "\t");
		for (size_t i = (bundle_size/rowsz*rowsz); i < bundle_size; i += 1) {
			fprintf(f, "0x%02X, ", (unsigned char)bundle[i]);
		}
		fprintf(f, "\n");
	}

	fprintf(f, "};\n");

	free(bundle);

	return 0;
}