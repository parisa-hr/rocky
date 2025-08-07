/**
 * rocky c++
 * Copyright 2025 Pelican Mapping
 * MIT License
 */
#include "NodeLayer.h"
#include "VSGUtils.h"
#include "ecs/EntityNode.h"
#include <rocky/ecs/Visibility.h>

using namespace ROCKY_NAMESPACE;

Result<> NodeLayer::openImplementation(const IOOptions& io)
{
    util::forEach<EntityNode>(node, [&](EntityNode* entityNode)
        {
            auto [lock, r] = entityNode->registry.write();
            for (auto e : entityNode->entities)
                r.emplace_or_replace<ActiveState>(e);
        });

    return ResultVoidOK;
}

void NodeLayer::closeImplementation()
{
    util::forEach<EntityNode>(node, [&](EntityNode* entityNode)
        {
            auto [lock, r] = entityNode->registry.write();
            r.remove<ActiveState>(entityNode->entities.begin(), entityNode->entities.end());
        });
}
