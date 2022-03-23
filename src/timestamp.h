#pragma once

#include <iostream>
#include <string>

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microseconds_since_epoch_);

    // Get a timestamp of now.
    static Timestamp now();

    // Get the formated string of timestamp
    // eg. 2022/03/19 21:12:13
    std::string toString() const;

private:
    // micro second since 1970/01/01 00:00:00 UTC
    int64_t microseconds_since_epoch_;
};
