#ifndef PARTICLE_INOUT_H
#define PARTICLE_INOUT_H

#ifdef PARTICLE_ARG_INOUT
layout(binding = 0, std430) buffer ParticleInArgBuffer { ParticleArg argsRead; };
layout(binding = 1, std430) buffer ParticleOutArgBuffer { ParticleArg argsWrite; };
layout(binding = 2, std140) uniform ParticleUBOBuffer { ParticleUBO ubo; };
#endif // PARTICLE_ARG_INOUT

#ifdef PARTICLE_SIM_INOUT
layout(std430, binding = 0) buffer ParticleInArgBuffer { ParticleArg argsRead; };
layout(std430, binding = 1) buffer ParticleInBuffer { Particle particlesRead[]; };
layout(std430, binding = 2) buffer ParticleOutArgBuffer { ParticleArg argsWrite; };
layout(std430, binding = 3) buffer ParticleOutBuffer { Particle particlesWrite[]; };
layout(std140, binding = 4) uniform ParticleUBOBuffer { ParticleUBO ubo; };
#endif // PARTICLE_SIM_INOUT

#endif // PARTICLE_INOUT_H