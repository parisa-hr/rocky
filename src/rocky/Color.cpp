/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "Color.h"
#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <random>

using namespace ROCKY_NAMESPACE;
using namespace ROCKY_NAMESPACE::util;

namespace
{
    glm::fvec4& rgb2hsv_in_place(glm::fvec4& c)
    {
        float minval = std::min(c.r, std::min(c.g, c.b));
        float maxval = std::max(c.r, std::max(c.g, c.b));
        float delta = maxval - minval;
        float h = 0.0f, s = 0.0, v = maxval;
        if (delta != 0.0f)
        {
            s = delta / maxval;
            float dr = (((maxval - c.r) / 6.0f) + (delta / 2.0f)) / delta;
            float dg = (((maxval - c.g) / 6.0f) + (delta / 2.0f)) / delta;
            float db = (((maxval - c.b) / 6.0f) + (delta / 2.0f)) / delta;
            if (c.r == maxval) h = db - dg;
            else if (c.g == maxval) h = (1.0f / 3.0f) + dr - db;
            else if (c.b == maxval) h = (2.0f / 3.0f) + dg - dr;
            if (h < 0.0f) h += 1.0f;
            if (h > 1.0f) h -= 1.0f;
        }
        c = glm::fvec4(h, s, v, c.a);
        return c;
    }

    glm::fvec4& hsv2rgb_in_place(glm::fvec4& c)
    {
        float h = c[0], s = c[1], v = c[2];
        if (s == 0.0f) {
            c.r = c.g = c.b = 1.0f;
        }
        else {
            float vh = h * 6.0f;
            float vi = floor(vh);
            float v1 = v * (1.0f - s);
            float v2 = v * (1.0f - s * (vh - vi));
            float v3 = v * (1.0f - s * (1.0f - (vh - vi)));
            float vr, vg, vb;
            if (vi == 0.0f) { vr = v, vg = v3, vb = v1; }
            else if (vi == 1.0f) { vr = v2, vg = v, vb = v1; }
            else if (vi == 2.0f) { vr = v1, vg = v, vb = v3; }
            else if (vi == 3.0f) { vr = v1, vg = v2, vb = v; }
            else if (vi == 4.0f) { vr = v3, vg = v1, vb = v; }
            else { vr = v, vg = v1, vb = v2; }
            c = glm::fvec4(vr, vg, vb, c.a);
        }
        return c;
    }

    float hue2rgb(float v1, float v2, float vH)
    {
        if (vH < 0.0f) vH += 1.0f;
        if (vH > 1.0f) vH -= 1.0f;
        if ((6.0f * vH) < 1.0f) return (v1 + (v2 - v1) * 6.0f * vH);
        if ((2.0f * vH) < 1.0f) return (v2);
        if ((3.0f * vH) < 2.0f) return (v1 + (v2 - v1) * ((2.0f / 3.0f) - vH) * 6.0f);
        return (v1);
    }

    glm::fvec4& hsl2rgb_in_place(glm::fvec4& c)
    {
        float H = c.x;
        float S = c.y;
        float L = c.z;

        float R, G, B;
        if (S == 0) //HSL values = 0 - 1
        {
            R = L; //RGB results = 0 - 1
            G = L;
            B = L;
        }
        else
        {
            float var_2, var_1;
            if (L < 0.5)
                var_2 = L * (1 + S);
            else
                var_2 = (L + S) - (S * L);

            var_1 = 2 * L - var_2;

            R = hue2rgb(var_1, var_2, H + (1.0f / 3.0f));
            G = hue2rgb(var_1, var_2, H);
            B = hue2rgb(var_1, var_2, H - (1.0f / 3.0f));
        }
        c.r = R;
        c.g = G;
        c.b = B;

        return c;
    }
}

