//
// Created by engin on 8.06.2018.
//

#ifndef LIMONENGINE_ANIMATIONSEQUENCE_H
#define LIMONENGINE_ANIMATIONSEQUENCE_H


#include <vector>
#include "../libs/ImGuizmo/ImSequencer.h"
#include "Transformation.h"

class PhysicalRenderable;
class AnimationCustom;
class AnimationNode;

class AnimationSequenceInterface : public ImSequencer::SequenceInterface {
    int startTime = 0;
    Transformation originalTransformation;
    struct AnimationSequenceItem {
        int mFrameEnd;
        //it should save a transformation, that is the difference from start. We can safely assume point 0 is ineffective transform
        Transformation transformation;
    };
    std::vector<AnimationSequenceItem> sections;
    PhysicalRenderable* animatingObject;
    AnimationCustom* animationInProgress = nullptr;

    /********************IMGUI variables *************************/
    char animationNameBuffer[32] {0};
    int selectedEntry = 0;//to show this, we must have created 1 element. that one is selected
    int firstFrame = 0;
    bool expanded = true;
    /********************IMGUI variables *************************/

    /**
     * Animation to fill should be created(not null, and its animation node set to not null too), and empty.
     *
     * @param item
     * @param animationToFill
     */
    void fillAnimationFromItem(uint32_t itemIndex, AnimationCustom *animationToFill, const Transformation& sourceTransform) const;
    void pushTransformToAnimationTime(AnimationNode *animationNode, float time, const Transformation &transformation) const;
    void pushIdentityToAnimationTime(AnimationNode *animationNode, float time) const;


public:
    int mFrameCount = 120;


    int GetFrameCount() const { return mFrameCount; }
    int GetItemCount() const { return (int)sections.size(); }

    int GetItemTypeCount() const { return 1; }
    const char *GetItemTypeName(int typeIndex __attribute((unused))) const { return "Section"; }
    const char *GetItemLabel(int index) const {
        static char tmps[512];
        sprintf(tmps, "[%02d] %s", index, "Section");
        return tmps;
    }

    void Get(int index, int** start, int** end, int *type, unsigned int *color) {
        AnimationSequenceItem &item = sections[index];
        if (color) {
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        }
        if (start) {
            if(index == 0) {
                *start = &startTime;
            } else {
                *start = &sections[index-1].mFrameEnd;
            }
        }
        if (end) {
            *end = &item.mFrameEnd;
        }
        if (type) {
            *type = 0;//we don't have multiple types
        }
    }
    void Add(int type __attribute((unused)));
    void setTransform(int32_t indexToSet);


    void Del(int index) {
        //don't allow deleting last element
        if(sections.size()>1) {
            sections.erase(sections.begin() + index);
        }
    }

    void Duplicate(int index);

    const PhysicalRenderable *getAnimatingObject() const {
        return animatingObject;
    }

    AnimationSequenceInterface(PhysicalRenderable* animatingObject);
    ~AnimationSequenceInterface();
    AnimationCustom* buildAnimationFromCurrentItems();

    /**
     * Returns whether animation finished or not
     * @return
     */
    void addAnimationSequencerToEditor(bool &finished, bool &cancelled);
};


#endif //LIMONENGINE_ANIMATIONSEQUENCE_H
