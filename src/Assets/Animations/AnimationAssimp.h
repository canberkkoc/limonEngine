//
// Created by engin on 12.05.2018.
//

#ifndef LIMONENGINE_ANIMATIONASSIMP_H
#define LIMONENGINE_ANIMATIONASSIMP_H


#include <assimp/anim.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <tinyxml2.h>

class AnimationNode;

class AnimationAssimp {
    float ticksPerSecond;
    float duration;
    //This map keeps the animations for node(bone)
    std::unordered_map<std::string, AnimationNode*> nodes;

public:
    AnimationAssimp(aiAnimation *assimpAnimation);

    glm::mat4 calculateTransform(const std::string& nodeName, float time, bool &isFound) const;

    float getTicksPerSecond() const {
        return ticksPerSecond;
    }

    float getDuration() const {
        return duration;
    }
};


#endif //LIMONENGINE_ANIMATIONASSIMP_H
