#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cmath>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/config.h>
#include "SOIL2/SOIL2.h"
#include "Shader.h"

class AnimatedModel
{
public:
	AnimatedModel(const std::string& path, const std::string& texturaForzada = "")
	{
		texturaExtra = texturaForzada;
		cargarModelo(path);
	}

	void Reproducir(int indice = 0, bool loop = true)
	{
		if (indice < 0 || indice >= (int)animaciones.size()) return;
		actual = indice;
		tiempo = 0.0f;
		repetir = loop;
		terminada = false;
	}

	void Actualizar(float dt)
	{
		if (actual < 0 || actual >= (int)animaciones.size()) return;
		const Animacion& a = animaciones[actual];
		if (!terminada) {
			tiempo += a.ticksPorSegundo * dt;
			if (tiempo >= a.duracion) {
				if (repetir) tiempo = fmod(tiempo, a.duracion);
				else { tiempo = a.duracion; terminada = true; }
			}
		}
		calcularNodos(raiz, glm::mat4(1.0f), a);
	}

	void Draw(Shader& shader, const glm::mat4& matrizBase)
	{
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		for (const Nodo& n : nodosPlanos) {
			if (n.indicesMallas.empty()) continue;
			glm::mat4 mFinal = matrizBase * n.transformGlobal;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mFinal));
			for (unsigned idx : n.indicesMallas) {
				const Malla& m = mallas[idx];
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m.textura);
				glBindVertexArray(m.VAO);
				glDrawElements(GL_TRIANGLES, m.numIndices, GL_UNSIGNED_INT, 0);
			}
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

