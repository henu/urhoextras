#include "terraingrid.hpp"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/TerrainPatch.h>
#include <Urho3D/IO/Compression.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>

#include <vector>

namespace UrhoExtras
{

namespace Graphics
{

unsigned const DEFAULT_HEIGHTMAP_WIDTH = 513;
float const DEFAULT_HEIGHTMAP_SQUARE_WIDTH = 0.5;
float const DEFAULT_HEIGHTMAP_STEP = 1;
unsigned const DEFAULT_TEXTURE_REPEATS = 32;
unsigned const DEFAULT_TEXTUREWEIGHT_WIDTH = 1024;

TerrainGrid::TerrainGrid(Urho3D::Context* context) :
    Urho3D::Component(context),
    heightmap_width(DEFAULT_HEIGHTMAP_WIDTH),
    heightmap_square_width(DEFAULT_HEIGHTMAP_SQUARE_WIDTH),
    heightmap_step(DEFAULT_HEIGHTMAP_STEP),
    texture_repeats(DEFAULT_TEXTURE_REPEATS),
    textureweight_width(DEFAULT_TEXTUREWEIGHT_WIDTH),
    viewmask(Urho3D::DEFAULT_VIEWMASK)
{
}

TerrainGrid::~TerrainGrid()
{
}

void TerrainGrid::addTexture(Urho3D::Image* tex_img)
{
    Urho3D::SharedPtr<Urho3D::Texture2D> tex(new Urho3D::Texture2D(context_));
    tex->SetData(tex_img);
    texs.Push(Urho3D::SharedPtr<Urho3D::Texture>(tex));
    texs_images.Push(Urho3D::SharedPtr<Urho3D::Image>(tex_img));
}

void TerrainGrid::setViewmask(unsigned viewmask)
{
    this->viewmask = viewmask;
    for (Urho3D::Terrain* chunk : chunks) {
        chunk->SetViewMask(viewmask);
    }
}

Urho3D::Vector3 TerrainGrid::getSize() const
{
    return Urho3D::Vector3(
        grid_size.x_ * getChunkWidth(),
        heightmap_step * 255.0f + (255.0f / 256),
        grid_size.y_ * getChunkWidth()
    );
}

Urho3D::IntVector2 TerrainGrid::getHeightmapSize() const
{
    return Urho3D::IntVector2(
        grid_size.x_ * (heightmap_width - 1) + 1,
        grid_size.y_ * (heightmap_width - 1) + 1
    );
}

Urho3D::IntVector2 TerrainGrid::getTextureweightsSize() const
{
    return Urho3D::IntVector2(
        grid_size.x_ * textureweight_width,
        grid_size.y_ * textureweight_width
    );
}

float TerrainGrid::getHeightmapSquareWidth() const
{
    return heightmap_square_width;
}

float TerrainGrid::getChunkWidth() const
{
    return (heightmap_width - 1) * heightmap_square_width;
}

unsigned TerrainGrid::getChunkHeightmapWidth() const
{
    return heightmap_width;
}

unsigned TerrainGrid::getChunkTextureweightsWidth() const
{
    return textureweight_width;
}

void TerrainGrid::generateFlatland(Urho3D::IntVector2 const& grid_size)
{
    this->grid_size = grid_size;

    // Prepare buffers
    Urho3D::IntVector2 heightmap_size = getHeightmapSize();
    Urho3D::IntVector2 textureweights_size = getTextureweightsSize();
    heightmap.Resize(heightmap_size.x_ * heightmap_size.y_, 0);
    textureweights.Resize(textureweights_size.x_ * textureweights_size.y_ * texs.Size(), 0);

    // Choose first texture
    for (unsigned i = 0; i < textureweights.Size(); i += texs.Size()) {
        textureweights[i] = 1;
    }

    buildFromBuffers();
}

void TerrainGrid::generateFromImages(Urho3D::Image* terrainweight, Urho3D::Image* heightmap, unsigned heightmap_blur)
{
    if (heightmap->GetWidth() < int(heightmap_width) || (heightmap->GetWidth() - 1) % (heightmap_width - 1) != 0) {
        throw std::runtime_error("Invalid heightmap width!");
    }
    if (heightmap->GetHeight() < int(heightmap_width) || (heightmap->GetHeight() - 1) % (heightmap_width - 1) != 0) {
        throw std::runtime_error("Invalid heightmap height!");
    }

    grid_size.x_ = (heightmap->GetWidth() - 1) / (heightmap_width - 1);
    grid_size.y_ = (heightmap->GetHeight() - 1) / (heightmap_width - 1);

    if (terrainweight->GetWidth() != grid_size.x_ * int(textureweight_width)) {
        throw std::runtime_error("Invalid terrainweight width!");
    }
    if (terrainweight->GetHeight() != grid_size.y_ * int(textureweight_width)) {
        throw std::runtime_error("Invalid terrainweight height!");
    }

    // Prepare buffers
    Urho3D::IntVector2 heightmap_size = getHeightmapSize();
    Urho3D::IntVector2 textureweights_size = getTextureweightsSize();
    this->heightmap.Reserve(heightmap_size.x_ * heightmap_size.y_);
    textureweights.Reserve(textureweights_size.x_ * textureweights_size.y_ * texs.Size());

    for (int y = 0; y < heightmap->GetHeight(); ++ y) {
        for (int x = 0; x < heightmap->GetWidth(); ++ x) {
            float height_f = 0;
            unsigned samples = 0;
            for (int blur_y = -heightmap_blur; blur_y <= int(heightmap_blur); ++ blur_y) {
                if (y + blur_y < 0 || y + blur_y >= heightmap->GetHeight()) {
                    continue;
                }
                for (int blur_x = -heightmap_blur; blur_x <= int(heightmap_blur); ++ blur_x) {
                    if (x + blur_x < 0 || x + blur_x >= heightmap->GetWidth()) {
                        continue;
                    }
                    Urho3D::Color color = heightmap->GetPixel(x + blur_x, y + blur_y);
                    height_f += color.r_ + color.g_ + color.b_;
                    samples += 3;
                }
            }
            height_f /= samples;
            uint16_t height = Urho3D::Clamp<int>(0xffff * height_f, 0, 0xffff);
            this->heightmap.Push(height);
        }
    }

    // Choose first texture
    for (int y = 0; y < terrainweight->GetHeight(); ++ y) {
        for (int x = 0; x < terrainweight->GetWidth(); ++ x) {
            unsigned color = terrainweight->GetPixelInt(x, y);
            textureweights.Push(color & 0xff);
            textureweights.Push((color >> 8) & 0xff);
            textureweights.Push((color >> 16) & 0xff);
        }
    }

    buildFromBuffers();
}

void TerrainGrid::generateFromVectors(Urho3D::IntVector2 const& grid_size, HeightData heightmap, WeightData textureweights)
{
    // Validate vector sizes
    float expected_heightmap_size = (grid_size.x_ * (heightmap_width - 1) + 1) * (grid_size.y_ * (heightmap_width - 1) + 1);
    if (expected_heightmap_size != heightmap.Size()) {
        throw std::runtime_error(("Unexpected heightmap size " + Urho3D::String(heightmap.Size()) + ". Should be " + Urho3D::String(expected_heightmap_size)).CString());
    }
    float expected_textureweights_size = grid_size.x_ * grid_size.y_ * textureweight_width * textureweight_width * 3;
    if (expected_textureweights_size != textureweights.Size()) {
        throw std::runtime_error(("Unexpected textureweights size " + Urho3D::String(textureweights.Size()) + ". Should be " + Urho3D::String(expected_textureweights_size)).CString());
    }

    // Copy data
    this->grid_size = grid_size;
    this->heightmap = heightmap;
    this->textureweights = textureweights;

    buildFromBuffers();
}

void TerrainGrid::forgetSourceData()
{
    heightmap.Clear();
    textureweights.Clear();
}

float TerrainGrid::getHeight(Urho3D::Vector3 const& world_pos) const
{
    Urho3D::Terrain* terrain = getChunkAt(world_pos.x_, world_pos.z_);
    if (terrain) {
        return terrain->GetHeight(world_pos);
    }
    return 0;
}

Urho3D::Vector3 TerrainGrid::getNormal(Urho3D::Vector3 const& world_pos) const
{
    Urho3D::Terrain* terrain = getChunkAt(world_pos.x_, world_pos.z_);
    if (terrain) {
        return terrain->GetNormal(world_pos);
    }
    return Urho3D::Vector3::UP;
}

void TerrainGrid::getTerrainPatches(Urho3D::PODVector<Urho3D::TerrainPatch*>& result, Urho3D::Vector2 const& pos, float radius)
{
    Urho3D::Rect bounds(pos - Urho3D::Vector2::ONE * radius, pos + Urho3D::Vector2::ONE * radius);

    Urho3D::IntVector2 bounds_min_i(
        Urho3D::Clamp(Urho3D::FloorToInt(bounds.min_.x_ / getChunkWidth()), 0, grid_size.x_ - 1),
        Urho3D::Clamp(Urho3D::FloorToInt(bounds.min_.y_ / getChunkWidth()), 0, grid_size.y_ - 1)
    );
    Urho3D::IntVector2 bounds_max_i(
        Urho3D::Clamp(Urho3D::CeilToInt(bounds.max_.x_ / getChunkWidth()), 0, grid_size.x_),
        Urho3D::Clamp(Urho3D::CeilToInt(bounds.max_.y_ / getChunkWidth()), 0, grid_size.y_)
    );

    for (int y = bounds_min_i.y_; y < bounds_max_i.y_; ++ y) {
        for (int x = bounds_min_i.x_; x < bounds_max_i.x_; ++ x) {
            Urho3D::Terrain* chunk = chunks[x + y * grid_size.x_];
            Urho3D::IntVector2 patches_size = chunk->GetNumPatches();
            for (int y = 0; y < patches_size.y_; ++ y) {
                for (int x = 0; x < patches_size.x_; ++ x) {
                    Urho3D::TerrainPatch* patch = chunk->GetPatch(x, y);
                    Urho3D::BoundingBox patch_bb = patch->GetWorldBoundingBox();
                    // If patch is southern
                    if (pos.y_ < patch_bb.min_.z_) {
                        // If patch is south west
                        if (pos.x_ < patch_bb.min_.x_) {
                            if ((pos - Urho3D::Vector2(patch_bb.min_.x_, patch_bb.min_.z_)).Length() > radius) {
                                continue;
                            }
                        }
                        // If patch is south east
                        else if (pos.x_ > patch_bb.max_.x_) {
                            if ((pos - Urho3D::Vector2(patch_bb.max_.x_, patch_bb.min_.z_)).Length() > radius) {
                                continue;
                            }
                        }
                        // If patch is south
                        else {
                            if (patch_bb.min_.z_ - pos.y_ > radius) {
                                continue;
                            }
                        }
                    }
                    // If patch is northern
                    if (pos.y_ > patch_bb.max_.z_) {
                        // If patch is north west
                        if (pos.x_ < patch_bb.min_.x_) {
                            if ((pos - Urho3D::Vector2(patch_bb.min_.x_, patch_bb.max_.z_)).Length() > radius) {
                                continue;
                            }
                        }
                        // If patch is north east
                        else if (pos.x_ > patch_bb.max_.x_) {
                            if ((pos - Urho3D::Vector2(patch_bb.max_.x_, patch_bb.max_.z_)).Length() > radius) {
                                continue;
                            }
                        }
                        // If patch is north
                        else {
                            if (pos.y_ - patch_bb.max_.z_ > radius) {
                                continue;
                            }
                        }
                    }
                    // If patch is on a same latitude
                    else {
                        // If patch is west
                        if (patch_bb.min_.x_ - pos.x_ > radius) {
                            continue;
                        }
                        // If patch is east
                        else if (pos.x_ - patch_bb.max_.x_ > radius) {
                            continue;
                        }
                    }
                    result.Push(patch);
                }
            }
        }
    }
}

void TerrainGrid::buildFromBuffers()
{
    Urho3D::ResourceCache* resources = GetSubsystem<Urho3D::ResourceCache>();

    Urho3D::Node* node = GetNode();

    // Clear possible old stuff
    for (Urho3D::Terrain* chunk : chunks) {
        chunk->GetNode()->Remove();
    }
    chunks.Clear();

    Urho3D::SharedPtr<Urho3D::Material> original_mat(new Urho3D::Material(context_));
    original_mat->SetNumTechniques(1);
    original_mat->SetTechnique(0, resources->GetResource<Urho3D::Technique>("Techniques/TerrainBlend.xml"));
    original_mat->SetShaderParameter("DetailTiling", Urho3D::Vector2(texture_repeats, texture_repeats));
    for (unsigned i = 0; i < texs.Size(); ++ i) {
        original_mat->SetTexture(static_cast<Urho3D::TextureUnit>(i + 1), texs[i]);
    }

    // Create Terrain objects
    for (int y = 0; y < grid_size.y_; ++ y) {
        for (int x = 0; x < grid_size.x_; ++ x) {
            // Node
            Urho3D::Node* chunk_node = node->CreateChild(Urho3D::String::EMPTY, Urho3D::LOCAL);
            chunk_node->SetPosition(Urho3D::Vector3(
                (x + 0.5) * (heightmap_width - 1) * heightmap_square_width,
                0,
                (y + 0.5) * (heightmap_width - 1) * heightmap_square_width
            ));

            // Heightmap for this Chunk
            std::vector<unsigned char> chunk_heightmap_data;
            chunk_heightmap_data.reserve(heightmap_width * heightmap_width * 3);
            for (unsigned y2 = 0; y2 < heightmap_width; ++ y2) {
                unsigned ofs = x * (heightmap_width - 1) + ((heightmap_width - y2 - 1) + y * (heightmap_width - 1)) * ((heightmap_width - 1) * grid_size.x_ + 1);
                for (unsigned x2 = 0; x2 < heightmap_width; ++ x2) {
                    uint16_t height = heightmap[ofs ++];
                    chunk_heightmap_data.push_back(height / 256);
                    chunk_heightmap_data.push_back(height % 256);
                    chunk_heightmap_data.push_back(0);
                }
            }
            Urho3D::SharedPtr<Urho3D::Image> chunk_heightmap(new Urho3D::Image(context_));
            chunk_heightmap->SetSize(heightmap_width, heightmap_width, 3);
            assert(chunk_heightmap_data.size() == heightmap_width * heightmap_width * 3);
            chunk_heightmap->SetData(chunk_heightmap_data.data());

            // Weight texture for material
            std::vector<unsigned char> chunk_textureweight_data;
            chunk_textureweight_data.reserve(textureweight_width * textureweight_width * texs.Size());
            for (unsigned y2 = 0; y2 < textureweight_width; ++ y2) {
                unsigned ofs = (x * textureweight_width + ((textureweight_width - y2 - 1) + y * textureweight_width) * textureweight_width * grid_size.x_) * texs.Size();
                for (unsigned x2 = 0; x2 < textureweight_width; ++ x2) {
                    for (unsigned i = 0; i < texs.Size(); ++ i) {
                        float weight = textureweights[ofs ++];
                        chunk_textureweight_data.push_back(weight);
                    }
                }
            }
            Urho3D::SharedPtr<Urho3D::Image> chunk_textureweight_img(new Urho3D::Image(context_));
            chunk_textureweight_img->SetSize(textureweight_width, textureweight_width, texs.Size());
            chunk_textureweight_img->SetData(&chunk_textureweight_data[0]);
            Urho3D::SharedPtr<Urho3D::Texture2D> chunk_textureweight_tex(new Urho3D::Texture2D(context_));
            chunk_textureweight_tex->SetAddressMode(Urho3D::COORD_U, Urho3D::ADDRESS_CLAMP);
            chunk_textureweight_tex->SetAddressMode(Urho3D::COORD_V, Urho3D::ADDRESS_CLAMP);
            chunk_textureweight_tex->SetData(chunk_textureweight_img);

            // Material
            Urho3D::SharedPtr<Urho3D::Material> chunk_mat(original_mat->Clone());
            chunk_mat->SetTexture(static_cast<Urho3D::TextureUnit>(0), chunk_textureweight_tex);

            // Terrain
            Urho3D::Terrain* chunk_terrain = chunk_node->CreateComponent<Urho3D::Terrain>(Urho3D::LOCAL);
            chunk_terrain->SetSpacing(Urho3D::Vector3(heightmap_square_width, heightmap_step, heightmap_square_width));
            chunk_terrain->SetHeightMap(chunk_heightmap);
            chunk_terrain->SetMaterial(chunk_mat);
            chunk_terrain->SetViewMask(viewmask);

            chunks.Push(chunk_terrain);
        }
    }

    // Set Terrain neighbors
    for (int y = 0; y < grid_size.y_; ++ y) {
        for (int x = 0; x < grid_size.x_; ++ x) {
            Urho3D::Terrain* terrain = chunks[x + y * grid_size.x_];
            if (x > 0) {
                terrain->SetWestNeighbor(chunks[x - 1 + y * grid_size.x_]);
            }
            if (x < grid_size.x_ - 1) {
                terrain->SetEastNeighbor(chunks[x + 1 + y * grid_size.x_]);
            }
            if (y > 0) {
                terrain->SetSouthNeighbor(chunks[x + (y - 1) * grid_size.x_]);
            }
            if (y < grid_size.y_ - 1) {
                terrain->SetNorthNeighbor(chunks[x + (y + 1) * grid_size.x_]);
            }
        }
    }
}

void TerrainGrid::registerObject(Urho3D::Context* context)
{
    context->RegisterFactory<TerrainGrid>();

    // These are only used for network replication for now
    URHO3D_ATTRIBUTE("Heightmap width", unsigned, heightmap_width, DEFAULT_HEIGHTMAP_WIDTH, Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Heightmap square width", float, heightmap_square_width, DEFAULT_HEIGHTMAP_SQUARE_WIDTH, Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Heightmap step", float, heightmap_step, DEFAULT_HEIGHTMAP_STEP, Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Texture repeats", unsigned, texture_repeats, DEFAULT_TEXTURE_REPEATS, Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Textureweight width", unsigned, textureweight_width, DEFAULT_TEXTUREWEIGHT_WIDTH, Urho3D::AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Texture Images", getTexturesImagesAttr, setTexturesImagesAttr, Urho3D::ResourceRefList, Urho3D::ResourceRefList(Urho3D::Image::GetTypeStatic()), Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Grid size", Urho3D::IntVector2, grid_size, Urho3D::IntVector2::ZERO, Urho3D::AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Heightmap", getHeightmapAttr, setHeightmapAttr, Urho3D::PODVector<unsigned char>, Urho3D::Variant::emptyBuffer, Urho3D::AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Textureweights", getTextureweightsAttr, setTextureweightsAttr, Urho3D::PODVector<unsigned char>, Urho3D::Variant::emptyBuffer, Urho3D::AM_DEFAULT);
    URHO3D_ATTRIBUTE("Viewask", unsigned, viewmask, Urho3D::DEFAULT_VIEWMASK, Urho3D::AM_DEFAULT);
}

void TerrainGrid::ApplyAttributes()
{
    buildFromBuffers();
}

Urho3D::Terrain* TerrainGrid::getChunkAt(float x, float z) const
{
    int x_i = Urho3D::FloorToInt(x / (heightmap_width - 1) / heightmap_square_width);
    if (x_i < 0) return NULL;
    if (x_i >= grid_size.x_) return NULL;
    int z_i = Urho3D::FloorToInt(z / (heightmap_width - 1) / heightmap_square_width);
    if (z_i < 0) return NULL;
    if (z_i >= grid_size.y_) return NULL;
    return chunks[x_i + z_i * grid_size.x_];
}

Urho3D::ResourceRefList TerrainGrid::getTexturesImagesAttr() const
{
    Urho3D::ResourceRefList texs_images_attr(Urho3D::Image::GetTypeStatic());
    for (auto tex_image : texs_images) {
        texs_images_attr.names_.Push(Urho3D::GetResourceName(tex_image));
    }
    return texs_images_attr;
}

void TerrainGrid::setTexturesImagesAttr(Urho3D::ResourceRefList const& value)
{
    Urho3D::ResourceCache* resources = GetSubsystem<Urho3D::ResourceCache>();

    // Clear existing stuff
    texs.Clear();
    texs_images.Clear();

    for (Urho3D::String res_name : value.names_) {
        Urho3D::SharedPtr<Urho3D::Image> tex_img(resources->GetResource<Urho3D::Image>(res_name));
        texs_images.Push(tex_img);
        // Convert image to texture
        Urho3D::SharedPtr<Urho3D::Texture2D> tex(new Urho3D::Texture2D(context_));
        tex->SetData(tex_img);
        texs.Push(Urho3D::SharedPtr<Urho3D::Texture>(tex));
    }
}

Urho3D::PODVector<unsigned char> TerrainGrid::getHeightmapAttr() const
{
    Urho3D::VectorBuffer heightmap_vbuf;
    for (unsigned i = 0; i < heightmap.Size(); ++ i) {
        heightmap_vbuf.WriteUShort(heightmap[i]);
    }
    heightmap_vbuf.Seek(0);
    Urho3D::VectorBuffer heightmap_compressed_vbuf;
    if (!Urho3D::CompressStream(heightmap_compressed_vbuf, heightmap_vbuf)) {
        throw std::runtime_error("Unable to compress TerrainGrid.heightmap for attribute serialization!");
    }
    return heightmap_compressed_vbuf.GetBuffer();
}

void TerrainGrid::setHeightmapAttr(Urho3D::PODVector<unsigned char> const& value)
{
    Urho3D::MemoryBuffer heightmap_compressed_buf(value);
    Urho3D::VectorBuffer heightmap_vbuf;
    if (!Urho3D::DecompressStream(heightmap_vbuf, heightmap_compressed_buf)) {
        throw std::runtime_error("Unable to decompress TerrainGrid.heightmap for attribute deserialization!");
    }
    heightmap_vbuf.Seek(0);
    heightmap.Clear();
    while (!heightmap_vbuf.IsEof()) {
        heightmap.Push(heightmap_vbuf.ReadUShort());
    }
}

Urho3D::PODVector<unsigned char> TerrainGrid::getTextureweightsAttr() const
{
    Urho3D::MemoryBuffer textureweights_buf(textureweights);
    Urho3D::VectorBuffer textureweights_compressed_vbuf;
    if (!Urho3D::CompressStream(textureweights_compressed_vbuf, textureweights_buf)) {
        throw std::runtime_error("Unable to compress TerrainGrid.textureweights for attribute serialization!");
    }
    return textureweights_compressed_vbuf.GetBuffer();
}

void TerrainGrid::setTextureweightsAttr(Urho3D::PODVector<unsigned char> const& value)
{
    Urho3D::MemoryBuffer textureweights_compressed_buf(value);
    Urho3D::VectorBuffer textureweights_vbuf;
    if (!Urho3D::DecompressStream(textureweights_vbuf, textureweights_compressed_buf)) {
        throw std::runtime_error("Unable to decompress TerrainGrid.textureweights for attribute deserialization!");
    }
    textureweights.Clear();
// TODO: Make some kind of fast memory copy!
    textureweights_vbuf.Seek(0);
    while (!textureweights_vbuf.IsEof()) {
        textureweights.Push(textureweights_vbuf.ReadUByte());
    }
}

}

}
