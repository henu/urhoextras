#ifndef URHOEXTRAS_JSON_HPP
#define URHOEXTRAS_JSON_HPP

#include <Urho3D/Core/Context.h> // TODO: Useless include? Bug in Urho3D?
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/IO/Serializer.h>

namespace UrhoExtras
{

bool saveJson(Urho3D::JSONValue const& json, Urho3D::Serializer& dest);

}



#endif
