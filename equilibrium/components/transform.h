#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "cglm_components.h"

typedef struct Position {
    float x;
    float y;
    float z;
} Position;

typedef struct Scale {
    float x;
    float y;
    float z;
} Scale;

typedef struct Rotation {
    float x;
    float y;
    float z;
} Rotation;

typedef struct Quaternion {
    float x;
    float y;
    float z;
    float w;
} Quaternion;

typedef struct Transform {
    mat4 value;
} Transform;

typedef struct Project {
    mat4 value;
} Project;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Position);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Scale);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Rotation);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Quaternion);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Transform);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Project);

EQUILIBRIUM_API
void TransformComponentsImport(world_t *world);

#endif