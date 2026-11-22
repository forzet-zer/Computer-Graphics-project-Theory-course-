#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
// Solo para modelos con esqueleto (SkinnedModel.h): hasta 4 huesos por vertice y su peso
layout (location = 3) in ivec4 boneIds;
layout (location = 4) in vec4 weights;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
uniform int skinned;                       // 0 = modelo normal, 1 = animacion de huesos
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    vec4 localPos = vec4(position, 1.0f);
    vec3 localNormal = normal;

    if (skinned == 1)
    {
        // Mezcla de las matrices de los huesos que afectan al vertice (linear blend skinning)
        mat4 skin = mat4(0.0f);
        for (int i = 0; i < 4; i++)
        {
            if (boneIds[i] < 0 || boneIds[i] >= MAX_BONES) continue;
            skin += finalBonesMatrices[boneIds[i]] * weights[i];
        }
        if (weights.x + weights.y + weights.z + weights.w < 0.001f) skin = mat4(1.0f); // vertice sin huesos
        localPos = skin * localPos;
        localNormal = mat3(skin) * normal;
    }

    gl_Position = projection * view * model * localPos;
    FragPos = vec3(model * localPos);
    Normal = mat3(transpose(inverse(model))) * localNormal;
    TexCoords = texCoords;
}
