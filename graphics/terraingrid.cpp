#include "terraingrid.hpp"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
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

TerrainGrid::TerrainGrid(Urho3D::Context* context) :
    Urho3D::Component(context),
    heightmap_width(DEFAULT_HEIGHTMAP_WIDTH),
    heightmap_square_width(0.5),
    heightmap_step(1),
    texture_repeats(32),
    textureweight_width(1024)
{
}

TerrainGrid::~TerrainGrid()
{
}

void TerrainGrid::addTexture(Urho3D::Texture* tex)
{
    texs.Push(Urho3D::SharedPtr<Urho3D::Texture>(tex));
}

void TerrainGrid::addTexture(Urho3D::Image* tex_img)
{
    Urho3D::SharedPtr<Urho3D::Texture2D> tex(new Urho3D::Texture2D(context_));
    tex->SetData(tex_img);
    addTexture(tex);
}

Urho3D::Vector3 TerrainGrid::getSize() const
{
    return Urho3D::Vector3(
        grid_size.x_ * (heightmap_width - 1) * heightmap_square_width,
        heightmap_step * 255.0f + (255.0f / 256),
        grid_size.y_ * (heightmap_width - 1) * heightmap_square_width
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

void TerrainGrid::generateFlatland(Urho3D::IntVector2 const& size)
{
    grid_size = size;

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

bool TerrainGrid::Load(Urho3D::Deserializer& source)
{
    unsigned buf_size = source.ReadUInt();

    Urho3D::PODVector<unsigned char> buf_v;
    buf_v.Resize(buf_size);
    Urho3D::MemoryBuffer buf(buf_v);

    if (!DecompressStream(buf, source)) {
        return false;
    }
    buf.Seek(0);

    // Grid size
    grid_size = buf.ReadIntVector2();

    // Heightmap
    Urho3D::IntVector2 heightmap_size = getHeightmapSize();
    heightmap.Resize(heightmap_size.x_ * heightmap_size.y_);
    for (unsigned i = 0; i < heightmap.Size(); ++ i) {
        heightmap[i] = buf.ReadUShort();
    }

    // Textureweights
    Urho3D::IntVector2 textureweights_size = getTextureweightsSize();
    textureweights.Resize(textureweights_size.x_ * textureweights_size.y_ * texs.Size());
    for (unsigned i = 0; i < textureweights.Size(); ++ i) {
        textureweights[i] = buf.ReadUByte();
    }

    return true;
}

bool TerrainGrid::Save(Urho3D::Serializer& dest) const
{
    Urho3D::PODVector<unsigned char> buf_v;
    unsigned buf_size =
        8 +
        heightmap.Size() * 2 +
        textureweights.Size();
    buf_v.Resize(buf_size);
    Urho3D::MemoryBuffer buf(buf_v);
// TODO: Write size topions, etc. and also textures somehow!

    // Grid size
    buf.WriteIntVector2(grid_size);

    // Heightmap
    for (unsigned i = 0; i < heightmap.Size(); ++ i) {
        buf.WriteUShort(heightmap[i]);
    }

    // Textureweights
    for (unsigned i = 0; i < textureweights.Size(); ++ i) {
        buf.WriteUByte(textureweights[i]);
    }

    // Write original data length and compress the data
    dest.WriteUInt(buf_size);
    buf.Seek(0);
    return Urho3D::CompressStream(dest, buf);
}

void TerrainGrid::buildFromBuffers()
{
    Urho3D::ResourceCache* resources = GetSubsystem<Urho3D::ResourceCache>();

    Urho3D::Node* node = GetNode();

    // Clear possible old stuff
    chunks.Clear();
    node->RemoveChildren(true, true, true);

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

}

}
