/* SPDX-License-Identifier: Zlib
 * Runtime GUID; button/axis indices from LoveRetro/NextUI tg5040 platform.h.
 * No mouse translation. Only the explicitly requested built-in Brick joystick.
 */
#include <SDL.h>
#include <stdio.h>
int main(void)
{
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) { fprintf(stderr,"SDL input init: %s\n",SDL_GetError());return 1; }
    for(int i=0;i<SDL_NumJoysticks();i++) {
        char guid[33];SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i),guid,sizeof(guid));
        fprintf(stderr,"Joystick %d: %s GUID=%s\n",i,SDL_JoystickNameForIndex(i),guid);
    }
    if(SDL_NumJoysticks()<1){fprintf(stderr,"No joystick detected; existing SDL mapping retained\n");SDL_Quit();return 1;}
    char guid[33];SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(0),guid,sizeof(guid));
    printf("%s,TrimUI Brick Pocket,a:b1,b:b0,x:b3,y:b2,leftshoulder:b4,rightshoulder:b5,back:b6,start:b7,dpup:-a1,dpdown:+a1,dpleft:-a0,dpright:+a0,lefttrigger:a2,righttrigger:a5,platform:Linux,\n",guid);
    SDL_Quit();return 0;
}
