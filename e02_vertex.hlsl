cbuffer ModelViewProjectionConstantBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
};

struct VS_INPUT {
    float3 pos   : POSITION;
    float3 color : COLOR;
};

struct VS_OUTPUT {
    float4 pos   : SV_POSITION;
    float4 color : COLOR;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;

    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, world);
    pos = mul(pos, view);
    pos = mul(pos, projection);

    output.pos = pos;
    output.color = float4(input.color, 1.0f);

    return output;
}
