#include "includes/utils.h"

SpatialTree * SpatialTree::instance() {
    if (SpatialTree::singleton == nullptr) SpatialTree::singleton = new SpatialTree();
    return SpatialTree::singleton;
}

void SpatialTree::loadComponents(std::vector<Component *> & components) {
    // TODO: implement bulk loading strategy
    
};

std::vector<Component *> SpatialTree::findBBoxIntersect(BoundingBox bbox) {
    std::vector<Component *> results;
    
    // TODO: implement
    
    return results;
}

void SpatialTree::destroy() {
    if (SpatialTree::singleton != nullptr) {
        delete SpatialTree::singleton;
        SpatialTree::singleton = nullptr;
    }
}


SpatialTree * SpatialTree::singleton = nullptr;
