#ifndef SRC_INCLUDES_THREADING_H_
#define SRC_INCLUDES_THREADING_H_

#include "shared.h"

class CommandBufferQueue final {
    private:
        std::unique_ptr<std::thread> queueThread = nullptr;
        bool isStopping = true;
        bool hasStopped = true;
        std::mutex lock;
        std::vector<std::queue<VkCommandBuffer>> commmandBuffers;
        uint16_t maxItems = 3;
    public:
        VkCommandBuffer getNextCommandBuffer(uint16_t frameIndex) {            
            std::lock_guard<std::mutex> lock(this->lock);

            if (this->isStopping || this->commmandBuffers[frameIndex].empty()) return nullptr;
            
            VkCommandBuffer ret = this->commmandBuffers[frameIndex].front();
            this->commmandBuffers[frameIndex].pop();
            
            return ret;
        }
        
        uint16_t getNumberOfItems(uint16_t frameIndex) {
            if (this->isStopping) return 0;
            
            return this->commmandBuffers[frameIndex].size();
        }
        
        void startQueue(std::function<VkCommandBuffer(uint16_t)> commandBufferCreation, uint16_t numberOfFrames) {
            this->commmandBuffers.resize(numberOfFrames);

            this->queueThread = std::make_unique<std::thread>([this, commandBufferCreation]() {
                this->isStopping = false;
                this->hasStopped = false;

                while(!this->isStopping) {
                    for (uint16_t i=0;i<this->commmandBuffers.size();i++) {
                        if (this->isStopping) break;

                        {
                            std::lock_guard<std::mutex> lock(this->lock);

                            uint16_t numberOfCommandBuffers = this->commmandBuffers[i].size();
                            if (numberOfCommandBuffers < maxItems) {
                                VkCommandBuffer buf = commandBufferCreation(i);
                                if (buf != nullptr) {
                                    this->commmandBuffers[i].push(std::move(buf));
                                }
                            }
                        }
                    }
                }
                this->hasStopped = true;
            });
            
            this->queueThread->detach();
        }
        
        void stopQueue() {
            if (this->isStopping || this->hasStopped) return;
            
            this->isStopping = true;
            
            std::chrono::high_resolution_clock::time_point shutdownStart = std::chrono::high_resolution_clock::now();
            while (!this->hasStopped) {
                std::chrono::duration<double, std::milli> shutdownPeriod = std::chrono::high_resolution_clock::now() - shutdownStart;
                if (shutdownPeriod.count() > 5000) break;
            }
            
            this->commmandBuffers.clear();
        }
        
        bool isRunning() {
            return !this->isStopping && !this->hasStopped;
        }
};

#endif
