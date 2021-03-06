
#include "voxel_def.hlsl"

/////////////////////////////////// VERTEX SHADER ///////////////////////////////////////

cbuffer PerObject {

	float4x4 gWorld;							// World matrix of the object to voxelize, in grid space.

};

struct VSIn {

	float3 position : SV_Position;

	uint instance_id : SV_InstanceID;

};

struct VSOut {

	float4 position : SV_Position;

	int cascade : Cascade;

};

VSOut VSMain(VSIn input){
	
	VSOut output;

	float4 pos = mul(gWorld, float4(input.position,1));								// World space
	
	output.cascade = -(int)(input.instance_id);										// Cascades have negative MIP level (positive ones are for the downscaled pyramid)

	float grid_size = gVoxelSize * gVoxelResolution;

	float3 cascade_center = GetMIPCenter(GetVoxelSize(output.cascade));

	output.position = (pos - float4(cascade_center, 0.0f)) * (2.f / grid_size);		// Cascade space [-1;+1]
	
	return output;

}

/////////////////////////////////// GEOMETRY SHADER ////////////////////////////////////

struct GSOut {

	float4 position_ps: SV_Position;					// Vertex position in voxel-grid projection space.

	unsigned int projection_plane : ProjectionPlane;	// Projection plane. 0: (Right) ZY(-X), 1: (Above) XZ(-Y), 2: (Front) XY(Z)

	int cascade : Cascade;								// Voxel cascade index.

};

[maxvertexcount(3)]
void GSMain(triangle VSOut input[3], inout TriangleStream<GSOut> output_stream) {
	
	// TODO: Optimize the branchiness here

	GSOut output;

	// Normal of the triangle N = (A-B) x (C-B), P = (A-B), Q = (C-B)
	//
	// N = <PyQz - PzQy,
	//	    PxQz - PzQx,
	//      PxQy - PyQx>
	//
	//
	// Which d in {<1;0;0>, <0;1;0>, <0;0;1>} maximizes |d dot N| ? i.e. i | N[i] >= N[j] forall J in [0;2]

	float3 abs_normal = abs(cross(input[1].position.xyz - input[0].position.xyz,
								  input[2].position.xyz - input[0].position.xyz));

	// Determine which axis maximizes the rasterized area and shuffle the polygon coordinates accordingly

	if (abs_normal.x > abs_normal.y && abs_normal.x > abs_normal.z) {

		// From the right: -X is the depth
		output.projection_plane = 0;

		input[0].position = input[0].position.zyxw;
		input[1].position = input[1].position.zyxw;
		input[2].position = input[2].position.zyxw;

	}
	else if (abs_normal.y > abs_normal.z) {

		// From the above: -Y is the depth
		output.projection_plane = 1;

		input[0].position = input[0].position.xzyw;
		input[1].position = input[1].position.xzyw;
		input[2].position = input[2].position.xzyw;

	}
	else {
		
		// From the front: +Z is the depth
		output.projection_plane = 2;

		//input[0] = input[0].xyzw;
		//input[1] = input[1].xyzw;
		//input[2] = input[2].xyzw;

	}

	// Output one primitive per cascade.
	// In each cascade the primitive is shrunk by a factor of 2 with respect to the last iteration

	[unroll]
	for (unsigned int vertex_index = 0; vertex_index < 3; ++vertex_index) {

		output.position_ps = float4(input[vertex_index].position.x,						// [-1;+1]
									input[vertex_index].position.y * -1.0f,				// [+1;-1]. Compensate for the V axis which grows downwards.
									input[vertex_index].position.z,
									1.0f);

		output.cascade = input[vertex_index].cascade;

		output.position_ps.xyz *= (1 << -output.cascade);								// Enlarge the primitive according to the cascade it should be written to.

		output.position_ps.z = output.position_ps.z * 0.5f + 0.5f,						// [ 0;+1]. Will kill geometry outside the Z boundaries
			
		output_stream.Append(output);

	}

}

/////////////////////////////////// PIXEL SHADER ///////////////////////////////////////

void PSMain(GSOut input){

	uint linear_coordinate;		// Linear coordinate of the voxel within its cascade.

	// Restore the unshuffled coordinates from the GS

	input.position_ps.z *= (gVoxelResolution - 1);

	if (input.projection_plane == 0) {
		
		input.position_ps = input.position_ps.zyxw;

	}
	else if (input.projection_plane == 1) {

		input.position_ps = input.position_ps.xzyw;

	}
	else {
				
		input.position_ps = input.position_ps.xyzw;
		
	}
	
	// Store a new opaque black voxel

	Voxelize((uint3) input.position_ps, input.cascade);
	
}
