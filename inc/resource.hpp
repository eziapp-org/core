#pragma once

#include "platform.hpp"
#include "json.hpp"
#include <unordered_map>
#include <span>
#if OS(WINDOWS)
    #include <gdiplus.h>
using namespace Gdiplus;
#endif

namespace ezi
{
    struct AssetMeta
    {
        size_t offset;
        size_t size;
    };

    typedef std::span<const uint8_t>              BinaryData;
    typedef std::unordered_map<String, AssetMeta> AssetMetaMap;

    class Resource
    {
    private:
        AssetMetaMap assetsMetas;
        BinaryData   assetsBinarys;
        Json         config;

    private:
        Resource();
        Resource(const Resource&)            = delete;
        Resource& operator=(const Resource&) = delete;
        ~Resource();

    public:
        static Resource& GetInstance();

        std::vector<uint8_t> GetAssetData(const String& uri);

        Gdiplus::Image* GetImage(const String& uri);

        Json& GetConfig();

        template <typename T> T GetConfigValue(const String& path, T default_value)
        {
            return at<T>(config, path, default_value);
        }
    };
}

#define CFGRES ezi::Resource::GetInstance().GetConfigValue