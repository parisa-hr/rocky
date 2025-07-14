/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "Profile.h"
#include "TileKey.h"
#include "Math.h"
#include "Utils.h"
#include "json.h"

using namespace ROCKY_NAMESPACE;
using namespace ROCKY_NAMESPACE::util;

#define LC "[Profile] "

void
Profile::setup(const SRS& srs, const Box& bounds, unsigned width0, unsigned height0)
{
    if (srs.valid())
    {
        Box b;
        unsigned tx = width0;
        unsigned ty = height0;

        if (bounds.valid())
        {
            b = bounds;
        }
        else
        {
            b = srs.bounds();
        }

        if (tx == 0 || ty == 0)
        {
            if (b.valid())
            {
                double ar = b.width() / b.height();
                if (ar >= 1.0) {
                    int ari = (int)ar;
                    tx = ari;
                    ty = 1;
                }
                else {
                    int ari = (int)(1.0 / ar);
                    tx = 1;
                    ty = ari;
                }
            }
            else // default.
            {
                tx = 1;
                ty = 1;
            }
        }

        _shared->extent = GeoExtent(srs, b);

        _shared->numTilesBaseX = tx;
        _shared->numTilesBaseY = ty;

        // automatically calculate the lat/long extents:
        _shared->geodeticExtent = srs.isGeodetic() ?
            _shared->extent :
            _shared->extent.transform(srs.geodeticSRS());

        // make a profile sig (sans srs) and an srs sig for quick comparisons.
        std::string temp = to_json();
        _shared->hash = std::hash<std::string>()(temp);
    }
}

// invalid profile
Profile::Profile()
{
    _shared = std::make_shared<Data>();
}

bool
Profile::equivalentTo(const Profile& rhs) const
{
    if (!valid() || !rhs.valid())
        return false;

    if (_shared == rhs._shared)
        return true;

    if (!_shared->wellKnownName.empty() && _shared->wellKnownName == rhs._shared->wellKnownName)
        return true;

    if (_shared->numTilesBaseY != rhs._shared->numTilesBaseY)
        return false;

    if (_shared->numTilesBaseX != rhs._shared->numTilesBaseX)
        return false;

    if (_shared->extent != rhs._shared->extent)
        return false;

    return _shared->extent.srs().equivalentTo(rhs._shared->extent.srs());
}

bool
Profile::horizontallyEquivalentTo(const Profile& rhs) const
{
    if (equivalentTo(rhs))
        return true;

    if (!valid() || !rhs.valid())
        return false;

    return _shared->extent.srs().horizontallyEquivalentTo(rhs._shared->extent.srs());
}

Profile::Profile(const std::string& wellKnownName)
{
    _shared = std::make_shared<Data>();
    setup(wellKnownName);
}

Profile::Profile(
    const SRS& srs,
    const Box& bounds,
    unsigned x_tiles_at_lod0,
    unsigned y_tiles_at_lod0)
{
    _shared = std::make_shared<Data>();
    setup(srs, bounds, x_tiles_at_lod0, y_tiles_at_lod0);
}

void
Profile::setup(const std::string& name)
{
    if (util::ciEquals(name, "plate-carree") ||
        util::ciEquals(name, "plate-carre") ||
        util::ciEquals(name, "eqc-wgs84"))
    {
        _shared->wellKnownName = name;

        // Yes I know this is not really Plate Carre but it will stand in for now.
        glm::dvec3 ex;

        SRS::WGS84.to(SRS::PLATE_CARREE).transform(glm::dvec3(180, 90, 0), ex);

        setup(
            SRS::PLATE_CARREE,
            Box(-ex.x, -ex.y, ex.x, ex.y),
            2u, 1u);
    }
    else if (util::ciEquals(name, "global-geodetic"))
    {
        _shared->wellKnownName = name;

        setup(
            SRS::WGS84,
            Box(-180.0, -90.0, 180.0, 90.0),
            2, 1);
    }
    else if (util::ciEquals(name, "spherical-mercator"))
    {
        _shared->wellKnownName = name;

        setup(
            SRS::SPHERICAL_MERCATOR,
            Box(-20037508.34278925, -20037508.34278925, 20037508.34278925, 20037508.34278925),
            1, 1);
    }
    else if (util::ciEquals(name, "moon"))
    {
        _shared->wellKnownName = name;

        setup(
            SRS("moon"),
            Box(-180.0, -90.0, 180.0, 90.0),
            2, 1);
    }
    else if (name.find("+proj=longlat") != std::string::npos)
    {
        setup(
            SRS(name),
            Box(-180.0, -90.0, 180.0, 90.0),
            2, 1);
    }
}

bool
Profile::valid() const {
    return _shared && _shared->extent.valid();
}

const SRS&
Profile::srs() const {
    return _shared->extent.srs();
}

const GeoExtent&
Profile::extent() const {
    return _shared->extent;
}

const GeoExtent&
Profile::geographicExtent() const {
    return _shared->geodeticExtent;
}

