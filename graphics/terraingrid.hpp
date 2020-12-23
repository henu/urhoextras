#ifndef URHOEXTRAS_GRAPHICS_TERRAINGRID_HPP
#define URHOEXTRAS_GRAPHICS_TERRAINGRID_HPP

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Scene/Component.h>

namespace UrhoExtras
{

namespace Graphics
{

class TerrainGrid : public Urho3D::Component
{
    URHO3D_OBJECT(TerrainGrid, Urho3D::Component);

public:

    TerrainGrid(Urho3D::Context* context);
    virtual ~TerrainGrid();

    void addTexture(Urho3D::Texture* tex);
    void addTexture(Urho3D::Image* tex_img);

    Urho3D::Vector3 getSize() const;
    Urho3D::IntVector2 getHeightmapSize() const;
    Urho3D::IntVector2 getTextureweightsSize() const;

    void generateFlatland(Urho3D::IntVector2 const& size);

    void generateFromImages(Urho3D::Image* terrainweight, Urho3D::Image* heightmap, unsigned heightmap_blur = 0);

    void forgetSourceData();

    float getHeight(Urho3D::Vector3 const& world_pos) const;

    Urho3D::Vector3 getNormal(Urho3D::Vector3 const& world_pos) const;

    bool Load(Urho3D::Deserializer& source) override;
    bool Save(Urho3D::Serializer& dest) const override;

    void buildFromBuffers();

    static void registerObject(Urho3D::Context* context);

private:

    typedef Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Texture> > Textures;

    typedef Urho3D::PODVector<Urho3D::Terrain*> Chunks;

    typedef Urho3D::PODVector<uint16_t> HeightData;
    typedef Urho3D::PODVector<uint8_t> WeightData;

    unsigned heightmap_width;
    float heightmap_square_width;
    float heightmap_step;

    unsigned texture_repeats;
    unsigned textureweight_width;

    Textures texs;

    Urho3D::IntVector2 grid_size;

    // This is the source when building
    HeightData heightmap;
    WeightData textureweights;

    Chunks chunks;

    Urho3D::Terrain* getChunkAt(float x, float z) const;
};

}

}

#endif
