#ifndef PARTICLE_STRUCT_H
#define PARTICLE_STRUCT_H

struct Particle
{
    vec3 Position;
    float LifeTime;
    vec3 Velocity;
    float Size;
};

struct ParticleUBO
{

};

#endif // PARTICLE_STRUCT_H