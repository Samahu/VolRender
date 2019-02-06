#pragma once

struct Vector3I
{
	int x, y, z;
	
	Vector3I() {}
	Vector3I(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}

	bool operator==(const Vector3I &v) { return x == v.x && y == v.y && z == v.z; }
};