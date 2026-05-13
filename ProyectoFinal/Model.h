#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "SOIL2/SOIL2.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"

using namespace std;

// Prototipos de funciones globales
GLint TextureFromFile(const char* path, string directory);
GLint TextureFromEmbedded(const aiTexture* embTexture);

// Estructura para guardar los datos básicos de cada hueso
struct BoneInfo
{
	int id;               // ID numérico para el Shader
	glm::mat4 offset;     // Matriz que mueve el vértice de su posición original al hueso
};

class Model
{
public:
	/* Functions   */
	Model(GLchar* path)
	{
		this->loadModel(path);
	}

	void Draw(Shader shader)
	{
		for (GLuint i = 0; i < this->meshes.size(); i++)
		{
			this->meshes[i].Draw(shader);
		}
	}

	// --- METODOS DE ACCESO PARA EL ANIMADOR (FASE 3) ---
	auto& GetBoneInfoMap() { return m_BoneInfoMap; }
	int& GetBoneCount() { return m_BoneCounter; }
	// ---------------------------------------------------

private:
	/* Model Data  */
	vector<Mesh> meshes;
	string directory;
	vector<Texture> textures_loaded;

	// --- VARIABLES DE ANIMACIÓN ---
	std::map<string, BoneInfo> m_BoneInfoMap; // Diccionario (Nombre del hueso -> ID numérico)
	int m_BoneCounter = 0;                    // Contador total de huesos descubiertos
	// ------------------------------

	/* Functions   */
	void loadModel(string path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		this->directory = path.substr(0, path.find_last_of('/'));
		this->processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		for (GLuint i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			this->meshes.push_back(this->processMesh(mesh, scene));
		}
		for (GLuint i = 0; i < node->mNumChildren; i++)
		{
			this->processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		vector<Vertex> vertices;
		vector<GLuint> indices;
		vector<Texture> textures;

		// 1. Extraer posiciones, normales y texturas de los vértices
		for (GLuint i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector;

			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;

			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
			{
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vertex);
		}

		// 2. Extraer caras (indices)
		for (GLuint i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (GLuint j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		// 3. Process materials
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}

		// --- FASE 2: EXTRACCIÓN DE PESOS Y HUESOS DESDE ASSIMP ---
		// A. Inicializamos todos los vértices con valores "vacíos"
		for (int i = 0; i < vertices.size(); i++) {
			for (int j = 0; j < 4; j++) {
				vertices[i].m_BoneIDs[j] = -1;
				vertices[i].m_Weights[j] = 0.0f;
			}
		}

		// B. Extraemos los huesos del archivo
		for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			string boneName = mesh->mBones[boneIndex]->mName.C_Str();

			// Si el hueso es nuevo, lo registramos en el diccionario
			if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = m_BoneCounter;

				// Convertimos la matriz offset de Assimp a matriz de GLM
				aiMatrix4x4 aiMat = mesh->mBones[boneIndex]->mOffsetMatrix;
				glm::mat4 glmMat;
				for (int row = 0; row < 4; row++)
					for (int col = 0; col < 4; col++)
						glmMat[row][col] = aiMat[col][row];

				newBoneInfo.offset = glmMat;
				m_BoneInfoMap[boneName] = newBoneInfo;
				boneID = m_BoneCounter;
				m_BoneCounter++;
			}
			else
			{
				boneID = m_BoneInfoMap[boneName].id;
			}

			// C. Asignamos los pesos a cada vértice correspondiente
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;

				// Buscamos un espacio libre (de 0 a 3) en el vértice para guardar este hueso
				for (int i = 0; i < 4; ++i)
				{
					if (vertices[vertexId].m_BoneIDs[i] < 0)
					{
						vertices[vertexId].m_Weights[i] = weight;
						vertices[vertexId].m_BoneIDs[i] = boneID;
						break;
					}
				}
			}
		}
		// ---------------------------------------------------------

		return Mesh(vertices, indices, textures);
	}

	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName, const aiScene* scene)
	{
		vector<Texture> textures;

		for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			GLboolean skip = false;

			for (GLuint j = 0; j < textures_loaded.size(); j++)
			{
				if (textures_loaded[j].path == str)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}

			if (!skip)
			{
				Texture texture;
				const aiTexture* embeddedTexture = nullptr;

				if (str.length > 0 && str.C_Str()[0] == '*')
				{
					int textureIndex = atoi(&str.C_Str()[1]);
					if (textureIndex >= 0 && textureIndex < scene->mNumTextures)
					{
						embeddedTexture = scene->mTextures[textureIndex];
					}
				}

				if (embeddedTexture != nullptr)
				{
					texture.id = TextureFromEmbedded(embeddedTexture);
				}
				else
				{
					texture.id = TextureFromFile(str.C_Str(), this->directory);
				}

				texture.type = typeName;
				texture.path = str;
				textures.push_back(texture);

				this->textures_loaded.push_back(texture);
			}
		}

		return textures;
	}
};

// =========================================================================
// FUNCIÓN GLOBAL 1: CARGA DE ARCHIVOS EXTERNOS (.JPG, .PNG)
// =========================================================================
GLint TextureFromFile(const char* path, string directory)
{
	string filename = string(path);
	size_t lastSlash = filename.find_last_of("/\\");
	if (lastSlash != string::npos) {
		filename = filename.substr(lastSlash + 1);
	}
	filename = directory + '/' + filename;

	GLuint textureID;
	glGenTextures(1, &textureID);
	int width, height;

	unsigned char* image = SOIL_load_image(filename.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);

	if (image)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::cout << "¡ERROR DE MODELO! No se pudo encontrar la textura externa: " << filename << std::endl;
	}

	SOIL_free_image_data(image);
	return textureID;
}

// =========================================================================
// FUNCIÓN GLOBAL 2: CARGA DE TEXTURAS INCRUSTADAS EN MEMORIA (FBX EMBEDDED)
// =========================================================================
GLint TextureFromEmbedded(const aiTexture* embTexture)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, channels;
	unsigned char* image = nullptr;

	if (embTexture->mHeight == 0)
	{
		image = SOIL_load_image_from_memory(reinterpret_cast<const unsigned char*>(embTexture->pcData), embTexture->mWidth, &width, &height, &channels, SOIL_LOAD_RGBA);
	}
	else
	{
		image = reinterpret_cast<unsigned char*>(embTexture->pcData);
		width = embTexture->mWidth;
		height = embTexture->mHeight;
	}

	if (image)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		if (embTexture->mHeight == 0) {
			SOIL_free_image_data(image);
		}
	}
	else
	{
		std::cout << "¡ERROR DE MODELO! Falló la extracción de la textura incrustada." << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}