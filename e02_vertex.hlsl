cbuffer ModelViewProjectionConstantBuffer : register(b0) {
    matrix tWorld;
    matrix tView;
    matrix tProjection;
};

struct VSInput {
    float3 pos : POSITION;
    float3 color : COLOR;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VSOutput main(VSInput input) {
    float4 pos4 = float4(input.pos, 1.0f);

    pos4 = mul(pos4, tWorld);
    pos4 = mul(pos4, tView);
    pos4 = mul(pos4, tProjection);

    VSOutput output = { pos4, float4(input.color, 1.0f) };

    return output;
}
