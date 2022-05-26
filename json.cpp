#include "json.hpp"

namespace UrhoExtras
{

bool writeStringWithoutEnding(Urho3D::String const& str, Urho3D::Serializer& dest)
{
	return dest.Write(str.CString(), str.Length());
}

Urho3D::String escapeString(Urho3D::String const& str)
{
// TODO: Escape unicode characters!
	Urho3D::String result = str;
	result.Replace("\\", "\\\\");
	result.Replace("\n", "\\n");
	result.Replace("\"", "\\\"");
	result.Replace("\'", "\\\'");
	return result;
}

bool saveJson(Urho3D::JSONValue const& json, Urho3D::Serializer& dest)
{
	if (json.IsBool()) {
		if (json.GetBool()) {
			if (!writeStringWithoutEnding("true", dest)) return false;
		} else {
			if (!writeStringWithoutEnding("false", dest)) return false;
		}
	} else if (json.IsNull()) {
		if (!writeStringWithoutEnding("null", dest)) return false;
	} else if (json.IsNumber()) {
		if (json.GetNumberType() == Urho3D::JSONNT_NAN) {
			if (!writeStringWithoutEnding("null", dest)) return false;
		} else if (json.GetNumberType() == Urho3D::JSONNT_INT) {
			if (!writeStringWithoutEnding(Urho3D::String(json.GetInt()), dest)) return false;
		} else if (json.GetNumberType() == Urho3D::JSONNT_UINT) {
			if (!writeStringWithoutEnding(Urho3D::String(json.GetUInt()), dest)) return false;
		} else if (json.GetNumberType() == Urho3D::JSONNT_FLOAT_DOUBLE) {
			if (!writeStringWithoutEnding(Urho3D::String(json.GetDouble()), dest)) return false;
		}
	} else if (json.IsString()) {
		if (!dest.WriteByte('\"')) return false;
		if (!writeStringWithoutEnding(escapeString(json.GetString()), dest)) return false;
		if (!dest.WriteByte('\"')) return false;
	} else if (json.IsArray()) {
		if (!dest.WriteByte('[')) return false;
		Urho3D::JSONArray const& arr = json.GetArray();
		for (unsigned i = 0; i < arr.Size(); ++ i) {
			if (!saveJson(arr[i], dest)) return false;
			if (i < arr.Size() - 1) {
				if (!dest.WriteByte(',')) return false;
			}
		}
		if (!dest.WriteByte(']')) return false;
	} else if (json.IsObject()) {
		if (!dest.WriteByte('{')) return false;
		for (Urho3D::JSONObject::ConstIterator it = json.GetObject().Begin(); it != json.GetObject().End(); ++ it) {
			Urho3D::String const& key = it->first_;
			Urho3D::JSONValue const& value = it->second_;
			if (!dest.WriteByte('\"')) return false;
			if (!writeStringWithoutEnding(escapeString(key), dest)) return false;
			if (!writeStringWithoutEnding("\":", dest)) return false;
			if (!saveJson(value, dest)) return false;
			Urho3D::JSONObject::ConstIterator it_next = it;
			++ it_next;
			if (it_next != json.GetObject().End()) {
				if (!dest.WriteByte(',')) return false;
			}
		}
		if (!dest.WriteByte('}')) return false;
	} else {
		return false;
	}
	return true;
}

Urho3D::String getJsonString(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    Urho3D::Vector<Urho3D::String> const& allowed_values
)
{
    if (!json.Contains(key)) {
        throw JsonValidatorError(error_prefix + "Not found!");
    }
    if (!json.Get(key).IsString()) {
        throw JsonValidatorError(error_prefix + "Must be a string!");
    }
    Urho3D::String result = json.Get(key).GetString();
    if (!allowed_values.Empty() && !allowed_values.Contains(result)) {
        Urho3D::String error = error_prefix + "Must be a one of the following values: ";
        bool first = true;
        for (auto value : allowed_values) {
            if (first) {
                first = false;
            } else {
                error += ", ";
            }
            error += "\"" + value + "\"";
        }
        throw JsonValidatorError(error);
    }
    return result;
}

Urho3D::String getJsonStringIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    Urho3D::String const& default_value,
    Urho3D::Vector<Urho3D::String> const& allowed_values
)
{
    if (!json.Contains(key)) {
        return default_value;
    }
    return getJsonString(json, key, error_prefix, allowed_values);
}

