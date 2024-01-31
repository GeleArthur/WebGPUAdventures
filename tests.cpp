#include <webgpu/webgpu.hpp>

struct ButWhatIs
{
    float myMutableEater;
};

struct PointersAndYou
{
    char* label;
    ButWhatIs* chain;
};

float getTheEater(const PointersAndYou& thing)
{
    return thing.chain->myMutableEater;
}

void letsMakeItDisappear()
{
    ButWhatIs nothing{3.14567890f};

    char label[]{"coolBeans"};
    PointersAndYou yourBreakfast{label, &nothing};

    std::cout << getTheEater(yourBreakfast);
}
