#include "resource.hpp"
#if OS(WINDOWS)
    #include <shlwapi.h>
    #pragma comment(lib, "Shlwapi.lib")
#endif
#include "fstream"
#include "application.hpp"
#include "utils.hpp"
#include <zstd.h>
#include <vector>

namespace ezi
{
    Resource::Resource()
    {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(1004), RT_RCDATA);
        if(!hRes)
            throw std::runtime_error("Failed to find resource ezi.assets.binary");

        HGLOBAL hData = LoadResource(NULL, hRes);

        if(!hData)
            throw std::runtime_error("Failed to load resource ezi.assets.binary");

        void* pData = LockResource(hData);
        DWORD size  = SizeofResource(NULL, hRes);

        assetsBinarys = std::span<const uint8_t>(static_cast<const uint8_t*>(pData), size);

#define MANIFESTSIZEFLAG 4

        std::span<const uint8_t> manifestSizePos = assetsBinarys.subspan(size - MANIFESTSIZEFLAG, MANIFESTSIZEFLAG);

        uint32_t manifestSize = 0;
        std::memcpy(&manifestSize, manifestSizePos.data(), sizeof(manifestSize));

        if(manifestSize == 0 || manifestSize > assetsBinarys.size())
            throw std::runtime_error("Invalid asset manifest size");

        std::span<const uint8_t> manifestData
            = assetsBinarys.subspan(size - manifestSize - MANIFESTSIZEFLAG, manifestSize);

        size_t unZipManifestDataSize = ZSTD_getFrameContentSize(manifestData.data(), manifestData.size());
        if(unZipManifestDataSize == ZSTD_CONTENTSIZE_ERROR || unZipManifestDataSize == ZSTD_CONTENTSIZE_UNKNOWN)
        {
            throw std::runtime_error("无法确定解压后的大小");
        }
        std::vector<uint8_t> unZipManifestData(unZipManifestDataSize);

        size_t result = ZSTD_decompress(
            unZipManifestData.data(), unZipManifestData.size(), manifestData.data(), manifestData.size());
        if(ZSTD_isError(result))
        {
            throw std::runtime_error(ZSTD_getErrorName(result));
        }

        String manifestJsonStr(reinterpret_cast<const char*>(unZipManifestData.data()), unZipManifestData.size());

        auto manifestJson = Json::parse(manifestJsonStr);
        for(auto& [key, value] : manifestJson.items())
        {
            assetsMetas[key] = { value["offset"].get<size_t>(), value["size"].get<size_t>() };
        }
#if BUILDTYPE(DEBUG)
        auto cwd = Utils::GetArg("--cwd");
        if(!cwd.empty())
        {
            SetCurrentDirectoryW(utf8ToUtf16(cwd).c_str());
        }
        auto configPath = Utils::GetArg("--configpath");
        if(configPath.empty())
        {
            configPath = "temp/ezi.config.json";
        }
        std::ifstream file(configPath);
        if(file.is_open())
        {
            file >> config;
            file.close();
        }
        else
        {
            MessageBox(nullptr, (std::string("cannt open ") + configPath).c_str(), "error", MB_OK | MB_ICONERROR);
            exit(1);
        }
#else
        auto   configData = GetAssetData("ezi.config.manifest");
        String configJsonStr(reinterpret_cast<const char*>(configData.data()), configData.size());
        config = Json::parse(configJsonStr);
#endif
    }

    Resource::~Resource()
    {
    }

    Resource& Resource::GetInstance()
    {
        static Resource instance;
        return instance;
    }

    std::vector<uint8_t> Resource::GetAssetData(const String& uri)
    {
        auto it = assetsMetas.find(uri);
        if(it != assetsMetas.end())
        {
            AssetMeta&               meta    = it->second;
            std::span<const uint8_t> zipData = assetsBinarys.subspan(meta.offset, meta.size);

            size_t unZipDataSize = ZSTD_getFrameContentSize(zipData.data(), zipData.size());
            if(unZipDataSize == ZSTD_CONTENTSIZE_ERROR || unZipDataSize == ZSTD_CONTENTSIZE_UNKNOWN)
            {
                throw std::runtime_error("无法确定解压后的大小");
            }
            std::vector<uint8_t> unZipData(unZipDataSize);
            size_t result = ZSTD_decompress(unZipData.data(), unZipData.size(), zipData.data(), zipData.size());
            if(ZSTD_isError(result))
            {
                throw std::runtime_error(ZSTD_getErrorName(result));
            }
            return unZipData;
        }
        return {};
    }

    Json& Resource::GetConfig()
    {
        return config;
    }

    Gdiplus::Image* Resource::GetImage(const String& uri)
    {
#if BUILDTYPE(DEBUG)
        return Gdiplus::Image::FromFile(utf8ToUtf16(uri).c_str());
#else
        auto data = GetAssetData("ezi.splashscreen-" + uri);
        if(data.size() == 0)
            return nullptr;
        IStream* pStream = SHCreateMemStream(data.data(), static_cast<UINT>(data.size()));
        if(!pStream)
        {
            return nullptr;
        }
        Gdiplus::Image* image = Gdiplus::Image::FromStream(pStream);
        pStream->Release();

        return image;
#endif
    }
}