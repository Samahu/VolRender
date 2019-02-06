#include "StdAfx.h"
#include "AABB.h"
#include <math.h>

AABB::AABB()
: _min(0.0f, 0.0f, 0.0f), _max(0.0f, 0.0f, 0.0f)
{
}

AABB::AABB(const D3DXVECTOR3 &min_, const D3DXVECTOR3 &max_)
: _min(min_), _max(max_)
{
}

void AABB::Merge(const AABB &aabb)
{
	_min.x = min(_min.x, aabb._min.x);
	_min.y = min(_min.y, aabb._min.y);
	_min.z = min(_min.z, aabb._min.z);

	_max.x = max(_max.x, aabb._max.x);
	_max.y = max(_max.y, aabb._max.y);
	_max.z = max(_max.z, aabb._max.z);
}