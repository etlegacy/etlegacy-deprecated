/* lightMapping_fp.glsl */
#include "lib/reliefMapping"

uniform bool SHOW_LIGHTMAP;
uniform bool SHOW_DELUXEMAP;

uniform sampler2D u_DiffuseMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_SpecularMap;
uniform sampler2D u_LightMap;
uniform sampler2D u_DeluxeMap;
uniform int       u_AlphaTest;
uniform vec3      u_ViewOrigin;
uniform float     u_DepthScale;
uniform vec4      u_PortalPlane;

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec2 var_TexSpecular;
varying vec2 var_TexLight;

varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec3 var_Normal;

varying vec4 var_Color;
//new
uniform vec3  u_AmbientColor;


void main()
{

	vec4 lightmapColor  = texture2D(u_LightMap, var_TexLight);
	vec4 deluxemapColor = texture2D(u_DeluxeMap, var_TexLight);

	
#if defined(USE_PORTAL_CLIPPING)
	float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
	if (dist < 0.0)
	{
		discard;
		return;
	}
#endif

#if defined(USE_NORMAL_MAPPING)

	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
	vec2 texSpecular = var_TexSpecular.st;

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

	// compute view direction in world space
	vec3 I = normalize(u_ViewOrigin - var_Position);

#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction
    mat3 worldToTangentMatrix;
	worldToTangentMatrix = transpose(tangentToWorldMatrix);
   
    // compute view direction in tangent space
	vec3 V = worldToTangentMatrix * (u_ViewOrigin - var_Position.xyz);
	 V = normalize(V);

	// size and start position of search in texture space
	vec2 S = V.xy * -u_DepthScale / V.z;

   float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;


	texDiffuse.st  += texOffset;
	texNormal.st   += texOffset;
	texSpecular.st += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

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


	// compute normal in world space from normalmap
	vec3 N = (2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5));
	//just normalize as we dont parralax
	N = normalize(N);

	// compute light direction in world space
	vec3 L = 2.0 * (deluxemapColor.xyz - 0.5);
	//always normalize light
	normalize(L);

	float NdotL = clamp(dot(N, L), 0.0, 1.0);

	// compute half angle in world space
	vec3 H = normalize(L + I);
	 
	 //compute the ambient term
	//taken from the map
    vec3 ambient = u_AmbientColor * lightmapColor.rgb;
	
	// compute light color from world space lightmap
	vec3 lightColor = lightmapColor.rgb *NdotL;

	// compute the specular term
	vec3 reflectDir = reflect(-L, N);
	vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb*pow(max(dot(I, reflectDir), 0.0), r_SpecularExponent) * r_SpecularScale;

	

	// compute final color
	vec4 color = diffuse;
	
	color.rgb *= lightColor + ambient +specular;
	
	  
	// for smooth terrain blending
	color.a   *= var_Color.a; 

	gl_FragColor = color;
	gl_FragColor.a = color.a;
#else  // RENDER GENERIC
//GENERIC NOT touched!
	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, var_TexDiffuseNormal.st);

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

	vec3 N;

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

	vec3 specular = vec3(0.0, 0.0, 0.0);

	// compute light color from object space lightmap
	vec3 lightColor = lightmapColor.rgb;

	vec4 color = diffuse;
	color.rgb *= lightColor;
	// for smooth terrain blending
	color.a   *= var_Color.a; 
#endif

	// convert normal to [0,1] color space
	N = N * 0.5 + 0.5;

	gl_FragColor = color;

	if (SHOW_DELUXEMAP)
	{
		color = deluxemapColor;
	}
	else if (SHOW_LIGHTMAP)
	{
		color = lightmapColor;
	}

	gl_FragColor = color;
}
