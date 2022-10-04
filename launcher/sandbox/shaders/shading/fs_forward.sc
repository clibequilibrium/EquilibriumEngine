$input v_worldpos, v_normal, v_tangent, v_texcoord0

// all unit-vectors need to be normalized in the fragment shader, the interpolation of vertex shader
// output doesn't preserve length

// define samplers and uniforms for retrieving material parameters
#define READ_MATERIAL

#include "common.sh"
#include <bgfx_shader.sh>
#include <bgfx_compute.sh>
#include "util.sh"
#include "pbr.sh"
#include "lights.sh"

uniform vec4 u_camPos;
uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;
uniform vec4 u_skyLuminance;
uniform vec4 u_parameters;

void main() {
  PBRMaterial mat = pbrMaterial(v_texcoord0);
  // convert normal map from tangent space -> world space (= space of v_tangent, etc.)
  vec3 N = convertTangentNormal(v_normal, v_tangent, mat.normal);
  mat.a = specularAntiAliasing(N, mat.a);

  // shading

  vec3 camPos = u_camPos.xyz;
  vec3 fragPos = v_worldpos;

  vec3 V = normalize(camPos - fragPos);
  float NoV = abs(dot(N, V)) + 1e-5;

  if (whiteFurnaceEnabled()) {
    mat.F0 = vec3_splat(1.0);
    vec3 msFactor = multipleScatteringFactor(mat, NoV);
    vec3 radianceOut = whiteFurnace(NoV, mat) * msFactor;
    gl_FragColor = vec4(radianceOut, 1.0);
    return;
  }

  vec3 msFactor = multipleScatteringFactor(mat, NoV);

  vec3 radianceOut = vec3_splat(0.0);

  uint lights = pointLightCount();
  for (uint i = 0; i < lights; i++) {
    PointLight light = getPointLight(i);
    float dist = distance(light.position, fragPos);
    float attenuation = smoothAttenuation(dist, light.radius);
    if (attenuation > 0.0) {
      vec3 L = normalize(light.position - fragPos);
      vec3 radianceIn = light.intensity * attenuation;
      float NoL = saturate(dot(N, L));
      radianceOut += BRDF(V, L, N, NoV, NoL, mat) * msFactor * radianceIn * NoL;
    }
  }

  // Directional light
  vec3 skyDirection = vec3(0.0, 0.0, 1.0);
  vec3 L = normalize(u_sunDirection.xyz);

  float diffuseSun = max(0.0, dot(N, L));
  float diffuseSky = 1.0 + 0.5 * dot(N, skyDirection);

  float NoL = saturate(dot(N, L));

  // Apply sky color
  vec3 color =
      diffuseSun * u_sunLuminance.rgb + (diffuseSky * u_skyLuminance.rgb + 0.01) * mat.occlusion;
  color *= 0.5;

  radianceOut += BRDF(V, L, N, NoV, NoL, mat) * color * msFactor * NoL;
  radianceOut += getAmbientLight().irradiance * mat.diffuseColor * mat.occlusion;
  radianceOut += mat.emissive;

  // output goes straight to HDR framebuffer, no clamping
  // tonemapping happens in final blit

  gl_FragColor.rgb = radianceOut;
  gl_FragColor.a = mat.albedo.a;
}
