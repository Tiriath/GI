/// \brief Appends the voxel stored inside a voxel address table inside an append buffer.

#define SH_READ_ONLY

#include "voxel_def.hlsl"

#define N 8
#define TOTAL_THREADS (N)

#define MIP_AUTO 1000

RWBuffer<uint> gVoxelIndirectArguments;
RWBuffer<uint> gSHIndirectArguments;

AppendStructuredBuffer<VoxelInfo> gVoxelAppendBuffer;		// Append buffer containing the list of voxels in the current frame. (Read/Write)

cbuffer PerFrame {

    float4x4 gViewProjection;				            	// View-projection matrix
    int gMIP;                                               // MIP limit

};

[numthreads(N, N, N)]
void CSMain(uint3 dispatch_thread_id : SV_DispatchThreadID) {

    VoxelInfo voxel_info;

    int cascade = (gMIP == MIP_AUTO) ?
                  -(int)(dispatch_thread_id.y / gVoxelResolution) :
                  gMIP;

    uint resolution = gVoxelResolution >> max(cascade, 0);
    uint3 voxel_coordinates = dispatch_thread_id % resolution;

    // Reject if there's a more detailed cascade (AUTO mode only)

    if (gMIP == MIP_AUTO &&
        cascade > -(int)(gCascades) &&
        GetChebyshevDistance((int3)(voxel_coordinates), (int3)(resolution >> 1)) <= (resolution >> 2)) {

        return;

    }

	if (GetVoxelInfo(voxel_coordinates, cascade, voxel_info)) {

		float3 center = abs(voxel_info.center - gCameraCenter);
		            
        // Reject if the center of the voxel is not visible from the current camera

        float4 transformed_center = mul(gViewProjection, float4(voxel_info.center, 1));

        if (abs(transformed_center.x) < transformed_center.w &&
            abs(transformed_center.y) < transformed_center.w &&
            transformed_center.z < transformed_center.w &&
            transformed_center.z > 0){

            uint dummy;

			InterlockedAdd(gVoxelIndirectArguments[1], 1, dummy);
			InterlockedAdd(gSHIndirectArguments[1], 1, dummy);

			gVoxelAppendBuffer.Append(voxel_info);

        }

	}
	
}