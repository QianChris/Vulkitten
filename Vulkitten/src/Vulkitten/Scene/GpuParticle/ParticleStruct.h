#ifndef PARTICLE_STRUCT_H
#define PARTICLE_STRUCT_H

#ifdef __cplusplus
#include <glm/glm.hpp>
using vec3 = glm::vec3;
using uint = uint32_t;
#endif

#define PARTICLE_MAX_WGS 128

struct ParticleArg {
    uint ngx;
    uint ngy;
    uint ngz;
    uint padding0;

    uint vertCount;
    uint primCount;
    uint firstVert;
    uint firstPrim;

    uint currCount;
    uint maxCount;
    uint padding1;
    uint padding2;
};

struct Particle
{
    vec3 Position;
    float LifeTime;
    vec3 Velocity;
    float Size;
};

struct ParticleUBO
{
    float DeltaTime;
    float TotalTime;
    uint MaxParticles;
    uint EmitCount;
};

#endif // PARTICLE_STRUCT_H