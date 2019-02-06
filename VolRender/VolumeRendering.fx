
float4x4 WorldViewProj;

int3 Resolution = int3(32, 32, 32);
float4 InvResolution = float4(1.0f / 32, 1.0f / 32, 1.0f / 32, 0.0f);
int Iterations = 256;

float3 BoxMin;
float3 BoxMax;


texture2D FrontT;
texture2D BackT;
texture3D VolumeT;
texture1D TransferFunctionT;

sampler2D FrontS = sampler_state
{
	Texture = <FrontT>;
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
	BorderColor = float4(0,0,0,0);	// outside of border should be black
};

sampler2D BackS = sampler_state
{
	Texture = <BackT>;
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
	BorderColor = float4(0,0,0,0);	// outside of border should be black
};

sampler3D VolumeS = sampler_state
{
	Texture = <VolumeT>;
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;				// border sampling in U
	AddressV = Border;				// border sampling in V
	AddressW = Border;
	BorderColor = float4(0,0,0,0);	// outside of border should be black
};

sampler1D TransferFunctionS = sampler_state
{
	Texture = <TransferFunctionT>;
	Filter = MIN_MAG_MIP_LINEAR;	
	AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VertexShaderInput
{
    float4 Position : POSITION0;
    float2 texC		: TEXCOORD0;
};

struct VertexShaderOutput
{
    float4 Position		: POSITION0;
    float3 texC			: TEXCOORD0;
    float4 pos			: TEXCOORD1;
};

VertexShaderOutput PositionVS(VertexShaderInput input)
{
    VertexShaderOutput output;
	
    output.Position = mul(input.Position, WorldViewProj);
    
    output.texC = (float3)input.Position;
    output.pos = output.Position;

    return output;
}

float4 PositionPS(VertexShaderOutput input) : COLOR0
{
    return float4(input.texC, 1.0f);
}

//draws the front or back positions, or the ray direction through the volume
float4 DirectionPS(VertexShaderOutput input) : COLOR0
{
	float2 texC = input.pos.xy /= input.pos.w;
	texC.x = +0.5f * texC.x + 0.5f; 
	texC.y = -0.5f * texC.y + 0.5f;
	
    float3 front = tex2D(FrontS, texC).rgb;
    float3 back = tex2D(BackS, texC).rgb;
	
	return float4(back - front, .9f);
	/*
	return float4(front, .9f);
	return float4(back, .9f);
    return float4(back - front, .9f);
    */
}

int rand() 
{
	static int random_seed = 756765;
	random_seed = random_seed * 1103515245 + 12345; 
	return (int)(random_seed / 65536) % 32768;
}

float frand()
{
	return rand() / 32768.0f;
}

bool pointWithinSphere(float4 c, float r, float4 p)
{
	float4 d = c - p;

	return d.x * d.x + d.y * d.y + d.z * d.z < r * r;
}

bool pointWithinEllipsoid(float4 c, float4 r, float4 p)
{
	float4 d = c - p;
	
	d.x /= r.x;
	d.y /= r.y;
	d.z /= r.z;
	
	return d.x * d.x + d.y * d.y + d.z * d.z < 1;
}

bool pointWithinCuboid(float4 c, float4 r, float4 p)
{
	float4 d = abs(c - p);
	
	return d.x < r.x && d.y < r.y && d.z < r.z;
}

float FloorCeil(int f, float v)
{
	return 0 == f ? floor(v) : ceil(v);
}

float tex3DSpecial(float4 pos)
{
	float4 g = float4(pos.x * Resolution.x, pos.y * Resolution.y, pos.z * Resolution.z, 0);
	
	int i, j, k;
	
	for (i = 0; i < 2; ++i)
		for (j = 0; j < 2; ++j)
			for (k = 0; k < 2; ++k)
			{
				float4 voxel = float4(FloorCeil(i, g.x) / Resolution.x, FloorCeil(j, g.y) / Resolution.y, FloorCeil(k, g.z) / Resolution.z, 0);
				
				if (pointWithinCuboid(voxel, 0.5f * InvResolution, pos))
					return tex3Dlod(VolumeS, voxel).r;
			}
			
	return 0.0f;
}

struct Ray {
	float3 o;	// origin
	float3 d;	// direction
};

struct IntersectBoxResult
{
	float tnear;
	float tfar;
	int hit;
};

IntersectBoxResult intersectBox(Ray r, float3 boxmin, float3 boxmax)
{
    // compute intersection of ray with all six bbox planes
    float3 invR = float3(1.0f, 1.0f, 1.0f) / r.d;
    float3 tbot = invR * (boxmin - r.o);
    float3 ttop = invR * (boxmax - r.o);

    // re-order intersections to find smallest and largest on each axis
    float3 tmin = min(ttop, tbot);
    float3 tmax = max(ttop, tbot);

    // find the largest tmin and the smallest tmax
    float largest_tmin = max(max(tmin.x, tmin.y), max(tmin.x, tmin.z));
    float smallest_tmax = min(min(tmax.x, tmax.y), min(tmax.x, tmax.z));
    
    IntersectBoxResult ox;

	ox.tnear = largest_tmin;
	ox.tfar = smallest_tmax;
	ox.hit = smallest_tmax > largest_tmin;
	
	return ox;
}

struct StepStruct
{
	float3 pos;
	float3 step;
	float4 dst;
	bool cont;
};

void StepFunction(StepStruct ss)
{
	ss.cont = false;
	ss.dst = float4(1, 0.0, 0.0, 1.0f);
	return;
	
	float4 pos2 = float4(ss.pos.x, 1 - ss.pos.y, ss.pos.z, 0);
	float value = tex3Dlod(VolumeS, pos2).r;	// tex3DSpecial(pos2);	
	float4 src = float4(0, 1, 0, 0.1f);	// tex1Dlod(TransferFunctionS, value);

	// fade = min + (max - min) * pos2.y;	// pos.y [0..1]
	// src.a *= fade;	// fade [min..max]

	// Front to back blending
	ss.dst.rgb += src.rgb * src.a * (1 - ss.dst.a);
	ss.dst.a   += src.a * (1 - ss.dst.a);
	
		
	//break from the loop when alpha gets high enough
	if(ss.dst.a >= .95f)
	{
		ss.cont = false;
		return;
	}
	
	//advance the current position
	ss.pos += ss.step;
	
	//break if the position is greater than <1, 1, 1>
	if(ss.pos.x > 1.0f || ss.pos.y > 1.0f || ss.pos.z > 1.0f)
		ss.cont = false;
}

float4 RayCastSimplePS(VertexShaderOutput input) : COLOR0
{ 
	//calculate projective texture coordinates
	//used to project the front and back position textures onto the cube
	float2 texC = input.pos.xy /= input.pos.w;
	texC.x = +0.5f * texC.x + 0.5f;
	texC.y = -0.5f * texC.y + 0.5f;
	
    float3 front = tex2D(FrontS, texC).xyz;
    float3 back = tex2D(BackS, texC).xyz;
    
    float3 dir = normalize(back - front);
    
    float3 pos = input.texC + float3(0.5f, 0.5f, 0.5f);

	/*
	
	Ray ray;
	ray.o = front;
	ray.d = dir;
	
	IntersectBoxResult ibr = intersectBox(ray, BoxMin, BoxMax);
	
	if (ibr.tnear < 0.0f)
		ibr.tnear = 0.0f;
	
	pos.xyz = ray.o + ray.d * ibr.tnear;
	
    */

	/*
	float min = 1.0f;
	float max = 1.0f;	// shouldn't be greater than one.
	float fade;
	*/
	
	StepStruct ss;
	ss.pos = pos;
	ss.step = dir * (1.0f / Iterations);
	ss.dst = float4(0, 0, 0, 0);
	ss.cont = true;
	
	// Workaround for Iterations greater than 256
	int n1 = Iterations / 254;
	int n2 = Iterations % 254;
	
	int i, j;
	
	for (i = 0; i < n1 && ss.cont; ++i)
		for (j = 0; j < 254 && ss.cont; ++j)
			StepFunction(ss);
	
	for (i = 0; i < n2 && ss.cont; ++i)
	{
		float4 pos2 = float4(ss.pos.x, 1 - ss.pos.y, ss.pos.z, 0);
		float value = tex3Dlod(VolumeS, pos2).r;	// tex3DSpecial(pos2);
		float4 src = tex1Dlod(TransferFunctionS, value);	// float4(0, value, 0, 0.1f * value);

		// fade = min + (max - min) * pos2.y;	// pos.y [0..1]
		// src.a *= fade;	// fade [min..max]

		// Front to back blending
		ss.dst.rgb += src.rgb * src.a * (1 - ss.dst.a);
		ss.dst.a   += src.a * (1 - ss.dst.a);
		
			
		//break from the loop when alpha gets high enough
		if(ss.dst.a >= .95f)
		{
			ss.cont = false;
			break;
		}
		
		//advance the current position
		ss.pos += ss.step;
		
		//break if the position is greater than <1, 1, 1>
		if(ss.pos.x > 1.0f || ss.pos.y > 1.0f || ss.pos.z > 1.0f)
			ss.cont = false;
	}
	//	StepFunction(ss);
	
	return ss.dst;
}

technique RenderPosition
{
    pass Pass1
    {		
        VertexShader = compile vs_2_0 PositionVS();
        PixelShader = compile ps_2_0 PositionPS();
    }
}

technique RayCast
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 PositionVS();
        PixelShader = compile ps_3_0 RayCastSimplePS();
        
        /*
        AlphaBlendEnable = true;
    	SrcBlend = SrcAlpha;
    	DestBlend = InvSrcColor;
    	*/
    }
}