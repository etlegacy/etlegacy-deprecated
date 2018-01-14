/* vertexLighting_DBS_entity_fp.glsl */
#include "lib/reliefMapping"

uniform sampler2D u_DiffuseMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_SpecularMap;

uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;

uniform int   u_AlphaTest;
uniform vec3  u_ViewOrigin;
uniform vec3  u_AmbientColor;
uniform vec3  u_LightDir;
uniform vec3  u_LightColor;
uniform float u_SpecularExponent;
uniform float u_DepthScale;
uniform vec4  u_PortalPlane;
uniform float u_LightWrapAround;

varying vec3 var_Position;
varying vec2 var_TexDiffuse;
#if defined(USE_NORMAL_MAPPING)
varying vec2 var_TexNormal;
varying vec2 var_TexSpecular;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
#endif
varying vec3 var_Normal;

void main()
{
#if defined(USE_PORTAL_CLIPPING)
	{
		float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if (dist < 0.0)
		{
			discard;
			return;
		}
	}
#endif

	// compute light direction in world space
	vec3 L = normalize(u_LightDir);

	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position);

	vec2 texDiffuse = var_TexDiffuse.st;

#if defined(USE_NORMAL_MAPPING)
	// invert tangent space for two sided surfaces
	mat3 tangentToWorldMatrix;

#if defined(TWOSIDED)
	if (gl_FrontFacing)
	{
		tangentToWorldMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif
	{
		tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	}

	vec2 texNormal   = var_TexNormal.st;
	vec2 texSpecular = var_TexSpecular.st;

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction

	mat3 worldToTangentMatrix;
	worldToTangentMatrix = transpose(tangentToWorldMatrix);

	// compute view direction in tangent space
	vec3 Vts = worldToTangentMatrix * (u_ViewOrigin - var_Position.xyz);
	Vts = normalize(Vts);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;


	texDiffuse.st  += texOffset;
	texNormal.st   += texOffset;
	texSpecular.st += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute normal in world space from normalmap
	vec3 N = normalize(tangentToWorldMatrix * (2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5)));

	// compute half angle in world space
	vec3 H = normalize(L + V);

	// compute the specular term
#if defined(USE_REFLECTIVE_SPECULAR)

	vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb;

	vec4 envColor0 = textureCube(u_EnvironmentMap0, reflect(-V, N)).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, reflect(-V, N)).rgba;

	specular *= mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;

	// Blinn-Phong
	float NH = clamp(dot(N, H), 0, 1);
	specular *= u_LightColor * pow(NH, r_SpecularExponent2) * r_SpecularScale;


#else

	// compute the specular term
	//using Gouraud shading
	vec3 reflectDir = reflect(-L, N);
	
	vec3  specular = texture2D(u_SpecularMap, texSpecular).rgb * u_LightColor * pow(max(dot(V, reflectDir), 0.0), r_SpecularExponent2) * r_SpecularScale;

#endif // USE_REFLECTIVE_SPECULAR


#else // USE_NORMAL_MAPPING
    {
	vec3 N;
	}
#if defined(TWOSIDED)
	if (gl_FrontFacing)
	{
		N = -normalize(var_Normal);
	}
	else
#endif
	{
		N = normalize(var_Normal);
	}

	

#endif // USE_NORMAL_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

//dont know if this works, have to test


#if defined(USE_ALPHA_TESTING)
	if (u_AlphaTest == ATEST_GT_0 && diffuse.a <= 0.0)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_LT_128 && diffuse.a >= 0.5)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_GE_128 && diffuse.a < 0.5)
	{
		discard;
		return;
	}
#endif

    // compute the light term
	#if defined(r_WrapAroundLighting)
	float NL = max(dot(N, L) + u_LightWrapAround, 0.0) / clamp(1.0 + u_LightWrapAround, 0.0);

#else
    //floating the lightraypower
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif
    //compute the ambient term
	//taken from the map
    
    vec3 ambient = u_AmbientColor * u_LightColor;

	// compute diffuse lightning
	float NormalizedLight = clamp(dot(N, L), 0.0, 1.0);
	vec3 light = u_LightColor * NormalizedLight;
	//clamp(light, 0.0, 1.0);

	
	
	// compute final color
#if defined(USE_NORMAL_MAPPING)
	vec3 result = (ambient + light + specular) * diffuse.rgb;
#else
    vec3 result = (ambient + light) * diffuse.rgb;
#endif	
   // convert normal to [0,1] color space
	N = N * 0.5 + 0.5;

	gl_FragColor = vec4(result, 1.0);

}
