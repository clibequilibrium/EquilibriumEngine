#include "common.sh"
#include <bgfx_shader.sh>
#include "samplers.sh"
#include "pbr.sh"
#include "lights.sh"
#include "util.sh"

// G-Buffer
SAMPLER2D(s_texDiffuseA, SAMPLER_DEFERRED_DIFFUSE_A);
SAMPLER2D(s_texNormal, SAMPLER_DEFERRED_NORMAL);
SAMPLER2D(s_texF0Metallic, SAMPLER_DEFERRED_F0_METALLIC);
SAMPLER2D(s_texEmissiveOcclusion, SAMPLER_DEFERRED_EMISSIVE_OCCLUSION);
SAMPLER2D(s_texDepth, SAMPLER_DEFERRED_DEPTH);

uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;
uniform vec4 u_skyLuminance;
uniform vec4 u_parameters;

void main() {
  vec2 texcoord = gl_FragCoord.xy / u_viewRect.zw;
  vec3 diffuseColor = texture2D(s_texDiffuseA, texcoord).xyz;
  vec4 emissiveOcclusion = texture2D(s_texEmissiveOcclusion, texcoord);
  vec3 emissive = emissiveOcclusion.xyz;
  float occlusion = emissiveOcclusion.w;
  vec4 diffuseA = texture2D(s_texDiffuseA, texcoord);
  vec3 N = unpackNormal(texture2D(s_texNormal, texcoord).xy);
  vec4 F0Metallic = texture2D(s_texF0Metallic, texcoord);

  // unpack material parameters used by the PBR BRDF function
  PBRMaterial mat;
  mat.diffuseColor = diffuseA.xyz;
  mat.a = diffuseA.w;
  mat.F0 = F0Metallic.xyz;
  mat.metallic = F0Metallic.w;

  // get fragment position
  // rendering happens in view space
  vec4 screen = gl_FragCoord;
  screen.z = texture2D(s_texDepth, texcoord).x;
  vec3 fragPos = screen2Eye(screen).xyz;

  vec3 radianceOut = vec3_splat(0.0);

  // Directional light
  vec3 skyDirection = vec3(0.0, 0.0, 1.0);
  vec3 L = mul(u_view, vec4(u_sunDirection.xyz, 0.0)).xyz;
  vec3 V = normalize(-fragPos);
  float NoV = abs(dot(N, V)) + 1e-5;
  vec3 msFactor = multipleScatteringFactor(mat, NoV);

  float diffuseSun = max(0.0, dot(N, L));
  float diffuseSky = 1.0 + 0.5 * dot(N, skyDirection);

  float NoL = saturate(dot(N, L));

  // Apply sky color
  vec3 color =
      diffuseSun * u_sunLuminance.rgb + (diffuseSky * u_skyLuminance.rgb + 0.01) * occlusion;
  color *= 0.5;

  radianceOut += BRDF(V, L, N, NoV, NoL, mat) * color * msFactor * NoL;

  radianceOut += getAmbientLight().irradiance * diffuseColor * occlusion;
  radianceOut += emissive;

  gl_FragColor = vec4(radianceOut, 1.0);
}
