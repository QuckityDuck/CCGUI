hey!
this is for CGUI+ (thats its name)

some simple commands are:
create a window:
cg.window("window1", 50, 50, false);
this is just giving the window a name size and if it can be resized,
others are
cg.keydown("e");
cg.keypresses("e")
cg.sprite("sprite1", "dinosaur_cool.jpg")
there is more but im afraid im to lazy (and this is ai made and i dont know all the commands) to type it, if you want an example there is both one (a better one) in the file stuff
and also this one:
wasd to move (keyboard only sry)





#include <cgui.hpp>
#include <iostream>

int main()
{

    int plrx = 100;
    int plry = 100;
    cg.window("Game", 600, 200, true);
    cg.sprite("player", "dinosaur cool pic.jpg");




    while (cg.running())
    {
        cg.clear();
        cg.dsprite("player", plrx, plry);
        cg.text("Hello!", 50, 50);
        
        if(cg.keydown("a")){
            plrx -= 10;
        }
        else if(cg.keydown("d")){
            plrx += 10;
        }
        else if(cg.keydown("s")){
            plry += 10;
        }
        else if(cg.keydown("w")){
            plry -= 10;
        }
        cg.sscale("player", 0.5, 0.5);
        
        cg.update();
    }

    return 0;
}

