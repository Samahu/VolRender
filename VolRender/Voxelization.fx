//--------------------------------------------------------------------------------------
// File: Voxelization.fx
//
// The effect file for the Voxelization process.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
texture2D SolidT;

sampler2D SolidS = sampler_state
{
	Texture = <SolidT>;
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Border;
	AddressV = Border;
	BorderColor = float4(0,0,0,0);	// outside of border should be black
};

float4 RenderSceneVS( float4 vPos : POSITION ) : POSITION
{
	return mul(vPos, g_mWorldViewProjection);	// Transform the position from object space to homogeneous projection space
}

float4 RenderStencilPS( uniform float4 color ) : COLOR
{
	return color;
}

struct VS_OUTPUT
{
    float4 Position	: POSITION0;
    float3 texC		: TEXCOORD0;
};

float4 EdgeDetectionPS(VS_OUTPUT In)
{
	float4 value = tex2D(SolidS, In.texC);
	
	return value;
}

//--------------------------------------------------------------------------------------
// Voxelize
//--------------------------------------------------------------------------------------
technique Voxelize
{
    pass P0
    {
		VertexShader = compile vs_2_0 RenderSceneVS();
        PixelShader  = null;
        
        ZWriteEnable = false;
        StencilEnable = true;
        
        ShadeMode = Flat;
        
        StencilFunc = Always;
        StencilZFail = Keep;
        StencilFail = Keep;
        StencilPass = Incr;
        
        StencilRef = 1;
        StencilMask = 0xFFFFFFFF;
        StencilWriteMask = 0xFFFFFFFF;
        
        // Disale writing to the frame buffer
        AlphaBlendEnable = true;
        SrcBlend = Zero;
        DestBlend = One;
        
        // Disable writing to depth buffer
        
        ZFunc = Less;
        
        // Setup stencil states
        TwoSidedStencilMode = true;
        
        
        Ccw_StencilFunc = Always;
        Ccw_StencilZFail = Keep;
        Ccw_StencilFail = Keep;
        Ccw_StencilPass = Decr;
        
        CullMode = None;
    }
}

technique DumpStencil
{
	pass P0
	{
		VertexShader = null;
		PixelShader = compile ps_2_0 RenderStencilPS( float4( 1.0f, 1.0f, 1.0f, 1.0f ) );

		ZEnable = false;
		StencilEnable = true;
		
		StencilRef = 1;
		StencilFunc = LessEqual;
		StencilPass = Keep;
		
		AlphaBlendEnable = true;
		SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
	}
}