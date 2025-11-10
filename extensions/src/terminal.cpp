#include "extensions.hpp"
#include "bridge.hpp"
#include "terminal.hpp"
#include <iostream>
#include <regex>

namespace ezi
{
#if BUILDTYPE(DEBUG)
    namespace Private
    {
        static std::string color(const std::string& str)
        {
            std::regex key(R"===("([^"]+)":)===");

            auto colorStr = std::regex_replace(str, key, "\033[38;5;186m$1\033[0m: ");

            std::regex string(R"===(([, ])("[^"]+"))===");

            colorStr = std::regex_replace(colorStr, string, "$1\033[38;5;209m$2\033[0m");

            std::regex number(R"===((-?\d+)([,\]]))===");
            colorStr = std::regex_replace(colorStr, number, "\033[38;5;57m$1\033[0m$2");

            return colorStr;
        }

        static std::string formatLogMessage(const Object& message)
        {
            if(message.is_string())
            {
                return message;
            }
            else if(message.is_number())
            {
                return "\033[38;5;57m" + message.dump() + "\033[0m";
            }
            else
            {
                auto jsonStr = message.dump();
                if(jsonStr.length() <= 100)
                {
                    return color(jsonStr);
                }
                return color("\n" + message.dump(4));
            }
        }
    }
    namespace terminal
    {
        String log(Object args)
        {
            auto argv = args["argv"];
            for(size_t i = 0; i < argv.size(); i++)
            {
                std::cout << Private::formatLogMessage(argv[i]) << " ";
            }
            std::cout << std::endl;
            return "success";
        }

        String error(Object args)
        {
            auto argv = args["argv"];
            for(size_t i = 0; i < argv.size(); i++)
            {
                std::cerr << argv[i].get<std::string>() << " ";
            }
            std::cerr << std::endl;
            return "success";
        }

        void Mount()
        {
            REG(terminal, log);
            REG(terminal, error);
        }
    }
#endif
}