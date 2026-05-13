#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "Model.h"

// --- FUNCIONES AUXILIARES DE CONVERSIÓN ---
inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from) {
    glm::mat4 to;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            to[x][y] = from[y][x];
    return to;
}
inline glm::vec3 GetGLMVec(const aiVector3D& vec) { return glm::vec3(vec.x, vec.y, vec.z); }
inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation) { return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z); }

// --- ESTRUCTURAS DEL ÁRBOL ---
struct AssimpNodeData {
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

// ==========================================
// CLASE 1: HUESO (Lee los fotogramas clave)
// ==========================================
struct KeyPosition { glm::vec3 position; float timeStamp; };
struct KeyRotation { glm::quat orientation; float timeStamp; };
struct KeyScale { glm::vec3 scale; float timeStamp; };

class Bone {
private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    int m_NumPositions, m_NumRotations, m_NumScales;
    glm::mat4 m_LocalTransform;
    std::string m_Name;
    int m_ID;

    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
        float scaleFactor = 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        scaleFactor = midWayLength / framesDiff;
        return scaleFactor;
    }

    glm::mat4 InterpolatePosition(float animationTime) {
        if (1 == m_NumPositions) return glm::translate(glm::mat4(1.0f), m_Positions[0].position);
        int p0Index = -1;
        for (int i = 0; i < m_NumPositions - 1; ++i) { if (animationTime < m_Positions[i + 1].timeStamp) { p0Index = i; break; } }
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
        glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    glm::mat4 InterpolateRotation(float animationTime) {
        if (1 == m_NumRotations) { auto rotation = glm::normalize(m_Rotations[0].orientation); return glm::toMat4(rotation); }
        int p0Index = -1;
        for (int i = 0; i < m_NumRotations - 1; ++i) { if (animationTime < m_Rotations[i + 1].timeStamp) { p0Index = i; break; } }
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
        finalRotation = glm::normalize(finalRotation);
        return glm::toMat4(finalRotation);
    }

    glm::mat4 InterpolateScaling(float animationTime) {
        if (1 == m_NumScales) return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);
        int p0Index = -1;
        for (int i = 0; i < m_NumScales - 1; ++i) { if (animationTime < m_Scales[i + 1].timeStamp) { p0Index = i; break; } }
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
        glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }

public:
    Bone(const std::string& name, int ID, const aiNodeAnim* channel) : m_Name(name), m_ID(ID), m_LocalTransform(1.0f) {
        m_NumPositions = channel->mNumPositionKeys;
        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex) {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            float timeStamp = channel->mPositionKeys[positionIndex].mTime;
            m_Positions.push_back({ GetGLMVec(aiPosition), timeStamp });
        }
        m_NumRotations = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex) {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            m_Rotations.push_back({ GetGLMQuat(aiOrientation), timeStamp });
        }
        m_NumScales = channel->mNumScalingKeys;
        for (int keyIndex = 0; keyIndex < m_NumScales; ++keyIndex) {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            float timeStamp = channel->mScalingKeys[keyIndex].mTime;
            m_Scales.push_back({ GetGLMVec(scale), timeStamp });
        }
    }
    void Update(float animationTime) { m_LocalTransform = InterpolatePosition(animationTime) * InterpolateRotation(animationTime) * InterpolateScaling(animationTime); }
    glm::mat4 GetLocalTransform() { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() { return m_ID; }
};

// ==========================================
// CLASE 2: ANIMACIÓN (Carga el árbol del FBX)
// ==========================================
class Animation {
public:
    Animation(const std::string& animationPath, Model* model) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
        assert(scene && scene->mRootNode);
        auto animation = scene->mAnimations[0]; // Carga la primera animación del archivo
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 24.0f; // 24 FPS por defecto
        ReadHeirarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, *model);
    }
    Bone* FindBone(const std::string& name) {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(), [&](const Bone& Bone) { return Bone.GetBoneName() == name; });
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }
    float GetTicksPerSecond() { return m_TicksPerSecond; }
    float GetDuration() { return m_Duration; }
    const AssimpNodeData& GetRootNode() { return m_RootNode; }
    const std::map<std::string, BoneInfo>& GetBoneIDMap() { return m_BoneInfoMap; }

private:
    void ReadMissingBones(const aiAnimation* animation, Model& model) {
        int size = animation->mNumChannels;
        auto& boneInfoMap = model.GetBoneInfoMap();
        int& boneCount = model.GetBoneCount();
        for (int i = 0; i < size; i++) {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;
            if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }
            m_Bones.push_back(Bone(boneName, boneInfoMap[boneName].id, channel));
        }
        m_BoneInfoMap = boneInfoMap;
    }
    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src) {
        assert(src);
        dest.name = src->mName.data;
        dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;
        for (int i = 0; i < src->mNumChildren; i++) {
            AssimpNodeData newData;
            ReadHeirarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }
    float m_Duration;
    int m_TicksPerSecond;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
};

// ==========================================
// CLASE 3: ANIMADOR (Reproduce los fotogramas)
// ==========================================
class Animator {
public:
    Animator(Animation* animation) {
        m_CurrentTime = 0.0;
        m_CurrentAnimation = animation;
        m_FinalBoneMatrices.reserve(100);
        for (int i = 0; i < 100; i++) m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }
    void UpdateAnimation(float dt) {
        if (m_CurrentAnimation) {
            m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        }
    }
    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform) {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;
        Bone* Bone = m_CurrentAnimation->FindBone(nodeName);
        if (Bone) {
            Bone->Update(m_CurrentTime);
            nodeTransform = Bone->GetLocalTransform();
        }
        glm::mat4 globalTransformation = parentTransform * nodeTransform;
        auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
            int index = boneInfoMap[nodeName].id;
            glm::mat4 offset = boneInfoMap[nodeName].offset;
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }
        for (int i = 0; i < node->childrenCount; i++) CalculateBoneTransform(&node->children[i], globalTransformation);
    }
    std::vector<glm::mat4> GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
};