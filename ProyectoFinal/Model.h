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

class Model
{
public:
	/* Functions   */
	// Constructor, expects a filepath to a 3D model.
	Model(GLchar* path)
	{
		this->loadModel(path);
	}

	// Draws the model, and thus all its meshes
	void Draw(Shader shader)
	{
		for (GLuint i = 0; i < this->meshes.size(); i++)
		{
			this->meshes[i].Draw(shader);
		}
	}

private:
	/* Model Data  */
	vector<Mesh> meshes;
	string directory;
	vector<Texture> textures_loaded;	// Stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.

	/* Functions   */
	// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(string path)
	{
		// Read file via ASSIMP
		Assimp::Importer importer;
		// Aplicamos aiProcess_GenSmoothNormals para asegurar que la luz siempre rebote
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

		// Check for errors
		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// Retrieve the directory path of the filepath
		this->directory = path.substr(0, path.find_last_of('/'));

		// Process ASSIMP's root node recursively
		this->processNode(scene->mRootNode, scene);
	}

	// Processes a node in a recursive fashion.
	void processNode(aiNode* node, const aiScene* scene)
	{
		// Process each mesh located at the current node
		for (GLuint i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			this->meshes.push_back(this->processMesh(mesh, scene));
		}

		// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (GLuint i = 0; i < node->mNumChildren; i++)
		{
			this->processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		// Data to fill
		vector<Vertex> vertices;
		vector<GLuint> indices;
		vector<Texture> textures;

		// Walk through each of the mesh's vertices
		for (GLuint i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector;

			// Positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			// Normals
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;

			// Texture Coordinates
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

		// Indices
		for (GLuint i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (GLuint j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		// Process materials
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			// 1. Diffuse maps (Pasamos la escena para extraer texturas incrustadas)
			vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

			// 2. Specular maps
			vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}

		return Mesh(vertices, indices, textures);
	}

	// Versión actualizada y robusta para buscar texturas
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName, const aiScene* scene)
	{
		vector<Texture> textures;

		for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);

			GLboolean skip = false;

			// Evitar recargar la misma textura múltiples veces
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

				// --- BÚSQUEDA DE TEXTURA INCRUSTADA (ESTILO CLÁSICO ASSIMP) ---
				// Si el nombre comienza con '*', significa que es un índice de la memoria (Ej: *0, *1)
				if (str.length > 0 && str.C_Str()[0] == '*')
				{
					// Convertimos el texto (ej: "0", "1") a un entero
					int textureIndex = atoi(&str.C_Str()[1]);

					// Si el índice es válido, tomamos la textura de la escena
					if (textureIndex >= 0 && textureIndex < scene->mNumTextures)
					{
						embeddedTexture = scene->mTextures[textureIndex];
					}
				}

				// Cargamos desde la RAM (si está incrustada) o desde el Disco Duro (si es externa)
				if (embeddedTexture != nullptr)
				{
					texture.id = TextureFromEmbedded(embeddedTexture);
				}
				else
				{
					texture.id = TextureFromFile(str.C_Str(), this->directory);
				}
				// -------------------------------------------------------------

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

	// Limpieza de rutas absolutas basura que el FBX pueda traer guardadas
	size_t lastSlash = filename.find_last_of("/\\");
	if (lastSlash != string::npos) {
		filename = filename.substr(lastSlash + 1);
	}

	filename = directory + '/' + filename;

	GLuint textureID;
	glGenTextures(1, &textureID); // ¡Inicializa el ID oficial de OpenGL!

	int width, height;

	// SOIL_LOAD_RGBA permite leer canales alfa para evitar texturas negras/blancas
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

	// Assimp marca mHeight como 0 si la textura interna está comprimida (png/jpg)
	if (embTexture->mHeight == 0)
	{
		// Usamos SOIL2 para decodificar la imagen desde los bytes en RAM
		image = SOIL_load_image_from_memory(reinterpret_cast<const unsigned char*>(embTexture->pcData), embTexture->mWidth, &width, &height, &channels, SOIL_LOAD_RGBA);
	}
	else
	{
		// Textura sin comprimir
		image = reinterpret_cast<unsigned char*>(embTexture->pcData);
		width = embTexture->mWidth;
		height = embTexture->mHeight;
	}

	if (image)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		if (embTexture->mHeight == 0) {
			SOIL_free_image_data(image); // Libera solo si SOIL2 reservó la memoria nueva
		}
	}
	else
	{
		std::cout << "¡ERROR DE MODELO! Falló la extracción de la textura incrustada." << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}