private:
	struct Vertice { glm::vec3 pos, normal; glm::vec2 uv; };
	struct Malla { GLuint VAO = 0, VBO = 0, EBO = 0; GLsizei numIndices = 0; GLuint textura = 0; };
	struct Nodo {
		std::string nombre;
		glm::mat4 transformLocal{ 1.0f };
		glm::mat4 transformGlobal{ 1.0f };
		std::vector<unsigned> indicesMallas;
		std::vector<int> hijos;
	};
	struct KeyVec { float t; glm::vec3 v; };
	struct KeyQuat { float t; glm::quat q; };
	struct Canal { std::vector<KeyVec> pos, esc; std::vector<KeyQuat> rot; };
	struct Animacion { std::string nombre; float duracion = 0, ticksPorSegundo = 25; std::map<std::string, Canal> canales; };

	std::vector<Malla> mallas;
	std::vector<Nodo> nodosPlanos;
	int raiz = 0;
	std::vector<Animacion> animaciones;
	int actual = -1;
	float tiempo = 0.0f;
	bool repetir = true, terminada = false;
	std::string directorio, texturaExtra;

	static glm::mat4 aGlm(const aiMatrix4x4& m)
	{
		glm::mat4 r;
		r[0][0] = m.a1; r[1][0] = m.a2; r[2][0] = m.a3; r[3][0] = m.a4;
		r[0][1] = m.b1; r[1][1] = m.b2; r[2][1] = m.b3; r[3][1] = m.b4;
		r[0][2] = m.c1; r[1][2] = m.c2; r[2][2] = m.c3; r[3][2] = m.c4;
		r[0][3] = m.d1; r[1][3] = m.d2; r[2][3] = m.d3; r[3][3] = m.d4;
		return r;
	}

	// Crea una textura de 1x1 pixel con el color del material para que el shader no lo pinte negro
	static GLuint crearTexturaColor(unsigned char r, unsigned char g, unsigned char b)
	{
		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		unsigned char pixel[4] = { r, g, b, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return id;
	}

	void cargarModelo(const std::string& path)
	{
		// SIN PRESERVE_PIVOTS: fusiona traslacion, rotacion y escala en un solo nodo limpio por pieza
		const aiScene* scene = aiImportFile(path.c_str(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

		if (!scene || !scene->mRootNode) {
			std::cout << "ERROR::ANIMATED_MODEL " << path << ": " << aiGetErrorString() << std::endl;
			return;
		}
		directorio = path.substr(0, path.find_last_of("/\\"));

		for (unsigned i = 0; i < scene->mNumMeshes; i++)
			mallas.push_back(procesarMalla(scene->mMeshes[i], scene));

		raiz = construirNodos(scene->mRootNode);

		for (unsigned i = 0; i < scene->mNumAnimations; i++) {
			const aiAnimation* src = scene->mAnimations[i];
			Animacion a;
			a.nombre = src->mName.C_Str();
			a.duracion = (float)src->mDuration;
			a.ticksPorSegundo = src->mTicksPerSecond > 0 ? (float)src->mTicksPerSecond : 25.0f;
			for (unsigned c = 0; c < src->mNumChannels; c++) {
				const aiNodeAnim* ch = src->mChannels[c];
				Canal& canal = a.canales[ch->mNodeName.C_Str()];
				for (unsigned k = 0; k < ch->mNumPositionKeys; k++)
					canal.pos.push_back({ (float)ch->mPositionKeys[k].mTime, glm::vec3(ch->mPositionKeys[k].mValue.x, ch->mPositionKeys[k].mValue.y, ch->mPositionKeys[k].mValue.z) });
				for (unsigned k = 0; k < ch->mNumRotationKeys; k++)
					canal.rot.push_back({ (float)ch->mRotationKeys[k].mTime, glm::quat(ch->mRotationKeys[k].mValue.w, ch->mRotationKeys[k].mValue.x, ch->mRotationKeys[k].mValue.y, ch->mRotationKeys[k].mValue.z) });
				for (unsigned k = 0; k < ch->mNumScalingKeys; k++)
					canal.esc.push_back({ (float)ch->mScalingKeys[k].mTime, glm::vec3(ch->mScalingKeys[k].mValue.x, ch->mScalingKeys[k].mValue.y, ch->mScalingKeys[k].mValue.z) });
			}
			animaciones.push_back(a);
		}
		aiReleaseImport(scene);
		if (!animaciones.empty()) { Reproducir(0); Actualizar(0.0f); }
	}

	int construirNodos(const aiNode* src)
	{
		int id = (int)nodosPlanos.size();
		nodosPlanos.push_back(Nodo());
		nodosPlanos[id].nombre = src->mName.C_Str();
		nodosPlanos[id].transformLocal = aGlm(src->mTransformation);
		for (unsigned i = 0; i < src->mNumMeshes; i++)
			nodosPlanos[id].indicesMallas.push_back(src->mMeshes[i]);
		for (unsigned i = 0; i < src->mNumChildren; i++) {
			int hijoId = construirNodos(src->mChildren[i]);
			nodosPlanos[id].hijos.push_back(hijoId);
		}
		return id;
	}

	Malla procesarMalla(const aiMesh* mesh, const aiScene* scene)
	{
		std::vector<Vertice> vertices(mesh->mNumVertices);
		for (unsigned i = 0; i < mesh->mNumVertices; i++) {
			vertices[i].pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			vertices[i].normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0, 1, 0);
			vertices[i].uv = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
		}
		std::vector<GLuint> indices;
		for (unsigned i = 0; i < mesh->mNumFaces; i++)
			for (unsigned j = 0; j < mesh->mFaces[i].mNumIndices; j++)
				indices.push_back(mesh->mFaces[i].mIndices[j]);

		Malla m;
		m.numIndices = (GLsizei)indices.size();

		// Cargar textura forzada, textura del FBX, o el color difuso del material
		const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
		aiString ruta;
		if (!texturaExtra.empty()) {
			size_t corte = texturaExtra.find_last_of("/\\");
			m.textura = TextureFromFile(texturaExtra.substr(corte + 1).c_str(), texturaExtra.substr(0, corte));
		}
		else if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0 && mat->GetTexture(aiTextureType_DIFFUSE, 0, &ruta) == AI_SUCCESS) {
			std::string r = ruta.C_Str();
			size_t corte = r.find_last_of("/\\");
			m.textura = TextureFromFile(r.substr(corte == std::string::npos ? 0 : corte + 1).c_str(), directorio);
		}
		else {
			aiString nombreMat;
			mat->Get(AI_MATKEY_NAME, nombreMat);
			std::string matStr = nombreMat.C_Str();

			if (matStr.find("Wooden") != std::string::npos) {
				// Pon aqui el nombre real de la textura de madera que esta en tu carpeta Models/Stands/
				m.textura = TextureFromFile("stands_wood.png", "Models/Stands");
			}
			else if (matStr.find("Metal") != std::string::npos) {
				// Pon aqui el nombre real de la textura de metal que esta en tu carpeta Models/Stands/
				m.textura = TextureFromFile("stands_metal.png", "Models/Stands");
			}
			else {
				aiColor4D color(0.75f, 0.75f, 0.75f, 1.0f);
				aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color);
				m.textura = crearTexturaColor((unsigned char)(color.r * 255), (unsigned char)(color.g * 255), (unsigned char)(color.b * 255));
			}
		}

		glGenVertexArrays(1, &m.VAO);
		glGenBuffers(1, &m.VBO);
		glGenBuffers(1, &m.EBO);
		glBindVertexArray(m.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertice), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertice), (void*)offsetof(Vertice, pos));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertice), (void*)offsetof(Vertice, normal));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertice), (void*)offsetof(Vertice, uv));
		glBindVertexArray(0);
		return m;
	}

	template <typename K>
	static size_t buscarKey(const std::vector<K>& keys, float t, float& factor)
	{
		size_t i = 0;
		while (i + 2 < keys.size() && t >= keys[i + 1].t) i++;
		float dt = keys[i + 1].t - keys[i].t;
		factor = dt > 0.0f ? glm::clamp((t - keys[i].t) / dt, 0.0f, 1.0f) : 0.0f;
		return i;
	}

	static glm::mat4 interpolar(const Canal& c, float t, const glm::mat4& baseLocal)
	{
		// 1. Extraer posicion, rotacion y escala originales del nodo (para no mandarlas a 0)
		aiMatrix4x4 aiBase;
		aiBase.a1 = baseLocal[0][0]; aiBase.a2 = baseLocal[1][0]; aiBase.a3 = baseLocal[2][0]; aiBase.a4 = baseLocal[3][0];
		aiBase.b1 = baseLocal[0][1]; aiBase.b2 = baseLocal[1][1]; aiBase.b3 = baseLocal[2][1]; aiBase.b4 = baseLocal[3][1];
		aiBase.c1 = baseLocal[0][2]; aiBase.c2 = baseLocal[1][2]; aiBase.c3 = baseLocal[2][2]; aiBase.c4 = baseLocal[3][2];
		aiBase.d1 = baseLocal[0][3]; aiBase.d2 = baseLocal[1][3]; aiBase.d3 = baseLocal[2][3]; aiBase.d4 = baseLocal[3][3];

		aiVector3D aiEsc, aiPos;
		aiQuaternion aiRot;
		aiBase.Decompose(aiEsc, aiRot, aiPos);

		glm::vec3 p(aiPos.x, aiPos.y, aiPos.z);
		glm::vec3 s(aiEsc.x, aiEsc.y, aiEsc.z);
		glm::quat q(aiRot.w, aiRot.x, aiRot.y, aiRot.z);

		// 2. Sobrescribir SOLO lo que realmente tenga keyframes en la animacion
		float f;
		if (c.pos.size() == 1 || (c.pos.size() > 1 && t <= c.pos[0].t)) p = c.pos[0].v;
		else if (c.pos.size() > 1) { size_t i = buscarKey(c.pos, t, f); p = glm::mix(c.pos[i].v, c.pos[i + 1].v, f); }

		if (c.rot.size() == 1 || (c.rot.size() > 1 && t <= c.rot[0].t)) q = c.rot[0].q;
		else if (c.rot.size() > 1) { size_t i = buscarKey(c.rot, t, f); q = glm::slerp(c.rot[i].q, c.rot[i + 1].q, f); }

		if (c.esc.size() == 1 || (c.esc.size() > 1 && t <= c.esc[0].t)) s = c.esc[0].v;
		else if (c.esc.size() > 1) { size_t i = buscarKey(c.esc, t, f); s = glm::mix(c.esc[i].v, c.esc[i + 1].v, f); }

		return glm::translate(glm::mat4(1.0f), p) * glm::mat4_cast(glm::normalize(q)) * glm::scale(glm::mat4(1.0f), s);
	}

	void calcularNodos(int id, const glm::mat4& padre, const Animacion& a)
	{
		Nodo& n = nodosPlanos[id];
		glm::mat4 local = n.transformLocal;
		auto it = a.canales.find(n.nombre);
		if (it != a.canales.end()) local = interpolar(it->second, tiempo, n.transformLocal);
		n.transformGlobal = padre * local;
		for (int hijo : n.hijos) calcularNodos(hijo, n.transformGlobal, a);
	}
};