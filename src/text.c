/*
   text.c

   Mike Hufnagel & Bill Kendrick
   Last modified: 11/18/95 (clean up)
*/

#include <X11/Xlib.h>
#include "text.h"

TTF_Font *sdl_font;



void sdl_drawtext(SDL_Renderer *renderer, SDL_Color color, int x, int y, char *s)
{
    if (sdl_font == NULL) return;
	SDL_Surface* text = TTF_RenderText_Blended(sdl_font, s, color);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, text);
    SDL_FreeSurface(text);
    SDL_Rect srcrect,dstrect;
    int w,h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = w;
    srcrect.h = h;
    dstrect.x = x;
    dstrect.y = y;
    dstrect.w = w;
    dstrect.h = h;
    SDL_RenderCopy(renderer, tex, &srcrect, &dstrect);
    SDL_DestroyTexture(tex);
}

