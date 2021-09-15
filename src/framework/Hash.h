#pragma once


static unsigned int hash(const char* str) //Implentmation of FNV-1a Hash
{
    static const unsigned int FNV_PRIME = 16777619u;
    static const unsigned int OFFSET_BASIS = 2166136261u;

    const size_t length = strlen(str) + 1;
    unsigned int hash = OFFSET_BASIS;
    for (size_t i = 0; i < length; ++i)
    {
        hash ^= *str++;
        hash *= FNV_PRIME;
    }
    return hash;
}