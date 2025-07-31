/**
 * rocky c++
 * Copyright 2025 Pelican Mapping
 * MIT License
 */
#include "GeometryPool.h"
#include "TerrainSettings.h"
#include <vsg/commands/DrawIndexed.h>

#undef LC
#define LC "[GeometryPool] "

using namespace ROCKY_NAMESPACE;

GeometryPool::GeometryPool(const Profile& profile)
{
    _worldSRS = profile.srs().isGeodetic() ? profile.srs().geocentricSRS() : profile.srs();

    // activate debugging mode
    if (util::isEnvVarSet("ROCKY_DEBUG_REX_GEOMETRY_POOL"))
    {
        _debug = true;
    }

    if (util::isEnvVarSet("ROCKY_NO_GEOMETRY_POOL"))
    {
        _enabled = false;
        Log()->info(LC "Geometry pool disabled (environment)");
    }
}

vsg::ref_ptr<SharedGeometry>
GeometryPool::getPooledGeometry(const TileKey& tileKey, const Settings& settings, Cancelable* progress) const
{
    vsg::ref_ptr<SharedGeometry> out;

    // convert to a unique-geometry key:
    GeometryKey geomKey;
    createKeyForTileKey(tileKey, settings.tileSize, geomKey);

    // make our globally shared EBO if we need it
    {
        std::scoped_lock lock(_mutex);
        if (_defaultIndices == nullptr)
        {
            _defaultIndices = createIndices(settings);
        }
    }

    ROCKY_TODO("MeshEditor meshEditor(tileKey, tileSize, map, nullptr);");

    if ( _enabled )
    {
        // Protect access on a per key basis to prevent the same key from being created twice.  
        // This was causing crashes with multiple windows opening and closing.
        util::ScopedGate<GeometryKey> gatelock(_keygate, geomKey);

        // first check the sharing cache:
        //if (!meshEditor.hasEdits())
        {
            std::scoped_lock lock(_mutex);
            auto i = _sharedGeometries.find(geomKey);
            if (i != _sharedGeometries.end())
            {
                // found it:
                out = i->second;
            }
        }

        if (!out.valid())
        {
            out = createGeometry(
                tileKey,
                settings,
                //meshEditor,
                progress);

            // only store as a shared geometry if there are no constraints.
            if (out.valid()) //&& !meshEditor.hasEdits())
            {
                std::scoped_lock lock(_mutex);
                _sharedGeometries.emplace(geomKey, out);
            }
        }
    }

    else
    {
        out = createGeometry(
            tileKey,
            settings,
            //meshEditor,
            progress);
    }

    ROCKY_SOFT_ASSERT_AND_RETURN(out->indices, nullptr);

    return out;
}

void
GeometryPool::createKeyForTileKey(const TileKey& key, unsigned tileSize, GeometryKey& out) const
{
    out.lod  = key.level;
    out.tileY = key.profile.srs().isGeodetic()? key.y : 0;
    out.size = tileSize;
}

int
GeometryPool::getNumSkirtElements(
    const Settings& settings) const
{
    return settings.skirtRatio > 0.0f ? (settings.tileSize-1) * 4 * 6 : 0;
}

namespace
{
    int getMorphNeighborIndexOffset(unsigned col, unsigned row, int rowSize)
    {
        if ( (col & 0x1)==1 && (row & 0x1)==1 ) return rowSize+2;
        if ( (row & 0x1)==1 )                   return rowSize+1;
        if ( (col & 0x1)==1 )                   return 2;
        return 1;
    }
}

#define addSkirtDataForIndex(P, INDEX, HEIGHT) \
{ \
    verts->set(P, verts->at(INDEX)); \
    normals->set(P, normals->at(INDEX)); \
    uvs->set(P, uvs->at(INDEX)); \
    uvs->at(P).z = (float)((int)uvs->at(P).z | VERTEX_SKIRT); \
    if ( neighbors ) neighbors->set(P, neighbors->at(INDEX)); \
    if ( neighborNormals ) neighborNormals->set(P, neighborNormals->at(INDEX)); \
    ++P; \
    verts->set(P, verts->at(INDEX) - (normals->at(INDEX)*(HEIGHT))); \
    normals->set(P, normals->at(INDEX)); \
    uvs->set(P, uvs->at(INDEX)); \
    uvs->at(P).z = (float)((int)uvs->at(P).z | VERTEX_SKIRT); \
    if ( neighbors ) neighbors->set(P, neighbors->at(INDEX) - (normals->at(INDEX)*(HEIGHT))); \
    if ( neighborNormals ) neighborNormals->set(P, neighborNormals->at(INDEX)); \
    ++P; \
}

