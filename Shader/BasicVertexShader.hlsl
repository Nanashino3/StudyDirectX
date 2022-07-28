#include "BasicShaderHeader.hlsli"

S_OUTPUT BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD) {
	S_OUTPUT output;
	output.svpos = pos;
	output.uv = uv;
	return output;
}
