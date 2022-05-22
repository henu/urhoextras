#ifndef URHOEXTRAS_GRAPHICS_TERRAINGRID_HPP
#define URHOEXTRAS_GRAPHICS_TERRAINGRID_HPP

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Scene/Component.h>
#include <cstdint>

namespace UrhoExtras
{

namespace Graphics
{

class TerrainGrid : public Urho3D::Component
{
    URHO3D_OBJECT(TerrainGrid, Urho3D::Component);

public:

    typedef Urho3D::PODVector<uint16_t> HeightData;
    typedef Urho3D::PODVector<uint8_t> WeightData;

    TerrainGrid(Urho3D::Context* context);
    virtual ~TerrainGrid();

    void addTexture(Urho3D::Image* tex_img);

    void setViewmask(unsigned viewmask);

    Urho3D::Vector3 getSize() const;
    Urho3D::IntVector2 getHeightmapSize() const;
    Urho3D::IntVector2 getTextureweightsSize() const;
    float getHeightmapSquareWidth() const;
    float getChunkWidth() const;
    unsigned getChunkHeightmapWidth() const;
    unsigned getChunkTextureweightsWidth() const;

    void generateFlatland(Urho3D::IntVector2 const& grid_size);

    void generateFromImages(Urho3D::Image* terrainweight, Urho3D::Image* heightmap, unsigned heightmap_blur = 0);

    void generateFromVectors(Urho3D::IntVector2 const& grid_size, HeightData heightmap, WeightData textureweights);

    void forgetSourceData();

    float getHeight(Urho3D::Vector3 const& world_pos) const;

    Urho3D::Vector3 getNormal(Urho3D::Vector3 const& world_pos) const;

    void getTerrainPatches(Urho3D::PODVector<Urho3D::TerrainPatch*>& result, Urho3D::Vector2 const& pos, float radius);

    void buildFromBuffers();

    static void registerObject(Urho3D::Context* context);

    void ApplyAttributes() override;

private:

    typedef Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Texture> > Textures;
    typedef Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Image> > TextureImages;

    typedef Urho3D::PODVector<Urho3D::Terrain*> Chunks;

    unsigned heightmap_width;
    float heightmap_square_width;
    float heightmap_step;

    unsigned texture_repeats;
    unsigned textureweight_width;

    Textures texs;
    TextureImages texs_images;

    Urho3D::IntVector2 grid_size;

    // This is the source when building
    HeightData heightmap;
    WeightData textureweights;

    unsigned viewmask;

    Chunks chunks;

    Urho3D::Terrain* getChunkAt(float x, float z) const;

    Urho3D::ResourceRefList getTexturesImagesAttr() const;
    void setTexturesImagesAttr(Urho3D::ResourceRefList const& value);

    Urho3D::PODVector<unsigned char> getHeightmapAttr() const;
    void setHeightmapAttr(Urho3D::PODVector<unsigned char>const& value);

    Urho3D::PODVector<unsigned char> getTextureweightsAttr() const;
    void setTextureweightsAttr(Urho3D::PODVector<unsigned char>const& value);
};

}

}

#endif
