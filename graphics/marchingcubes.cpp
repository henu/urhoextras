#include "marchingcubes.hpp"

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Compression.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/ReplicationState.h>

float const DEFAULT_CUBE_WIDTH = 0.1;
unsigned const DEFAULT_CHUNK_WIDTH = 10;
Urho3D::IntVector3 const DEFAULT_CHUNKS_SIZE(10, 10, 10);

float const MERGE_VERTEX_THRESHOLD = 0.001;
float const MERGE_VERTEX_THRESHOLD_TO_2 = MERGE_VERTEX_THRESHOLD * MERGE_VERTEX_THRESHOLD;

namespace UrhoExtras
{

namespace Graphics
{

Urho3D::Quaternion const ROT_QUATERNIONS[26] = {
    Urho3D::Quaternion::IDENTITY,
    Urho3D::Quaternion(0.707107, 0.707107, 0, 0),
    Urho3D::Quaternion(0, 0, 0.707107, 0.707107),
    Urho3D::Quaternion(0.707107, -0.707107, 0, 0),
    Urho3D::Quaternion(0, 0, -0.707107, 0.707107),
    Urho3D::Quaternion(0.707107, 0, 0, 0.707107),
    Urho3D::Quaternion(0.707107, 0, 0, -0.707107),
    Urho3D::Quaternion(0, 0, 0, 1),
    Urho3D::Quaternion(0.707107, 0, 0.707107, 0),
    Urho3D::Quaternion(0, 0, 1, 0),
    Urho3D::Quaternion(0, -0.707107, 0.707107, 0),
    Urho3D::Quaternion(0, 0.707107, 0.707107, 0),
    Urho3D::Quaternion(0, 1, 0, 0),
    Urho3D::Quaternion(0, 0, 0.707107, -0.707107),
    Urho3D::Quaternion(0.707107, 0, -0.707107, 0),
    Urho3D::Quaternion(0.5, 0.5, 0.5, 0.5),
    Urho3D::Quaternion(0.5, 0.5, 0.5, -0.5),
    Urho3D::Quaternion(0.5, -0.5, 0.5, -0.5),
    Urho3D::Quaternion(0.5, 0.5, -0.5, 0.5),
    Urho3D::Quaternion(0.5, -0.5, -0.5, 0.5),
    Urho3D::Quaternion(0.5, 0.5, -0.5, -0.5),
    Urho3D::Quaternion(0.5, -0.5, -0.5, -0.5),
    Urho3D::Quaternion(0.5, -0.5, 0.5, 0.5),
    Urho3D::Quaternion(0, 0.707107, 0, 0.707107),
    Urho3D::Quaternion(0, 0.707107, 0, -0.707107),
    Urho3D::Quaternion(0, 0.707107, -0.707107, 0)
};


uint8_t ROT_CORNER_MAPPINGS[26][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {2, 3, 6, 7, 0, 1, 4, 5},
    {1, 0, 5, 4, 3, 2, 7, 6},
    {4, 5, 0, 1, 6, 7, 2, 3},
    {7, 6, 3, 2, 5, 4, 1, 0},
    {1, 3, 0, 2, 5, 7, 4, 6},
    {2, 0, 3, 1, 6, 4, 7, 5},
    {3, 2, 1, 0, 7, 6, 5, 4},
    {4, 0, 6, 2, 5, 1, 7, 3},
    {5, 4, 7, 6, 1, 0, 3, 2},
    {7, 5, 6, 4, 3, 1, 2, 0},
    {4, 6, 5, 7, 0, 2, 1, 3},
    {6, 7, 4, 5, 2, 3, 0, 1},
    {7, 6, 3, 2, 5, 4, 1, 0},
    {1, 5, 3, 7, 0, 4, 2, 6},
    {0, 2, 4, 6, 1, 3, 5, 7},
    {6, 2, 7, 3, 4, 0, 5, 1},
    {6, 4, 2, 0, 7, 5, 3, 1},
    {3, 7, 2, 6, 1, 5, 0, 4},
    {5, 7, 1, 3, 4, 6, 0, 2},
    {3, 1, 7, 5, 2, 0, 6, 4},
    {0, 4, 1, 5, 2, 6, 3, 7},
    {5, 1, 4, 0, 7, 3, 6, 2},
    {2, 6, 0, 4, 3, 7, 1, 5},
    {7, 3, 5, 1, 6, 2, 4, 0},
    {7, 5, 6, 4, 3, 1, 2, 0}
};

MarchingCubes::MarchingCubes(Urho3D::Context* context) :
    Urho3D::Component(context),
    cube_width(DEFAULT_CUBE_WIDTH),
    chunk_width(DEFAULT_CHUNK_WIDTH),
    chunks_size(DEFAULT_CHUNKS_SIZE),
    mat(nullptr),
    some_chunks_dirty(false),
    all_chunks_dirty(true)
{
    wmap.Resize(chunks_size.x_ * chunks_size.y_ * chunks_size.z_ * chunk_width * chunk_width * chunk_width, 0);
}

float MarchingCubes::getCubeWidth() const
{
    return cube_width;
}

unsigned MarchingCubes::getChunkWidth() const
{
    return chunk_width;
}

Urho3D::IntVector3 MarchingCubes::getChunksSize() const
{
    return chunks_size;
}

uint8_t MarchingCubes::getPoint(Urho3D::IntVector3 const& pos) const
{
    Urho3D::IntVector3 total_size = chunks_size * chunk_width;
    if (pos.x_ < 0 || pos.x_ >= total_size.x_ ||
        pos.y_ < 0 || pos.y_ >= total_size.y_ ||
        pos.z_ < 0 || pos.z_ >= total_size.z_) {
        URHO3D_LOGWARNING("Trying to get point outside the marching cubes region!");
        return 0xff;
    }
    return wmap[pos.x_ + pos.y_ * total_size.x_ + pos.z_ * total_size.x_ * total_size.y_];
}

void MarchingCubes::setCubeWidth(float width)
{
    if (cube_width != width) {
        cube_width = width;
        all_chunks_dirty = true;
// TODO: Try not to rebuild immediately!
        rebuildChunksIfNeeded();
        MarkNetworkUpdate();
    }
}

void MarchingCubes::setChunkWidth(unsigned width)
{
    if (chunk_width != width) {
        chunk_width = width;
        wmap.Resize(chunks_size.x_ * chunks_size.y_ * chunks_size.z_ * chunk_width * chunk_width * chunk_width, 0);
        all_chunks_dirty = true;
// TODO: Try not to rebuild immediately!
        rebuildChunksIfNeeded();
        MarkNetworkUpdate();
    }
}

void MarchingCubes::setChunksSize(Urho3D::IntVector3 const& size)
{
    if (chunks_size != size) {
        chunks_size = size;
        wmap.Resize(chunks_size.x_ * chunks_size.y_ * chunks_size.z_ * chunk_width * chunk_width * chunk_width, 0);
        all_chunks_dirty = true;
// TODO: Try not to rebuild immediately!
        rebuildChunksIfNeeded();
        MarkNetworkUpdate();
    }
}

void MarchingCubes::setMaterial(Urho3D::Material* mat)
{
    if (this->mat != mat) {
        this->mat = mat;

        // Set material to chunks
        for (auto i = chunks.Begin(); i != chunks.End(); ++ i) {
            i->second_->setMaterial(mat);
        }

        MarkNetworkUpdate();
    }
}

void MarchingCubes::setPoint(Urho3D::IntVector3 const& pos, uint8_t value)
{
    Urho3D::IntVector3 total_size = chunks_size * chunk_width;

    wmap[pos.x_ + pos.y_ * total_size.x_ + pos.z_ * total_size.x_ * total_size.y_] = value;
// TODO: Only invalidate some of the chunks!
    all_chunks_dirty = true;
    MarkNetworkUpdate();
}

void MarchingCubes::registerObject(Urho3D::Context* context)
{
    context->RegisterFactory<MarchingCubes>();
    context->RegisterFactory<MarchingCubesChunk>();

    URHO3D_ATTRIBUTE("Cube width", float, cube_width, DEFAULT_CUBE_WIDTH, Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Chunk width", unsigned, chunk_width, DEFAULT_CHUNK_WIDTH, Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Size in chunks", Urho3D::IntVector3, chunks_size, DEFAULT_CHUNKS_SIZE, Urho3D::AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Weightmap", getWeightmapAttr, setWeightmapAttr, Urho3D::PODVector<unsigned char>, Urho3D::Variant::emptyBuffer, Urho3D::AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Material", getMaterialAttr, setMaterialAttr, Urho3D::ResourceRef, Urho3D::ResourceRef(Urho3D::Material::GetTypeStatic()), Urho3D::AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Urho3D::Drawable);
}

void MarchingCubes::ApplyAttributes()
{
    rebuildChunksIfNeeded();
}

void MarchingCubes::rebuildChunksIfNeeded()
{
    if (!some_chunks_dirty && !all_chunks_dirty) return;

    if (!node_) {
        some_chunks_dirty = false;
        all_chunks_dirty = false;
        return;
    }

    // Build chunks
    WeightMap chunk_wmap;
    Urho3D::IntVector3 chunk_pos;
    for (chunk_pos.z_ = 0; chunk_pos.z_ < chunks_size.z_; ++ chunk_pos.z_) {
        for (chunk_pos.y_ = 0; chunk_pos.y_ < chunks_size.y_; ++ chunk_pos.y_) {
            for (chunk_pos.x_ = 0; chunk_pos.x_ < chunks_size.x_; ++ chunk_pos.x_) {
                // Get or create chunk and chunk node
                Urho3D::Node* chunk_node = nullptr;
                MarchingCubesChunk* chunk = nullptr;
                bool new_node = false;
                auto chunks_find = chunks.Find(chunk_pos);
                if (chunks_find != chunks.End()) {
                    chunk = chunks_find->second_;
                    chunk_node = chunk->GetNode();
                } else {
                    new_node = true;
                    // Create child as local and temporary, so it won't be serialized to network nor to disk.
                    chunk_node = node_->CreateTemporaryChild(Urho3D::String::EMPTY, Urho3D::LOCAL);
                    // Create the actual Chunk
                    chunk = chunk_node->CreateComponent<MarchingCubesChunk>();
                    chunk->setMaterial(mat);
                    chunks[chunk_pos] = chunk;
                }

                // Set position, if needed
                if (new_node || all_chunks_dirty) {
                    chunk_node->SetPosition(Urho3D::Vector3(
                        chunk_pos.x_ * chunk_width * cube_width,
                        chunk_pos.y_ * chunk_width * cube_width,
                        chunk_pos.z_ * chunk_width * cube_width
                    ));
                }

                if (all_chunks_dirty || chunk->isRebuildNeeded()) {
                    // Get a chunk of data from weightmap. This is a little bit more than
                    // the chunk volume, because it needs neighbor cubes for smooth surface.
                    Urho3D::IntVector3 begin = chunk_pos * chunk_width - Urho3D::IntVector3::ONE;
                    Urho3D::IntVector3 end = begin + Urho3D::IntVector3::ONE * (chunk_width + 3);
                    getWeightMapChunk(chunk_wmap, begin, end, wmap);

                    // Do the rebuilding
                    chunk->rebuild(chunk_wmap, chunk_width, cube_width);
                }
            }
        }
    }

    some_chunks_dirty = false;
    all_chunks_dirty = false;
}

Urho3D::PODVector<unsigned char> MarchingCubes::getWeightmapAttr() const
{
    Urho3D::MemoryBuffer wmap_buf(wmap);
    Urho3D::VectorBuffer wmap_compressed_vbuf;
    if (!Urho3D::CompressStream(wmap_compressed_vbuf, wmap_buf)) {
        throw std::runtime_error("Unable to compress MarchingCubes.wmap for attribute serialization!");
    }
    return wmap_compressed_vbuf.GetBuffer();
}

void MarchingCubes::setWeightmapAttr(Urho3D::PODVector<unsigned char> const& value)
{
    // Store old weightmap, so it is possible to compare what changed
    WeightMap wmap_old;
    wmap.Swap(wmap_old);

    // Decompress the new weightmap
    Urho3D::MemoryBuffer wmap_compressed_buf(value);
    Urho3D::VectorBuffer wmap_vbuf;
    if (!Urho3D::DecompressStream(wmap_vbuf, wmap_compressed_buf)) {
        throw std::runtime_error("Unable to decompress MarchingCubes.wmap for attribute deserialization!");
    }
// TODO: Make some kind of fast memory copy!
    wmap_vbuf.Seek(0);
    while (!wmap_vbuf.IsEof()) {
        wmap.Push(wmap_vbuf.ReadUByte());
    }

    // Now compare old and new weightmap, and check what chunks need rebuiding.
    // However, if all chunks are marked as dirty, then this can be skipped.
    if (all_chunks_dirty) {
        return;
    }
    WeightMap chunk_wmap_old;
    WeightMap chunk_wmap_new;
    Urho3D::IntVector3 chunk_pos;
    for (chunk_pos.z_ = 0; chunk_pos.z_ < chunks_size.z_; ++ chunk_pos.z_) {
        for (chunk_pos.y_ = 0; chunk_pos.y_ < chunks_size.y_; ++ chunk_pos.y_) {
            for (chunk_pos.x_ = 0; chunk_pos.x_ < chunks_size.x_; ++ chunk_pos.x_) {
                // Get chunk. If rebuilding is already needed for it, then skip it
                MarchingCubesChunk* chunk = chunks.Find(chunk_pos)->second_;
                if (chunk->isRebuildNeeded()) {
                    continue;
                }

                // Get a chunk of data from both old and new weightmaps. If they differ, then rebuilding is needed.
                Urho3D::IntVector3 begin = chunk_pos * chunk_width - Urho3D::IntVector3::ONE;
                Urho3D::IntVector3 end = begin + Urho3D::IntVector3::ONE * (chunk_width + 3);
                getWeightMapChunk(chunk_wmap_old, begin, end, wmap_old);
                getWeightMapChunk(chunk_wmap_new, begin, end, wmap);
                if (chunk_wmap_new != chunk_wmap_old) {
                    chunk->markRebuildingNeeded();
                    some_chunks_dirty = true;
                }
            }
        }
    }
}

Urho3D::ResourceRef MarchingCubes::getMaterialAttr() const
{
    return GetResourceRef(mat, Urho3D::Material::GetTypeStatic());
}

void MarchingCubes::setMaterialAttr(Urho3D::ResourceRef const& value)
{
    Urho3D::ResourceCache* resources = GetSubsystem<Urho3D::ResourceCache>();
    setMaterial(resources->GetResource<Urho3D::Material>(value.name_));
}

void MarchingCubes::getWeightMapChunk(WeightMap& result, Urho3D::IntVector3 const& begin, Urho3D::IntVector3 const& end, WeightMap const& source) const
{
// TODO: It would be nice to handle also situation when chunk is totally out of volume!
    Urho3D::IntVector3 size = end - begin;

    result.Clear();
    result.Reserve(size.x_ * size.y_ * size.z_);

    Urho3D::IntVector3 cubes_size = chunks_size * chunk_width;

    // If begin.z is negative, then skip some squares
    if (begin.z_ < 0) {
        for (int i = 0; i < size.x_ * size.y_ * -begin.z_; ++ i) {
            result.Push(255);
        }
    }

    // Loop
    for (int z = Urho3D::Max(0, begin.z_); z < Urho3D::Min(cubes_size.z_, end.z_); ++ z) {

        // If begin.y is negative, then skip some lines
        if (begin.y_ < 0) {
            for (int i = 0; i < size.x_ * -begin.y_; ++ i) {
                result.Push(255);
            }
        }

        // Loop
        for (int y = Urho3D::Max(0, begin.y_); y < Urho3D::Min(cubes_size.y_, end.y_); ++ y) {

            // If begin.x is negative, then skip some values
            if (begin.x_ < 0) {
                for (int i = 0; i < -begin.x_; ++ i) {
                    result.Push(255);
                }
            }

            // Loop
            unsigned ofs = Urho3D::Max(0, begin.x_) + y * cubes_size.x_ + z * cubes_size.x_ * cubes_size.y_;
            unsigned copy_amount = Urho3D::Min(cubes_size.x_, end.x_) - Urho3D::Max(0, begin.x_);
            result.Insert(result.End(), source.Begin() + ofs, source.Begin() + ofs + copy_amount);

            // If end.x was too big, then skip some values
            if (end.x_ > cubes_size.x_) {
                for (int i = 0; i < end.x_ - cubes_size.x_; ++ i) {
                    result.Push(255);
                }
            }

        }

        // If end.y was too big, then skip some lines
        if (end.y_ > cubes_size.y_) {
            for (int i = 0; i < size.x_ * (end.y_ - cubes_size.y_); ++ i) {
                result.Push(255);
            }
        }

    }

    // If end.z was too big, then skip some squares
    if (end.z_ > cubes_size.z_) {
        for (int i = 0; i < size.x_ * size.y_ * (end.z_ - cubes_size.z_); ++ i) {
            result.Push(255);
        }
    }

    assert(int(result.Size()) == size.x_ * size.y_ * size.z_);
}

MarchingCubesChunk::MarchingCubesChunk(Urho3D::Context* context) :
    Urho3D::Drawable(context, Urho3D::DRAWABLE_GEOMETRY),
    geometry(new Urho3D::Geometry(context)),
    vbuf(new Urho3D::VertexBuffer(context)),
    ibuf(new Urho3D::IndexBuffer(context)),
    rebuild_needed(true),
    total_width(0)
{
    geometry->SetVertexBuffer(0, vbuf);
    geometry->SetIndexBuffer(ibuf);
    batches_.Resize(1);
    batches_[0].geometry_ = geometry;
    batches_[0].geometryType_ = Urho3D::GEOM_STATIC_NOINSTANCING;
}

void MarchingCubesChunk::setMaterial(Urho3D::Material* mat)
{
    batches_[0].material_ = mat;
}

void MarchingCubesChunk::markRebuildingNeeded()
{
    rebuild_needed = true;
}

bool MarchingCubesChunk::isRebuildNeeded() const
{
    return rebuild_needed;
}

void MarchingCubesChunk::rebuild(MarchingCubes::WeightMap const& wmap, unsigned chunk_width, float cube_width)
{
    total_width = chunk_width * cube_width;

    // Chunk width with the extra margin
    unsigned cwe = chunk_width + 3;
    assert(wmap.Size() == cwe * cwe * cwe);

    Triangles tris;
    // Start by building all cubes. This also includes the extra cubes at edges,
    // that will only be used for smooth shading and are later discarded.
    unsigned ofs = 0;
    for (int z = -1; z < int(cwe - 2); z ++) {
        for (int y = -1; y < int(cwe - 2); y ++) {
            for (int x = -1; x < int(cwe - 2); x ++) {
                bool temporary = (x == -1 || y == -1 || z == -1 || x >= int(chunk_width) || y >= int(chunk_width) || z >= int(chunk_width));

                // Get corner values
                uint8_t corners[8] = {
                    wmap[ofs],
                    wmap[ofs + 1],
                    wmap[ofs + cwe],
                    wmap[ofs + 1 + cwe],
                    wmap[ofs + cwe * cwe],
                    wmap[ofs + 1 + cwe * cwe],
                    wmap[ofs + cwe + cwe * cwe],
                    wmap[ofs + 1 + cwe + cwe * cwe]
                };

                uint8_t mask = 0;
                for (unsigned i = 0; i < 8; ++ i) {
                    mask += int(corners[i] >= 128) << i;
                }
                switch (mask) {
                case 0x00:
                    // Empty
                    break;
                case 0x01:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeOneCorner, corners);
                    break;
                case 0x02:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_90, makeOneCorner, corners);
                    break;
                case 0x03:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeOneEdge, corners);
                    break;
                case 0x04:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_NEG90, makeOneCorner, corners);
                    break;
                case 0x05:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_NEG90, makeOneEdge, corners);
                    break;
                case 0x06:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0x07:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x08:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_180, makeOneCorner, corners);
                    break;
                case 0x09:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeTwoCorners, corners);
                    break;
                case 0x0A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_90, makeOneEdge, corners);
                    break;
                case 0x0B:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90, makeBigHalfCorner, corners);
                    break;
                case 0x0C:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_180, makeOneEdge, corners);
                    break;
                case 0x0D:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_180, makeBigHalfCorner, corners);
                    break;
                case 0x0E:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0x0F:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90, makePlane, corners);
                    break;
                case 0x10:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeOneCorner, corners);
                    break;
                case 0x11:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeOneEdge, corners);
                    break;
                case 0x12:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90, makeTwoCorners, corners);
                    break;
                case 0x13:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeBigHalfCorner, corners);
                    break;
                case 0x14:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeTwoCorners, corners);
                    break;
                case 0x15:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0x16:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_180, makeThreeCorners, corners);
                    break;
                case 0x17:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeHexagon, corners);
                    break;
                case 0x18:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeOpposingCorners, corners);
                    break;
                case 0x19:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0x1A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x1B:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90, makeZigZag1, corners);
                    break;
                case 0x1C:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_180, makeCornerAndEdge, corners);
                    break;
                case 0x1D:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeZigZag2, corners);
                    break;
                case 0x1E:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x1F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x20:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeOneCorner, corners);
                    break;
                case 0x21:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeTwoCorners, corners);
                    break;
                case 0x22:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeOneEdge, corners);
                    break;
                case 0x23:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0x24:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeOpposingCorners, corners);
                    break;
                case 0x25:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x26:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0x27:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeZigZag2, corners);
                    break;
                case 0x28:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0x29:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90, makeThreeCorners, corners);
                    break;
                case 0x2A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0x2B:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeHexagon, corners);
                    break;
                case 0x2C:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90, makeCornerAndEdge, corners);
                    break;
                case 0x2D:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_180, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x2E:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeZigZag1, corners);
                    break;
                case 0x2F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0x30:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeOneEdge, corners);
                    break;
                case 0x31:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x32:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeBigHalfCorner, corners);
                    break;
                case 0x33:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makePlane, corners);
                    break;
                case 0x34:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeCornerAndEdge, corners);
                    break;
                case 0x35:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeZigZag1, corners);
                    break;
                case 0x36:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x37:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x38:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x39:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x3A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeZigZag2, corners);
                    break;
                case 0x3B:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_180, makeBigHalfCorner, corners);
                    break;
                case 0x3C:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_NEG90, makeTwoEdges, corners);
                    break;
                case 0x3D:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180, makeCornerAndEdge, corners);
                    break;
                case 0x3E:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_180, makeCornerAndEdge, corners);
                    break;
                case 0x3F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180, makeOneEdge, corners);
                    break;
                case 0x40:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180, makeOneCorner, corners);
                    break;
                case 0x41:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0x42:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeOpposingCorners, corners);
                    break;
                case 0x43:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_180, makeCornerAndEdge, corners);
                    break;
                case 0x44:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeOneEdge, corners);
                    break;
                case 0x45:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0x46:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0x47:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_NEG90, makeZigZag1, corners);
                    break;
                case 0x48:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeTwoCorners, corners);
                    break;
                case 0x49:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeThreeCorners, corners);
                    break;
                case 0x4A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0x4B:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x4C:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0x4D:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180, makeHexagon, corners);
                    break;
                case 0x4E:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeZigZag2, corners);
                    break;
                case 0x4F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x50:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_NEG90, makeOneEdge, corners);
                    break;
                case 0x51:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x52:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x53:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeZigZag2, corners);
                    break;
                case 0x54:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x55:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_NEG90, makePlane, corners);
                    break;
                case 0x56:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x57:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0x58:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0x59:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x5A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeTwoEdges, corners);
                    break;
                case 0x5B:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0x5C:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeZigZag1, corners);
                    break;
                case 0x5D:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0x5E:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x5F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeOneEdge, corners);
                    break;
                case 0x60:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeTwoCorners, corners);
                    break;
                case 0x61:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_UP_NEG90, makeThreeCorners, corners);
                    break;
                case 0x62:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x63:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x64:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x65:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x66:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90, makeTwoEdges, corners);
                    break;
                case 0x67:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x68:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeThreeCorners, corners);
                    break;
                case 0x69:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeFourCorners, corners);
                    break;
                case 0x6A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x6B:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeThreeCorners, corners);
                    break;
                case 0x6C:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_UP_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x6D:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180, makeThreeCorners, corners);
                    break;
                case 0x6E:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x6F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0x70:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_180, makeBigHalfCorner, corners);
                    break;
                case 0x71:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeHexagon, corners);
                    break;
                case 0x72:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeZigZag1, corners);
                    break;
                case 0x73:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180, makeBigHalfCorner, corners);
                    break;
                case 0x74:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeZigZag2, corners);
                    break;
                case 0x75:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0x76:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0x77:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_NEG90, makeOneEdge, corners);
                    break;
                case 0x78:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_180, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x79:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeThreeCorners, corners);
                    break;
                case 0x7A:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0x7B:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90, makeTwoCorners, corners);
                    break;
                case 0x7C:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeCornerAndEdge, corners);
                    break;
                case 0x7D:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeTwoCorners, corners);
                    break;
                case 0x7E:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeOpposingCorners, corners);
                    break;
                case 0x7F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_180, makeOneCorner, corners);
                    break;
                case 0x80:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_180, makeOneCorner, corners);
                    break;
                case 0x81:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeOpposingCorners, corners);
                    break;
                case 0x82:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_NEG90, makeTwoCorners, corners);
                    break;
                case 0x83:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeCornerAndEdge, corners);
                    break;
                case 0x84:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90, makeTwoCorners, corners);
                    break;
                case 0x85:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0x86:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeThreeCorners, corners);
                    break;
                case 0x87:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_180, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x88:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_NEG90, makeOneEdge, corners);
                    break;
                case 0x89:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0x8A:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0x8B:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeZigZag2, corners);
                    break;
                case 0x8C:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180, makeBigHalfCorner, corners);
                    break;
                case 0x8D:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeZigZag1, corners);
                    break;
                case 0x8E:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeHexagon, corners);
                    break;
                case 0x8F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_180, makeBigHalfCorner, corners);
                    break;
                case 0x90:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0x91:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x92:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180, makeThreeCorners, corners);
                    break;
                case 0x93:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_UP_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x94:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_180, makeThreeCorners, corners);
                    break;
                case 0x95:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x96:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeFourCorners, corners);
                    break;
                case 0x97:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeThreeCorners, corners);
                    break;
                case 0x98:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x99:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90, makeTwoEdges, corners);
                    break;
                case 0x9A:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x9B:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x9C:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0x9D:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0x9E:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_UP_NEG90, makeThreeCorners, corners);
                    break;
                case 0x9F:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeTwoCorners, corners);
                    break;
                case 0xA0:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeOneEdge, corners);
                    break;
                case 0xA1:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0xA2:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xA3:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_UP_90, makeZigZag1, corners);
                    break;
                case 0xA4:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0xA5:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeTwoEdges, corners);
                    break;
                case 0xA6:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0xA7:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0xA8:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0xA9:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0xAA:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_90, makePlane, corners);
                    break;
                case 0xAB:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xAC:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeZigZag2, corners);
                    break;
                case 0xAD:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0xAE:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xAF:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_NEG90, makeOneEdge, corners);
                    break;
                case 0xB0:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xB1:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_NOTHING, makeZigZag2, corners);
                    break;
                case 0xB2:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180, makeHexagon, corners);
                    break;
                case 0xB3:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0xB4:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0xB5:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_90, makeCornerAndEdge, corners);
                    break;
                case 0xB6:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeThreeCorners, corners);
                    break;
                case 0xB7:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeTwoCorners, corners);
                    break;
                case 0xB8:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_NEG90, makeZigZag1, corners);
                    break;
                case 0xB9:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0xBA:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0xBB:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeOneEdge, corners);
                    break;
                case 0xBC:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_180, makeCornerAndEdge, corners);
                    break;
                case 0xBD:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeOpposingCorners, corners);
                    break;
                case 0xBE:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0xBF:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180, makeOneCorner, corners);
                    break;
                case 0xC0:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180, makeOneEdge, corners);
                    break;
                case 0xC1:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_90_UP_180, makeCornerAndEdge, corners);
                    break;
                case 0xC2:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180, makeCornerAndEdge, corners);
                    break;
                case 0xC3:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_90, makeTwoEdges, corners);
                    break;
                case 0xC4:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_FORWARD_180, makeBigHalfCorner, corners);
                    break;
                case 0xC5:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeZigZag2, corners);
                    break;
                case 0xC6:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0xC7:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0xC8:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xC9:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0xCA:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeZigZag1, corners);
                    break;
                case 0xCB:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeCornerAndEdge, corners);
                    break;
                case 0xCC:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_180, makePlane, corners);
                    break;
                case 0xCD:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeBigHalfCorner, corners);
                    break;
                case 0xCE:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xCF:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeOneEdge, corners);
                    break;
                case 0xD0:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0xD1:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeZigZag1, corners);
                    break;
                case 0xD2:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_180, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0xD3:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90, makeCornerAndEdge, corners);
                    break;
                case 0xD4:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeHexagon, corners);
                    break;
                case 0xD5:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0xD6:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90, makeThreeCorners, corners);
                    break;
                case 0xD7:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0xD8:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeZigZag2, corners);
                    break;
                case 0xD9:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0xDA:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0xDB:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeOpposingCorners, corners);
                    break;
                case 0xDC:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0xDD:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeOneEdge, corners);
                    break;
                case 0xDE:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90_UP_NEG90, makeTwoCorners, corners);
                    break;
                case 0xDF:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeOneCorner, corners);
                    break;
                case 0xE0:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xE1:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeBigHalfCornerAndCorner, corners);
                    break;
                case 0xE2:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_180_FORWARD_90, makeZigZag2, corners);
                    break;
                case 0xE3:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_180, makeCornerAndEdge, corners);
                    break;
                case 0xE4:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90, makeZigZag1, corners);
                    break;
                case 0xE5:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_NEG90, makeCornerAndEdge, corners);
                    break;
                case 0xE6:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeCornerAndEdge, corners);
                    break;
                case 0xE7:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeOpposingCorners, corners);
                    break;
                case 0xE8:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_NEG90, makeHexagon, corners);
                    break;
                case 0xE9:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_180, makeThreeCorners, corners);
                    break;
                case 0xEA:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_UP_90, makeBigHalfCorner, corners);
                    break;
                case 0xEB:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeTwoCorners, corners);
                    break;
                case 0xEC:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_180, makeBigHalfCorner, corners);
                    break;
                case 0xED:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_NEG90, makeTwoCorners, corners);
                    break;
                case 0xEE:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeOneEdge, corners);
                    break;
                case 0xEF:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_UP_90, makeOneCorner, corners);
                    break;
                case 0xF0:
                    tris += finalizeTriangles(temporary, false, cube_width, x, y, z, ROT_RIGHT_NEG90, makePlane, corners);
                    break;
                case 0xF1:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_90, makeBigHalfCorner, corners);
                    break;
                case 0xF2:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_180, makeBigHalfCorner, corners);
                    break;
                case 0xF3:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_180, makeOneEdge, corners);
                    break;
                case 0xF4:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90, makeBigHalfCorner, corners);
                    break;
                case 0xF5:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_90, makeOneEdge, corners);
                    break;
                case 0xF6:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeTwoCorners, corners);
                    break;
                case 0xF7:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_180, makeOneCorner, corners);
                    break;
                case 0xF8:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_RIGHT_90_FORWARD_NEG90, makeBigHalfCorner, corners);
                    break;
                case 0xF9:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_90, makeTwoCorners, corners);
                    break;
                case 0xFA:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_NEG90, makeOneEdge, corners);
                    break;
                case 0xFB:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_NEG90, makeOneCorner, corners);
                    break;
                case 0xFC:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeOneEdge, corners);
                    break;
                case 0xFD:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_FORWARD_90, makeOneCorner, corners);
                    break;
                case 0xFE:
                    tris += finalizeTriangles(temporary, true, cube_width, x, y, z, ROT_NOTHING, makeOneCorner, corners);
                    break;
                case 0xFF:
                    // Full
                    break;
                }

                ++ ofs;
            }
            ++ ofs;
        }
        ofs += cwe;
    }
    assert(ofs + cwe * cwe == wmap.Size());

    // Calculate normals for vertices, and if they are used my multiple Triangles, then smooth them.
    PositionsAndNormals poss_nrms;
    for (Triangle& tri : tris) {
        Urho3D::Vector3 normal = tri.getNormal();
        for (unsigned corner_i = 0; corner_i < 3; ++ corner_i) {
            Urho3D::Vector3 const& pos = tri.poss[corner_i];
            bool found = false;
            for (unsigned pos_nrm_i = 0; pos_nrm_i < poss_nrms.Size(); ++ pos_nrm_i) {
                PositionAndNormal& pos_nrm = poss_nrms[pos_nrm_i];
                if ((pos_nrm.pos - pos).LengthSquared() < MERGE_VERTEX_THRESHOLD_TO_2) {
                    found = true;
                    pos_nrm.normal += normal;
                    tri.poss_nrms_i[corner_i] = pos_nrm_i;
                    break;
                }
            }
            if (!found) {
                tri.poss_nrms_i[corner_i] = poss_nrms.Size();
                poss_nrms.Push(PositionAndNormal(pos, normal));
            }
        }
    }
    // Normalize all normals
    for (PositionAndNormal& pos_nrm : poss_nrms) {
        pos_nrm.normal.Normalize();
    }

    // Remove temporary triangles
    for (unsigned i = 0; i < tris.Size(); ) {
        Triangle& tri = tris[i];
        if (tri.temporary) {
            tris.EraseSwap(i);
        } else {
            ++ i;
        }
    }

    // Convert triangles to vertex and index data.
    Urho3D::PODVector<float> vdata_raw;
    Urho3D::PODVector<unsigned> idata_raw;
    Urho3D::PODVector<float> corner_vdata_raw;
    for (Triangle const& tri : tris) {
        // Precalculate some normal and tangent stuff
        Urho3D::Vector3 vrts_normals[3];
        Urho3D::Vector3 vrts_normals_avg;
        for (unsigned corner_i = 0; corner_i < 3; ++ corner_i) {
            Urho3D::Vector3 const& pos_nrm_normal = poss_nrms[tri.poss_nrms_i[corner_i]].normal;
            vrts_normals[corner_i] = pos_nrm_normal;
            vrts_normals_avg += pos_nrm_normal;
        }
        Urho3D::Vector3 normal = tri.getNormal();
        Urho3D::Vector3 tangent = (Urho3D::Vector3::RIGHT - normal * normal.DotProduct(Urho3D::Vector3::RIGHT)).Normalized();
        Urho3D::Vector3 normal_abs = Urho3D::VectorAbs(normal);
        // Loop corners
        for (unsigned corner_i = 0; corner_i < 3; ++ corner_i) {
            corner_vdata_raw.Clear();
            Urho3D::Vector3 const& pos = tri.poss[corner_i];
            // Position
            corner_vdata_raw.Push(pos.x_);
            corner_vdata_raw.Push(pos.y_);
            corner_vdata_raw.Push(pos.z_);
            // Normal
            Urho3D::Vector3 vrt_normal = vrts_normals[corner_i];
            corner_vdata_raw.Push(vrt_normal.x_);
            corner_vdata_raw.Push(vrt_normal.y_);
            corner_vdata_raw.Push(vrt_normal.z_);
            // Texture coordinates
            Urho3D::Vector2 texcoord;
            if (normal_abs.x_ > normal_abs.y_ && normal_abs.x_ > normal_abs.z_) {
                if (normal.x_ > 0) texcoord.x_ = pos.z_;
                else texcoord.x_ = -pos.z_;
                texcoord.y_ = -pos.y_;
            } else if (normal_abs.y_ > normal_abs.z_) {
                if (normal.y_ > 0) texcoord.x_ = -pos.x_;
                else texcoord.x_ = pos.x_;
                texcoord.y_ = pos.z_;
            } else {
                if (normal.z_ > 0) texcoord.x_ = -pos.x_;
                else texcoord.x_ = pos.x_;
                texcoord.y_ = -pos.y_;
            }
            corner_vdata_raw.Push(texcoord.x_);
            corner_vdata_raw.Push(texcoord.y_);
            // Tangent
            corner_vdata_raw.Push(tangent.x_);
            corner_vdata_raw.Push(tangent.y_);
            corner_vdata_raw.Push(tangent.z_);
            corner_vdata_raw.Push(1.0f);
            // Add to main containers
            addVertexToRawData(corner_vdata_raw, vdata_raw, idata_raw);
        }
    }

    // Create actual Vertex and IndexBuffers
    if (vbuf->GetVertexCount() != vdata_raw.Size() / 12) {
        vbuf->SetSize(vdata_raw.Size() / 12, Urho3D::MASK_POSITION | Urho3D::MASK_NORMAL | Urho3D::MASK_TEXCOORD1 | Urho3D::MASK_TANGENT);
    }
    if (ibuf->GetIndexCount() != idata_raw.Size()) {
        ibuf->SetSize(idata_raw.Size(), true);
    }
    if (!tris.Empty()) {
        float* vbuf_data = (float*)vbuf->Lock(0, vbuf->GetVertexCount());
        for (float f : vdata_raw) {
            *vbuf_data ++ = f;
        }
        vbuf->Unlock();
        vbuf->ClearDataLost();
        unsigned* ibuf_data = (unsigned*)ibuf->Lock(0, ibuf->GetIndexCount());
        for (unsigned i : idata_raw) {
            *ibuf_data ++ = i;
        }
        ibuf->Unlock();
        ibuf->ClearDataLost();
    }
    if (!geometry->SetDrawRange(Urho3D::TRIANGLE_LIST, 0, idata_raw.Size(), 0, vdata_raw.Size() / 12)) {
        throw std::runtime_error("Unable to set Geometry draw range!");
    }

    rebuild_needed = false;
}

