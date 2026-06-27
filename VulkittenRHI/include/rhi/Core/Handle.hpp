#pragma once

#include <cstdint>
#include <functional>

namespace rhi {

// ============================================================
// Handle<Tag> — strongly-typed GPU resource handle
//
// Combines a slot index with a generation counter for ABA
// (use-after-free) protection. Id == 0 means Null handle.
// Tag prevents implicit conversion between handle types.
// ============================================================

template <typename Tag>
class Handle
{
public:
    Handle() = default;
    explicit Handle(uint32_t id, uint32_t generation = 0)
        : m_Id(id)
        , m_Generation(generation)
    {
    }

    uint32_t GetId() const { return m_Id; }
    uint32_t GetGeneration() const { return m_Generation; }
    bool IsValid() const { return m_Id != 0; }

    bool operator==(Handle other) const
    {
        return m_Id == other.m_Id && m_Generation == other.m_Generation;
    }
    bool operator!=(Handle other) const
    {
        return !(*this == other);
    }
    bool operator<(Handle other) const
    {
        return m_Id < other.m_Id;
    }

    struct Hash
    {
        size_t operator()(Handle h) const
        {
            return std::hash<uint64_t>{}(
                (static_cast<uint64_t>(h.m_Generation) << 32) | h.m_Id);
        }
    };

private:
    uint32_t m_Id = 0;
    uint32_t m_Generation = 0;
};

// ============================================================
// Resource Tags (empty structs for type differentiation)
// ============================================================

struct BufferTag {};
struct TextureTag {};
struct ShaderTag {};
struct PipelineTag {};
struct GeometryTag {};
struct SamplerTag {};
struct RenderPassTag {};
struct FramebufferTag {};
struct QueryPoolTag {};
struct AccelerationStructureTag {};  // [RESERVE: RayTracing]

// ============================================================
// Strongly-typed Handle Aliases
// ============================================================

using BufferHandle      = Handle<BufferTag>;
using TextureHandle     = Handle<TextureTag>;
using ShaderHandle      = Handle<ShaderTag>;
using PipelineHandle    = Handle<PipelineTag>;
using GeometryHandle    = Handle<GeometryTag>;
using SamplerHandle     = Handle<SamplerTag>;
using RenderPassHandle  = Handle<RenderPassTag>;
using FramebufferHandle = Handle<FramebufferTag>;
using QueryPoolHandle   = Handle<QueryPoolTag>;
using AccelerationStructureHandle = Handle<AccelerationStructureTag>;  // [RESERVE]

} // namespace rhi