const std::string&
Profile::wellKnownName() const {
    return _shared->wellKnownName;
}

std::string
Profile::to_json() const
{
    json temp;
    ROCKY_NAMESPACE::to_json(temp, *this);
    return temp.dump();
}

void
Profile::from_json(const std::string& input)
{
    auto j = parse_json(input);
    ROCKY_NAMESPACE::from_json(j, *this);
}

Profile
Profile::overrideSRS(const SRS& srs) const
{
    return Profile(
        srs,
        Box(_shared->extent.xmin(), _shared->extent.ymin(), _shared->extent.xmax(), _shared->extent.ymax()),
        _shared->numTilesBaseX, _shared->numTilesBaseY);
}

std::vector<TileKey>
Profile::rootKeys() const
{
    return allKeysAtLOD(0);
}

std::vector<TileKey>
Profile::allKeysAtLOD(unsigned lod) const
{
    ROCKY_SOFT_ASSERT_AND_RETURN(valid(), {});

    std::vector<TileKey> result;

    auto[tx, ty] = numTiles(lod);

    for (unsigned c = 0; c < tx; ++c)
    {
        for (unsigned r = 0; r < ty; ++r)
        {
            result.push_back(TileKey(lod, c, r, *this));
        }
    }

    return result;
}

GeoExtent
Profile::tileExtent(unsigned lod, unsigned tileX, unsigned tileY) const
{
    auto [width, height] = tileDimensions(lod);

    double xmin = extent().xmin() + (width * (double)tileX);
    double ymax = extent().ymax() - (height * (double)tileY);
    double xmax = xmin + width;
    double ymin = ymax - height;

    return GeoExtent(srs(), xmin, ymin, xmax, ymax);
}

std::pair<double,double>
Profile::tileDimensions(unsigned int lod) const
{
    double out_width  = _shared->extent.width() / (double)_shared->numTilesBaseX;
    double out_height = _shared->extent.height() / (double)_shared->numTilesBaseY;

    double factor = double(1u << lod);
    out_width /= (double)factor;
    out_height /= (double)factor;

    return std::make_pair(out_width, out_height);
}

std::pair<unsigned, unsigned>
Profile::numTiles(unsigned lod) const
{
    unsigned out_tiles_wide = _shared->numTilesBaseX;
    unsigned out_tiles_high = _shared->numTilesBaseY;

    auto factor = 1u << lod;
    out_tiles_wide *= factor;
    out_tiles_high *= factor;

    return std::make_pair(out_tiles_wide, out_tiles_high);
}

unsigned int
Profile::levelOfDetailForHorizResolution( double resolution, int tileSize ) const
{
    if ( tileSize <= 0 || resolution <= 0.0 ) return 23;

    double tileRes = (_shared->extent.width() / (double)_shared->numTilesBaseX) / (double)tileSize;
    unsigned int level = 0;
    while( tileRes > resolution ) 
    {
        level++;
        tileRes *= 0.5;
    }
    return level;
}

GeoExtent
Profile::clampAndTransformExtent(const GeoExtent& input, bool* out_clamped) const
{
    // initialize the output flag
    if ( out_clamped )
        *out_clamped = false;

    // null checks
    if (!input.valid())
        return GeoExtent::INVALID;

    if (input.isWholeEarth())
    {
        if (out_clamped && !extent().isWholeEarth())
            *out_clamped = true;
        return extent();
    }

    // begin by transforming the input extent to this profile's SRS.
    GeoExtent inputInMySRS = input.transform(srs());

    if (inputInMySRS.valid())
    {
        // Compute the intersection of the two:
        GeoExtent intersection = inputInMySRS.intersectionSameSRS(extent());

        // expose whether clamping took place:
        if (out_clamped != 0L)
        {
            *out_clamped = intersection != extent();
        }

        return intersection;
    }

    else
    {
        // The extent transformation failed, probably due to an out-of-bounds condition.
        // Go to Plan B: attempt the operation in lat/long
        auto geo_srs = srs().geodeticSRS();

        // get the input in lat/long:
        GeoExtent gcs_input =
            input.srs().isGeodetic() ?
            input :
            input.transform(geo_srs);

        // bail out on a bad transform:
        if (!gcs_input.valid())
            return GeoExtent::INVALID;

        // bail out if the extent's do not intersect at all:
        if (!gcs_input.intersects(geographicExtent(), false))
            return GeoExtent::INVALID;

        // clamp it to the profile's extents:
        GeoExtent clamped_gcs_input = GeoExtent(
            gcs_input.srs(),
            clamp(gcs_input.xmin(), geographicExtent().xmin(), geographicExtent().xmax()),
            clamp(gcs_input.ymin(), geographicExtent().ymin(), geographicExtent().ymax()),
            clamp(gcs_input.xmax(), geographicExtent().xmin(), geographicExtent().xmax()),
            clamp(gcs_input.ymax(), geographicExtent().ymin(), geographicExtent().ymax()));

        if (out_clamped)
            *out_clamped = (clamped_gcs_input != gcs_input);

        // finally, transform the clamped extent into this profile's SRS and return it.
        GeoExtent result =
            clamped_gcs_input.srs() == srs() ?
            clamped_gcs_input :
            clamped_gcs_input.transform(srs());

        ROCKY_SOFT_ASSERT(result.valid());

        return result;
    }    
}

