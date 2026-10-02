#include "gltf_loader.h"

#include <stdint.h>
#include <SDL3/SDL.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

bool gltf_load_mesh(
    SDL_GPUDevice *device,
    Mesh *mesh,
    const char *relative_path
) {
    if (!device || !mesh || !relative_path) {
        return false;
    }

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
    Vertex *vertices = NULL;
    Uint16 *indices = NULL;
    bool success = false;

    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result == cgltf_result_success) {
        result = cgltf_load_buffers(&options, data, path);
    }
    if (result != cgltf_result_success) {
        SDL_Log("Could not load %s (cgltf result %d)",
                path, (int)result);
        goto cleanup;
    }

    // The first importer handles one indexed triangle primitive.
    if (data->meshes_count != 1 ||
        data->meshes[0].primitives_count != 1) {
        SDL_Log("Expected one mesh with one primitive in %s", path);
        goto cleanup;
    }

    const cgltf_primitive *primitive =
        &data->meshes[0].primitives[0];

    if (primitive->type != cgltf_primitive_type_triangles ||
        !primitive->indices) {
        SDL_Log("Expected indexed triangles in %s", path);
        goto cleanup;
    }

    const cgltf_accessor *positions = NULL;
    const cgltf_accessor *normals = NULL;
    const cgltf_accessor *uvs = NULL;

    for (cgltf_size i = 0; i < primitive->attributes_count; ++i) {
        const cgltf_attribute *attribute = &primitive->attributes[i];

        if (attribute->type == cgltf_attribute_type_position) {
            positions = attribute->data;
        } else if (attribute->type == cgltf_attribute_type_normal) {
            normals = attribute->data;
        } else if (attribute->type == cgltf_attribute_type_texcoord &&
                   attribute->index == 0) {
            uvs = attribute->data;
        }
    }

    if (!positions || !normals || !uvs ||
        positions->type != cgltf_type_vec3 ||
        normals->type != cgltf_type_vec3 ||
        uvs->type != cgltf_type_vec2 ||
        normals->count != positions->count ||
        uvs->count != positions->count) {
        SDL_Log("Unsupported vertex attributes in %s", path);
        goto cleanup;
    }

    cgltf_size vertex_count = positions->count;
    cgltf_size index_count = primitive->indices->count;

    if (vertex_count == 0 ||
        vertex_count > (cgltf_size)UINT16_MAX + 1 ||
        index_count == 0 ||
        index_count > UINT32_MAX ||
        index_count % 3 != 0) {
        SDL_Log("Unsupported mesh size in %s", path);
        goto cleanup;
    }

    vertices = SDL_calloc(vertex_count, sizeof(*vertices));
    indices = SDL_malloc(index_count * sizeof(*indices));
    if (!vertices || !indices) {
        SDL_Log("Could not allocate mesh geometry for %s", path);
        goto cleanup;
    }

    for (cgltf_size i = 0; i < vertex_count; ++i) {
        if (!cgltf_accessor_read_float(
                positions, i, vertices[i].position, 3) ||
            !cgltf_accessor_read_float(
                normals, i, vertices[i].normal, 3) ||
            !cgltf_accessor_read_float(
                uvs, i, vertices[i].uv, 2)) {
            SDL_Log("Could not read vertex %zu in %s", (size_t)i, path);
            goto cleanup;
        }

        // glTF is right-handed; this engine uses +X right and +Z forward.
        vertices[i].position[0] = -vertices[i].position[0];
        vertices[i].normal[0] = -vertices[i].normal[0];
    }

    for (cgltf_size i = 0; i < index_count; i += 3) {
        cgltf_size a = cgltf_accessor_read_index(primitive->indices, i);
        cgltf_size b = cgltf_accessor_read_index(primitive->indices, i + 1);
        cgltf_size c = cgltf_accessor_read_index(primitive->indices, i + 2);

        if (a >= vertex_count || b >= vertex_count || c >= vertex_count) {
            SDL_Log("Invalid triangle index in %s", path);
            goto cleanup;
        }

        // Reflecting X reverses winding, so swap the final two indices.
        indices[i] = (Uint16)a;
        indices[i + 1] = (Uint16)c;
        indices[i + 2] = (Uint16)b;
    }

    success = mesh_create(
        device, mesh, vertices, (Uint32)vertex_count,
        indices, (Uint32)index_count
    );

    if (success) {
        SDL_Log("Uploaded mesh '%s': %zu vertices, %zu indices",
                relative_path, (size_t)vertex_count, (size_t)index_count);
    }

cleanup:
    SDL_free(vertices);
    SDL_free(indices);
    cgltf_free(data);
    SDL_free(path);
    return success;
}
