/**
 * rocky c++
 * Copyright 2025 Pelican Mapping
 * MIT License
 */
#pragma once
#include <rocky/vsg/VSGContext.h>

#if defined(ROCKY_HAS_IMGUI) && __has_include(<imgui.h>)
#include <imgui.h>
#include <rocky/Image.h>

namespace ROCKY_NAMESPACE
{
    /**
    * ImGuiImage encapsulates a texture that you can use in ImGui::Image() with its
    * Vulkan backend.
    * - Create the ImGuiImage
    * - call ImGui::Image(im->id(deviceID), im->size())
    */
    class ROCKY_EXPORT ImGuiImage
    {
    public:
        //! Default constructor - invalid object
        ImGuiImage() = default;

        //! Construct a new widget texture from an Image::Ptr.
        ImGuiImage(Image::Ptr image, VSGContext context);

        //! Opaque image handle to pass to ImGui::Image()
        ImTextureID id(std::uint32_t deviceID) const;

        //! Native image size to pass to ImGui::Image()
        ImVec2 size() const {
            return ImVec2(_image->width(), _image->height());
        }

        //! Is this image valid?
        operator bool() const {
            return _internal;
        }

    protected:
        struct Internal;
        Image::Ptr _image;
        Internal* _internal = nullptr;
    };
};

#endif
