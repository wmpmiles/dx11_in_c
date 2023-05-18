cbuffer ModelViewProjectionConstantBuffer : register(b0) {
    matrix tWorld;
    matrix tView;
    matrix tProjection;
};

float4 main(float3 pos3 : POSITION) : SV_POSITION {
    float4 pos4 = float4(pos3, 1.0f);

    pos4 = mul(pos4, tWorld);
    pos4 = mul(pos4, tView);
    pos4 = mul(pos4, tProjection);

    return pos4;
}