void MarchingCubesChunk::OnWorldBoundingBoxUpdate()
{
    Urho3D::BoundingBox bb(Urho3D::Vector3::ZERO, Urho3D::Vector3::ONE * total_width);
    worldBoundingBox_ = bb.Transformed(node_->GetWorldTransform());
}

MarchingCubesChunk::Triangles MarchingCubesChunk::finalizeTriangles(bool temporary, bool flip_normals, float cube_width, int x, int y, int z, Rotation rot, MakeFunction func, uint8_t const* corners)
{
    // Rotate corners
    uint8_t corners_fixed[8];
    for (unsigned i = 0; i < 8; ++ i) {
        corners_fixed[i] = corners[ROT_CORNER_MAPPINGS[rot][i]];
    }

    // Flip values, if normal flipping is needed
    if (flip_normals) {
        for (unsigned i = 0; i < 8; ++ i) {
            corners_fixed[i] = 255 - corners_fixed[i];
        }
    }

    // Generate Triangles. If normals are flipped, then it means more faces are needed.
    Triangles tris = func(corners_fixed, flip_normals);

    // Prepare transformation that will be applied to Triangles
    Urho3D::Quaternion const& rot_q = ROT_QUATERNIONS[rot];
    Urho3D::Vector3 transl(-0.5, -0.5, -0.5);
    transl = rot_q * transl;
    transl += Urho3D::Vector3(0.5, 0.5, 0.5);
    transl += Urho3D::Vector3(x, y, z);
    transl = Urho3D::VectorRound(transl);
    transl *= cube_width;
    Urho3D::Matrix3x4 transf(transl, rot_q, cube_width);

    // Modify triangles
    for (Triangle& tri : tris) {
        tri.temporary = temporary;
        for (Urho3D::Vector3& pos : tri.poss) {
            assert(pos.x_ >= 0 && pos.x_ <= 1);
            assert(pos.y_ >= 0 && pos.y_ <= 1);
            assert(pos.z_ >= 0 && pos.z_ <= 1);
            pos = transf * pos;
        }
        if (flip_normals) {
            Urho3D::Swap(tri.poss[1], tri.poss[2]);
        }
    }

    return tris;
}

