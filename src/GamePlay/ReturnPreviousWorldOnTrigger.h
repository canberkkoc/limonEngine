//
// Created by engin on 6.08.2018.
//

#ifndef LIMONENGINE_RETURNPREVIOUSWORLDONTRIGGER_H
#define LIMONENGINE_RETURNPREVIOUSWORLDONTRIGGER_H


#include "TriggerInterface.h"

class ReturnPreviousWorldOnTrigger : public TriggerInterface {
    static TriggerRegister<ReturnPreviousWorldOnTrigger> reg;
public:
    ReturnPreviousWorldOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::string getName() const override {
        return "ReturnPreviousWorldOnTrigger";
    }
};


#endif //LIMONENGINE_RETURNPREVIOUSWORLDONTRIGGER_H
