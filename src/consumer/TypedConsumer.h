#pragma once

#include <queue>
#include <optional>
#include "DataConsumer.h"

template<typename T>
class TypedConsumer : public DataConsumer {
protected:
    std::queue<T> queue;

    virtual T decode(std::vector<uint8_t> &data) = 0;

public:
    void consume(std::vector<uint8_t> data) override {
        T decodedVal = decode(data);

        queue.push(decodedVal);
    }

    std::optional<T> pop() {
        if (queue.empty()) {
            return std::nullopt;
        }
        T val = queue.front();
        queue.pop();
        return val;
    }

    bool isEmpty() {
        return queue.empty();
    }

    bool isFillingUpTooFast() { //My assumption is that, if the program is working normally, then the throughput will be high enough s.t. the number of elements in the queue will never exceed a certain value
        return (queue.size()>100);
    } //This function checks if the queue has exceeded that value and returns true if it has
};
