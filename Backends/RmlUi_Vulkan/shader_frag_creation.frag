#version 450

// Creation effect parameters (matches the DX12 renderer's creation 'SharedConstantBuffer', 84 bytes used).
layout(set = 0, binding = 0) uniform SharedConstantBuffer
{
	mat4 m_transform;
	vec2 m_translate;
	vec2 m_dimensions;
	float m_value;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 finalColor;

void main() {
	// Bound singularities at the center, cell centers, and zero-height geometry.
	const float epsilon = 0.000001;
	float t = m_value;
	vec3 c;
	float l;
	for (int i = 0; i < 3; i++) {
		vec2 p = fragTexCoord;
		vec2 uv = p;
		p -= .5;
		p.x *= abs(m_dimensions.y) > epsilon ? m_dimensions.x / m_dimensions.y : 0.0;
		float z = t + float(i) * .07;
		l = length(p);
		uv += p / max(l, epsilon) * (sin(z) + 1.) * abs(sin(l * 9. - z - z));
		c[i] = .01 / max(length(mod(uv, 1.) - .5), epsilon);
	}
	finalColor = vec4(c / max(l, epsilon), fragColor.a);
}
