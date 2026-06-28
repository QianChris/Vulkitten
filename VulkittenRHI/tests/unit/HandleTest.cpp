// ============================================================
// Handle<Tag> Unit Tests
//
// Tests the strongly-typed GPU resource handle with ABA protection.
// ============================================================

#include <gtest/gtest.h>
#include "rhi/Core/Handle.hpp"

using namespace rhi;

TEST(HandleTest, DefaultConstruction_IsNull)
{
    BufferHandle h;
    EXPECT_FALSE(h.IsValid());
    EXPECT_EQ(h.GetId(), 0u);
    EXPECT_EQ(h.GetGeneration(), 0u);
}

TEST(HandleTest, ExplicitConstruction_StoresIdAndGeneration)
{
    BufferHandle h{42, 3};
    EXPECT_TRUE(h.IsValid());
    EXPECT_EQ(h.GetId(), 42u);
    EXPECT_EQ(h.GetGeneration(), 3u);
}

TEST(HandleTest, IdZero_IsNull_RegardlessOfGeneration)
{
    BufferHandle h{0, 5};
    EXPECT_FALSE(h.IsValid()); // Id==0 => null
    EXPECT_EQ(h.GetId(), 0u);
    EXPECT_EQ(h.GetGeneration(), 5u);
}

TEST(HandleTest, Equality_SameIdAndGen_AreEqual)
{
    BufferHandle a{1, 2};
    BufferHandle b{1, 2};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(HandleTest, Equality_DifferentId_NotEqual)
{
    BufferHandle a{1, 2};
    BufferHandle b{2, 2};
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(HandleTest, Equality_DifferentGeneration_NotEqual)
{
    BufferHandle a{1, 2};
    BufferHandle b{1, 3};
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(HandleTest, Equality_DefaultConstructed_BothNull)
{
    BufferHandle a;
    BufferHandle b;
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(HandleTest, LessThan_ById)
{
    BufferHandle a{1, 100};
    BufferHandle b{2, 1};
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(HandleTest, LessThan_SameId)
{
    BufferHandle a{5, 1};
    BufferHandle b{5, 2};
    EXPECT_FALSE(a < b); // same m_Id => !(5 < 5)
    EXPECT_FALSE(b < a); // same m_Id => !(5 < 5)
}

TEST(HandleTest, Hash_DifferentHandles_DifferentOrSame)
{
    BufferHandle::Hash hasher;
    BufferHandle a{1, 1};
    BufferHandle b{1, 1};
    BufferHandle c{2, 1};

    EXPECT_EQ(hasher(a), hasher(b)); // same handle => same hash
    // Different handles may or may not collide (hash function dependent)
    // Just verify it compiles and returns a value
    EXPECT_GT(hasher(a), 0u);
}

TEST(HandleTest, Hash_GenerationChangesHash)
{
    BufferHandle::Hash hasher;
    BufferHandle a{1, 1};
    BufferHandle b{1, 2};
    EXPECT_NE(hasher(a), hasher(b)); // different generation => different hash
}

TEST(HandleTest, DifferentTagTypes_AreSeparateTypes)
{
    // These are different types and should not be implicitly convertible
    BufferHandle buf{1, 1};
    TextureHandle tex{1, 1};

    // They share the same underlying ID/generation but are different types
    EXPECT_EQ(buf.GetId(), tex.GetId());
    EXPECT_EQ(buf.GetGeneration(), tex.GetGeneration());

    // BufferHandle and TextureHandle cannot be compared directly
    // (this is enforced at compile time by the template)
    static_assert(!std::is_same_v<BufferHandle, TextureHandle>,
                  "Different handle types must be distinct types");
}

TEST(HandleTest, MaximumIdValue)
{
    BufferHandle h{0xFFFFFFFF, 0xFFFFFFFF};
    EXPECT_TRUE(h.IsValid());
    EXPECT_EQ(h.GetId(), 0xFFFFFFFFu);
    EXPECT_EQ(h.GetGeneration(), 0xFFFFFFFFu);
}

TEST(HandleTest, AllResourceHandleTypes_CanBeCreated)
{
    // Verify all handle types compile and construct correctly
    BufferHandle buf{1, 1};
    TextureHandle tex{1, 1};
    ShaderHandle shd{1, 1};
    PipelineHandle pso{1, 1};
    GeometryHandle geo{1, 1};
    SamplerHandle smp{1, 1};
    RenderPassHandle rp{1, 1};
    FramebufferHandle fb{1, 1};
    QueryPoolHandle qp{1, 1};

    EXPECT_TRUE(buf.IsValid());
    EXPECT_TRUE(tex.IsValid());
    EXPECT_TRUE(shd.IsValid());
    EXPECT_TRUE(pso.IsValid());
    EXPECT_TRUE(geo.IsValid());
    EXPECT_TRUE(smp.IsValid());
    EXPECT_TRUE(rp.IsValid());
    EXPECT_TRUE(fb.IsValid());
    EXPECT_TRUE(qp.IsValid());
}
