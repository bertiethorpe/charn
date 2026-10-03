#include <SDL3_image/SDL_image.h>
#include <stdint.h>

#include "texture.h"

void texture_destroy(
    SDL_GPUDevice *device,
    Texture *texture
) {
    if (!texture) {
        return;
    }

    if (device) {
        if (texture->sampler) {
            SDL_ReleaseGPUSampler(
                device,
                texture->sampler
            );
        }

        if (texture->texture) {
            SDL_ReleaseGPUTexture(
                device,
                texture->texture
            );
        }
    }

    *texture = (Texture){0};
}

bool texture_create_rgba8(
    SDL_GPUDevice *device,
    Texture *texture,
    Uint32 width,
    Uint32 height,
    const Uint8 *pixels
) {
    if (!device ||
        !texture ||
        !pixels ||
        width == 0 ||
        height == 0) {
        SDL_Log("Cannot create a texture from empty data");
        return false;
    }

    texture_destroy(device, texture);

    const Uint32 bytes_per_pixel = 4;
    const Uint32 data_size =
        width * height * bytes_per_pixel;

    SDL_GPUTextureCreateInfo texture_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1
    };

    texture->texture = SDL_CreateGPUTexture(
        device,
        &texture_info
    );

    if (!texture->texture) {
        SDL_Log(
            "Could not create texture: %s",
            SDL_GetError()
        );
        texture_destroy(device, texture);
        return false;
    }

    SDL_GPUSamplerCreateInfo sampler_info = {
        .min_filter     = SDL_GPU_FILTER_NEAREST,
        .mag_filter     = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT
    };

    texture->sampler = SDL_CreateGPUSampler(
        device,
        &sampler_info
    );

    if (!texture->sampler) {
        SDL_Log(
            "Could not create texture sampler: %s",
            SDL_GetError()
        );
        texture_destroy(device, texture);
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = data_size
    };

    SDL_GPUTransferBuffer *transfer_buffer =
        SDL_CreateGPUTransferBuffer(
            device,
            &transfer_info
        );

    if (!transfer_buffer) {
        SDL_Log(
            "Could not create texture transfer buffer: %s",
            SDL_GetError()
        );
        texture_destroy(device, texture);
        return false;
    }

    void *mapped_data = SDL_MapGPUTransferBuffer(
        device,
        transfer_buffer,
        false
    );

    if (!mapped_data) {
        SDL_Log(
            "Could not map texture transfer buffer: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(
            device,
            transfer_buffer
        );
        texture_destroy(device, texture);
        return false;
    }

    SDL_memcpy(
        mapped_data,
        pixels,
        data_size
    );

    SDL_UnmapGPUTransferBuffer(
        device,
        transfer_buffer
    );

    SDL_GPUCommandBuffer *command_buffer =
        SDL_AcquireGPUCommandBuffer(device);

    if (!command_buffer) {
        SDL_Log(
            "Could not acquire texture upload command buffer: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(
            device,
            transfer_buffer
        );
        texture_destroy(device, texture);
        return false;
    }

    SDL_GPUCopyPass *copy_pass =
        SDL_BeginGPUCopyPass(command_buffer);

    if (!copy_pass) {
        SDL_Log(
            "Could not begin texture copy pass: %s",
            SDL_GetError()
        );
        SDL_CancelGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(
            device,
            transfer_buffer
        );
        texture_destroy(device, texture);
        return false;
    }

    SDL_GPUTextureTransferInfo source = {
        .transfer_buffer = transfer_buffer,
        .offset = 0,
        .pixels_per_row = width,
        .rows_per_layer = height
    };

    SDL_GPUTextureRegion destination = {
        .texture = texture->texture,
        .mip_level = 0,
        .layer = 0,
        .x = 0,
        .y = 0,
        .z = 0,
        .w = width,
        .h = height,
        .d = 1
    };

    SDL_UploadToGPUTexture(
        copy_pass,
        &source,
        &destination,
        false
    );

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_Log(
            "Could not submit texture upload: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(
            device,
            transfer_buffer
        );
        texture_destroy(device, texture);
        return false;
    }

    SDL_ReleaseGPUTransferBuffer(
        device,
        transfer_buffer
    );

    return true;
}

bool texture_load_image(
    SDL_GPUDevice *device,
    Texture *texture,
    const char *path
) {
    if (!device || !texture || !path) {
        return false;
    }

    SDL_Surface *loaded = IMG_Load(path);
    if (!loaded) {
        SDL_Log("Could not load image %s: %s", path, SDL_GetError());
        return false;
    }

    SDL_Surface *rgba = SDL_ConvertSurface(
        loaded, SDL_PIXELFORMAT_RGBA32
    );
    SDL_DestroySurface(loaded);
    if (!rgba) {
        SDL_Log("Could not convert image %s: %s", path, SDL_GetError());
        return false;
    }

    bool success = false;
    if (rgba->w <= 0 || rgba->h <= 0 ||
        (Uint64)rgba->w * (Uint64)rgba->h * 4 > UINT32_MAX ||
        rgba->pitch <= 0 ||
        (size_t)rgba->pitch < (size_t)rgba->w * 4) {
        SDL_Log("Unsupported image dimensions: %s", path);
        goto cleanup;
    }

    size_t row_bytes = (size_t)rgba->w * 4;
    size_t data_size = row_bytes * (size_t)rgba->h;
    Uint8 *pixels = SDL_malloc(data_size);

    if (!pixels) {
        SDL_Log("Could not allocate pixels for %s", path);
        goto cleanup;
    }

    if (!SDL_LockSurface(rgba)) {
        SDL_Log("Could not access pixels for %s: %s", path, SDL_GetError());
        SDL_free(pixels);
        goto cleanup;
    }

    for (int y = 0; y < rgba->h; ++y) {
        SDL_memcpy(
            pixels + (size_t)y * row_bytes,
            (const Uint8 *)rgba->pixels + (size_t)y * rgba->pitch,
            row_bytes
        );
    }

    SDL_UnlockSurface(rgba);

    success = texture_create_rgba8(
        device, texture, (Uint32)rgba->w, (Uint32)rgba->h, pixels
    );

    SDL_free(pixels);

cleanup:
    SDL_DestroySurface(rgba);
    return success;
}
