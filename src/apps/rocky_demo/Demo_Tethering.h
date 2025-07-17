/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/Viewpoint.h>
#include <rocky/SRS.h>
#include <rocky/vsg/MapManipulator.h>
#include <rocky/ecs/Motion.h>

#include "helpers.h"
using namespace ROCKY_NAMESPACE;

using namespace std::chrono_literals;

auto Demo_Tethering = [](Application& app)
{
    auto main_window = app.display.windowsAndViews.begin();
    auto view = main_window->second.front();
    
    auto manip = MapManipulator::get(view);
    if (!manip)
        return;

    // Make an entity for us to tether to and set it in motion
    static entt::entity entity = entt::null;
    static Status status;

    if (status.failed())
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Image load failed");
        ImGui::TextColored(ImVec4(1, 0, 0, 1), status.message.c_str());
        return;
    }

    const double s = 20.0;

    // Make an entity to tether to.
    if (entity == entt::null)
    {
        auto [lock, registry] = app.registry.write();

        // Create a host entity:
        entity = registry.create();

        // add an icon:
        auto io = app.vsgcontext->io;
        auto image = io.services.readImageFromURI("https://github.com/gwaldron/osgearth/blob/master/data/airport.png?raw=true", io);
        if (image.status.ok())
        {
            auto& icon = registry.emplace<Icon>(entity);
            icon.image = image.value;
            icon.style = IconStyle{ 48.0f, 0.0f }; // pixels, rotation(rad)
        }

        // add a mesh plane:
        auto& mesh = registry.emplace<Mesh>(entity);
        glm::dvec3 verts[4] = { { -s, -s, 0 }, {  s, -s, 0 }, {  s,  s, 0 }, { -s,  s, 0 } };
        unsigned indices[6] = { 0,1,2, 0,2,3 };
        glm::vec4 color{ 1, 1, 0, 0.55f };
        for (unsigned i = 0; i < 6; )
        {
            mesh.triangles.emplace_back(Triangle{
                {verts[indices[i++]], verts[indices[i++]], verts[indices[i++]]},
                {color, color, color} });
        }

        // add an arrow line:
        auto& arrow = registry.emplace<Line>(entity);
        arrow.points = { 
            glm::dvec3{ s * 1.5, s * 0.5, 0.0 },
            glm::dvec3{ s * 2.0, 0.0, 0.0 },
            glm::dvec3{ s * 2.0, 0.0, 0.0 },
            glm::dvec3{ s * 1.5, -s * 0.5, 0.0 },
            glm::dvec3{ s * 2.0, 0.0, 0.0 },
            glm::dvec3{ s * 0.0, 0.0, 0.0 }
        };

        arrow.style = LineStyle{ {1,0.5,0,1}, 4.0f };
        arrow.topology = Line::Topology::Segments;

        // Add a transform:
        auto& xform = registry.emplace<Transform>(entity);
        xform.position = GeoPoint(SRS::WGS84, -121, 55, 50000);

        // Add a motion component to animate the entity:
        auto& motion = registry.emplace<Motion>(entity);
        motion.velocity = { 1000, 0, 0 };
        motion.acceleration = { 0, 0, 0 };
    }

    if (ImGuiLTable::Begin("tethering"))
    {
        auto [lock, registry] = app.registry.read();

        bool tethering = manip->isTethering();
        if (ImGuiLTable::Checkbox("Tether active:", &tethering))
        {
            if (tethering)
            {
                auto vp = manip->viewpoint();
                vp.pointFunction = [&app]() {
                    auto [lock, registry] = app.registry.read();
                    return registry.get<TransformDetail>(entity).sync.position;
                    };
                vp.range = s * 12.0;
                vp.pitch = -45;
                vp.heading = 45;
                manip->setViewpoint(vp, 2.0s);
            }
            else
            {
                manip->home();
            }
        }

        auto& motion = registry.get<Motion>(entity);
        ImGuiLTable::SliderDouble("Speed", &motion.velocity.x, 0.0, 10000.0, "%.0lf");
        ImGuiLTable::SliderDouble("Acceleration", &motion.acceleration.x, -100.0, 100.0, "%.1lf");

        ImGuiLTable::End();
    }
};
