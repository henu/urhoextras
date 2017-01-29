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

}
