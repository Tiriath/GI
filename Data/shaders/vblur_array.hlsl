// Vertical blur with a kernel of 11 elements
// Based on "Blur.fx - Frank Luna (C) 2011 All Rights Reserved. (from 3D Game Programming with DirectX11)"

#define N 256
#define SAMPLE_SIZE (N + 2 * kBlurRadius)

cbuffer BlurParameters {

	static const int kBlurRadius = 5;			// Radius of the blur

};

StructuredBuffer<float> gBlurKernel;			// Blurring kernel

Texture2DArray gSource;							// Source image array
RWTexture2DArray<float4> gBlurred;				// Destination image array
 
groupshared float4 samples[SAMPLE_SIZE];		// Stores texture samples

[numthreads(1,N,1)]
void CSMain(int3 dispatch_thread_id : SV_DispatchThreadID, int3 group_thread_id : SV_GroupThreadID){

	// Store the samples inside the "cache". Out-of-bounds threads will be clamped to the nearest pixel.

	if (group_thread_id.y < kBlurRadius) {

		samples[group_thread_id.y] = gSource[int3(dispatch_thread_id.x,
												  max(dispatch_thread_id.y - kBlurRadius, 0),
												  dispatch_thread_id.z)];

	}
	else if (group_thread_id.y >= N - kBlurRadius - 1) {

		samples[group_thread_id.y + 2 * kBlurRadius] = gSource[int3(dispatch_thread_id.x,
																	min(dispatch_thread_id.y + kBlurRadius, gSource.Length.y - 1),
																	dispatch_thread_id.z)];

	}

	samples[group_thread_id.y + kBlurRadius] = gSource[int3(min(dispatch_thread_id.xy, gSource.Length.xy - 1),
															dispatch_thread_id.z)];

	// Sync

	GroupMemoryBarrierWithGroupSync();
		
	// Blur

	float4 color = float4(0, 0, 0, 0);

	[unroll]
	for (int sample_index = -kBlurRadius; sample_index <= kBlurRadius; ++sample_index) {

		color += gBlurKernel[sample_index + kBlurRadius] * samples[group_thread_id.y + sample_index + kBlurRadius];

	}

	// Write the output
	gBlurred[dispatch_thread_id.xyz] = color;

}