#define addSkirtTriangles(P, INDEX0, INDEX1) \
{ \
    indices->set(P++, (INDEX0));   \
    indices->set(P++, (INDEX0)+1); \
    indices->set(P++, (INDEX1));   \
    indices->set(P++, (INDEX1));   \
    indices->set(P++, (INDEX0)+1); \
    indices->set(P++, (INDEX1)+1); \
}

vsg::ref_ptr<vsg::ushortArray>
GeometryPool::createIndices(const Settings& settings) const
{
    // Attempt to calculate the number of verts in the surface geometry.
    bool needsSkirt = settings.skirtRatio > 0.0f;
    uint32_t tileSize = std::max(settings.tileSize, 2u);

    unsigned numVertsInSurface = (tileSize*tileSize);
    unsigned numVertsInSkirt = needsSkirt ? (tileSize - 1) * 2u * 4u : 0;
    unsigned numVerts = numVertsInSurface + numVertsInSkirt;
    unsigned numIndicesInSurface = (tileSize - 1) * (tileSize - 1) * 6;
    unsigned numIncidesInSkirt = getNumSkirtElements(settings);
    unsigned numIndices = numIndicesInSurface + numIncidesInSkirt;

    auto indices = vsg::ushortArray::create(numIndices);

    // tessellate the surface:
    unsigned p = 0;
    for (unsigned j = 0; j < tileSize - 1; ++j)
    {
        for (unsigned i = 0; i < tileSize - 1; ++i)
        {
            int i00 = j * tileSize + i;
            int i01 = i00 + tileSize;
            int i10 = i00 + 1;
            int i11 = i01 + 1;

            unsigned k = j * tileSize + i;

            indices->set(p++, i01);
            indices->set(p++, i00);
            indices->set(p++, i11);

            indices->set(p++, i00);
            indices->set(p++, i10);
            indices->set(p++, i11);
        }
    }

    if (needsSkirt)
    {
        // add the elements for the skirt:
        int skirtBegin = numVertsInSurface;
        int skirtEnd = skirtBegin + numVertsInSkirt;
        int i;
        for (i = skirtBegin; i < (int)skirtEnd - 3; i += 2)
        {
            addSkirtTriangles(p, i, i + 2);
        }
        addSkirtTriangles(p, i, skirtBegin);
    }

    return indices;
}

namespace
{
    struct Locator
    {
        GeoExtent tile_extent;
        Ellipsoid ellipsoid;
        SRSOperation tile_to_world;

        Locator(const GeoExtent& extent, const SRS& worldSRS)
        {
            tile_extent = extent;
            tile_to_world = tile_extent.srs().to(worldSRS);
        }

        inline glm::dvec3 unitToWorld(const glm::dvec3& unit) const
        {
            // unit to tile:
            glm::dvec3 tile(
                unit.x * tile_extent.width() + tile_extent.xmin(),
                unit.y * tile_extent.height() + tile_extent.ymin(),
                unit.z);

            glm::dvec3 world;
            tile_to_world(tile, world);

            return world;
        }
    };

    //template<class SPHERE, class VEC3>
    inline void expandSphereToInclude(vsg::dsphere& sphere, const vsg::dvec3& p)
    {
        auto dv = p - sphere.center;
        double r = length(dv);
        if (r > sphere.radius) {
            double dr = 0.5 * (r - sphere.radius);
            sphere.center += dv * (dr / r);
            sphere.radius += dr;
        }
    }
}

