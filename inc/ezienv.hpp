#pragma once

#include <string>
#include "json.hpp"

namespace ezi
{
    struct Position;

    class EziEnv
    {
    private:
        Json   envData;
        bool   isNeedReset = false;
        String envFilePath;

    private:
        EziEnv();
        EziEnv(const EziEnv&)            = delete;
        EziEnv& operator=(const EziEnv&) = delete;

        void        SaveVar(std::string key, Object value);
        std::string GetVar(std::string key);

    public:
        bool IsNeedReset() const;

    public:
        static EziEnv& GetInstance();

        Position GetRememberedWindowPosition();
        void     SetRememberedWindowPosition(const Position& pos);
    };

}