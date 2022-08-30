#pragma once

#define LOGI(s) std::cerr << __TIME__ << " [  \033[32mINFO\033[0m  ] " << __FILE__ << ": " << s << std::endl;
#define LOGE(s) std::cerr << __TIME__ << " [  \033[31mERROR\033[0m  ] " << __FILE__ << ": " << s << std::endl;

#define CHECK(a, s)          \
    if (a)                   \
    {                        \
        LOGI(s);             \
    }                        \
    else                     \
    {                        \
        LOGE(s);             \
        return SERVER_ERROR; \
    }
