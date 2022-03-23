#include <time.h>

#include "timestamp.h"

Timestamp::Timestamp() : microseconds_since_epoch_(0)
{
}

Timestamp::Timestamp(int64_t microseconds_since_epoch_) : microseconds_since_epoch_(microseconds_since_epoch_)
{
}

Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
};

std::string Timestamp::toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microseconds_since_epoch_);
    snprintf(buf, 128, "XXXXX%4d/%02d/%02d %2d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}