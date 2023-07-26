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
        heightmap_step * (255.0f + 255.0f / 256),
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

float TerrainGrid::getTextureweightsSquareWidth() const
{
    return getChunkWidth() / textureweight_width;
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

    chunks_not_dirty.Clear();
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

    chunks_not_dirty.Clear();
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

    chunks_not_dirty.Clear();
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

    chunks.Resize(grid_size.x_ * grid_size.y_, nullptr);

    // Clear possible old stuff
    Urho3D::IntVector2 i;
    unsigned offset = 0;
    for (i.y_ = 0; i.y_ < grid_size.y_; ++ i.y_) {
        for (i.x_ = 0; i.x_ < grid_size.x_; ++ i.x_) {
            if (chunks[offset] && !chunks_not_dirty.Contains(i)) {
                chunks[offset]->GetNode()->Remove();
                chunks[offset] = nullptr;
            }
            ++ offset;
        }
    }

    Urho3D::SharedPtr<Urho3D::Material> original_mat(new Urho3D::Material(context_));
    original_mat->SetNumTechniques(1);
    original_mat->SetTechnique(0, resources->GetResource<Urho3D::Technique>("Techniques/TerrainBlend.xml"));
    original_mat->SetShaderParameter("DetailTiling", Urho3D::Vector2(texture_repeats, texture_repeats));
    for (unsigned i = 0; i < texs.Size(); ++ i) {
        original_mat->SetTexture(static_cast<Urho3D::TextureUnit>(i + 1), texs[i]);
    }

    // Create Terrain objects
    offset = 0;
    for (int y = 0; y < grid_size.y_; ++ y) {
        for (int x = 0; x < grid_size.x_; ++ x) {
            // If there is already a chunk, it doesn't need to be recreated
            if (chunks[offset]) {
                ++ offset;
                continue;
            }

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

            chunks_not_dirty.Insert(Urho3D::IntVector2(x, y));

            chunks[offset] = chunk_terrain;
            ++ offset;
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

void TerrainGrid::drawTo(Urho3D::Vector3 const& pos, Urho3D::Image* terrain_mod, Urho3D::Image* height_mod, float height_mod_strength, Urho3D::Vector2 const& size, float angle, bool update_over_network)
{
    // Calculate relative position
    Urho3D::Vector3 terrain_pos = GetNode()->GetWorldPosition();
    Urho3D::Vector3 total_size = getSize();
    Urho3D::Vector2 pos_rel((pos.x_ - terrain_pos.x_) / total_size.x_, (pos.z_ - terrain_pos.z_) / total_size.z_);

    // Calculate relative bounds
    float bounds_radius = size.Length() / 2;
    Urho3D::Vector2 bounds_rel_min(pos_rel.x_ - bounds_radius / total_size.x_, pos_rel.y_ - bounds_radius / total_size.z_);
    Urho3D::Vector2 bounds_rel_max(pos_rel.x_ + bounds_radius / total_size.x_, pos_rel.y_ + bounds_radius / total_size.z_);

    if (terrain_mod) {
        // Calculate position and bounds in textureweights
        Urho3D::IntVector2 texmap_total_size = getTextureweightsSize();
        Urho3D::Vector2 texmap_pos(pos_rel.x_ * texmap_total_size.x_, pos_rel.y_ * texmap_total_size.y_);
        Urho3D::IntVector2 texmap_bounds_min(
            Urho3D::Max(0, Urho3D::FloorToInt(bounds_rel_min.x_ * texmap_total_size.x_)),
            Urho3D::Max(0, Urho3D::FloorToInt(bounds_rel_min.y_ * texmap_total_size.y_))
        );
        Urho3D::IntVector2 texmap_bounds_max(
            Urho3D::Min(texmap_total_size.x_ - 1, Urho3D::CeilToInt(bounds_rel_max.x_ * texmap_total_size.x_)),
            Urho3D::Min(texmap_total_size.y_ - 1, Urho3D::CeilToInt(bounds_rel_max.y_ * texmap_total_size.y_))
        );
        // Iterate pixels at the area
        Urho3D::IntVector2 i;
// TODO: What if size is not square?
        float texmap_scale = terrain_mod->GetWidth() * total_size.x_ / texmap_total_size.x_ / size.x_;
        for (i.x_ = texmap_bounds_min.x_; i.x_ <= texmap_bounds_max.x_; ++ i.x_) {
            float x_rel = i.x_ - texmap_pos.x_;
            for (i.y_ = texmap_bounds_min.y_; i.y_ <= texmap_bounds_max.y_; ++ i.y_) {
                // Convert from texture space to image space
                float z_rel = i.y_ - texmap_pos.y_;
                float x_rel2 = x_rel * Urho3D::Cos(angle) - z_rel * Urho3D::Sin(angle);
                float z_rel2 = z_rel * Urho3D::Cos(angle) + x_rel * Urho3D::Sin(angle);
                float i_pos_x = x_rel2 * texmap_scale;
                float i_pos_z = z_rel2 * texmap_scale;
                i_pos_x += terrain_mod->GetWidth() / 2;
                i_pos_z += terrain_mod->GetHeight() / 2;
                int i_pos_x_i = Urho3D::RoundToInt(i_pos_x);
                int i_pos_z_i = Urho3D::RoundToInt(i_pos_z);
                if (i_pos_x_i < 0 || i_pos_x_i >= terrain_mod->GetWidth() || i_pos_z_i < 0 || i_pos_z_i >= terrain_mod->GetHeight()) {
                    continue;
                }
                // Do the modification
                Urho3D::Color color = terrain_mod->GetPixel(i_pos_x_i, terrain_mod->GetHeight() - i_pos_z_i - 1);
                unsigned offset = (i.x_ + i.y_ * texmap_total_size.x_) * 3;
                Urho3D::Vector3 final_color(
                    textureweights[offset] / 255.0 * (1 - color.a_) + color.r_ * color.a_,
                    textureweights[offset + 1] / 255.0 * (1 - color.a_) + color.g_ * color.a_,
                    textureweights[offset + 2] / 255.0 * (1 - color.a_) + color.b_ * color.a_
                );
                final_color.Normalize();
                textureweights[offset] = Urho3D::Clamp(Urho3D::RoundToInt(final_color.x_ * 255), 0, 255);
                textureweights[offset + 1] = Urho3D::Clamp(Urho3D::RoundToInt(final_color.y_ * 255), 0, 255);
                textureweights[offset + 2] = Urho3D::Clamp(Urho3D::RoundToInt(final_color.z_ * 255), 0, 255);
            }
        }
    }

    if (height_mod) {
        // Calculate position and bounds in textureweights
        Urho3D::IntVector2 hmap_total_size = getHeightmapSize();
        Urho3D::Vector2 hmap_pos(pos_rel.x_ * hmap_total_size.x_, pos_rel.y_ * hmap_total_size.y_);
        Urho3D::IntVector2 hmap_bounds_min(
            Urho3D::Max(0, Urho3D::FloorToInt(bounds_rel_min.x_ * hmap_total_size.x_)),
            Urho3D::Max(0, Urho3D::FloorToInt(bounds_rel_min.y_ * hmap_total_size.y_))
        );
        Urho3D::IntVector2 hmap_bounds_max(
            Urho3D::Min(hmap_total_size.x_ - 1, Urho3D::CeilToInt(bounds_rel_max.x_ * hmap_total_size.x_)),
            Urho3D::Min(hmap_total_size.y_ - 1, Urho3D::CeilToInt(bounds_rel_max.y_ * hmap_total_size.y_))
        );
        // Iterate pixels at the area
        Urho3D::IntVector2 i;
// TODO: What if size is not square?
        float hmap_scale = height_mod->GetWidth() * total_size.x_ / hmap_total_size.x_ / size.x_;
        for (i.x_ = hmap_bounds_min.x_; i.x_ <= hmap_bounds_max.x_; ++ i.x_) {
            float x_rel = i.x_ - hmap_pos.x_;
            for (i.y_ = hmap_bounds_min.y_; i.y_ <= hmap_bounds_max.y_; ++ i.y_) {
                // Convert from height space to image space
                float z_rel = i.y_ - hmap_pos.y_;
                float x_rel2 = x_rel * Urho3D::Cos(angle) - z_rel * Urho3D::Sin(angle);
                float z_rel2 = z_rel * Urho3D::Cos(angle) + x_rel * Urho3D::Sin(angle);
                float i_pos_x = x_rel2 * hmap_scale;
                float i_pos_z = z_rel2 * hmap_scale;
                i_pos_x += height_mod->GetWidth() / 2;
                i_pos_z += height_mod->GetHeight() / 2;
                int i_pos_x_i = Urho3D::RoundToInt(i_pos_x);
                int i_pos_z_i = Urho3D::RoundToInt(i_pos_z);
                if (i_pos_x_i < 0 || i_pos_x_i >= height_mod->GetWidth() || i_pos_z_i < 0 || i_pos_z_i >= height_mod->GetHeight()) {
                    continue;
                }
                // Do the modification
                Urho3D::Color color = height_mod->GetPixel(i_pos_x_i, height_mod->GetHeight() - i_pos_z_i - 1);
                float height_increase = (color.Average() - 0.5) * 2 * height_mod_strength;
                int height_increase_i = 65535.0 * height_increase / total_size.z_;
                unsigned offset = i.x_ + i.y_ * hmap_total_size.x_;
                heightmap[offset] = Urho3D::Clamp(int(heightmap[offset]) + height_increase_i, 0, 0xffff);
            }
        }
    }

    // Mark chunks dirty
// TODO: Would it be a good idea to more accurate here? Now some chunks are marked dirty even if no changes happen on them.
    Urho3D::IntVector2 chunks_bounds_min(Urho3D::FloorToInt(bounds_rel_min.x_ * grid_size.x_), Urho3D::FloorToInt(bounds_rel_min.y_ * grid_size.y_));
    Urho3D::IntVector2 chunks_bounds_max(Urho3D::CeilToInt(bounds_rel_max.x_ * grid_size.x_), Urho3D::CeilToInt(bounds_rel_max.y_ * grid_size.y_));
    Urho3D::IntVector2 i;
    for (i.x_ = chunks_bounds_min.x_; i.x_ <= chunks_bounds_max.x_; ++ i.x_) {
        for (i.y_ = chunks_bounds_min.y_; i.y_ <= chunks_bounds_max.y_; ++ i.y_) {
            chunks_not_dirty.Erase(i);
        }
    }

    if (update_over_network) {
        MarkNetworkUpdate();
    }

    buildFromBuffers();
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
    HeightData old_heightmap;
    heightmap.Swap(old_heightmap);
    while (!heightmap_vbuf.IsEof()) {
        heightmap.Push(heightmap_vbuf.ReadUShort());
    }
    // Compare new and old heightdata to find out which chunks have changed
    if (old_heightmap.Empty()) {
        chunks_not_dirty.Clear();
    } else {
        Urho3D::IntVector2 chunk_pos;
        for (chunk_pos.y_ = 0; chunk_pos.y_ < grid_size.y_; ++ chunk_pos.y_) {
            for (chunk_pos.x_ = 0; chunk_pos.x_ < grid_size.x_; ++ chunk_pos.x_) {
                if (chunks_not_dirty.Contains(chunk_pos)) {
                    unsigned offset_begin = chunk_pos.x_ * (heightmap_width - 1) + chunk_pos.y_ * (heightmap_width - 1) * (grid_size.x_ * (heightmap_width - 1) + 1);
                    bool removed = false;
                    for (unsigned z = 0; z < heightmap_width && !removed; ++ z) {
                        unsigned offset = offset_begin + z * (grid_size.x_ * (heightmap_width - 1) + 1);
                        for (unsigned x = 0; x < heightmap_width; ++ x) {
                            if (heightmap[offset] != old_heightmap[offset]) {
                                chunks_not_dirty.Erase(chunk_pos);
                                removed = true;
                                break;
                            }
                            ++ offset;
                        }
                    }
                }
            }
        }
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
    WeightData old_textureweights;
    textureweights.Swap(old_textureweights);
// TODO: Make some kind of fast memory copy!
    textureweights_vbuf.Seek(0);
    while (!textureweights_vbuf.IsEof()) {
        textureweights.Push(textureweights_vbuf.ReadUByte());
    }
    // Compare new and old heightdata to find out which chunks have changed
    if (old_textureweights.Empty()) {
        chunks_not_dirty.Clear();
    } else {
        Urho3D::IntVector2 chunk_pos;
        for (chunk_pos.y_ = 0; chunk_pos.y_ < grid_size.y_; ++ chunk_pos.y_) {
            for (chunk_pos.x_ = 0; chunk_pos.x_ < grid_size.x_; ++ chunk_pos.x_) {
                if (chunks_not_dirty.Contains(chunk_pos)) {
                    unsigned offset_begin = (chunk_pos.x_ * textureweight_width + chunk_pos.y_ * textureweight_width * grid_size.x_ * textureweight_width) * 3;
                    bool removed = false;
                    for (unsigned z = 0; z < textureweight_width && !removed; ++ z) {
                        unsigned offset = offset_begin + z * grid_size.x_ * textureweight_width * 3;
                        for (unsigned x = 0; x < textureweight_width * 3; ++ x) {
                            if (textureweights[offset] != old_textureweights[offset]) {
                                chunks_not_dirty.Erase(chunk_pos);
                                removed = true;
                                break;
                            }
                            ++ offset;
                        }
                    }
                }
            }
        }
    }
}

}

}
