//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

static float softeningSquared	= 0.0012500000f * 0.0012500000f;
static float g_fG				= 6.67300e-11f * 10000.0f;
static float g_fParticleMass	= g_fG * 10000.0f * 10000.0f;

#define blocksize 256
groupshared float4 sharedPos[blocksize];

//
// Body to body interaction, acceleration of the particle at position 
// bi is updated.
//
void bodyBodyInteraction(inout float3 ai, float4 bj, float4 bi, float mass, int particles) 
{
	float3 r = bj.xyz - bi.xyz;

	float distSqr = dot(r, r);
	distSqr += softeningSquared;

	float invDist = 1.0f / sqrt(distSqr);
	float invDistCube =  invDist * invDist * invDist;
	
	float s = mass * invDistCube * particles;

	ai += r * s;
}

cbuffer cbCS : register(b0)
{
	uint4   g_param;	// param[0] = MAX_PARTICLES;
						// param[1] = dimx;
	float4  g_paramf;	// paramf[0] = 0.1f;
						// paramf[1] = 1; 
};

struct PosVelo
{
	float4 pos;
	float4 velo;
};

StructuredBuffer<PosVelo> oldPosVelo	: register(t0);	// SRV
RWStructuredBuffer<PosVelo> newPosVelo	: register(u0);	// UAV

[numthreads(blocksize, 1, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
	// Each thread of the CS updates one of the particles.
	float4 pos = oldPosVelo[DTid.x].pos;
	float4 vel = oldPosVelo[DTid.x].velo;
	float3 accel = 0;
	float mass = g_fParticleMass;

	// Update current particle using all other particles.
	[loop]
	for (uint tile = 0; tile < g_param.y; tile++)
	{
		// Cache a tile of particles unto shared memory to increase IO efficiency.
		sharedPos[GI] = oldPosVelo[tile * blocksize + GI].pos;
	   
		GroupMemoryBarrierWithGroupSync();

		[unroll]
		for (uint counter = 0; counter < blocksize; counter += 8 ) 
		{
			bodyBodyInteraction(accel, sharedPos[counter], pos, mass, 1);
			bodyBodyInteraction(accel, sharedPos[counter+1], pos, mass, 1);
			bodyBodyInteraction(accel, sharedPos[counter+2], pos, mass, 1);
			bodyBodyInteraction(accel, sharedPos[counter+3], pos, mass, 1);
			bodyBodyInteraction(accel, sharedPos[counter+4], pos, mass, 1);
			bodyBodyInteraction(accel, sharedPos[counter+5], pos, mass, 1);
			bodyBodyInteraction(accel, sharedPos[counter+6], pos, mass, 1);
			bodyBodyInteraction(accel, sharedPos[counter+7], pos, mass, 1);
		}
		
		GroupMemoryBarrierWithGroupSync();
	}  

	// g_param.x is the number of our particles, however this number might not 
	// be an exact multiple of the tile size. In such cases, out of bound reads 
	// occur in the process above, which means there will be tooManyParticles 
	// "phantom" particles generating false gravity at position (0, 0, 0), so 
	// we have to subtract them here. NOTE, out of bound reads always return 0 in CS.
	const int tooManyParticles = g_param.y * blocksize - g_param.x;
	bodyBodyInteraction(accel, float4(0, 0, 0, 0), pos, mass, -tooManyParticles);

	// Update the velocity and position of current particle using the 
	// acceleration computed above.
	vel.xyz += accel.xyz * g_paramf.x;		//deltaTime;
	vel.xyz *= g_paramf.y;					//damping;
	pos.xyz += vel.xyz * g_paramf.x;		//deltaTime;

	if (DTid.x < g_param.x)
	{
		newPosVelo[DTid.x].pos = pos;
		newPosVelo[DTid.x].velo = float4(vel.xyz, length(accel));
	}
}