float MarchingCubesChunk::getEdgeMultiplier(uint8_t corner_solid, uint8_t corner_empty)
{
    assert(corner_solid >= 128);
    assert(corner_empty < 128);
    float result = (corner_solid + corner_empty - 127) / 256.0f;
    return result;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeOneCorner(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_00x = getEdgeMultiplier(corners[0], corners[1]);
    float em_x00 = getEdgeMultiplier(corners[0], corners[4]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(0, 0, em_x00)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeOneEdge(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_x00 = getEdgeMultiplier(corners[0], corners[4]);
    float em_0x1 = getEdgeMultiplier(corners[1], corners[3]);
    float em_x01 = getEdgeMultiplier(corners[1], corners[5]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(0, 0, em_x00),
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(0, em_0x0, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makePlane(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_0x1 = getEdgeMultiplier(corners[1], corners[3]);
    float em_1x0 = getEdgeMultiplier(corners[4], corners[6]);
    float em_1x1 = getEdgeMultiplier(corners[5], corners[7]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(1, em_1x1, 1),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeBigHalfCorner(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_00x = getEdgeMultiplier(corners[1], corners[0]);
    float em_x00 = getEdgeMultiplier(corners[4], corners[0]);
    float em_0x1 = getEdgeMultiplier(corners[1], corners[3]);
    float em_1x1 = getEdgeMultiplier(corners[5], corners[7]);
    float em_1x0 = getEdgeMultiplier(corners[4], corners[6]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(1 - em_00x, 0, 0),
        Urho3D::Vector3(0, 0, 1 - em_x00),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, 0, 1 - em_x00),
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, em_0x1, 0),
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeHexagon(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_00x = getEdgeMultiplier(corners[0], corners[1]);
    float em_x01 = getEdgeMultiplier(corners[5], corners[1]);
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_1x1 = getEdgeMultiplier(corners[5], corners[7]);
    float em_x10 = getEdgeMultiplier(corners[6], corners[2]);
    float em_11x = getEdgeMultiplier(corners[6], corners[7]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(1, em_1x1, 1),
        Urho3D::Vector3(1, 0, 1 - em_x01)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(em_11x, 1, 1),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(em_11x, 1, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(0, 1, 1 - em_x10),
        Urho3D::Vector3(em_11x, 1, 1)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeCornerAndEdge(uint8_t const* corners, bool extra_faces)
{
    float em_11x = 1 - getEdgeMultiplier(corners[7], corners[6]);
    float em_1x1 = 1 - getEdgeMultiplier(corners[7], corners[5]);
    float em_x11 = 1 - getEdgeMultiplier(corners[7], corners[3]);
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_x00 = getEdgeMultiplier(corners[0], corners[4]);
    float em_0x1 = getEdgeMultiplier(corners[1], corners[3]);
    float em_x01 = getEdgeMultiplier(corners[1], corners[5]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_11x, 1, 1),
        Urho3D::Vector3(1, 1, em_x11),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, 0, em_x00),
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(0, em_0x0, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    if (extra_faces) {
        tris.Push(Triangle(
            Urho3D::Vector3(1, 0, em_x01),
            Urho3D::Vector3(1, em_1x1, 1),
            Urho3D::Vector3(1, 1, em_x11)
        ));
        tris.Push(Triangle(
            Urho3D::Vector3(1, 0, em_x01),
            Urho3D::Vector3(1, 1, em_x11),
            Urho3D::Vector3(1, em_0x1, 0)
        ));
    }
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeZigZag1(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_00x = 1 - getEdgeMultiplier(corners[1], corners[0]);
    float em_x00 = 1 - getEdgeMultiplier(corners[4], corners[0]);
    float em_x10 = 1 - getEdgeMultiplier(corners[6], corners[2]);
    float em_11x = getEdgeMultiplier(corners[6], corners[7]);
    float em_0x1 = getEdgeMultiplier(corners[1], corners[3]);
    float em_1x1 = getEdgeMultiplier(corners[5], corners[7]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, 1, em_x10),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, 0, em_x00),
        Urho3D::Vector3(0, 1, em_x10)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(1, em_1x1, 1),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, 1, em_x10),
        Urho3D::Vector3(em_11x, 1, 1),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeZigZag2(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_00x = getEdgeMultiplier(corners[0], corners[1]);
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_x01 = 1 - getEdgeMultiplier(corners[5], corners[1]);
    float em_1x0 = getEdgeMultiplier(corners[4], corners[6]);
    float em_x11 = 1 - getEdgeMultiplier(corners[7], corners[3]);
    float em_11x = 1 - getEdgeMultiplier(corners[7], corners[6]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(1, 1, em_x11)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(0, em_1x0, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(1, 1, em_x11),
        Urho3D::Vector3(1, 0, em_x01)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(em_11x, 1, 1),
        Urho3D::Vector3(1, 1, em_x11)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeTwoCorners(uint8_t const* corners, bool extra_faces)
{
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_00x = getEdgeMultiplier(corners[0], corners[1]);
    float em_x00 = getEdgeMultiplier(corners[0], corners[4]);
    float em_01x = 1 - getEdgeMultiplier(corners[3], corners[2]);
    float em_0x1 = 1 - getEdgeMultiplier(corners[3], corners[1]);
    float em_x11 = getEdgeMultiplier(corners[3], corners[7]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(0, 0, em_x00)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, em_0x1, 0),
        Urho3D::Vector3(1, 1, em_x11),
        Urho3D::Vector3(em_01x, 1, 0)
    ));
    if (extra_faces) {
        tris.Push(Triangle(
            Urho3D::Vector3(em_00x, 0, 0),
            Urho3D::Vector3(1, em_0x1, 0),
            Urho3D::Vector3(em_01x, 1, 0)
        ));
        tris.Push(Triangle(
            Urho3D::Vector3(em_00x, 0, 0),
            Urho3D::Vector3(em_01x, 1, 0),
            Urho3D::Vector3(0, em_0x0, 0)
        ));
    }
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeOpposingCorners(uint8_t const* corners, bool extra_faces)
{
    (void)extra_faces;
    float em_00x = getEdgeMultiplier(corners[0], corners[1]);
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_x00 = getEdgeMultiplier(corners[0], corners[4]);
    float em_11x = 1 - getEdgeMultiplier(corners[7], corners[6]);
    float em_1x1 = 1 - getEdgeMultiplier(corners[7], corners[5]);
    float em_x11 = 1 - getEdgeMultiplier(corners[7], corners[3]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(0, 0, em_x00)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, 1, em_x11),
        Urho3D::Vector3(1, em_1x1, 1),
        Urho3D::Vector3(em_11x, 1, 1)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeBigHalfCornerAndCorner(uint8_t const* corners, bool extra_faces)
{
    float em_00x = 1 - getEdgeMultiplier(corners[1], corners[0]);
    float em_x00 = 1 - getEdgeMultiplier(corners[4], corners[0]);
    float em_0x1 = getEdgeMultiplier(corners[1], corners[3]);
    float em_1x1 = getEdgeMultiplier(corners[5], corners[7]);
    float em_1x0 = getEdgeMultiplier(corners[4], corners[6]);
    float em_0x0 = 1 - getEdgeMultiplier(corners[2], corners[0]);
    float em_01x = getEdgeMultiplier(corners[2], corners[3]);
    float em_x10 = getEdgeMultiplier(corners[2], corners[6]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, 0, em_x00),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, 0, em_x00),
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, em_0x1, 0),
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(em_01x, 1, 0),
        Urho3D::Vector3(0, 1, em_x10)
    ));
    if (extra_faces) {
        tris.Push(Triangle(
            Urho3D::Vector3(em_00x, 0, 0),
            Urho3D::Vector3(1, em_0x1, 0),
            Urho3D::Vector3(em_01x, 1, 0)
        ));
        tris.Push(Triangle(
            Urho3D::Vector3(em_00x, 0, 0),
            Urho3D::Vector3(em_01x, 1, 0),
            Urho3D::Vector3(0, em_0x0, 0)
        ));
        tris.Push(Triangle(
            Urho3D::Vector3(0, 0, em_x00),
            Urho3D::Vector3(0, 0, em_x00),
            Urho3D::Vector3(0, 1, em_x10)
        ));
        tris.Push(Triangle(
            Urho3D::Vector3(0, 0, em_x00),
            Urho3D::Vector3(0, 1, em_x10),
            Urho3D::Vector3(0, em_1x0, 1)
        ));
    }
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeTwoEdges(uint8_t const* corners, bool extra_faces)
{
// TODO: Is extra faces needed here?
    (void)extra_faces;
    float em_00x = getEdgeMultiplier(corners[0], corners[1]);
    float em_x00 = getEdgeMultiplier(corners[0], corners[4]);
    float em_01x = getEdgeMultiplier(corners[2], corners[3]);
    float em_x10 = getEdgeMultiplier(corners[2], corners[6]);
    float em_x01 = 1 - getEdgeMultiplier(corners[5], corners[1]);
    float em_10x = 1 - getEdgeMultiplier(corners[5], corners[4]);
    float em_x11 = 1 - getEdgeMultiplier(corners[7], corners[3]);
    float em_11x = 1 - getEdgeMultiplier(corners[7], corners[6]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(em_11x, 1, 1),
        Urho3D::Vector3(1, 1, em_x11)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(em_10x, 0, 1),
        Urho3D::Vector3(em_11x, 1, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(em_01x, 1, 0),
        Urho3D::Vector3(0, 1, em_x10)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, 1, em_x10),
        Urho3D::Vector3(0, 0, em_x00)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeThreeCorners(uint8_t const* corners, bool extra_faces)
{
// TODO: Is extra faces needed here?
    (void)extra_faces;
    float em_00x = 1 - getEdgeMultiplier(corners[1], corners[0]);
    float em_0x1 = getEdgeMultiplier(corners[1], corners[3]);
    float em_x01 = getEdgeMultiplier(corners[1], corners[5]);
    float em_01x = getEdgeMultiplier(corners[2], corners[3]);
    float em_0x0 = 1 - getEdgeMultiplier(corners[2], corners[0]);
    float em_x10 = getEdgeMultiplier(corners[2], corners[6]);
    float em_11x = 1 - getEdgeMultiplier(corners[7], corners[6]);
    float em_1x1 = 1 - getEdgeMultiplier(corners[7], corners[5]);
    float em_x11 = 1 - getEdgeMultiplier(corners[7], corners[3]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(1, em_0x1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(em_01x, 1, 0),
        Urho3D::Vector3(0, 1, em_x10),
        Urho3D::Vector3(0, em_0x0, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, 1, em_x11),
        Urho3D::Vector3(1, em_1x1, 1),
        Urho3D::Vector3(em_11x, 1, 1)
    ));
    return tris;
}

MarchingCubesChunk::Triangles MarchingCubesChunk::makeFourCorners(uint8_t const* corners, bool extra_faces)
{
// TODO: Is extra faces needed here?
    (void)extra_faces;
    float em_0x0 = getEdgeMultiplier(corners[0], corners[2]);
    float em_00x = getEdgeMultiplier(corners[0], corners[1]);
    float em_x00 = getEdgeMultiplier(corners[0], corners[4]);
    float em_01x = 1 - getEdgeMultiplier(corners[3], corners[2]);
    float em_0x1 = 1 - getEdgeMultiplier(corners[3], corners[1]);
    float em_x11 = getEdgeMultiplier(corners[3], corners[7]);
    float em_10x = 1 - getEdgeMultiplier(corners[5], corners[4]);
    float em_1x1 = getEdgeMultiplier(corners[5], corners[7]);
    float em_x01 = 1 - getEdgeMultiplier(corners[5], corners[1]);
    float em_11x = getEdgeMultiplier(corners[6], corners[7]);
    float em_1x0 = 1 - getEdgeMultiplier(corners[6], corners[4]);
    float em_x10 = 1 - getEdgeMultiplier(corners[6], corners[2]);
    Triangles tris;
    tris.Push(Triangle(
        Urho3D::Vector3(em_00x, 0, 0),
        Urho3D::Vector3(0, em_0x0, 0),
        Urho3D::Vector3(0, 0, em_x00)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, em_0x1, 0),
        Urho3D::Vector3(1, 1, em_x11),
        Urho3D::Vector3(em_01x, 1, 0)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(1, 0, em_x01),
        Urho3D::Vector3(em_10x, 0, 1),
        Urho3D::Vector3(1, em_1x1, 1)
    ));
    tris.Push(Triangle(
        Urho3D::Vector3(0, em_1x0, 1),
        Urho3D::Vector3(0, 1, em_x10),
        Urho3D::Vector3(em_11x, 1, 0)
    ));
    return tris;
}

void MarchingCubesChunk::addVertexToRawData(Urho3D::PODVector<float> const& vertex, Urho3D::PODVector<float>& vdata, Urho3D::PODVector<unsigned>& idata)
{
    // Try to find existing vertex
    unsigned index = 0;
    for (unsigned ofs = 0; ofs < vdata.Size(); ofs += vertex.Size()) {
        bool match = true;
        for (unsigned i = 0; i < vertex.Size(); ++ i) {
            if (Urho3D::Abs(vertex[i] - vdata[ofs + i]) > 0.0001) {
                match = false;
                break;
            }
        }
        if (match) {
            idata.Push(index);
            return;
        }
        index += 1;
    }
    // If existing one was not found, then add a new one
    vdata.Insert(vdata.End(), vertex.Begin(), vertex.End());
    idata.Push(index);
}

}

}
