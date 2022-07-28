#include "BasicShaderHeader.hlsli"

float4 BasicPS(S_OUTPUT input) : SV_TARGET{
	return float4(input.uv, 1, 1);
}