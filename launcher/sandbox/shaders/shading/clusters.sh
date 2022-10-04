#ifndef CLUSTERS_SH_HEADER_GUARD
#define CLUSTERS_SH_HEADER_GUARD

#include <bgfx_shader.sh>
#include <bgfx_compute.sh>
#include "samplers.sh"
#include "util.sh"

// taken from Doom
// http://advances.realtimerendering.com/s2016/Siggraph2016_idTech6.pdf

#define CLUSTERS_X 16
#define CLUSTERS_Y 8
#define CLUSTERS_Z 24

// workgroup size of the culling compute shader
// D3D compute shaders only allow up to 1024 threads per workgroup
// GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS also only guarantees 1024
#define CLUSTERS_X_THREADS 16
#define CLUSTERS_Y_THREADS 8
#define CLUSTERS_Z_THREADS 4

#define MAX_LIGHTS_PER_CLUSTER 100

uniform vec4 u_clusterSizesVec; // cluster size in screen coordinates (pixels)
uniform vec4 u_zNearFarVec;

#define u_clusterSizes u_clusterSizesVec.xy
#define u_zNear        u_zNearFarVec.x
#define u_zFar         u_zNearFarVec.y

#ifdef WRITE_CLUSTERS
    #define CLUSTER_BUFFER BUFFER_RW
#else
    #define CLUSTER_BUFFER BUFFER_RO
#endif

// light indices belonging to clusters
CLUSTER_BUFFER(b_clusterLightIndices, uint, SAMPLER_CLUSTERS_LIGHTINDICES);
// for each cluster: (start index in b_clusterLightIndices, number of point lights, empty, empty)
CLUSTER_BUFFER(b_clusterLightGrid, uvec4, SAMPLER_CLUSTERS_LIGHTGRID);

// these are only needed for building clusters and light culling, not in the fragment shader
#ifdef WRITE_CLUSTERS
// list of clusters (2 vec4's each, min + max pos for AABB)
CLUSTER_BUFFER(b_clusters, vec4, SAMPLER_CLUSTERS_CLUSTERS);
// atomic counter for building the light grid
// must be reset to 0 every frame
CLUSTER_BUFFER(b_globalIndex, uint, SAMPLER_CLUSTERS_ATOMICINDEX);
#endif

struct Cluster
{
    vec3 minBounds;
    vec3 maxBounds;
};

struct LightGrid
{
    uint offset;
    uint pointLights;
};

#ifdef WRITE_CLUSTERS
Cluster getCluster(uint index)
{
    Cluster cluster;
    cluster.minBounds = b_clusters[2 * index + 0].xyz;
    cluster.maxBounds = b_clusters[2 * index + 1].xyz;
    return cluster;
}
#endif

LightGrid getLightGrid(uint cluster)
{
    uvec4 gridvec = b_clusterLightGrid[cluster];
    LightGrid grid;
    grid.offset = gridvec.x;
    grid.pointLights = gridvec.y;
    return grid;
}

uint getGridLightIndex(uint start, uint offset)
{
    return b_clusterLightIndices[start + offset];
}

// cluster depth index from depth in screen coordinates (gl_FragCoord.z)
uint getClusterZIndex(float screenDepth)
{
    // this can be calculated on the CPU and passed as a uniform
    // only leaving it here to keep most of the relevant code in the shaders for learning purposes
    float scale = float(CLUSTERS_Z) / log(u_zFar / u_zNear);
    float bias = -(float(CLUSTERS_Z) * log(u_zNear) / log(u_zFar / u_zNear));

    float eyeDepth = screen2EyeDepth(screenDepth, u_zNear, u_zFar);
    uint zIndex = uint(max(log(eyeDepth) * scale + bias, 0.0));
    return zIndex;
}

// cluster index from fragment position in window coordinates (gl_FragCoord)
uint getClusterIndex(vec4 fragCoord)
{
    uint zIndex = getClusterZIndex(fragCoord.z);
    uvec3 indices = uvec3(uvec2(fragCoord.xy / u_clusterSizes.xy), zIndex);
    uint cluster = (CLUSTERS_X * CLUSTERS_Y) * indices.z +
                   CLUSTERS_X * indices.y +
                   indices.x;
    return cluster;
}

#endif // CLUSTERS_SH_HEADER_GUARD
