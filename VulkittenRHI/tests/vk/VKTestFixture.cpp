#include "VKTestFixture.hpp"
#include "vk/VKDevice.hpp"
#include "vk/VKCommandBuffer.hpp"
#include "vk/VKResources.hpp"

#include <GLFW/glfw3.h>
#include <cstring>
#include <cstdio>
#include <fstream>

namespace rhi {
namespace test {

std::vector<uint8_t> VKTestFixture::ReadFile(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    size_t size = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

void VKTestFixture::SetUp() {
    if (!glfwInit()) {
        FAIL() << "glfwInit() failed";
        return;
    }

    // Vulkan doesn't need a GL context - just a window handle for surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(static_cast<int>(kTestWidth),
                                 static_cast<int>(kTestHeight),
                                 "VulkittenRHI-VK-Test", nullptr, nullptr);
    if (!m_Window) {
        FAIL() << "glfwCreateWindow() failed";
        return;
    }

    m_Surface.SetWindow(m_Window);

    m_Device = std::make_unique<VKDevice>(&m_Surface, m_Resources);
    try {
        m_Device->Init();
    } catch (const std::exception& e) {
        FAIL() << "VKDevice::Init() failed: " << e.what();
        return;
    }

    m_CurrentFrame = m_Device->BeginFrame();
    m_CommandBuffer = m_Device->CreateCommandBuffer(m_CurrentFrame);
    m_CommandBuffer->Begin();
}

void VKTestFixture::TearDown() {
    if (m_CommandBuffer) {
        m_CommandBuffer->End();
        m_CommandBuffer.reset();
    }
    if (m_Device) {
        m_Device->EndFrame(m_CurrentFrame);
        m_Device->Shutdown();
        m_Device.reset();
    }
    m_Resources.DestroyAll();
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

ICommandBuffer& VKTestFixture::GetCommandBuffer() { return *m_CommandBuffer; }

BufferHandle VKTestFixture::CreateVertexBuffer(const void* data, uint64_t size) {
    BufferDesc desc;
    desc.Size = size;
    desc.Usage = BufferUsage::Vertex;
    desc.Memory = MemoryProperty::HostVisible;
    return m_Device->CreateBuffer(desc, data);
}

BufferHandle VKTestFixture::CreateStorageBuffer(uint64_t size, const void* data) {
    BufferDesc desc;
    desc.Size = size;
    desc.Usage = BufferUsage::Storage;
    desc.Memory = MemoryProperty::HostVisible;
    desc.CpuAccessible = true;
    return m_Device->CreateBuffer(desc, data);
}

BufferHandle VKTestFixture::CreateIndirectDispatchBuffer(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t params[3] = {x, y, z};
    return CreateStorageBuffer(sizeof(params), params);
}

ShaderHandle VKTestFixture::LoadSpirvFile(ShaderStage stage, const char* filepath) {
    // Prepend VULKITTEN_RHI_DIR to resolve relative paths
    std::string fullPath = std::string(VULKITTEN_RHI_DIR) + "/" + filepath;
    auto data = ReadFile(fullPath.c_str());
    if (data.empty()) {
        data = ReadFile(filepath); // fallback to path as-is
    }
    if (data.empty()) {
        fprintf(stderr, "VKTestFixture: failed to read %s\n", filepath);
        return {};
    }
    ShaderBytecode bc{data.data(), data.size(), "main"};
    return m_Device->CreateShader(stage, bc);
}

PipelineHandle VKTestFixture::CreateComputePipeline(ShaderHandle cs) {
    PipelineDesc desc;
    desc.ComputeShader = cs;
    return m_Device->CreatePipeline(desc);
}

GeometryHandle VKTestFixture::CreateSimpleGeometry(BufferHandle vb, uint32_t vtxCount, uint32_t stride) {
    GeometryDesc desc;
    desc.VertexBuffers[0] = vb;
    desc.VertexBufferCount = 1;
    desc.VertexCount = vtxCount;
    return m_Device->CreateGeometry(desc);
}

std::vector<uint8_t> VKTestFixture::ReadBufferData(BufferHandle handle, uint64_t offset, uint64_t size) {
    auto* buf = m_Device->GetBuffer(handle);
    if (!buf) return {};
    void* ptr = buf->Map(offset, size);
    if (!ptr) return {};
    std::vector<uint8_t> result(static_cast<size_t>(size));
    std::memcpy(result.data(), ptr, static_cast<size_t>(size));
    buf->Unmap();
    return result;
}

} // namespace test
} // namespace rhi
