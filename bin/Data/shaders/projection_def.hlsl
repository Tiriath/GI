/// \file projection_def.hlsl
/// \brief This file contains methods used to compute projections.
/// \author Raffaele D. Facendola

#ifndef PROJECTION_DEF_HLSL_
#define PROJECTION_DEF_HLSL_

cbuffer PerLight {

	float4x4 gLightMatrix;			// Light world matrix

	float gNearPlane;				// Near clipping plane

	float gFarPlane;				// Far clipping plane

	float2 padding;

};

/// \brief Convert a coordinate from projection space to texture space.
/// \param position_ps Coordinates in projection space.
/// \return Returns the associated coordinates in uv space.
float2 ProjectionSpaceToTextureSpace(float4 position_ps) {

	return float2(position_ps.x * 0.5f + 0.5f,
				  0.5f - position_ps.y * 0.5f);

}

/// \brief Convert a coordinate from texture space to projection space.
/// \param position_ts Coordinates in texture space. [0; 1] x [0; 1]
/// \param depth Depth of the projected point.
/// \return Returns the associated coordinates in projection space.
float4 TextureSpaceToProjectionSpace(float2 position_ts, float depth) {

	return float4(position_ts * float2(2.0, -2.0) + float2(-1.0f, 1.0f),
				  depth,
				  1.0f);

}

/// \brief Unproject a coordinate from projection space to another space.
/// \param space Matrix associated to the destination space.
float4 Unproject(float4x4 space, float4 position_ps) {

	float4 position_ws = mul(space, position_ps);

	position_ws /= position_ws.w;

	return position_ws;

}

float2 Rotate(float2 p, float angle) {

	float cos_angle = cos(angle);
	float sin_angle = sin(angle);

	return float2(p.x * cos_angle - p.y * sin_angle,
				  p.x * sin_angle + p.y * cos_angle);
	
}

/// \brief Project a point into octahedron space.
/// \param position Coordinates of the point to project.
/// \param near_clipping Near clipping plane.
/// \param far_plane Far clipping plane.
/// \param flip Whether to flip the coordinates to fit the position inside the back pyramid
float4 ProjectToOctahedronSpace(float3 position, float near_plane, float far_plane, bool flip) {

	float3 abs_position = abs(position);

	float depth = dot(1, abs_position);																// Manhattan depth

	float3 I = position / depth;																	// Normalize using Manhattan distance
	
	I.z = (far_plane * depth - far_plane * near_plane) * rcp(far_plane - near_plane);				// Corrected using a perspective projection in order to have perspective-corrected interpolation.

	// The octahedron space maps to a square space rotated by 45 degress on the Z axis.
	// If we compensate for this rotation and scale up the image we can double the actual shadowmap resolution for free.

	float cos_theta = cos(radians(45));													// cos(45deg) = sin(45deg)
	
	I.xy = Rotate(I.xy, radians(45)) * sqrt(2.f);

	if (flip) {

		I.x = I.x * -0.5f + 0.5f;

	}
	else {

		I.x = I.x * 0.5f - 0.5f;

	}

	return float4(I.x * depth,		// The depth value will be neutralized dividing the vector by W
				  I.y * depth,		// the term W is explicitly used inside the perspective-corrected interpolation of the attributes (such as UVs)
				  I.z,
				  depth);

}

/// \brief Unproject a point from octahedron space to world space
float3 UnprojectFromOctahedronSpace(float4 position_ps, float near_plane, float far_plane, bool flip) {

	if (flip) {

		position_ps.x = -2.f * (position_ps.x - 0.5f);

	}
	else {

		position_ps.x = 2.f * (position_ps.x + 0.5f);

	}
	
	position_ps.xy = Rotate(position_ps.xy / sqrt(2.f), radians(-45));
	
	float depth = -(far_plane * near_plane) * rcp(position_ps.z * (far_plane - near_plane) - far_plane);	// Neutralize perspective projection

	position_ps.z = 1 - abs(position_ps.x) - abs(position_ps.y);											// The position was normalized using Manhattan distance => |X| + |Y| + |Z| = 1
	
	if (flip) {

		position_ps.z *= -1;

	}
		
	return position_ps.xyz * depth;
	
}

/// \brief Project a point into paraboloid space.
/// \param position Coordinates of the point to project.
/// \param near_clipping Near clipping plane.
/// \param far_plane Far clipping plane.
float4 ProjectToParaboloidSpace(float3 position, float near_plane, float far_plane) {

	// The paraboloid is of the form 
	// f(x,y) = 0.5 - 0.5(x^2 + y^2)
	
	// T = d f(x,y) / dx = (1, 0, -x)					// tangent
	// B = d f(x,y) / dy = (0, 1, -y)					// bitangent
	// N = T x B = (x, y, 1)							// normal (*)

	// (*) tells us that knowing N, we know the projected position Q of the point on the paraboloid surface

	// Incident rays I = P/|P| are reflected along the direction R = (0, 0, 1) due to the paraboloid properties

	// R = I - 2*N*dot(N,I) =>							// Reflection R from incident I and normal N
	// R + I || N =>
	// (Ix + 0, Iy + 0, Iz + 1) || (Qx, Qy, 1) =>		// Normalize on the left
	
	// | Qx = Ix / Iz + 1
	// | Qy = Iy / Iz + 1
	// | Qz = 1

	float depth = length(position);					// Depth of the point wrt the center of the paraboloid projection (0;0;0)

	float3 I = position / depth;					

	return float4(I.x / (abs(I.z) + 1),
				  I.y / (abs(I.z) + 1),
				  saturate((depth - near_plane) / (far_plane - near_plane)),		// Fragments closer than the near plane are fully lit, while fragments beyond the far plane are fully shadowed.
				  1.0f);

}

#endif