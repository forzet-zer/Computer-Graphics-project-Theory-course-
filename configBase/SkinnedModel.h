#pragma once
// ---------------------------------------------------------------------------------------------
// SkinnedModel: carga un modelo con esqueleto (FBX de Mixamo, por ejemplo) y reproduce sus
// animaciones de huesos (skeletal animation).
//
// Idea general:
//  1. Cada vertice guarda hasta 4 huesos que lo afectan y el peso de cada uno.
//  2. Cada hueso tiene una "offset matrix" (inversa de su pose de reposo) que lleva el vertice
//     del espacio del modelo al espacio del hueso.
//  3. En cada frame se interpolan los keyframes (posicion, rotacion y escala) de cada hueso y
//     se recorre la jerarquia de nodos multiplicando transformaciones padre -> hijo.
//  4. La matriz final de cada hueso = transformacionGlobal * offset. El vertex
//     shader mezcla las 4 matrices de cada vertice segun sus pesos.
//
// Usa el mismo shader de iluminacion (lighting.vs/lighting.frag): Draw() activa la uniform
// "skinned" y sube "finalBonesMatrices"; al terminar la vuelve a 0 para los modelos estaticos.
// Requiere que Model.h se haya incluido antes (usa TextureFromFile).
// ---------------------------------------------------------------------------------------------

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
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

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

class SkinnedModel
{
public:
	// path: archivo con malla + esqueleto (+ animacion). texturaForzada: ruta a una imagen que se
	// usa si el FBX no trae la textura (o trae una ruta que no existe en esta computadora).
	SkinnedModel(const std::string& path, const std::string& texturaForzada = "")
	{
		texturaExtra = texturaForzada;
		cargarModelo(path);
	}

	// Agrega las animaciones de otro FBX con el MISMO esqueleto (Mixamo "Without Skin").
	// Devuelve el indice de la primera animacion agregada, o -1 si no se pudo.
	int CargarAnimacion(const std::string& path)
	{
		const aiScene* scene = importar(path);
		if (!scene) return -1;
		int primera = (int)animaciones.size();
		leerAnimaciones(scene);
		aiReleaseImport(scene);
		return (int)animaciones.size() > primera ? primera : -1;
	}

	int NumAnimaciones() const { return (int)animaciones.size(); }

	// Cambia la animacion actual y la reinicia. loop=false: se queda en el ultimo frame.
	void Reproducir(int indice, bool loop = true)
	{
		if (indice < 0 || indice >= (int)animaciones.size()) return;
		actual = indice;
		tiempo = 0.0f;
		repetir = loop;
		terminada = false;
	}

	bool Terminada() const { return terminada; }

	// Avanza la animacion dt segundos y recalcula las matrices de los huesos.
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
		calcularPose(raiz, glm::mat4(1.0f), a);
	}

	void Draw(Shader& shader)
	{
		glUniform1i(glGetUniformLocation(shader.Program, "skinned"), 1);
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "finalBonesMatrices"),
			MAX_BONES, GL_FALSE, glm::value_ptr(matricesFinales[0]));

		for (const MallaSkin& m : mallas) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m.textura);
			glBindVertexArray(m.VAO);
			glDrawElements(GL_TRIANGLES, m.numIndices, GL_UNSIGNED_INT, 0);
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUniform1i(glGetUniformLocation(shader.Program, "skinned"), 0);
	}

	// Caja envolvente de la malla en pose de reposo (sirve para escalar y apoyar en el suelo).
	glm::vec3 minimo{ 1e9f }, maximo{ -1e9f };

