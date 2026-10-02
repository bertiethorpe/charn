
#include "gltf_loader.h"

#include <SDL3/SDL.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

bool gltf_inspect(const char *relative_path) {
    const char *base_path = SDL_GetBasePath();
    if (!base_path) {
        SDL_Log("Could not find asset directory: %s", SDL_GetError());
        return false;
    }

    char *path = NULL;
    if (SDL_asprintf(&path, "%s%s", base_path, relative_path) < 0) {
        return false;
    }

    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, path, &data);

    if (result == cgltf_result_success) {
        result = cgltf_load_buffers(&options, data, path);
    }

    if (result != cgltf_result_success) {
        SDL_Log("Could not load %s (cgltf result %d)",
                path, (int)result);
        cgltf_free(data);
        SDL_free(path);
        return false;
    }

    SDL_Log("Loaded %s: %zu mesh(es)",
            relative_path, (size_t)data->meshes_count);

    for (cgltf_size i = 0; i < data->meshes_count; ++i) {
        const cgltf_mesh *mesh = &data->meshes[i];
        SDL_Log("Mesh %zu: %zu primitive(s)",
                (size_t)i, (size_t)mesh->primitives_count);

        for (cgltf_size j = 0; j < mesh->primitives_count; ++j) {
            const cgltf_primitive *primitive = &mesh->primitives[j];
            SDL_Log("  Primitive %zu: %zu indices",
                    (size_t)j,
                    primitive->indices
                        ? (size_t)primitive->indices->count
                        : 0);
        }
    }

    cgltf_free(data);
    SDL_free(path);
    return true;
}