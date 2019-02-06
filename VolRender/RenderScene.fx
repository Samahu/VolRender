//--------------------------------------------------------------------------------------
// File: RenderScene.fx
//
// The effect file for the Voxelization sample.  
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
float4 g_MaterialAmbientColor;      // Material's ambient color
float4 g_MaterialDiffuseColor;      // Material's diffuse color
float3 g_LightDir;                  // Light's direction in world space
float4 g_LightDiffuse;              // Light's diffuse color
texture g_MeshTexture;              // Color texture for mesh
float4x4 g_mWorld;                  // World matrix for object
float4x4 g_mWorldViewProjection;    // World * View * Projection matrix

//--------------------------------------------------------------------------------------
// Texture samplers
//--------------------------------------------------------------------------------------
sampler MeshTextureSampler = 
sampler_state
{
    Texture = <g_MeshTexture>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

//--------------------------------------------------------------------------------------
// Vertex shader output structure
//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 Diffuse    : COLOR0;     // vertex diffuse color (note that COLOR0 is clamped from 0..1)
    float2 TextureUV  : TEXCOORD0;  // vertex texture coords
};

//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT RenderSceneVS(float4 vPos : POSITION, float3 vNormal : NORMAL, float2 vTexCoord0 : TEXCOORD0)
{
    VS_OUTPUT ot;
    float3 vNormalWorldSpace;
    ot.Position = mul(vPos, g_mWorldViewProjection);	// Transform the position from object space to homogeneous projection space
    vNormalWorldSpace = normalize(mul(vNormal, (float3x3)g_mWorld)); // normal (world space)    
    ot.Diffuse.rgb = g_MaterialDiffuseColor * g_LightDiffuse * max(0, dot(vNormalWorldSpace, g_LightDir)) + g_MaterialAmbientColor;
    ot.Diffuse.a = 1.0f;
    ot.TextureUV = vTexCoord0;
    return ot;
}

//--------------------------------------------------------------------------------------
// Pixel shader output structure
//--------------------------------------------------------------------------------------
struct PS_OUTPUT
{
    float4 RGBColor : COLOR0;  // Pixel color
};

//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by modulating the texture's
// color with diffuse material color
//--------------------------------------------------------------------------------------
PS_OUTPUT RenderScenePS(VS_OUTPUT n)
{ 
    PS_OUTPUT ot;
    // ot.RGBColor = tex2D(MeshTextureSampler, n.TextureUV) * n.Diffuse;	// Lookup mesh texture and modulate it with diffuse
    ot.RGBColor = n.Diffuse;	// Lookup mesh texture and modulate it with diffuse
    return ot;
}

//--------------------------------------------------------------------------------------
// Renders scene 
//--------------------------------------------------------------------------------------
technique RenderScene
{
    pass P0
    {
		VertexShader = compile vs_2_0 RenderSceneVS();
        PixelShader  = compile ps_2_0 RenderScenePS();
    }
}