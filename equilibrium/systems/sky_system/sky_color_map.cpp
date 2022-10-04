#include "sky_color_map.h"
#include <map>
#include <bx/math.h>

static std::map<float, Color> sunLuminanceXYZTable = {
    {5.0f, {0.000000f, 0.000000f, 0.000000f}},     {7.0f, {12.703322f, 12.989393f, 9.100411f}},
    {8.0f, {13.202644f, 13.597814f, 11.524929f}},  {9.0f, {13.192974f, 13.597458f, 12.264488f}},
    {10.0f, {13.132943f, 13.535914f, 12.560032f}}, {11.0f, {13.088722f, 13.489535f, 12.692996f}},
    {12.0f, {13.067827f, 13.467483f, 12.745179f}}, {13.0f, {13.069653f, 13.469413f, 12.740822f}},
    {14.0f, {13.094319f, 13.495428f, 12.678066f}}, {15.0f, {13.142133f, 13.545483f, 12.526785f}},
    {16.0f, {13.201734f, 13.606017f, 12.188001f}}, {17.0f, {13.182774f, 13.572725f, 11.311157f}},
    {18.0f, {12.448635f, 12.672520f, 8.267771f}},  {20.0f, {0.000000f, 0.000000f, 0.000000f}},
};

// Precomputed luminance of sky in the zenith point in XYZ colorspace.
// Computed using code from Game Engine Gems, Volume One, chapter 15.
// Implementation based on Dr. Richard Bird model. This table is used for
// piecewise linear interpolation. Day/night transitions are highly inaccurate.
// The scale of luminance change in Day/night transitions is not preserved.
// Luminance at night was increased to eliminate need the of HDR render.
static std::map<float, Color> skyLuminanceXYZTable = {
    {0.0f, {0.308f, 0.308f, 0.411f}},           {1.0f, {0.308f, 0.308f, 0.410f}},
    {2.0f, {0.301f, 0.301f, 0.402f}},           {3.0f, {0.287f, 0.287f, 0.382f}},
    {4.0f, {0.258f, 0.258f, 0.344f}},           {5.0f, {0.258f, 0.258f, 0.344f}},
    {7.0f, {0.962851f, 1.000000f, 1.747835f}},  {8.0f, {0.967787f, 1.000000f, 1.776762f}},
    {9.0f, {0.970173f, 1.000000f, 1.788413f}},  {10.0f, {0.971431f, 1.000000f, 1.794102f}},
    {11.0f, {0.972099f, 1.000000f, 1.797096f}}, {12.0f, {0.972385f, 1.000000f, 1.798389f}},
    {13.0f, {0.972361f, 1.000000f, 1.798278f}}, {14.0f, {0.972020f, 1.000000f, 1.796740f}},
    {15.0f, {0.971275f, 1.000000f, 1.793407f}}, {16.0f, {0.969885f, 1.000000f, 1.787078f}},
    {17.0f, {0.967216f, 1.000000f, 1.773758f}}, {18.0f, {0.961668f, 1.000000f, 1.739891f}},
    {20.0f, {0.264f, 0.264f, 0.352f}},          {21.0f, {0.264f, 0.264f, 0.352f}},
    {22.0f, {0.290f, 0.290f, 0.386f}},          {23.0f, {0.303f, 0.303f, 0.404f}},
};

class DynamicValueController {
    typedef Color                      ValueType;
    typedef std::map<float, ValueType> KeyMap;

  public:
    DynamicValueController(const KeyMap &keymap) { m_keyMap = keymap; }

    ~DynamicValueController() {}

    ValueType GetValue(float time) const {
        typename KeyMap::const_iterator itUpper = m_keyMap.upper_bound(time + 1e-6f);
        typename KeyMap::const_iterator itLower = itUpper;
        --itLower;

        if (itLower == m_keyMap.end()) {
            return itUpper->second;
        }

        if (itUpper == m_keyMap.end()) {
            return itLower->second;
        }

        float            lowerTime = itLower->first;
        const ValueType &lowerVal  = itLower->second;
        float            upperTime = itUpper->first;
        const ValueType &upperVal  = itUpper->second;

        if (lowerTime == upperTime) {
            return lowerVal;
        }

        return interpolate(lowerTime, lowerVal, upperTime, upperVal, time);
    };

    void Clear() { m_keyMap.clear(); };

  private:
    ValueType interpolate(float lowerTime, const ValueType &lowerVal, float upperTime,
                          const ValueType &upperVal, float time) const {
        const float tt = (time - lowerTime) / (upperTime - lowerTime);

        bx::Vec3 low = {lowerVal.x, lowerVal.y, lowerVal.z};
        bx::Vec3 up  = {upperVal.x, upperVal.y, upperVal.z};

        const bx::Vec3 result = bx::lerp(low, up, tt);

        Color color;
        color.x = result.x;
        color.y = result.y;
        color.z = result.z;

        return color;
    };

    KeyMap m_keyMap;
};

static DynamicValueController m_sunLuminanceXYZ = DynamicValueController(sunLuminanceXYZTable);
static DynamicValueController m_skyLuminanceXYZ = DynamicValueController(skyLuminanceXYZTable);

Color getSunLuminanceXYZTableValue(float time) { return m_sunLuminanceXYZ.GetValue(time); }
Color getSkyLuminanceXYZTablefloat(float time) { return m_skyLuminanceXYZ.GetValue(time); }