const Color Color::White    ( 0xffffffff, Color::RGBA );
const Color Color::Silver   ( 0xc0c0c0ff, Color::RGBA );
const Color Color::Gray     ( 0x808080ff, Color::RGBA );
const Color Color::Black    ( 0x000000ff, Color::RGBA );
const Color Color::Red      ( 0xff0000ff, Color::RGBA );
const Color Color::Maroon   ( 0x800000ff, Color::RGBA );
const Color Color::Yellow   ( 0xffff00ff, Color::RGBA );
const Color Color::Olive    ( 0x808000ff, Color::RGBA );
const Color Color::Lime     ( 0x00ff00ff, Color::RGBA );
const Color Color::Green    ( 0x008000ff, Color::RGBA );
const Color Color::Aqua     ( 0x00ffffff, Color::RGBA );
const Color Color::Teal     ( 0x008080ff, Color::RGBA );
const Color Color::Blue     ( 0x0000ffff, Color::RGBA );
const Color Color::Navy     ( 0x000080ff, Color::RGBA );
const Color Color::Fuchsia  ( 0xff00ffff, Color::RGBA );
const Color Color::Purple   ( 0x800080ff, Color::RGBA );
const Color Color::Orange   ( 0xffa500ff, Color::RGBA );

const Color Color::DarkGray ( 0x404040ff, Color::RGBA );
const Color Color::Magenta  ( 0xc000c0ff, Color::RGBA );
const Color Color::Cyan     ( 0x00ffffff, Color::RGBA );
const Color Color::Brown    ( 0xaa5500ff, Color::RGBA );
const Color Color::Transparent(0x00000000,Color::RGBA);

Color::Color(std::uint32_t v, Format format)
{
    if (format == RGBA)
    {
        r = (float)((v >> 24)) / 255.0f;
        g = (float)((v & 0xFF0000) >> 16) / 255.0f;
        b = (float)((v & 0xFF00) >> 8) / 255.0f;
        a = (float)((v & 0xFF) / 255.0f );
    }
    else // format == ABGR
    {
        r = (float)((v & 0xFF)) / 255.0f;
        g = (float)((v & 0xFF00) >> 8) / 255.0f;
        b = (float)((v & 0xFF0000) >> 16) / 255.0f;
        a = (float)((v >> 24) / 255.0f);
    }
}

Color::Color(const Color& rhs, float alpha) :
    glm::fvec4(rhs)
{
    a = alpha;
}

