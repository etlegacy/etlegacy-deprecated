/* reflection_CB_vp.glsl */
#include "lib/vertexSkinning"
#include "lib/vertexAnimation"
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
attribute vec3 attr_Normal;

attribute vec4 attr_Position2;
attribute vec3 attr_Tangent2;
attribute vec3 attr_Binormal2;
attribute vec3 attr_Normal2;

uniform float u_VertexInterpolation;
uniform mat4  u_NormalTextureMatrix;
uniform mat4  u_ModelMatrix;
uniform mat4  u_ModelViewProjectionMatrix;

uniform float u_Time;

varying vec3 var_Position;
varying vec2 var_TexNormal;
varying vec4 var_Tangent;
varying vec4 var_Binormal;
varying vec4 var_Normal;

void    main()
{
	vec4 position;
	vec3 tangent;
	vec3 binormal;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)

	#if defined(USE_NORMAL_MAPPING)
	VertexSkinning_P_TBN(attr_Position, attr_Tangent, attr_Binormal, attr_Normal,
	                     position, tangent, binormal, normal);
	#else
	VertexSkinning_P_N(attr_Position, attr_Normal,
	                   position, normal);
	#endif

#elif defined(USE_VERTEX_ANIMATION)

	#if defined(USE_NORMAL_MAPPING)
	VertexAnimation_P_TBN(attr_Position, attr_Position2,
	                      attr_Tangent, attr_Tangent2,
	                      attr_Binormal, attr_Binormal2,
	                      attr_Normal, attr_Normal2,
	                      u_VertexInterpolation,
	                      position, tangent, binormal, normal);
	#else
	VertexAnimation_P_N(attr_Position, attr_Position2,
	                    attr_Normal, attr_Normal2,
	                    u_VertexInterpolation,
	                    position, normal);
	#endif

#else
	position = attr_Position;

	#if defined(USE_NORMAL_MAPPING)
	tangent  = attr_Tangent;
	binormal = attr_Binormal;
	#endif

	normal = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position,
	                           normal,
	                           attr_TexCoord0.st,
	                           u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz  = (u_ModelMatrix * vec4(tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(binormal, 0.0)).xyz;
	#endif

	var_Normal.xyz = (u_ModelMatrix * vec4(normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexNormal = (u_NormalTextureMatrix * attr_TexCoord0).st;
#endif
}