vsg::ref_ptr<SharedGeometry>
GeometryPool::createGeometry(const TileKey& tileKey, const Settings& settings, Cancelable* progress) const
{
    // Establish a local reference frame for the tile:
    GeoPoint centroid = tileKey.extent().centroid();
    centroid.transformInPlace(_worldSRS);
    glm::dmat4 world2local = glm::inverse(_worldSRS.topocentricToWorldMatrix(
        glm::dvec3(centroid.x, centroid.y, centroid.z)));

    // Attempt to calculate the number of verts in the surface geometry.
    bool needsSkirt = settings.skirtRatio > 0.0f;

    auto tileSize = settings.tileSize;
    const uint32_t numVertsInSurface    = (tileSize*tileSize);
    const uint32_t numVertsInSkirt      = needsSkirt ? (tileSize-1)*2u * 4u : 0;
    const uint32_t numVerts             = numVertsInSurface + numVertsInSkirt;
    const uint32_t numIndiciesInSurface = (tileSize-1) * (tileSize-1) * 6;
    const uint32_t numIncidesInSkirt    = getNumSkirtElements(settings);

    ROCKY_TODO("GLenum mode = gpuTessellation ? GL_PATCHES : GL_TRIANGLES;");

    vsg::dsphere tileBound;
    //Sphere tileBound;

    // the initial vertex locations:
    auto verts = vsg::vec3Array::create(numVerts);
    auto normals = vsg::vec3Array::create(numVerts);
    auto uvs = vsg::vec3Array::create(numVerts);

    vsg::ref_ptr<vsg::vec3Array> neighbors;
    vsg::ref_ptr<vsg::vec3Array> neighborNormals;

    if (settings.morphing == true)
    {
        neighbors = vsg::vec3Array::create(numVerts);
        neighborNormals = vsg::vec3Array::create(numVerts);
    }

    if (true) // no mesh constraints
    {
        glm::dvec3 unit;
        glm::dvec3 world;
        glm::dvec3 local;
        glm::dvec3 world_plus_one;
        glm::dvec3 normal;

        Locator locator(tileKey.extent(), _worldSRS);

        for (unsigned row = 0; row < tileSize; ++row)
        {
            float ny = (float)row / (float)(tileSize - 1);
            for (unsigned col = 0; col < tileSize; ++col)
            {
                float nx = (float)col / (float)(tileSize - 1);
                unsigned i = row * tileSize + col;

                unit = { nx, ny, 0.0 };
                world = locator.unitToWorld(unit);
                local = world2local * world;
                verts->set(i, vsg::vec3(local.x, local.y, local.z));

                expandSphereToInclude(tileBound, vsg::dvec3(local.x, local.y, local.z));

                // Use the Z coord as a type marker
                float marker = VERTEX_VISIBLE;
                uvs->set(i, vsg::vec3(nx, ny, marker));

                unit.z = 1.0;
                world_plus_one = locator.unitToWorld(unit);
                normal = glm::normalize((world2local*world_plus_one) - local);
                normals->set(i, vsg::vec3(normal.x, normal.y, normal.z));

                // neighbor:
                if (neighbors)
                {
                    auto& modelNeighborLTP = (*verts)[verts->size() - getMorphNeighborIndexOffset(col, row, tileSize)];
                    neighbors->set(i, modelNeighborLTP);
                }

                if (neighborNormals)
                {
                    auto& modelNeighborNormalLTP = (*normals)[normals->size() - getMorphNeighborIndexOffset(col, row, tileSize)];
                    neighborNormals->set(i, modelNeighborNormalLTP);
                }
            }
        }

        if (needsSkirt)
        {
            // calculate the skirt extrusion height
            float height = (float)tileBound.radius * settings.skirtRatio;

            // Normal tile skirt first:
            unsigned skirtIndex = numVertsInSurface; // verts->size();

            // first, create all the skirt verts, normals, and texcoords.
            for (int c = 0; c < (int)tileSize - 1; ++c)
                addSkirtDataForIndex(skirtIndex, c, height); //south

            for (int r = 0; r < (int)tileSize - 1; ++r)
                addSkirtDataForIndex(skirtIndex, r*tileSize + (tileSize - 1), height); //east

            for (int c = tileSize - 1; c > 0; --c)
                addSkirtDataForIndex(skirtIndex, (tileSize - 1)*tileSize + c, height); //north

            for (int r = tileSize - 1; r > 0; --r)
                addSkirtDataForIndex(skirtIndex, r*tileSize, height); //west
        }

        auto indices =
            _enabled ? _defaultIndices : createIndices(settings);

        // the geometry:
        auto geom = SharedGeometry::create();

        vsg::DataList arrays = { verts, normals, uvs };
        
        if (neighbors)
            arrays.emplace_back(neighbors);

        if (neighborNormals)
            arrays.emplace_back(neighborNormals);

        geom->assignArrays(arrays);

        geom->assignIndices(indices);

        geom->commands.push_back(
            vsg::DrawIndexed::create(
                indices->size(), // index count
                1,               // instance count
                0,               // first index
                0,               // vertex offset
                0));             // first instance

        // maintain for calculating proxy geometries
        geom->verts = verts;
        geom->normals = normals;
        geom->uvs = uvs;
        geom->indexArray = indices;

        return geom;
    }
}

void
GeometryPool::clear()
{
    std::scoped_lock lock(_mutex);
    _sharedGeometries.clear();
}

void
GeometryPool::sweep(VSGContext& context)
{
    std::scoped_lock lock(_mutex);
    SharedGeometries temp;
    for (auto& entry : _sharedGeometries)
    {
        if (entry.second->referenceCount() > 1)
            temp.emplace(entry.first, entry.second);
        else
            context->dispose(entry.second);

    }
    _sharedGeometries.swap(temp);
}
