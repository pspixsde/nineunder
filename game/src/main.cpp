#include "game_app.hpp"

#include "nu/core/log.hpp"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    nine::GameApp app;
    return app.run();
}
