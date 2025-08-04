/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "MBTilesElevationLayer.h"
#ifdef ROCKY_HAS_MBTILES

#include "Context.h"
#include "json.h"

using namespace ROCKY_NAMESPACE;

ROCKY_ADD_OBJECT_FACTORY(MBTilesElevation,
    [](std::string_view JSON, const IOOptions& io) {
        return MBTilesElevationLayer::create(JSON, io); })


MBTilesElevationLayer::MBTilesElevationLayer() :
    super()
{
    construct({}, {});
}
MBTilesElevationLayer::MBTilesElevationLayer(std::string_view JSON, const IOOptions& io) :
    super(JSON, io)
{
    construct(JSON, io);
}

void
MBTilesElevationLayer::construct(std::string_view JSON, const IOOptions& io)
{
    setLayerTypeName("MBTilesElevation");
    const auto j = parse_json(JSON);
    get_to(j, "uri", uri, io);
    get_to(j, "format", format);
    get_to(j, "compress", compress);
}

std::string
MBTilesElevationLayer::to_json() const
{
    auto j = parse_json(super::to_json());
    set(j, "uri", uri);
    set(j, "format", format);
    set(j, "compress", compress);
    return j.dump();
}

Result<>
MBTilesElevationLayer::openImplementation(const IOOptions& io)
{
    auto parent = super::openImplementation(io);
    if (parent.failed())
        return parent;

    Profile new_profile = profile;
    DataExtentList dataExtents;

    auto r = _driver.open(
        name,
        *this, // MBTiles::Options
        false, // isWritingRequested()
        new_profile,
        dataExtents,
        io);

    if (r.failed())
        return r.error();

    // install the profile if there is one
    if (!profile.valid() && new_profile.valid())
    {
        profile = new_profile;
    }

    setDataExtents(dataExtents);

    return {};
}

void
MBTilesElevationLayer::closeImplementation()
{
    _driver.close();
    super::closeImplementation();
}

Result<GeoHeightfield>
MBTilesElevationLayer::createHeightfieldImplementation(const TileKey& key, const IOOptions& io) const
{
    if (status().failed())
        return status().error();

    auto result = _driver.read(key, io);

    if (result)
        return GeoHeightfield(Heightfield::create(result.value().get()), key.extent());
    else
        return result.error();
}

#endif // ROCKY_HAS_MBTILES