/** Parses a hex color string ("#rrggbb", "#rrggbbaa", "0xrrggbb", etc.) into an OSG color. */
Color::Color(const std::string& input, Format format)
{
    // ascii-only toLower + trim:
    std::string t;
    t.reserve(input.size());
    for(auto c : input)
        if (!std::isspace(c))
            t += (c >= 'A' && c <= 'Z') ? (c | 0x20) : c;

    if (util::startsWith(t, "rgb("))
    {
        std::string sub = t.substr(4, t.size() - 5);
        auto components = util::StringTokenizer()
            .delim(",")
            .tokenize(sub);
        if (components.size() == 3)
        {
            unsigned R = std::atoi(components[0].c_str());
            unsigned G = std::atoi(components[1].c_str());
            unsigned B = std::atoi(components[2].c_str());
            set((float)R / 255.0f, (float)G / 255.0f, (float)B / 255.0f, 1.0f);
        }
    }
    else if (util::startsWith(t, "rgba("))
    {
        std::string sub = t.substr(5, t.size() - 6);
        auto components = util::StringTokenizer()
            .delim(",")
            .tokenize(sub);
        if (components.size() == 4)
        {
            unsigned R = std::atoi(components[0].c_str());
            unsigned G = std::atoi(components[1].c_str());
            unsigned B = std::atoi(components[2].c_str());
            float A = std::atof(components[3].c_str());
            set((float)R / 255.0f, (float)G / 255.0f, (float)B / 255.0f, (float)A);
        }
    }
    else if (util::startsWith(t, "hsl("))
    {
        std::string sub = t.substr(4, t.size() - 5);
        auto components = util::StringTokenizer()
            .delim(",")
            .tokenize(sub);
        if (components.size() == 3)
        {
            float H = std::atof(components[0].c_str());
            float S = 0.0f;
            if (util::endsWith(components[1], "%"))
            {
                std::string sub = components[1].substr(0, components[1].size() - 1);
                S = std::atof(sub.c_str());
            }
            else
            {
                S = std::atof(components[1].c_str());
            }
            float L = 0.0f;
            if (util::endsWith(components[2], "%"))
            {
                std::string sub = components[2].substr(0, components[2].size() - 1);
                L = std::atof(sub.c_str());
            }
            else
            {
                L = std::atof(components[2].c_str());
            }
            set(H / 255.0f, S / 100.0f, L / 100.0f, 1.0f);
            hsl2rgb_in_place(*this);
        }
    }
    else if (util::startsWith(t, "hsla("))
    {
        std::string sub = t.substr(5, t.size() - 6);
        auto components = util::StringTokenizer()
            .delim(",")
            .tokenize(sub);
        if (components.size() == 4)
        {
            float H = std::atof(components[0].c_str());
            float S = 0.0f;
            if (util::endsWith(components[1], "%"))
            {
                std::string sub = components[1].substr(0, components[1].size() - 1);
                S = std::atof(sub.c_str());
            }
            else
            {
                S = std::atof(components[1].c_str());
            }
            float L = 0.0f;
            if (util::endsWith(components[2], "%"))
            {
                std::string sub = components[2].substr(0, components[2].size() - 1);
                L = std::atof(sub.c_str());
            }
            else
            {
                L = std::atof(components[2].c_str());
            }
            float A = std::atof(components[3].c_str());
            set(H / 255.0f, S / 100.0f, L / 100.0f, A);
            hsl2rgb_in_place(*this);
        }
    }
    else
    {
        glm::u8vec4 c(0, 0, 0, 255);

        unsigned e =
            t.size() >= 2 && t[0] == '0' && t[1] == 'x' ? 2 :
            t.size() >= 1 && t[0] == '#' ? 1 :
            0;
        unsigned len = (unsigned)t.length() - e;
        if (len == 3)
        {
            // This is a 3 digit hex code, so turn it into a 6 digit hex code
            std::stringstream buf;
            buf << t[e + 0] << t[e + 0] << t[e + 1] << t[e + 1] << t[e + 2] << t[e + 2];
            t = buf.str();
            len = 6;
            e = 0;
        }

        if (len >= 6) {
            c.r |= t[e + 0] <= '9' ? (t[e + 0] - '0') << 4 : (10 + (t[e + 0] - 'a')) << 4;
            c.r |= t[e + 1] <= '9' ? (t[e + 1] - '0') : (10 + (t[e + 1] - 'a'));
            c.g |= t[e + 2] <= '9' ? (t[e + 2] - '0') << 4 : (10 + (t[e + 2] - 'a')) << 4;
            c.g |= t[e + 3] <= '9' ? (t[e + 3] - '0') : (10 + (t[e + 3] - 'a'));
            c.b |= t[e + 4] <= '9' ? (t[e + 4] - '0') << 4 : (10 + (t[e + 4] - 'a')) << 4;
            c.b |= t[e + 5] <= '9' ? (t[e + 5] - '0') : (10 + (t[e + 5] - 'a'));
            if (len >= 8) {
                c.a = 0;
                c.a |= t[e + 6] <= '9' ? (t[e + 6] - '0') << 4 : (10 + (t[e + 6] - 'a')) << 4;
                c.a |= t[e + 7] <= '9' ? (t[e + 7] - '0') : (10 + (t[e + 7] - 'a'));
            }
        }

        float W = ((float)c.r) / 255.0f;
        float X = ((float)c.g) / 255.0f;
        float Y = ((float)c.b) / 255.0f;
        float Z = ((float)c.a) / 255.0f;

        if (format == RGBA)
            set(W, X, Y, Z);
        else // ABGR
            set(Z, Y, X, W);
    }
}

void
Color::set(float in_r, float in_g, float in_b, float in_a)
{
    r = in_r, g = in_g, b = in_b, a = in_a;
}

/** Makes an HTML color ("#rrggbb" or "#rrggbbaa") from an OSG color. */
std::string
Color::toHTML(Format format) const
{
    float f[4];
    if (format == RGBA)
        f[0] = r, f[1] = g, f[2] = b, f[3] = a;
    else
        f[0] = a, f[1] = b, f[2] = g, f[3] = r;

    return util::make_string()
        << "#"
        << std::hex << std::setw(2) << std::setfill('0') << (int)(f[0] * 255.0f)
        << std::hex << std::setw(2) << std::setfill('0') << (int)(f[1] * 255.0f)
        << std::hex << std::setw(2) << std::setfill('0') << (int)(f[2] * 255.0f)
        << std::hex << std::setw(2) << std::setfill('0') << (int)(f[3] * 255.0f);
}

