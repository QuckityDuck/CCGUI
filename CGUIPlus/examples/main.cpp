#include <cgui.hpp>
#include <iostream>

int main()
{
    cg.window("Game", 800, 600, true);

    // This image is included with the example project.
    cg.sprite("player", "examples/assets/player.png");
    cg.sposition("player", 100, 100);
    cg.sscale("player", 2.0f, 2.0f);

    cg.font("Arial", "", 24);
    cg.background(3, 30, 30);

    while (cg.running())
    {
        cg.clear();

        cg.dsprite("player", 100, 100);
        cg.text("Hello World!", 50, 50);
        cg.rect(450, 100, 180, 90, 40, 120, 220);
        cg.circle(650, 200, 45, 220, 90, 90);
        cg.line(450, 250, 700, 350, 255, 255, 255);

        cg.button("Play", 100, 350, 200, 50);

        if (cg.clicked("Play"))
        {
            std::cout << "Play!\n";
        }

        if (cg.keypressed("E"))
        {
            cg.svisible("player", false);
            std::cout << "E pressed\n";
        }

        if (cg.keydown("SPACE"))
        {
            cg.svisible("player", true);
        }

        cg.update();
    }

    return 0;
}
