#ifndef TETRIS_VEC_H
#define TETRIS_VEC_H

union vec2i
{
    struct
    {
        int x, y;
    };
    int vals[2];

    int &operator[](int index) { return (&x)[index]; }
    int* operator&(){ return vals; }

    vec2i& operator+=(vec2i rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        return *this;
    }

    vec2i& operator-=(vec2i rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        return *this;
    }
};

vec2i vec2i_init(int x, int y)
{
    vec2i result;
    result.x = x;
    result.y = y;
    return result;
}

vec2i vec2i_init(int s)
{
    vec2i result;
    result.x = s;
    result.y = s;
    return result;
}

vec2i operator-(vec2i v)
{
    vec2i result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

vec2i operator+(vec2i a, vec2i b)
{
    vec2i result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

vec2i operator-(vec2i a, vec2i b)
{
    vec2i result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

vec2i operator*(vec2i v, int s)
{
    vec2i result;
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

vec2i operator/(vec2i v, int s)
{
    vec2i result;
    result.x = v.x / s;
    result.y = v.y / s;
    return result;
}

bool operator==(vec2i a, vec2i b)
{
    return a.x == b.x && a.y == b.y;
}

bool operator!=(vec2i a, vec2i b)
{
    return !(a == b);
}



union vec3
{
    struct
    {
        float x, y, z;
    };
    struct
    {
        float r, g, b;
    };
    float vals[3];

    float &operator[](int index) { return (&x)[index]; }
    float* operator&(){ return vals; }
};

vec3 vec3_init(float x, float y, float z)
{
    vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

vec3 vec3_init(float s)
{
    vec3 result;
    result.x = s;
    result.y = s;
    result.z = s;
    return result;
}

vec3 operator-(vec3 v)
{
    vec3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

vec3 operator+(vec3 a, vec3 b)
{
    vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

vec3 operator-(vec3 a, vec3 b)
{
    vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

vec3 operator*(vec3 v, float s)
{
    vec3 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

vec3 operator/(vec3 v, float s)
{
    vec3 result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.y = v.z / s;
    return result;
}

float Dot3x3(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 Cross(vec3 a, vec3 b)
{
    vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}



union vec4
{
    struct
    {
        float x, y, z, w;
    };
    struct 
    {
        float r, g, b, a;
    };
    float vals[4];

    float &operator[](int index) { return (&x)[index]; }
    float* operator&(){ return vals; }
};

vec4 vec4_init(float x, float y, float z, float w)
{
    vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

vec4 vec4_init(float s)
{
    vec4 result;
    result.x = s;
    result.y = s;
    result.z = s;
    result.w = s;
    return result;
}

vec4 vec4_init(vec3 v3, float w)
{
    vec4 result;
    result.x = v3.x;
    result.y = v3.y;
    result.z = v3.z;
    result.w = w;
    return result;
}



vec4 operator-(vec4 v)
{
    vec4 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return result;
}

vec4 operator+(vec4 a, vec4 b)
{
    vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

vec4 operator-(vec4 a, vec4 b)
{
    vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

vec4 operator*(vec4 v, float s)
{
    vec4 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;
    return result;
}

vec4 operator/(vec4 v, float s)
{
    vec4 result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    result.w = v.w / s;
    return result;
}

#endif