Color
Color::brightness(float perc) const
{
    return Color(r*perc, g*perc, b*perc, a);
}

unsigned
Color::as(Format format) const
{
    if (format == RGBA)
    {
        return
            (((unsigned)(r*255.0)) << 24) |
            (((unsigned)(g*255.0)) << 16) |
            (((unsigned)(b*255.0)) << 8) |
            (((unsigned)(a*255.0)));
    }
    else // format == ABGR
    {
        return
            (((unsigned)(a*255.0)) << 24) |
            (((unsigned)(b*255.0)) << 16) |
            (((unsigned)(g*255.0)) << 8) |
            (((unsigned)(r*255.0)));
    }
}

glm::fvec4
Color::asHSL() const
{
    static const glm::fvec4 K(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    glm::fvec4 A(b, g, K.w, K.z);
    glm::fvec4 B(g, b, K.x, K.y);
    glm::fvec4 p = glm::mix(A, B, step(b, g));
    A = glm::fvec4(p.x, p.y, p.w, r);
    B = glm::fvec4(r, p.y, p.z, p.x);
    glm::fvec4 q = glm::mix(A, B, step(p.x, r));
    float d = q.x - std::min(q.w, q.y);
    const float e = 1.0e-10f;
    return glm::fvec4(
        fabs(q.z + (q.w - q.y) / (6.0f*d + e)),
        d / (q.x + e),
        q.x,
        a);
}

void
Color::fromHSL(const glm::fvec4& hsl)
{
    set(hsl[0], hsl[1], hsl[2], a);
    float H = x, S = y, V = z;
    if (S == 0.0f) {
        set(1.0f, 1.0f, 1.0f, a);
    }
    else {
        float VH = H * 6.0f;
        float VI = floor(VH);
        float V1 = V * (1.0f - S);
        float V2 = V * (1.0f - S * (VH - VI));
        float V3 = V * (1.0f - S * (1.0f - (VH - VI)));
        float VR, VG, VB;
        if (VI == 0.0f) { VR = V, VG = V3, VB = V1; }
        else if (VI == 1.0f) { VR = V2, VG = V, VB = V1; }
        else if (VI == 2.0f) { VR = V1, VG = V, VB = V3; }
        else if (VI == 3.0f) { VR = V1, VG = V2, VB = V; }
        else if (VI == 4.0f) { VR = V3, VG = V1, VB = V; }
        else { VR = V, VG = V1, VB = V2; }
        set(VR, VG, VB, a);
    }
}

glm::u8vec4
Color::asNormalizedRGBA() const
{
    return glm::u8vec4(
        (char)(r * 255.0),
        (char)(g * 255.0),
        (char)(b * 255.0),
        (char)(a * 255.0));
}

std::vector<Color>
Color::createRandomColorRamp(unsigned count, int seed)
{
    // Code is adapted from QGIS random color ramp feature,
    // which found the idea here (http://basecase.org/env/on-rainbows)
    // of adding the "golden ratio" to the hue angle in order to
    // minimize hue overlap and repetition.

    constexpr int hueMin = 0;
    constexpr int hueMax = 360;
    constexpr float satMin = 0.5f;
    constexpr float satMax = 1.0f;
    constexpr float valMin = 0.5f;
    constexpr float valMax = 1.0f;

    std::mt19937 gen(seed >= 0 ? seed : 0);
    std::uniform_int_distribution<> prng(0, 360);
    
    float hueAngle = (float)prng(gen);// prng.next(360);
    glm::fvec4 hsv(0, 0, 0, 1);

    std::vector<Color> output;
    output.reserve(count);

    for (unsigned i = 0; i < count; ++i)
    {
        hueAngle = fmodf(hueAngle + 137.50776f, 360.0f);
        hsv[0] = hueAngle / 360.0f;
        hsv[1] = satMin + (float)prng(gen)*(satMax - satMin);
        hsv[2] = valMin + (float)prng(gen)*(valMax - valMin);        
        output.emplace_back(hsv2rgb_in_place(hsv));
    }

    return output;
}


#include "json.h"
namespace ROCKY_NAMESPACE
{
    void to_json(json& j, const Color& obj) {
        j = obj.toHTML();
    }

    void from_json(const json& j, Color& obj) {
        obj = Color(get_string(j));
    }
}
