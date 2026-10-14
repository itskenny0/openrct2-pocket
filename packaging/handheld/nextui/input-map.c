/* SPDX-License-Identifier: Zlib
 * Runtime GUID and actual joystick capabilities; native SDL controller input.
 */
#include <SDL.h>
#include <stdio.h>
int main(void)
{
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) { fprintf(stderr,"SDL input init: %s\n",SDL_GetError()); return 1; }
    if (SDL_NumJoysticks() < 1) { fprintf(stderr,"No joystick detected; existing SDL mapping retained\n"); SDL_Quit(); return 1; }
    SDL_Joystick* joy = SDL_JoystickOpen(0);
    if (!joy) { fprintf(stderr,"Joystick open: %s\n",SDL_GetError()); SDL_Quit(); return 1; }
    char guid[33]; SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy),guid,sizeof(guid));
    const int hats=SDL_JoystickNumHats(joy), axes=SDL_JoystickNumAxes(joy), buttons=SDL_JoystickNumButtons(joy);
    fprintf(stderr,"Pocket input v2: %s GUID=%s buttons=%d axes=%d hats=%d\n",SDL_JoystickName(joy),guid,buttons,axes,hats);
    if (hats < 1 && axes < 2) { fprintf(stderr,"No supported native D-pad source; mapping retained\n"); SDL_JoystickClose(joy); SDL_Quit(); return 1; }
    const char* dpad=hats>0 ? "dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2" : "dpup:-a1,dpdown:+a1,dpleft:-a0,dpright:+a0";
    fprintf(stderr,"Pocket D-pad source: %s\n",hats>0 ? "hat 0" : "NextUI axes 0/1");
    printf("%s,TrimUI Brick Pocket,a:b1,b:b0,x:b3,y:b2,leftshoulder:b4,rightshoulder:b5,back:b6,start:b7,%s,lefttrigger:a2,righttrigger:a5,platform:Linux,\n",guid,dpad);
    SDL_JoystickClose(joy); SDL_Quit(); return 0;
}
