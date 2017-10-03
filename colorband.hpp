#ifndef URHOEXTRAS_COLORBAND_HPP
#define URHOEXTRAS_COLORBAND_HPP

#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/MathDefs.h>
#include <map>

namespace UrhoExtras
{

class ColorBand
{

public:

	inline void set(float m, Urho3D::Color const& color)
	{
		colors[m] = color;
	}

	inline Urho3D::Color get(float m) const
	{
		Colors::const_iterator j = colors.upper_bound(m);
		if (j == colors.begin()) {
			return j->second;
		}
		Colors::const_iterator i = j;
		-- i;
		if (j == colors.end()) {
			return i->second;
		}
		m = Urho3D::InverseLerp(i->first, j->first, m);
		return i->second * (1 - m) + j->second * m;
	}

private:

	typedef std::map<float, Urho3D::Color> Colors;

	Colors colors;
};

}

#endif
