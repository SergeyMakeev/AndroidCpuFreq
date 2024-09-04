// The MIT License (MIT)
//
// 	Copyright (c) 2024 Sergey Makeev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//      The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

// This is an alternative implementation of the same idea as Francesco Carucci's Peuck library
// https://github.com/fcarucci-zz/peuck.git
#pragma once

#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <vector>

namespace CpuFreq
{

namespace detail
{

inline bool isWhitespace(char c)
{
    switch (c)
    {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return true;
    default:
        return false;
    }
}

inline char* findDelimiter(char* str, char delimiter)
{
    while (*str)
    {
        if (*str == delimiter)
        {
            return str;
        }
        str++;
    }
    return nullptr;
}

inline char* skipWhitespaceFront(char* front, char* back)
{
    for (; front != back; front++)
    {
        if (!isWhitespace(*front))
        {
            return front;
        }
    }
    return back;
}

inline char* skipWhitespaceBack(char* front, char* back)
{
    for (; back != front; back--)
    {
        if (!isWhitespace(*back))
        {
            return back;
        }
    }
    return front;
}

inline bool strEqual(const char* str, const char* pattern)
{
    // to prevent infinite loops if a string is not zero-terminated
    const size_t kMaxStringSize = 1000;
    for (size_t i = 0; i < kMaxStringSize; i++)
    {
        if (/*str[i] == 0 || */ str[i] != pattern[i])
        {
            return false;
        }

        if (pattern[i] == 0)
        {
            return true;
        }
    }
    return false;
}

struct KeyValue
{
    const char* key;
    const char* value;
};

inline KeyValue parseCpuInfoEntry(char* front, char* back)
{
    front = skipWhitespaceFront(front, back);
    back = skipWhitespaceBack(front, back);
    if (front == back)
    {
        // string is empty
        return KeyValue{nullptr, nullptr};
    }

    char* keyFront = front;
    char* valueBack = back;

    char* keyBack = findDelimiter(keyFront, ':');
    if (!keyBack)
    {
        // can't find delimiter
        return KeyValue{nullptr, nullptr};
    }

    char* valueFront = keyBack + 1; //  character next to the delimiter
    keyBack = keyBack - 1;          //  character before the delimiter

    keyBack = skipWhitespaceBack(keyFront, keyBack);
    if (keyFront == (keyBack + 1))
    {
        // key is empty
        return KeyValue{nullptr, nullptr};
    }

    valueFront = skipWhitespaceFront(valueFront, valueBack);
    if (valueFront == (valueBack + 1))
    {
        // value is empty
        return KeyValue{nullptr, nullptr};
    }

    // null-terminate both sub-strings
    keyBack[1] = '\0';
    valueBack[1] = '\0';

    return KeyValue{keyFront, valueFront};
}

inline bool readFile(const char* fileName, char* dstBuffer, size_t maxBufferSize)
{
    if (maxBufferSize == 0)
    {
        return false;
    }

    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        return false;
    }

    size_t bytesRead = read(fd, dstBuffer, maxBufferSize - 1);
    dstBuffer[bytesRead] = '\0';
    close(fd);
    return true;
}

} // namespace detail

struct CpuInfo
{
    int cpuIndex = -1;
    long cpuImplementer = 0;
    int cpuArch = 0;
    long cpuVariant = 0;
    long cpuPart = 0;
    int cpuRevision = 0;
    int minFreq = 0;
    int maxFreq = 0;
    int currentFreq = 0;
    int packageId = -1;
};

inline void readCpuInfo(std::vector<CpuInfo>& res)
{
    char tmp[16384];
    char fileName[512];
    char tmpBuffer[8192];

    res.clear();

    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (!fp)
    {
        return;
    }

    while (fgets(tmp, 16384, fp) != nullptr)
    {
        size_t len = strlen(tmp);
        if (len == 0)
        {
            continue;
        }
        detail::KeyValue kv = detail::parseCpuInfoEntry(tmp, tmp + len - 1);
        if (kv.key && kv.value)
        {
            if (detail::strEqual(kv.key, "processor"))
            {
                CpuInfo cpuInfo;
                cpuInfo.cpuIndex = atoi(kv.value);

                sprintf(fileName, "/sys/devices/system/cpu/cpu%" PRIu32 "/cpufreq/cpuinfo_min_freq", cpuInfo.cpuIndex);
                if (detail::readFile(fileName, tmpBuffer, 8192))
                {
                    cpuInfo.minFreq = atoi(tmpBuffer);
                }

                sprintf(fileName, "/sys/devices/system/cpu/cpu%" PRIu32 "/cpufreq/cpuinfo_max_freq", cpuInfo.cpuIndex);
                if (detail::readFile(fileName, tmpBuffer, 8192))
                {
                    cpuInfo.maxFreq = atoi(tmpBuffer);
                }

                sprintf(fileName, "/sys/devices/system/cpu/cpu%" PRIu32 "/cpufreq/scaling_cur_freq", cpuInfo.cpuIndex);
                if (detail::readFile(fileName, tmpBuffer, 8192))
                {
                    cpuInfo.currentFreq = atoi(tmpBuffer);
                }

                sprintf(fileName, "/sys/devices/system/cpu/cpu%" PRIu32 "/topology/physical_package_id", cpuInfo.cpuIndex);
                if (detail::readFile(fileName, tmpBuffer, 8192))
                {
                    cpuInfo.packageId = atoi(tmpBuffer);
                }
                res.push_back(cpuInfo);
            }
            else if (!res.empty())
            {
                CpuInfo& cpuInfo = res.back();
                if (detail::strEqual(kv.key, "CPU implementer"))
                {
                    cpuInfo.cpuImplementer = strtol(kv.value, nullptr, 16);
                }
                else if (detail::strEqual(kv.key, "CPU architecture"))
                {
                    cpuInfo.cpuArch = atoi(kv.value);
                }
                else if (detail::strEqual(kv.key, "CPU variant"))
                {
                    cpuInfo.cpuVariant = strtol(kv.value, nullptr, 16);
                }
                else if (detail::strEqual(kv.key, "CPU part"))
                {
                    cpuInfo.cpuPart = strtol(kv.value, nullptr, 16);
                }
                else if (detail::strEqual(kv.key, "CPU revision"))
                {
                    cpuInfo.cpuRevision = atoi(kv.value);
                }
            }
        }
    }
    fclose(fp);
}

} // namespace CpuFreq