unsigned
Profile::equivalentLOD(const Profile& rhsProfile, unsigned rhsLOD) const
{
    ROCKY_SOFT_ASSERT_AND_RETURN(rhsProfile.valid(), rhsLOD);

    static const Profile SPHERICAL_MERCATOR("spherical-mercator");
    static const Profile GLOBAL_GEODETIC("global-geodetic");

    //If the profiles are equivalent, just use the incoming lod
    if (horizontallyEquivalentTo(rhsProfile))
        return rhsLOD;

    // Special check for geodetic to mercator or vise versa, they should match up in LOD.
    if (rhsProfile.horizontallyEquivalentTo(SPHERICAL_MERCATOR) && horizontallyEquivalentTo(GLOBAL_GEODETIC))
        return rhsLOD;

    if (rhsProfile.horizontallyEquivalentTo(GLOBAL_GEODETIC) && horizontallyEquivalentTo(SPHERICAL_MERCATOR))
        return rhsLOD;

    auto[rhsWidth, rhsHeight] = rhsProfile.tileDimensions(rhsLOD);

    // safety catch
    if (equiv(rhsWidth, 0.0) || equiv(rhsHeight, 0.0))
    {
        Log()->warn("Profile: getEquivalentLOD: zero dimension");
        return rhsLOD;
    }

    double rhsTargetHeight = SRS::transformUnits(
        rhsHeight, rhsProfile.srs(), srs(), Angle());

    int currLOD = 0;
    int destLOD = currLOD;

    double delta = DBL_MAX;

    // Find the LOD that most closely matches the resolution of the incoming key.
    // We use the closest (under or over) so that you can match back and forth between profiles and be sure to get the same results each time.
    while (true)
    {
        double prevDelta = delta;

        auto[w, h] = tileDimensions(currLOD);

        delta = fabs(h - rhsTargetHeight);
        if (delta < prevDelta)
        {
            // We're getting closer so keep going
            destLOD = currLOD;
        }
        else
        {
            // We are further away from the previous lod so stop.
            break;
        }
        currLOD++;
    }
    return destLOD;
}

unsigned
Profile::levelOfDetail(double height) const
{
    int currLOD = 0;
    int destLOD = currLOD;

    double delta = DBL_MAX;

    // Find the LOD that most closely matches the target height in this profile.
    while (true)
    {
        double prevDelta = delta;

        auto[w, h] = tileDimensions(currLOD);

        delta = fabs(h - height);
        if (delta < prevDelta)
        {
            // We're getting closer so keep going
            destLOD = currLOD;
        }
        else
        {
            // We are further away from the previous lod so stop.
            break;
        }
        currLOD++;
    }
    return destLOD;
}

bool
Profile::transformAndExtractContiguousExtents(
    const GeoExtent& input,
    std::vector<GeoExtent>& output) const
{
    ROCKY_SOFT_ASSERT_AND_RETURN(valid() && input.valid(), false);

    GeoExtent targetextent = input;

    // reproject into the profile's SRS if necessary:
    if (!srs().horizontallyEquivalentTo(input.srs()))
    {
        // localize the extents and clamp them to legal values
        targetextent = clampAndTransformExtent(input);
        if (!targetextent.valid())
            return false;
    }

    if (targetextent.crossesAntimeridian())
    {
        GeoExtent first, second;
        if (targetextent.splitAcrossAntimeridian(first, second))
        {
            output.push_back(first);
            output.push_back(second);
        }
    }
    else
    {
        output.push_back(targetextent);
    }

    return true;
}

std::string
Profile::toReadableString() const
{
    if (!wellKnownName().empty())
    {
        return wellKnownName();
    }
    else
    {
        return to_json();
    }
}


#include "json.h"
namespace ROCKY_NAMESPACE
{
    void to_json(json& j, const Profile& obj)
    {
        if (obj.valid())
        {
            if (!obj.wellKnownName().empty())
            {
                j = obj.wellKnownName(); // string
            }
            else
            {
                j = json::object();
                set(j, "extent", obj.extent());
                auto [tx, ty] = obj.numTiles(0);
                set(j, "tx", tx);
                set(j, "ty", ty);
            }
        }
        else j = nullptr;
    }

    void from_json(const json& j, Profile& obj)
    {
        if (j.is_string())
        {
            obj = Profile(get_string(j));
        }
        else if (j.is_object())
        {
            GeoExtent extent;
            unsigned tx = 0, ty = 0;

            get_to(j, "extent", extent);
            get_to(j, "tx", tx);
            get_to(j, "ty", ty);

            if (extent.valid())
            {
                obj = Profile(extent.srs(), extent.bounds(), tx, ty);
            }
        }
        else
        {
            obj = Profile();
        }
    }
}