bool getJsonBoolean(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix
)
{
    if (!json.Contains(key)) {
        throw JsonValidatorError(error_prefix + "Not found!");
    }
    if (!json.Get(key).IsBool()) {
        throw JsonValidatorError(error_prefix + "Must be true or false!");
    }
    return json.Get(key).GetBool();
}

bool getJsonBooleanIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    bool default_value
)
{
    if (!json.Contains(key)) {
        return default_value;
    }
    return getJsonBoolean(json, key, error_prefix);
}

int getJsonInt(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    int min_limit,
    int max_limit
)
{
    if (!json.Contains(key)) {
        throw JsonValidatorError(error_prefix + "Not found!");
    }
    if (!json.Get(key).IsNumber()) {
        throw JsonValidatorError(error_prefix + "Must be an integer!");
    }
    if (json.Get(key).GetNumberType() == Urho3D::JSONNT_NAN || json.Get(key).GetNumberType() == Urho3D::JSONNT_FLOAT_DOUBLE) {
        throw JsonValidatorError(error_prefix + "Must be an integer!");
    }
    int result = json.Get(key).GetInt();
    if (result < min_limit) {
        throw JsonValidatorError(error_prefix + "Must be " + Urho3D::String(min_limit) + " or bigger!");
    }
    if (result > max_limit) {
        throw JsonValidatorError(error_prefix + "Must be " + Urho3D::String(max_limit) + " or smaller!");
    }
    return result;
}

float getJsonFloat(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    float min_limit,
    float max_limit
)
{
    if (!json.Contains(key)) {
        throw JsonValidatorError(error_prefix + "Not found!");
    }
    if (!json.Get(key).IsNumber()) {
        throw JsonValidatorError(error_prefix + "Must be a float!");
    }
    if (json.Get(key).GetNumberType() == Urho3D::JSONNT_NAN) {
        throw JsonValidatorError(error_prefix + "Must be a float!");
    }
    float result = json.Get(key).GetFloat();
    if (result < min_limit) {
        throw JsonValidatorError(error_prefix + "Must be " + Urho3D::String(min_limit) + " or bigger!");
    }
    if (result > max_limit) {
        throw JsonValidatorError(error_prefix + "Must be " + Urho3D::String(max_limit) + " or smaller!");
    }
    return result;
}

float getJsonFloatIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    float default_value,
    float min_limit,
    float max_limit
)
{
    if (!json.Contains(key)) {
        return default_value;
    }
    return getJsonFloat(json, key, error_prefix, min_limit, max_limit);
}

Urho3D::JSONArray getJsonArray(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    unsigned min_size_limit,
    unsigned max_size_limit
)
{
    if (!json.Contains(key)) {
        throw JsonValidatorError(error_prefix + "Not found!");
    }
    if (!json.Get(key).IsArray()) {
        throw JsonValidatorError(error_prefix + "Must be an array!");
    }
    Urho3D::JSONArray result = json.Get(key).GetArray();
    if (min_size_limit == max_size_limit && result.Size() != min_size_limit) {
        throw JsonValidatorError(error_prefix + "Must have exactly " + Urho3D::String(min_size_limit) + " items!");
    }
    if (result.Size() < min_size_limit) {
        throw JsonValidatorError(error_prefix + "Must have at least " + Urho3D::String(min_size_limit) + " items!");
    }
    if (result.Size() > max_size_limit) {
        throw JsonValidatorError(error_prefix + "Must have a maximum of " + Urho3D::String(min_size_limit) + " items!");
    }
    return result;
}

Urho3D::JSONArray getJsonArrayIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    unsigned min_size_limit,
    unsigned max_size_limit
)
{
    if (!json.Contains(key)) {
        return Urho3D::JSONArray();
    }
    return getJsonArray(json, key, error_prefix, min_size_limit, max_size_limit);
}