private:
	struct VerticeSkin {
		glm::vec3 posicion;
		glm::vec3 normal;
		glm::vec2 uv;
		int huesos[MAX_BONE_INFLUENCE];
		float pesos[MAX_BONE_INFLUENCE];
	};
	struct MallaSkin { GLuint VAO = 0, VBO = 0, EBO = 0; GLsizei numIndices = 0; GLuint textura = 0; };
	struct InfoHueso { int id; glm::mat4 offset; };
	struct Nodo { std::string nombre; glm::mat4 transformacion; std::vector<Nodo> hijos; };
	struct KeyVec { float t; glm::vec3 v; };
	struct KeyQuat { float t; glm::quat q; };
	struct Canal { std::vector<KeyVec> pos, esc; std::vector<KeyQuat> rot; };
	struct Animacion { std::string nombre; float duracion = 0, ticksPorSegundo = 25; std::map<std::string, Canal> canales; };

	std::vector<MallaSkin> mallas;
	std::map<std::string, InfoHueso> huesos;
	int contadorHuesos = 0;
	Nodo raiz;
	glm::mat4 inversaGlobal{ 1.0f };
	std::vector<Animacion> animaciones;
	std::vector<glm::mat4> matricesFinales = std::vector<glm::mat4>(MAX_BONES, glm::mat4(1.0f));
	int actual = -1;
	float tiempo = 0.0f;
	bool repetir = true, terminada = false;
	std::string directorio, texturaExtra;
	std::vector<VerticeSkin> verticesCPU;   // copia para medir la caja ya deformada

	// Aplica en CPU el mismo skinning que el shader para obtener la caja de la pose actual
	void calcularCaja()
	{
		minimo = glm::vec3(1e9f); maximo = glm::vec3(-1e9f);
		for (const VerticeSkin& v : verticesCPU) {
			glm::mat4 skin(0.0f);
			float suma = 0.0f;
			for (int k = 0; k < MAX_BONE_INFLUENCE; k++)
				if (v.huesos[k] >= 0) { skin += matricesFinales[v.huesos[k]] * v.pesos[k]; suma += v.pesos[k]; }
			if (suma < 0.001f) skin = glm::mat4(1.0f);
			glm::vec3 p = glm::vec3(skin * glm::vec4(v.posicion, 1.0f));
			minimo = glm::min(minimo, p); maximo = glm::max(maximo, p);
		}
		verticesCPU.clear(); verticesCPU.shrink_to_fit();
	}

	static glm::mat4 aGlm(const aiMatrix4x4& m)
	{
		glm::mat4 r;
		r[0][0] = m.a1; r[1][0] = m.a2; r[2][0] = m.a3; r[3][0] = m.a4;
		r[0][1] = m.b1; r[1][1] = m.b2; r[2][1] = m.b3; r[3][1] = m.b4;
		r[0][2] = m.c1; r[1][2] = m.c2; r[2][2] = m.c3; r[3][2] = m.c4;
		r[0][3] = m.d1; r[1][3] = m.d2; r[2][3] = m.d3; r[3][3] = m.d4;
		return r;
	}

	static const aiScene* importar(const std::string& path)
	{
		// Se conservan los nodos auxiliares del FBX ("<hueso>_$AssimpFbx$_Rotation", etc.): Mixamo pone
		// ahi la mayoria de las rotaciones animadas, y los canales de la animacion apuntan a esos nodos.
		aiPropertyStore* props = aiCreatePropertyStore();
		aiSetImportPropertyInteger(props, AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 1);
		const aiScene* scene = aiImportFileExWithProperties(path.c_str(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_LimitBoneWeights,
			nullptr, props);
		aiReleasePropertyStore(props);
		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
			std::cout << "ERROR::SKINNED::ASSIMP " << path << ": " << aiGetErrorString() << std::endl;
			if (scene) aiReleaseImport(scene);
			return nullptr;
		}
		return scene;
	}

	void cargarModelo(const std::string& path)
	{
		const aiScene* scene = importar(path);
		if (!scene) return;
		directorio = path.substr(0, path.find_last_of("/\\"));
		inversaGlobal = glm::inverse(aGlm(scene->mRootNode->mTransformation));
		copiarNodos(scene->mRootNode, raiz);
		procesarNodo(scene->mRootNode, scene, glm::mat4(1.0f));
		leerAnimaciones(scene);
		aiReleaseImport(scene);
		std::cout << "SkinnedModel " << path << ": " << mallas.size() << " mallas, " << contadorHuesos
			<< " huesos, " << animaciones.size() << " animaciones" << std::endl;
		if (!animaciones.empty()) Reproducir(0);
		Actualizar(0.0f);
		calcularCaja();
	}

	void copiarNodos(const aiNode* src, Nodo& dst)
	{
		dst.nombre = src->mName.C_Str();
		dst.transformacion = aGlm(src->mTransformation);
		dst.hijos.resize(src->mNumChildren);
		for (unsigned i = 0; i < src->mNumChildren; i++) copiarNodos(src->mChildren[i], dst.hijos[i]);
	}

	void procesarNodo(const aiNode* nodo, const aiScene* scene, const glm::mat4& padre)
	{
		glm::mat4 global = padre * aGlm(nodo->mTransformation);
		for (unsigned i = 0; i < nodo->mNumMeshes; i++)
			mallas.push_back(procesarMalla(scene->mMeshes[nodo->mMeshes[i]], scene, global));
		for (unsigned i = 0; i < nodo->mNumChildren; i++) procesarNodo(nodo->mChildren[i], scene, global);
	}

	MallaSkin procesarMalla(const aiMesh* mesh, const aiScene* scene, const glm::mat4& global)
	{
		std::vector<VerticeSkin> vertices(mesh->mNumVertices);
		for (unsigned i = 0; i < mesh->mNumVertices; i++) {
			VerticeSkin& v = vertices[i];
			v.posicion = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			v.normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0, 1, 0);
			v.uv = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
			for (int k = 0; k < MAX_BONE_INFLUENCE; k++) { v.huesos[k] = -1; v.pesos[k] = 0.0f; }
		}

		// Pesos de huesos: cada vertice se queda con los 4 de mayor peso
		for (unsigned b = 0; b < mesh->mNumBones; b++) {
			const aiBone* bone = mesh->mBones[b];
			std::string nombre = bone->mName.C_Str();
			if (huesos.find(nombre) == huesos.end()) {
				if (contadorHuesos >= MAX_BONES) continue;
				huesos[nombre] = { contadorHuesos++, aGlm(bone->mOffsetMatrix) };
			}
			int id = huesos[nombre].id;
			for (unsigned w = 0; w < bone->mNumWeights; w++) {
				VerticeSkin& v = vertices[bone->mWeights[w].mVertexId];
				float peso = bone->mWeights[w].mWeight;
				int menor = 0;
				for (int k = 1; k < MAX_BONE_INFLUENCE; k++) if (v.pesos[k] < v.pesos[menor]) menor = k;
				if (peso > v.pesos[menor]) { v.pesos[menor] = peso; v.huesos[menor] = id; }
			}
		}
		for (VerticeSkin& v : vertices) {
			float suma = v.pesos[0] + v.pesos[1] + v.pesos[2] + v.pesos[3];
			if (suma > 0.0f) for (int k = 0; k < MAX_BONE_INFLUENCE; k++) v.pesos[k] /= suma;
		}
		verticesCPU.insert(verticesCPU.end(), vertices.begin(), vertices.end());

		std::vector<GLuint> indices;
		for (unsigned i = 0; i < mesh->mNumFaces; i++)
			for (unsigned j = 0; j < mesh->mFaces[i].mNumIndices; j++) indices.push_back(mesh->mFaces[i].mIndices[j]);

		MallaSkin m;
		m.numIndices = (GLsizei)indices.size();
		m.textura = cargarTextura(scene, scene->mMaterials[mesh->mMaterialIndex]);
		std::cout << "  malla \"" << mesh->mName.C_Str() << "\": " << mesh->mNumVertices << " vertices, textura id " << m.textura << std::endl;

		glGenVertexArrays(1, &m.VAO);
		glGenBuffers(1, &m.VBO);
		glGenBuffers(1, &m.EBO);
		glBindVertexArray(m.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VerticeSkin), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VerticeSkin), (void*)offsetof(VerticeSkin, posicion));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VerticeSkin), (void*)offsetof(VerticeSkin, normal));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VerticeSkin), (void*)offsetof(VerticeSkin, uv));
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT, sizeof(VerticeSkin), (void*)offsetof(VerticeSkin, huesos));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VerticeSkin), (void*)offsetof(VerticeSkin, pesos));
		glBindVertexArray(0);
		return m;
	}

	GLuint cargarTextura(const aiScene* scene, const aiMaterial* mat)
	{
		if (!texturaExtra.empty()) {
			static std::map<std::string, GLuint> cache;
			if (!cache.count(texturaExtra)) {
				size_t corte = texturaExtra.find_last_of("/\\");
				cache[texturaExtra] = TextureFromFile(texturaExtra.substr(corte + 1).c_str(), texturaExtra.substr(0, corte));
			}
			return cache[texturaExtra];
		}
		aiString ruta;
		if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0 || mat->GetTexture(aiTextureType_DIFFUSE, 0, &ruta) != AI_SUCCESS) {
			std::cout << "  material sin textura difusa" << std::endl;
			return 0;
		}
		std::string r = ruta.C_Str();
		std::cout << "  textura del FBX: " << r << std::endl;
		if (!r.empty() && r[0] == '*') { // textura incrustada en el FBX
			const aiTexture* tex = scene->mTextures[atoi(r.c_str() + 1)];
			int w, h, c;
			unsigned char* img = tex->mHeight == 0
				? SOIL_load_image_from_memory((unsigned char*)tex->pcData, tex->mWidth, &w, &h, &c, SOIL_LOAD_RGBA)
				: nullptr;
			if (!img) return 0;
			GLuint id;
			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			SOIL_free_image_data(img);
			return id;
		}
		size_t corte = r.find_last_of("/\\");   // solo el nombre del archivo, junto al FBX
		return TextureFromFile(r.substr(corte == std::string::npos ? 0 : corte + 1).c_str(), directorio);
	}

	void leerAnimaciones(const aiScene* scene)
	{
		for (unsigned i = 0; i < scene->mNumAnimations; i++) {
			const aiAnimation* src = scene->mAnimations[i];
			Animacion a;
			a.nombre = src->mName.C_Str();
			a.duracion = (float)src->mDuration;
			a.ticksPorSegundo = src->mTicksPerSecond > 0 ? (float)src->mTicksPerSecond : 25.0f;
			for (unsigned c = 0; c < src->mNumChannels; c++) {
				const aiNodeAnim* ch = src->mChannels[c];
				Canal& canal = a.canales[ch->mNodeName.C_Str()];
				for (unsigned k = 0; k < ch->mNumPositionKeys; k++) {
					const aiVector3D& v = ch->mPositionKeys[k].mValue;
					canal.pos.push_back({ (float)ch->mPositionKeys[k].mTime, glm::vec3(v.x, v.y, v.z) });
				}
				for (unsigned k = 0; k < ch->mNumRotationKeys; k++) {
					const aiQuaternion& q = ch->mRotationKeys[k].mValue;
					canal.rot.push_back({ (float)ch->mRotationKeys[k].mTime, glm::quat(q.w, q.x, q.y, q.z) });
				}
				for (unsigned k = 0; k < ch->mNumScalingKeys; k++) {
					const aiVector3D& v = ch->mScalingKeys[k].mValue;
					canal.esc.push_back({ (float)ch->mScalingKeys[k].mTime, glm::vec3(v.x, v.y, v.z) });
				}
			}
			std::cout << "  animacion '" << a.nombre << "': " << a.duracion / a.ticksPorSegundo << " s, "
				<< a.canales.size() << " canales" << std::endl;
			animaciones.push_back(a);
		}
	}

	// Busca el par de keyframes que rodea al tiempo t y regresa el factor de mezcla entre ellos
	template <typename K>
	static size_t buscarKey(const std::vector<K>& keys, float t, float& factor)
	{
		size_t i = 0;
		while (i + 2 < keys.size() && t >= keys[i + 1].t) i++;
		float dt = keys[i + 1].t - keys[i].t;
		factor = dt > 0.0f ? glm::clamp((t - keys[i].t) / dt, 0.0f, 1.0f) : 0.0f;
		return i;
	}

	static glm::mat4 interpolar(const Canal& c, float t)
	{
		float f;
		glm::vec3 p(0.0f), s(1.0f);
		glm::quat q(1.0f, 0.0f, 0.0f, 0.0f);
		if (c.pos.size() == 1) p = c.pos[0].v;
		else if (c.pos.size() > 1) { size_t i = buscarKey(c.pos, t, f); p = glm::mix(c.pos[i].v, c.pos[i + 1].v, f); }
		if (c.rot.size() == 1) q = c.rot[0].q;
		else if (c.rot.size() > 1) { size_t i = buscarKey(c.rot, t, f); q = glm::slerp(c.rot[i].q, c.rot[i + 1].q, f); }
		if (c.esc.size() == 1) s = c.esc[0].v;
		else if (c.esc.size() > 1) { size_t i = buscarKey(c.esc, t, f); s = glm::mix(c.esc[i].v, c.esc[i + 1].v, f); }
		return glm::translate(glm::mat4(1.0f), p) * glm::mat4_cast(glm::normalize(q)) * glm::scale(glm::mat4(1.0f), s);
	}

	// Recorre la jerarquia: transformacion global del hijo = global del padre * local del hijo
	void calcularPose(const Nodo& nodo, const glm::mat4& padre, const Animacion& a)
	{
		glm::mat4 local = nodo.transformacion;
		auto it = a.canales.find(nodo.nombre);
		if (it != a.canales.end()) local = interpolar(it->second, tiempo);
		glm::mat4 global = padre * local;
		auto h = huesos.find(nodo.nombre);
		if (h != huesos.end()) matricesFinales[h->second.id] = global * h->second.offset;
		for (const Nodo& hijo : nodo.hijos) calcularPose(hijo, global, a);
	}
};
