#include "render_def.hlsl"
#include "voxel\voxel_def.hlsl"
#include "pbr_def.hlsl"

#define N 16
#define TOTAL_THREADS (N)

StructuredBuffer<uint> gVoxelAddressTable;					// Contains the "pointers" to the actual voxel infos.

Texture3D<float3> gFilteredSHPyramid;						// Pyramid part of the filtered SH 3D clipmap.
Texture3D<float3> gFilteredSHStack;							// Stack part of the filtered SH 3D clipmap.

Texture2D<float4> gLightAccumulation;
RWTexture2D<float4> gIndirectLight;

cbuffer gParameters {

	float4x4 inv_view_proj_matrix;		///< \brief Inverse view-projection matrix.
	float4 camera_position;				///< \brief Camera position in world space.
	uint point_lights;					///< \brief Number of point lights.
	uint directional_lights;			///< \brief Number of directional lights.

};

float3 SampleSpecularCone(SurfaceData surface, float3 light_direction, float3 view_direction) {

	float3 color = SampleCone(gFilteredSHPyramid,
							  gFilteredSHStack,
							  surface.position,
							  light_direction,
							  0.24f);

	float3 specular = ComputeCookTorrance(light_direction, view_direction, surface);	// Cook-Torrance specular (PBR).

	return surface.ks * specular * color;

}

float3 SampleDiffuseCone(SurfaceData surface, float3 light_direction, float3 view_direction) {

	float3 color = SampleCone(gFilteredSHPyramid,
							  gFilteredSHStack,
							  surface.position,
							  light_direction,
							  0.52f);

	float3 diffuse = ComputeLambert(light_direction, view_direction, surface);			// Lambertian diffuse

	return surface.kd * diffuse * color;

}

[numthreads(N, N, 1)]
void CSMain(uint3 dispatch_thread_id : SV_DispatchThreadID) {
	
	SurfaceData surface = GatherSurfaceData(dispatch_thread_id.xy, inv_view_proj_matrix);
	
	float3 V = normalize(camera_position.xyz - surface.position);	// From the camera to the surface

	float3 R = reflect(-V,
					   surface.normal);

	float3 color = 0;

	color += SampleSpecularCone(surface, R, V);	// Specular

	float3 x = cross(surface.normal, normalize(float3(1,1,1)));
	float3 y = cross(surface.normal, x);

	x = cross(surface.normal, y);

	x *= 3.f;
	y *= 3.f;

	color += SampleDiffuseCone(surface, surface.normal, V);						// Diffuse Up
	color += SampleDiffuseCone(surface, normalize(surface.normal + x), V);
	color += SampleDiffuseCone(surface, normalize(surface.normal - x), V);
	color += SampleDiffuseCone(surface, normalize(surface.normal + y), V);
	color += SampleDiffuseCone(surface, normalize(surface.normal - y), V);

	// Sum the indirect contribution inside the light accumulation buffer

    gIndirectLight[dispatch_thread_id.xy] = gLightAccumulation[dispatch_thread_id.xy] + max(0, float4(color.rgb, 1));

}