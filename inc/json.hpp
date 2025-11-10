#pragma once
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace ezi
{
    typedef nlohmann::json      Json;
    typedef Json                Object;
    typedef std::vector<Object> Array;
    typedef std::string         String;

    template <typename T> T at(const nlohmann::json& j, const std::string& path, T default_value)
    {
        std::istringstream ss(path);
        String             key;
        const Json*        current = &j;

        while(std::getline(ss, key, '.'))
        {
            if(current->contains(key))
            {
                current = &(*current)[key];
            }
            else
            {
                return default_value;
            }
        }

        if(current->is_null())
            return default_value;
        return current->get<T>();
    }
}