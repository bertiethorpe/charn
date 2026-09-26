#include "mesh.h"

void mesh_destroy(
    SDL_GPUDevice *device,
    Mesh *mesh
) {
    if (!mesh) {
        return;
    }

    if (device) {
        if (mesh->vertex_buffer) {
            SDL_ReleaseGPUBuffer(
                device,
                mesh->vertex_buffer
            );
        }

        if (mesh->index_buffer) {
            SDL_ReleaseGPUBuffer(
                device,
                mesh->index_buffer
            );
        }
    }

    *mesh = (Mesh){0};
}

bool mesh_create(
    SDL_GPUDevice *device,
    Mesh *mesh,
    const Vertex *vertices,
    Uint32 vertex_count,
    const Uint16 *indices,
    Uint32 index_count
) {
    if (!device ||
        !mesh ||
        !vertices ||
        vertex_count == 0 ||
        !indices ||
        index_count == 0) {
        SDL_Log("Cannot create a mesh from empty data");
        return false;
    }

    mesh_destroy(device, mesh);

    const Uint32 vertex_data_size =
        vertex_count * (Uint32)sizeof(Vertex);

    const Uint32 index_data_size =
        index_count * (Uint32)sizeof(Uint16);

    const Uint32 index_data_offset =
        (vertex_data_size + 3u) & ~3u;

    SDL_GPUBufferCreateInfo vertex_buffer_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertex_data_size
    };

    mesh->vertex_buffer = SDL_CreateGPUBuffer(
        device,
        &vertex_buffer_info
    );

    if (!mesh->vertex_buffer) {
        SDL_Log(
            "Could not create vertex buffer: %s",
            SDL_GetError()
        );
        mesh_destroy(device, mesh);
        return false;
    }

    SDL_GPUBufferCreateInfo index_buffer_info = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = index_data_size
    };

    mesh->index_buffer = SDL_CreateGPUBuffer(
        device,
        &index_buffer_info
    );

    if (!mesh->index_buffer) {
        SDL_Log(
            "Could not create index buffer: %s",
            SDL_GetError()
        );
        mesh_destroy(device, mesh);
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = index_data_offset + index_data_size
    };

    SDL_GPUTransferBuffer *transfer_buffer =
        SDL_CreateGPUTransferBuffer(
            device,
            &transfer_info
        );

    if (!transfer_buffer) {
        SDL_Log(
            "Could not create mesh transfer buffer: %s",
            SDL_GetError()
        );
        mesh_destroy(device, mesh);
        return false;
    }

    void *mapped_data = SDL_MapGPUTransferBuffer(
        device,
        transfer_buffer,
        false
    );

    if (!mapped_data) {
        SDL_Log(
            "Could not map mesh transfer buffer: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        mesh_destroy(device, mesh);
        return false;
    }

    SDL_memcpy(
        mapped_data,
        vertices,
        vertex_data_size
    );

    SDL_memcpy(
        (Uint8 *)mapped_data + index_data_offset,
        indices,
        index_data_size
    );

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer =
        SDL_AcquireGPUCommandBuffer(device);

    if (!command_buffer) {
        SDL_Log(
            "Could not acquire mesh upload command buffer: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        mesh_destroy(device, mesh);
        return false;
    }

    SDL_GPUCopyPass *copy_pass =
        SDL_BeginGPUCopyPass(command_buffer);

    if (!copy_pass) {
        SDL_Log(
            "Could not begin mesh copy pass: %s",
            SDL_GetError()
        );
        SDL_CancelGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        mesh_destroy(device, mesh);
        return false;
    }

    SDL_GPUTransferBufferLocation vertex_source = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };

    SDL_GPUBufferRegion vertex_destination = {
        .buffer = mesh->vertex_buffer,
        .offset = 0,
        .size = vertex_data_size
    };

    SDL_UploadToGPUBuffer(
        copy_pass,
        &vertex_source,
        &vertex_destination,
        false
    );

    SDL_GPUTransferBufferLocation index_source = {
        .transfer_buffer = transfer_buffer,
        .offset = index_data_offset
    };

    SDL_GPUBufferRegion index_destination = {
        .buffer = mesh->index_buffer,
        .offset = 0,
        .size = index_data_size
    };

    SDL_UploadToGPUBuffer(
        copy_pass,
        &index_source,
        &index_destination,
        false
    );

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_Log(
            "Could not submit mesh upload: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        mesh_destroy(device, mesh);
        return false;
    }

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    mesh->index_count = index_count;
    return true;
}
