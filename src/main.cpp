#include "application.hpp"
#include "window.hpp"
#include "entry.hpp"
#include "print.hpp"
#include "resource.hpp"

int entry(int argc, char* argv[])
{
    auto& app = ezi::Application::GetInstance();

    auto config = ezi::Resource::GetInstance().GetConfig();

    auto& win = app.CrtWindowByOption(config["window"]);

    win.Show();

    return app.Run();
}
