#ifndef URHOEXTRAS_GRAPHICS_MARCHINGCUBES_HPP
#define URHOEXTRAS_GRAPHICS_MARCHINGCUBES_HPP

#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/VertexBuffer.h>

namespace UrhoExtras
{

namespace Graphics
{

class MarchingCubesChunk;

class MarchingCubes : public Urho3D::Component
{
    URHO3D_OBJECT(MarchingCubes, Urho3D::Component);

public:

    typedef Urho3D::PODVector<unsigned char> WeightMap;

    MarchingCubes(Urho3D::Context* context);

    float getCubeWidth() const;

    unsigned getChunkWidth() const;

    Urho3D::IntVector3 getChunksSize() const;

    uint8_t getPoint(Urho3D::IntVector3 const& pos) const;

    void setCubeWidth(float width);

    void setChunkWidth(unsigned width);

    void setChunksSize(Urho3D::IntVector3 const& size);

    void setMaterial(Urho3D::Material* mat);

    void setPoint(Urho3D::IntVector3 const& pos, uint8_t value);

    void static registerObject(Urho3D::Context* context);

    // Called after scene load or network update
    void ApplyAttributes() override;

private:

    typedef Urho3D::HashMap<Urho3D::IntVector3, MarchingCubesChunk*> Chunks;

    float cube_width;
    unsigned chunk_width;
    Urho3D::IntVector3 chunks_size;

    WeightMap wmap;

    Urho3D::Material* mat;

    bool some_chunks_dirty;
    bool all_chunks_dirty;

    Chunks chunks;

    void rebuildChunksIfNeeded();

    Urho3D::PODVector<unsigned char> getWeightmapAttr() const;
    void setWeightmapAttr(Urho3D::PODVector<unsigned char> const& value);

    Urho3D::ResourceRef getMaterialAttr() const;
    void setMaterialAttr(Urho3D::ResourceRef const& value);

    void getWeightMapChunk(WeightMap& result, Urho3D::IntVector3 const& begin, Urho3D::IntVector3 const& end, WeightMap const& source) const;
};

class MarchingCubesChunk : public Urho3D::Drawable
{
    URHO3D_OBJECT(MarchingCubesChunk, Urho3D::Drawable);

public:

    MarchingCubesChunk(Urho3D::Context* context);

    void setMaterial(Urho3D::Material* mat);

    void markRebuildingNeeded();

    bool isRebuildNeeded() const;

    void rebuild(MarchingCubes::WeightMap const& wmap, unsigned chunk_width, float cube_width);

protected:

    void OnWorldBoundingBoxUpdate() override;

private:

    struct Triangle
    {
        Urho3D::Vector3 poss[3];
        unsigned poss_nrms_i[3];
        bool temporary;

        inline Triangle(Urho3D::Vector3 const& pos0, Urho3D::Vector3 const& pos1, Urho3D::Vector3 const& pos2) :
            temporary(false)
        {
            poss[0] = pos0;
            poss[1] = pos1;
            poss[2] = pos2;
        }

        inline Urho3D::Vector3 getNormal() const
        {
            Urho3D::Vector3 diff1 = poss[1] - poss[0];
            Urho3D::Vector3 diff2 = poss[2] - poss[0];
            return diff1.CrossProduct(diff2).Normalized();
        }
    };
    typedef Urho3D::PODVector<Triangle> Triangles;

    struct PositionAndNormal
    {
        Urho3D::Vector3 pos;
        Urho3D::Vector3 normal;
        inline PositionAndNormal(Urho3D::Vector3 const& pos, Urho3D::Vector3 const& normal) :
            pos(pos),
            normal(normal)
        {
        }
    };
    typedef Urho3D::PODVector<PositionAndNormal> PositionsAndNormals;

    enum Rotation
    {
        ROT_NOTHING = 0,
        ROT_RIGHT_90,
        ROT_RIGHT_90_FORWARD_180,
        ROT_RIGHT_NEG90,
        ROT_RIGHT_NEG90_FORWARD_180,
        ROT_FORWARD_90,
        ROT_FORWARD_NEG90,
        ROT_FORWARD_180,
        ROT_UP_90,
        ROT_UP_180,
        ROT_UP_180_FORWARD_90,
        ROT_RIGHT_180_FORWARD_90,
        ROT_RIGHT_180,
        ROT_RIGHT_90_UP_180,
        ROT_UP_NEG90,
        ROT_RIGHT_90_FORWARD_90,
        ROT_RIGHT_90_UP_90,
        ROT_RIGHT_NEG90_FORWARD_NEG90,
        ROT_RIGHT_90_UP_NEG90,
        ROT_RIGHT_NEG90_FORWARD_90,
        ROT_RIGHT_90_FORWARD_NEG90,
        ROT_RIGHT_NEG90_UP_NEG90,
        ROT_RIGHT_NEG90_UP_90,
        ROT_RIGHT_180_UP_NEG90,
        ROT_RIGHT_180_UP_90,
        ROT_RIGHT_180_FORWARD_NEG90
    };

    typedef Triangles (*MakeFunction)(uint8_t const* corners, bool extra_faces);

    Urho3D::SharedPtr<Urho3D::Geometry> geometry;
    Urho3D::SharedPtr<Urho3D::VertexBuffer> vbuf;
    Urho3D::SharedPtr<Urho3D::IndexBuffer> ibuf;

    bool rebuild_needed;

    float total_width;

    static Triangles finalizeTriangles(bool temporary, bool flip_normals, float cube_width, int x, int y, int z, Rotation rot, MakeFunction func, uint8_t const* corners);

    static float getEdgeMultiplier(uint8_t corner_solid, uint8_t corner_empty);

    static Triangles makeOneCorner(uint8_t const* corners, bool extra_faces);
    static Triangles makeOneEdge(uint8_t const* corners, bool extra_faces);
    static Triangles makePlane(uint8_t const* corners, bool extra_faces);
    static Triangles makeBigHalfCorner(uint8_t const* corners, bool extra_faces);
    static Triangles makeHexagon(uint8_t const* corners, bool extra_faces);
    static Triangles makeCornerAndEdge(uint8_t const* corners, bool extra_faces);
    static Triangles makeZigZag1(uint8_t const* corners, bool extra_faces);
    static Triangles makeZigZag2(uint8_t const* corners, bool extra_faces);
    static Triangles makeTwoCorners(uint8_t const* corners, bool extra_faces);
    static Triangles makeOpposingCorners(uint8_t const* corners, bool extra_faces);
    static Triangles makeBigHalfCornerAndCorner(uint8_t const* corners, bool extra_faces);
    static Triangles makeTwoEdges(uint8_t const* corners, bool extra_faces);
    static Triangles makeThreeCorners(uint8_t const* corners, bool extra_faces);
    static Triangles makeFourCorners(uint8_t const* corners, bool extra_faces);

    static void addVertexToRawData(Urho3D::PODVector<float> const& vertex, Urho3D::PODVector<float>& vdata, Urho3D::PODVector<unsigned>& idata);
};

}

}

#endif
