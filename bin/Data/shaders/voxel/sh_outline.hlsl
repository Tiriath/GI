
#include "voxel_def.hlsl"
#include "..\common\color.hlsl"

StructuredBuffer<VoxelInfo> gVoxelAppendBuffer;				// Append buffer containing the list of voxels in the current frame. (Read Only)

/////////////////////////////////// VERTEX SHADER ///////////////////////////////////////

cbuffer PerFrame {

	float4x4 gViewProjection;					// View-projection matrix

};

struct VSIn {

	float3 position : SV_Position;
	uint instance_id : SV_InstanceID;

};

struct VSOut {

	float4 position_ps : SV_Position;
	float4 color : Color;

};

VSOut VSMain(VSIn input){

	VoxelInfo voxel_info = gVoxelAppendBuffer[input.instance_id];
	
	VSOut output;

	float4 color = SampleVoxelColor(voxel_info.center, 
									normalize(input.position.xyz),
									voxel_info.size * 0.5f);

	output.color = color;

	// The debug draw is applied after the tonemap so we have to manually tonemap the result. (Reinhard's)

	output.color.rgb = output.color.rgb * rcp(1.f + output.color.rgb);

	output.color.rgb = pow(output.color.rgb, 1.f / 2.2f);

	// Deformation of the SH mesh

	float magnitude = (max(output.color.r, max(output.color.g, output.color.b)) + min(output.color.r, min(output.color.g, output.color.b))) * 0.5f;
	
	float3 position = (input.position.xyz * voxel_info.size * magnitude) + voxel_info.center;
	
	output.position_ps = mul(gViewProjection, float4(position, 1));
	
	return output;

}

/////////////////////////////////// PIXEL SHADER ///////////////////////////////////////

float4 PSMain(VSOut input) : SV_Target0{

	return input.color;

}