Urho3D::Vector2 getJsonVector2(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    Urho3D::Vector2 const& min_limit,
    Urho3D::Vector2 const& max_limit
)
{
    if (!json.Contains(key)) {
        throw JsonValidatorError(error_prefix + "Not found!");
    }
    if (!json.Get(key).IsArray()) {
        throw JsonValidatorError(error_prefix + "Must be an array!");
    }
    Urho3D::JSONArray array = json.Get(key).GetArray();
    if (array.Size() != 2) {
        throw JsonValidatorError(error_prefix + "Must have exactly two numbers!");
    }
    if (!array[0].IsNumber() || !array[1].IsNumber()) {
        throw JsonValidatorError(error_prefix + "Must have exactly two numbers!");
    }
    Urho3D::Vector2 result = Urho3D::Vector2(array[0].GetFloat(), array[1].GetFloat());
    if (result.x_ < min_limit.x_) {
        throw JsonValidatorError(error_prefix + "Must have an X component " + Urho3D::String(min_limit.x_) + " or bigger!");
    }
    if (result.x_ > max_limit.x_) {
        throw JsonValidatorError(error_prefix + "Must have an X component " + Urho3D::String(max_limit.x_) + " or smaller!");
    }
    if (result.y_ < min_limit.y_) {
        throw JsonValidatorError(error_prefix + "Must have an Y component " + Urho3D::String(min_limit.y_) + " or bigger!");
    }
    if (result.y_ > max_limit.y_) {
        throw JsonValidatorError(error_prefix + "Must have an Y component " + Urho3D::String(max_limit.y_) + " or smaller!");
    }
    return result;
}

Urho3D::Color getJsonColor(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix
)
{
    if (!json.Contains(key)) {
        throw JsonValidatorError(error_prefix + "Not found!");
    }
    if (json.Get(key).IsArray()) {
        Urho3D::JSONArray array = json.Get(key).GetArray();
        if (array.Size() < 3 || array.Size() > 4) {
            throw JsonValidatorError(error_prefix + "Must have three or four numbers between one and zero!");
        }
        if (!array[0].IsNumber() || !array[1].IsNumber() || !array[2].IsNumber()) {
            throw JsonValidatorError(error_prefix + "Must have three or four numbers between one and zero!");
        }
        float r = array[0].GetFloat();
        float g = array[1].GetFloat();
        float b = array[2].GetFloat();
        float a = array.Size() == 4 ? array[3].GetFloat() : 1.0f;
        if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1 || a < 0 || a > 1) {
            throw JsonValidatorError(error_prefix + "Must have three or four numbers between one and zero!");
        }
        return Urho3D::Color(r, g, b, a);
    }
    if (json.Get(key).IsString()) {
        Urho3D::String color_str = json.Get(key).GetString();
        if (color_str.Length() != 7 && color_str.Length() != 9) {
            throw JsonValidatorError(error_prefix + "Must be #rrggbb or #rrggbbaa!");
        }
        if (color_str[0] != '#') {
            throw JsonValidatorError(error_prefix + "Must be #rrggbb or #rrggbbaa!");
        }
        Urho3D::String hex_chars = "0123456789abcdefABCDEF";
        for (unsigned i = 1; i < color_str.Length(); ++ i) {
            if (hex_chars.Find(color_str[i]) == Urho3D::String::NPOS) {
                throw JsonValidatorError(error_prefix + "Must be #rrggbb or #rrggbbaa!");
            }
        }
        Urho3D::Color color;
        for (unsigned i = 1; i < color_str.Length(); i += 2) {
            Urho3D::String hex = color_str.Substring(i, 2);
            float value = ::strtol(hex.CString(), nullptr, 16) / 255.0f;
            if (i == 1) {
                color.r_ = value;
            } else if (i == 3) {
                color.g_ = value;
            } else if (i == 5) {
                color.b_ = value;
            } else {
                color.a_ = value;
            }
        }
        return color;
    }
    throw JsonValidatorError(error_prefix + "Must be an array or string!");
}

Urho3D::Color getJsonColorIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix,
    Urho3D::Color const& default_value
)
{
    if (!json.Contains(key)) {
        return default_value;
    }
    return getJsonColor(json, key, error_prefix);
}

}
