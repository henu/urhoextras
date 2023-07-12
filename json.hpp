#ifndef URHOEXTRAS_JSON_HPP
#define URHOEXTRAS_JSON_HPP

#include <Urho3D/Core/Context.h> // TODO: Useless include? Bug in Urho3D?
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/IO/Serializer.h>

namespace UrhoExtras
{

class JsonValidatorError: public std::runtime_error
{

public:

    inline JsonValidatorError(Urho3D::String const& str) :
        std::runtime_error(str.CString())
    {
    }
};

bool saveJson(Urho3D::JSONValue const& json, Urho3D::Serializer& dest);

int jsonToInt(
    Urho3D::JSONValue const& json,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    int min_limit = Urho3D::M_MIN_INT,
    int max_limit = Urho3D::M_MAX_INT
);
float jsonToFloat(
    Urho3D::JSONValue const& json,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    float min_limit = -Urho3D::M_INFINITY,
    float max_limit = Urho3D::M_INFINITY
);

Urho3D::JSONValue getJsonObject(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY
);
Urho3D::JSONValue getJsonObjectIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    Urho3D::JSONValue const& default_value = Urho3D::JSONValue::emptyObject
);
Urho3D::String getJsonString(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    Urho3D::Vector<Urho3D::String> const& allowed_values = Urho3D::Vector<Urho3D::String>()
);
Urho3D::String getJsonStringIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    Urho3D::String const& default_value = Urho3D::String::EMPTY,
    Urho3D::Vector<Urho3D::String> const& allowed_values = Urho3D::Vector<Urho3D::String>()
);
bool getJsonBoolean(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY
);
bool getJsonBooleanIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    bool default_value = false
);
int getJsonInt(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    int min_limit = Urho3D::M_MIN_INT,
    int max_limit = Urho3D::M_MAX_INT
);
int getJsonIntIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    int default_value = 0,
    int min_limit = Urho3D::M_MIN_INT,
    int max_limit = Urho3D::M_MAX_INT
);
float getJsonFloat(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    float min_limit = -Urho3D::M_INFINITY,
    float max_limit = Urho3D::M_INFINITY
);
float getJsonFloatIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    float default_value = 0,
    float min_limit = -Urho3D::M_INFINITY,
    float max_limit = Urho3D::M_INFINITY
);
Urho3D::JSONArray getJsonArray(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    unsigned min_size_limit = 0,
    unsigned max_size_limit = Urho3D::M_MAX_UNSIGNED
);
Urho3D::JSONArray getJsonArrayIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    unsigned min_size_limit = 0,
    unsigned max_size_limit = Urho3D::M_MAX_UNSIGNED
);
Urho3D::Vector2 getJsonVector2(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    Urho3D::Vector2 const& min_limit = Urho3D::Vector2(-Urho3D::M_INFINITY, -Urho3D::M_INFINITY),
    Urho3D::Vector2 const& max_limit = Urho3D::Vector2(Urho3D::M_INFINITY, Urho3D::M_INFINITY)
);
Urho3D::Vector3 getJsonVector3(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    Urho3D::Vector3 const& min_limit = Urho3D::Vector3(-Urho3D::M_INFINITY, -Urho3D::M_INFINITY, -Urho3D::M_INFINITY),
    Urho3D::Vector3 const& max_limit = Urho3D::Vector3(Urho3D::M_INFINITY, Urho3D::M_INFINITY, Urho3D::M_INFINITY)
);
Urho3D::Vector4 getJsonVector4(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    Urho3D::Vector4 const& min_limit = Urho3D::Vector4(-Urho3D::M_INFINITY, -Urho3D::M_INFINITY, -Urho3D::M_INFINITY, -Urho3D::M_INFINITY),
    Urho3D::Vector4 const& max_limit = Urho3D::Vector4(Urho3D::M_INFINITY, Urho3D::M_INFINITY, Urho3D::M_INFINITY, Urho3D::M_INFINITY)
);
Urho3D::Color getJsonColor(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY
);
Urho3D::Color getJsonColorIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    Urho3D::Color const& default_value = Urho3D::Color::WHITE
);
Urho3D::PODVector<int> getJsonIntArray(
    Urho3D::JSONValue const& json,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    unsigned min_size_limit = 0,
    unsigned max_size_limit = Urho3D::M_MAX_UNSIGNED
);
Urho3D::PODVector<int> getJsonIntArray(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    unsigned min_size_limit = 0,
    unsigned max_size_limit = Urho3D::M_MAX_UNSIGNED
);
Urho3D::PODVector<int> getJsonIntArrayIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    unsigned min_size_limit = 0,
    unsigned max_size_limit = Urho3D::M_MAX_UNSIGNED,
    Urho3D::PODVector<int> const& default_value = Urho3D::PODVector<int>()
);
Urho3D::PODVector<float> getJsonFloatArray(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    unsigned min_size_limit = 0,
    unsigned max_size_limit = Urho3D::M_MAX_UNSIGNED
);
Urho3D::PODVector<float> getJsonFloatArrayIfExists(
    Urho3D::JSONValue const& json,
    Urho3D::String const& key,
    Urho3D::String const& error_prefix = Urho3D::String::EMPTY,
    unsigned min_size_limit = 0,
    unsigned max_size_limit = Urho3D::M_MAX_UNSIGNED,
    Urho3D::PODVector<float> const& default_value = Urho3D::PODVector<float>()
);

}

#endif
