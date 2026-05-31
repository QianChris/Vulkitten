#include "vktpch.h"
#include "GpuParticle.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Perf/Instrumentor.h"

#include <glad/glad.h>

namespace Vulkitten {

    // =========================================================================
    // GpuEmitterManager
    // =========================================================================

    GpuEmitterInstance* GpuEmitterManager::GetOrCreateEmitterInstance(Entity entity)
    {
        if (!m_Initialized)
        {
            Initialize();
        }

        if (m_EmitterInstances.find(entity) != m_EmitterInstances.end())
        {
            return m_EmitterInstances[entity].get();
        }
        else
        {
            auto& emitterComponent = entity.GetComponent<GpuEmitterComponent>();
            auto& transformComponent = entity.GetComponent<TransformComponent>();

            auto emitterInstance = std::make_unique<GpuEmitterInstance>(
                this, emitterComponent.MaxParticles, transformComponent.Position);

            GpuEmitterInstance* emitterInstancePtr = emitterInstance.get();
            m_EmitterInstances[entity] = std::move(emitterInstance);
            return emitterInstancePtr;
        }
    }

    void GpuEmitterManager::Initialize()
    {
        VKT_PROFILE_FUNCTION();

        // Load compute shaders into the ShaderLibrary.
        // The .comp files use #include directives that are resolved by the
        // custom preprocessor in OpenGLShader::CompileCompute.
        m_ShaderLibrary.Add("ParticleSimArg",
            Shader::CreateCompute("ParticleSimArg",
                "sandbox://assets/computeshaders/ParticleSimArg.comp"));

        m_ShaderLibrary.Add("ParticleSim",
            Shader::CreateCompute("ParticleSim",
                "sandbox://assets/computeshaders/ParticleSim.comp"));

        m_ShaderLibrary.Add("ParticleEmit",
            Shader::CreateCompute("ParticleEmit",
                "sandbox://assets/computeshaders/ParticleEmit.comp"));

        m_ShaderLibrary.Add("ParticleRenderArg",
            Shader::CreateCompute("ParticleRenderArg",
                "sandbox://assets/computeshaders/ParticleRenderArg.comp"));

        // Load the particle render shader (vertex + fragment)
        m_ShaderLibrary.Load("sandbox://assets/shaders/Particle.shader");

        m_Initialized = true;
    }

    // =========================================================================
    // GpuEmitterInstance
    // =========================================================================

    GpuEmitterInstance::~GpuEmitterInstance()
    {
        if (m_Initialized)
        {
            glDeleteBuffers(2, m_ArgSSBO);
            glDeleteBuffers(2, m_ParticleSSBO);
            glDeleteBuffers(1, &m_UBO);
            glDeleteBuffers(1, &m_RenderUBO);
            glDeleteVertexArrays(1, &m_VAO);
        }
    }

    void GpuEmitterInstance::Initialize()
    {
        VKT_PROFILE_FUNCTION();

        // ---- Particle SSBOs (double-buffered) ----
        glGenBuffers(2, m_ParticleSSBO);
        for (int i = 0; i < 2; ++i)
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ParticleSSBO[i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                m_MaxParticles * sizeof(Particle),
                nullptr,
                GL_DYNAMIC_COPY);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        // ---- Arg SSBOs (double-buffered) ----
        {
            ParticleArg initialArg = {};
            initialArg.maxCount = m_MaxParticles;
            // currCount defaults to 0 (no particles initially)

            glGenBuffers(2, m_ArgSSBO);
            for (int i = 0; i < 2; ++i)
            {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ArgSSBO[i]);
                glBufferData(GL_SHADER_STORAGE_BUFFER,
                    sizeof(ParticleArg),
                    &initialArg,
                    GL_DYNAMIC_COPY);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            }
        }

        // ---- UBO ----
        glGenBuffers(1, &m_UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ParticleUBO), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // ---- Render UBO (projection, view, screendim) ----
        glGenBuffers(1, &m_RenderUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_RenderUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2 + sizeof(glm::vec2),
                     nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // ---- VAO for rendering particles as points ----
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        glEnableVertexAttribArray(0);
        glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(0, 0);

        glEnableVertexAttribArray(1);
        glVertexAttribFormat(1, 1, GL_FLOAT, GL_FALSE, offsetof(Particle, Size));
        glVertexAttribBinding(1, 0);

        glBindVertexArray(0);

        m_Initialized = true;
    }

