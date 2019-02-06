#pragma once

class AABB
{
	D3DXVECTOR3 _min, _max;

public:
	// Constructors
	AABB();

	AABB(const D3DXVECTOR3 &min, const D3DXVECTOR3 &max);

public:
	bool IsValid() const { return _min.x < _max.x && _min.y < _max.y && _min.z < _max.z; }

	const D3DXVECTOR3 & GetMin() const { return _min; }

	const D3DXVECTOR3 & GetMax() const { return _max; }

	void SetMinMax(const D3DXVECTOR3 &min, const D3DXVECTOR3 &max) { _min = min; _max = max; }

	D3DXVECTOR3 GetCenter() const { return 0.5f * (_min + _max); }

	D3DXVECTOR3 GetExtents() const { return _max - _min; }

	void SetCenterExtents(const D3DXVECTOR3 &center, const D3DXVECTOR3 &extents)
	{
		_min = center - 0.5f * extents;
		_max = center + 0.5f * extents;
	}

	void Scale(float s)
	{
		D3DXVECTOR3 c = GetCenter();
		D3DXVECTOR3 e = GetExtents();
		e *= s;
		SetCenterExtents(c, e);
	}

public:
	void Merge(const AABB &aabb);
};