    void GpuEmitterInstance::Update()
    {
        VKT_PROFILE_FUNCTION();

        if (m_MaxParticles == 0) return;
        if (!m_Initialized) Initialize();

        const uint32_t readIdx = m_FrameIndex;
        const uint32_t writeIdx = 1u - m_FrameIndex;
        auto& shaderLib = m_Manager->GetShaderLibrary();

        // ---- Fill UBO ----
        const float dt = 1.0f / 60.0f;  // TODO: get real deltaTime
        m_TotalTime += dt;

        // TODO: make Gravity / EmitCount configurable from GpuEmitterComponent
        const ParticleUBO ubo = {
            dt,                             // DeltaTime
            m_TotalTime,                    // TotalTime
            m_MaxParticles,                 // MaxParticles
            10u,                            // EmitCount
            glm::vec3(0.0f, -9.8f, 0.0f),  // Gravity
            0.0f                            // useCollide
        };

        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ParticleUBO), &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // ---- 1. ParticleSimArg — reset args for the new frame ----
        {
            auto shader = shaderLib.Get("ParticleSimArg");
            shader->Bind();

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ArgSSBO[readIdx]);  // argsRead
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ArgSSBO[writeIdx]); // argsWrite
            glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_UBO);                      // ubo

            glDispatchCompute(1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // ---- 2. ParticleSim — simulate existing particles, indirect dispatch ----
        {
            auto shader = shaderLib.Get("ParticleSim");
            shader->Bind();

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ArgSSBO[readIdx]);      // argsRead
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ParticleSSBO[readIdx]);  // particlesRead
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_ArgSSBO[writeIdx]);     // argsWrite
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_ParticleSSBO[writeIdx]); // particlesWrite
            glBindBufferBase(GL_UNIFORM_BUFFER, 4, m_UBO);                           // ubo

            // Indirect dispatch: ngx/ngy/ngz at offset 0 were set by ParticleSimArg
            glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_ArgSSBO[writeIdx]);
            glDispatchComputeIndirect(0);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
        }

        // ---- 3. ParticleEmit — emit new particles, direct dispatch ----
        {
            auto shader = shaderLib.Get("ParticleEmit");
            shader->Bind();

            // Bind the same output buffers — emit appends after sim survivors
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ArgSSBO[readIdx]);      // argsRead  (unused by emit)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ParticleSSBO[readIdx]);  // particlesRead (unused)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_ArgSSBO[writeIdx]);     // argsWrite
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_ParticleSSBO[writeIdx]); // particlesWrite
            glBindBufferBase(GL_UNIFORM_BUFFER, 4, m_UBO);                           // ubo

            uint32_t emitGroups = (ubo.EmitCount + PARTICLE_MAX_WGS - 1) / PARTICLE_MAX_WGS;
            if (emitGroups < 1) emitGroups = 1;
            glDispatchCompute(emitGroups, 1, 1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // ---- 4. ParticleRenderArg — set up indirect draw arguments ----
        {
            auto shader = shaderLib.Get("ParticleRenderArg");
            shader->Bind();

            // argsRead (binding 0) is unused by this shader; bind writeIdx for argsWrite (binding 1)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);                       // argsRead (unused)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ArgSSBO[writeIdx]);     // argsWrite
            glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_UBO);                           // ubo

            glDispatchCompute(1, 1, 1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
        }

        // ---- Swap ping-pong buffers ----
        m_FrameIndex = writeIdx;
    }

    void GpuEmitterInstance::Render(Camera& camera)
    {
        VKT_PROFILE_FUNCTION();

        if (!m_Initialized || m_MaxParticles == 0) return;

        // Enable program point size so gl_PointSize in the vertex shader takes effect
        glEnable(GL_PROGRAM_POINT_SIZE);

        const uint32_t readIdx = m_FrameIndex;
        auto& shaderLib = m_Manager->GetShaderLibrary();

        // --- Bind render shader ---
        auto shader = shaderLib.Get("Particle");
        shader->Bind();

        // --- Fill camera UBO (matches GLSL std140 layout) ---
        glm::mat4 projection = camera.GetProjectionMatrix();
        glm::mat4 viewProj = camera.GetViewProjectionMatrix();
        // Derive view = inverse(projection) * viewProjection
        glm::mat4 view = glm::inverse(projection) * viewProj;

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glm::vec2 screendim((float)viewport[2], (float)viewport[3]);

        // Layout: mat4(64) + mat4(64) + vec2(8) + pad(8) = 144 bytes
        struct CameraUBO {
            glm::mat4 projection;
            glm::mat4 view;
            glm::vec2 screendim;
            float _pad[2];
        };
        CameraUBO camUbo = { projection, view, screendim, {0.0f, 0.0f} };

        glBindBuffer(GL_UNIFORM_BUFFER, m_RenderUBO);

        //glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraUBO), &camUbo);

        void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
        memcpy(ptr, &camUbo, sizeof(CameraUBO));
        glUnmapBuffer(GL_UNIFORM_BUFFER);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_RenderUBO);

        // --- Bind VAO and set vertex buffer to current particle buffer ---
        glBindVertexArray(m_VAO);
        glBindVertexBuffer(0, m_ParticleSSBO[readIdx], 0, sizeof(Particle));

        glEnable(GL_PROGRAM_POINT_SIZE);

        // --- Indirect draw ---
        // ParticleRenderArg wrote the draw command at offsetof(ParticleArg, vertCount)
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_ArgSSBO[readIdx]);
        glDrawArraysIndirect(GL_POINTS, (const void*)offsetof(ParticleArg, vertCount));

        glDisable(GL_PROGRAM_POINT_SIZE);

        glBindVertexArray(0);
    }

}