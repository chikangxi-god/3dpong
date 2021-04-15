/*
  3dpong.c
  
  3D Pong 0.5
  
  by Bill Kendrick
  December 9, 1997 - January 26, 1998 / March 14, 1998 / April 28, 2004
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <sys/types.h>
#include <sys/time.h>
#include <math.h>
#include "randnum.h"
#include "text.h"


#define DOUBLE_BUFFER True


SDL_Renderer *renderer;
SDL_Window *sdl_window;
SDL_Color white_color= {255, 255, 255, 255};
SDL_Color sdl_colors[6] =
{
    {255,100,100,255},{100,100,255,255},{100,255,100,255},
    {150,0,0,255},{0,0,150,255},{0,150,0,255}
};
int font_height = 16;
/* Global X-Window Variables: */

char *colornames[6] = { "red", "blue", "green",
    "DarkRed", "DarkBlue", "DarkGreen"
};

int fh[2];


/* Window size: */

#define WIDTH 580
#define HEIGHT 580


/* Arena size: */

#define X_WIDTH 100
#define Y_HEIGHT 100
#define Z_DEPTH 150


/* Shape sizes: */

#define PADDLE_WIDTH 25
#define PADDLE_HEIGHT 25
#define BALL_SIZE 15


/* 3D Controls: */

#define DISTANCE ((float) Z_DEPTH + 100.0)
#define ASPECT 200
#define GLASS_OFFSET 10


/* Speed controls: */

#define FRAMERATE 100000
#define EVENT_LOOP 40
#define SHIMMER_TIME 5


/* Debris controls: */

#define DEBRIS_TIME 50
#define DEBRIS_MIN 5
#define DEBRIS_MAX 10
#define DEBRIS_SPEED 2


/* Computer player controls: */

#define COMPUTER_SPEED 5


/* Handball controls: */

#define MIN_HANDBALL_GRAVITY 0.25


/* Ball controls: */

#define BALL_SPEED 15
#define ANGLE_DIVIDE 3


/* Data struct. sizes: */

#define QUEUE_SIZE 5
#define NUM_DEBRIS 50


/* Game modes: */

#define ONE_PLAYER 0
#define TWO_PLAYERS 1
#define HANDBALL 2


/* Typedefs: */

typedef struct debris_type {
    int exist, time;
    float x, y, z;
    float xm, ym, zm;
} debris_type;


/* Global game Variables: */

int toggle, high_score, got_high_score, final_score, ball_speed,
    debriscount, use_sound, net_height;
int screen[2], black[2], white[2], has_color[2], glasses[2], score[2],
    view[2], shimmering[2], queue_position[2], noclick[2];
float x_queue[2][QUEUE_SIZE], y_queue[2][QUEUE_SIZE];
float player_x[2], player_y[2], cos_anglex[2], sin_anglex[2], anglex[2],
    cos_angley[2], sin_angley[2], angley[2];
float ball_x, ball_y, ball_z, ball_xm, ball_ym, ball_zm, ball_in_play,
    ball_waiting_for, gravity, net, game_mode, spin;
debris_type debris[NUM_DEBRIS];


/* View mode names: */

char *viewnames[6] = { "Normal", "Bleachers", "Above", "FreeView",
    "Follow the Ball", "From The Paddle"
};


/* Local function prototypes: */

void eventloop(void);
void drawline(int pln, float x1, float y1, float z1,
	      float x2, float y2, float z2);
void setup(int argc, char *argv[]);
void Xsetup(int pln);
void putballinplay(int pln);
void recalculatetrig(int i);
float total(float queue[]);
void adddebris(void);
void playsound(char *aufile);
void show_usage(char *arg, FILE * r, int ext);


/* ---- MAIN FUNCTION ---- */

int main(int argc, char *argv[])
{
    /* Program setup: */

    setup(argc, argv);
    randinit();


    /* Connect to the X Server and draw the program's window: */

    Xsetup(0);

    if (game_mode == TWO_PLAYERS)
	Xsetup(1);


    /* Run the main loop: */

    eventloop();
}


/* --- MAIN EVENT LOOP --- */

void eventloop(void)
{
    long time_padding;
    int done, pln, e_loop, i, num_screens, j, crap_x, crap_z, crap_xm,
	crap_zm;
    int old_x[2], old_y[2];
    unsigned int old_button[2];
    char string[128];
    char key[1024];
    struct timeval now, then;
    float x, z, x_change, y_change;


    /* Init debris: */

    for (i = 0; i < NUM_DEBRIS; i++)
	debris[i].exist = 0;
    debriscount = 0;


    /* Init player stuff: */

    for (i = 0; i < 2; i++) {
	old_button[i] = -1;

	player_x[i] = 0;
	player_y[i] = 0;

	shimmering[i] = 0;

	glasses[i] = 0;
	view[i] = 0;

	score[i] = 0;

	anglex[i] = 5;
	angley[i] = 5;
	recalculatetrig(i);

	for (j = 0; j < QUEUE_SIZE; j++) {
	    x_queue[i][j] = 0;
	    y_queue[i][j] = 0;

	    queue_position[i] = 0;
	}
    }


    /* Init. crap: */

    crap_x = X_WIDTH;
    crap_z = Z_DEPTH;
    crap_xm = 1;
    crap_zm = 0;


    /* How many screens to draw on: */

    num_screens = 1;

    if (game_mode == TWO_PLAYERS)
	num_screens = 2;


    /* Init. ball stuff: */

    ball_in_play = 0;
    ball_waiting_for = 0;


    /* Init. high score (for handball): */

    high_score = 0;
    final_score = -1;


    /* MAIN LOOP: */

    done = False;
    toggle = 0;

    do {
	toggle = 1 - toggle;
	gettimeofday(&then, NULL);

	for (e_loop = 0; e_loop < EVENT_LOOP; e_loop++) {
	    for (pln = 0; pln < num_screens; pln++) {
		x_change = 0;
		y_change = 0;


		/* Get and handle events: */

		strcpy(key, "");

		if (1) {
		    SDL_Event sdl_event;
		    while (SDL_PollEvent(&sdl_event)) {
			SDL_Event event = sdl_event;
			if (event.type == SDL_KEYDOWN) {
			    /* Get the key's name: */
			    key[0] = event.key.keysym.sym;
			    key[1] = '\0';

			} else if (event.type == SDL_MOUSEBUTTONDOWN) {
			    /* They clicked!  The beginning of a drag! */

			    old_button[pln] = event.button.button;
			    old_x[pln] = event.button.x;
			    old_y[pln] = event.button.y;


			    /* If the ball wasn't in play, this person
			       launched it: */

			    if (ball_in_play == 0
				&& ball_waiting_for == pln
				&& event.button.button == SDL_BUTTON_RIGHT)
				putballinplay(pln);
			} else if (event.type == SDL_MOUSEBUTTONUP) {
			    /* They released!  No more drag! */

			    old_button[pln] = -1;
			} else if (event.type == SDL_MOUSEMOTION) {
			    if (old_button[pln] == SDL_BUTTON_LEFT
				    || noclick[pln]) {
				x_change = (event.motion.x - old_x[pln]);
				y_change = (event.motion.y - old_y[pln]);


				/* Add their moves to their queue: */

				x_queue[pln][queue_position[pln]] =
				    x_change;
				y_queue[pln][queue_position[pln]] =
				    y_change;

				queue_position[pln] =
				    ((queue_position[pln] +
				      1) % QUEUE_SIZE);


				/* Move Paddle: */

				player_x[pln] = player_x[pln] + x_change;

				if (player_x[pln] <
				    -X_WIDTH + PADDLE_WIDTH)
				    player_x[pln] =
					-X_WIDTH + PADDLE_WIDTH;
				else if (player_x[pln] >=
					 X_WIDTH - PADDLE_WIDTH)
				    player_x[pln] = X_WIDTH - PADDLE_WIDTH;

				player_y[pln] = player_y[pln] + y_change;

				if (player_y[pln] <
				    -Y_HEIGHT + PADDLE_HEIGHT)
				    player_y[pln] =
					-Y_HEIGHT + PADDLE_HEIGHT;
				else if (player_y[pln] >
					 Y_HEIGHT - PADDLE_HEIGHT)
				    player_y[pln] =
					Y_HEIGHT - PADDLE_HEIGHT;

				old_x[pln] = event.motion.x;
				old_y[pln] = event.motion.y;
			    } else if (old_button[pln] == SDL_BUTTON_MIDDLE) {
				/* Rotate view: */

				anglex[pln] = anglex[pln] +
				    event.motion.x - old_x[pln];

				if (anglex[pln] < 0)
				    anglex[pln] = anglex[pln] + 360;

				if (anglex[pln] >= 360)
				    anglex[pln] = anglex[pln] - 360;

				angley[pln] = angley[pln] +
				    event.motion.y - old_y[pln];

				if (angley[pln] < 0)
				    angley[pln] = angley[pln] + 360;

				if (angley[pln] >= 360)
				    angley[pln] = angley[pln] - 360;

				recalculatetrig(pln);

				old_x[pln] = event.motion.x;
				old_y[pln] = event.motion.y;
			    }
			}
		    }
		}


		if (strcasecmp(key, "Q") == 0) {
		    /* Q: Quit: */

		    done = True;
		} else if (strcasecmp(key, "3") == 0) {
		    glasses[pln] = 1 - glasses[pln];
		} else if (strcasecmp(key, "V") == 0) {
		    view[pln] = (view[pln] + 1) % 6;
		} else if (strcasecmp(key, "C") == 0) {
		    noclick[pln] = 1 - noclick[pln];
		}
	    }
	}


	/* Control computer player, if any: */

	if (game_mode == ONE_PLAYER) {
	    /* Remove their "ball hit paddle" effect: */

	    if (shimmering[1] != 0)
		shimmering[1]--;


	    /* Move paddle to follow ball: */

	    if (ball_x > -player_x[1])
		player_x[1] = player_x[1] - (COMPUTER_SPEED + randnum(2));
	    else if (ball_x < -player_x[1])
		player_x[1] = player_x[1] + (COMPUTER_SPEED + randnum(2));

	    if (ball_y < player_y[1])
		player_y[1] = player_y[1] - (COMPUTER_SPEED + randnum(2));
	    else if (ball_y > player_y[1])
		player_y[1] = player_y[1] + (COMPUTER_SPEED + randnum(2));


	    /* Launch ball if it's our serve: */

	    if (ball_in_play == 0 && ball_waiting_for == 1
		&& randnum(10) < 1) {
		putballinplay(1);
	    }
	}


	/* Move debris: */

	for (i = 0; i < NUM_DEBRIS; i++) {
	    if (debris[i].exist != 0) {
		/* Move debris: */

		debris[i].x = debris[i].x + debris[i].xm;
		debris[i].y = debris[i].y + debris[i].ym;
		debris[i].z = debris[i].z + debris[i].zm;


		/* Add effect of gravity: */

		if (game_mode != HANDBALL)
		    debris[i].ym = debris[i].ym + gravity / 2;
		else
		    debris[i].zm = debris[i].zm - gravity / 2;


		/* Count it down, remove if it's old: */

		debris[i].time--;

		if (debris[i].time <= 0)
		    debris[i].exist = 0;
	    }
	}


	/* Move crap: */

	crap_x = crap_x + crap_xm;
	crap_z = crap_z + crap_zm;

	if (crap_x < -X_WIDTH) {
	    crap_x = -X_WIDTH;
	    crap_xm = 0;
	    crap_zm = 1;
	} else if (crap_x > X_WIDTH) {
	    crap_x = X_WIDTH;
	    crap_xm = 0;
	    crap_zm = -1;
	}

	if (crap_z < -Z_DEPTH) {
	    crap_z = -Z_DEPTH;
	    crap_xm = -1;
	    crap_zm = 0;
	} else if (crap_z > Z_DEPTH) {
	    crap_z = Z_DEPTH;
	    crap_xm = 1;
	    crap_zm = 0;
	}


	/* Move ball: */

	if (ball_in_play != 0) {
	    /* Move it left/right: */

	    ball_x = ball_x + ball_xm;

	    if (ball_x < -X_WIDTH + BALL_SIZE) {
		ball_x = -X_WIDTH + BALL_SIZE;
		ball_xm = -ball_xm;
		playsound("wall");
	    } else if (ball_x > X_WIDTH - BALL_SIZE) {
		ball_x = X_WIDTH - BALL_SIZE;
		ball_xm = -ball_xm;
		playsound("wall");
	    }


	    /* Move it up/down: */

	    ball_y = ball_y + ball_ym;

	    if (ball_y < -Y_HEIGHT + BALL_SIZE) {
		ball_y = -Y_HEIGHT + BALL_SIZE;
		ball_ym = -ball_ym;
		playsound("wall");
	    } else if (ball_y > Y_HEIGHT - BALL_SIZE) {
		ball_y = Y_HEIGHT - BALL_SIZE;
		ball_ym = -ball_ym;
		playsound("wall");
	    }


	    /* Add the effect of gravity: */

	    if (game_mode != HANDBALL) {
		ball_ym = ball_ym + gravity;
	    } else {
		ball_zm = ball_zm - gravity;
	    }


	    /* Move it in/out: */

	    ball_z = ball_z + ball_zm;


	    /* It's at a goal!: */

	    if (ball_z < -Z_DEPTH + BALL_SIZE) {
		if (ball_x + BALL_SIZE >= player_x[0] - PADDLE_WIDTH &&
		    ball_x - BALL_SIZE <= player_x[0] + PADDLE_WIDTH &&
		    ball_y + BALL_SIZE >= player_y[0] - PADDLE_HEIGHT
		    && ball_y - BALL_SIZE <= player_y[0] + PADDLE_HEIGHT) {
		    /* They hit it!  Bounce! */

		    adddebris();
		    playsound("hit");

		    ball_speed = ball_speed + 1;
		    ball_z = -Z_DEPTH + BALL_SIZE;
		    ball_zm = randnum(ball_speed / 2) + ball_speed / 2;

		    adddebris();

		    if (spin == 0) {
			ball_xm = (ball_x - player_x[0]) / ANGLE_DIVIDE;
			ball_ym = (ball_y - player_y[0]) / ANGLE_DIVIDE;
		    } else if (spin == 1) {
			ball_xm = total(x_queue[0]);
			ball_ym = total(y_queue[0]);
		    }

		    shimmering[0] = SHIMMER_TIME;


		    /* A hit in handball mode means score: */

		    if (game_mode == HANDBALL) {
			score[0]++;
			final_score = score[0];

			if (score[0] > high_score)
			    high_score = score[0];
		    }
		} else if (ball_z <= -Z_DEPTH) {
		    if (game_mode != HANDBALL) {
			/* They missed it!  Score to player 2! */

			playsound("score");
			ball_in_play = 0;
			ball_waiting_for = 1;
			score[1]++;
		    } else {
			/* They missed!  Game over! */

			final_score = score[0];

			if (final_score >= high_score)
			    got_high_score = 1;
			else
			    got_high_score = 0;

			ball_in_play = 0;
			ball_waiting_for = 0;
			score[0] = 0;
		    }
		}
	    } else if (ball_z > Z_DEPTH - BALL_SIZE) {
		if (game_mode != HANDBALL) {
		    if (ball_x + BALL_SIZE >=
			-player_x[1] - PADDLE_WIDTH
			&& ball_x - BALL_SIZE <=
			-player_x[1] + PADDLE_WIDTH
			&& ball_y + BALL_SIZE >=
			player_y[1] - PADDLE_HEIGHT
			&& ball_y - BALL_SIZE <=
			player_y[1] + PADDLE_HEIGHT) {
			/* They hit it!  Bounce! */

			adddebris();
			playsound("hit");

			ball_speed = ball_speed + 1;
			ball_z = Z_DEPTH - BALL_SIZE;
			ball_zm =
			    -randnum(ball_speed / 2) - ball_speed / 2;

			adddebris();

			if (spin == 0) {
			    ball_xm =
				(ball_x - player_x[1]) / ANGLE_DIVIDE;
			    ball_ym =
				(ball_y - player_y[1]) / ANGLE_DIVIDE;
			} else if (spin == 1) {
			    ball_xm = total(x_queue[1]);
			    ball_ym = total(y_queue[1]);
			}

			shimmering[1] = SHIMMER_TIME;
		    } else if (ball_z >= Z_DEPTH) {
			/* They missed it!  Score to player 1! */

			playsound("score");
			ball_in_play = 0;
			ball_waiting_for = 0;
			score[0]++;
		    }
		} else {
		    ball_z = Z_DEPTH - BALL_SIZE;
		    ball_zm = -ball_zm;
		    playsound("wall");
		}
	    }


	    /* Bounce ball off net: */

	    if (net_height != 0) {
		if (ball_z >= -BALL_SIZE && ball_z <= BALL_SIZE &&
		    ball_y >= net_height - BALL_SIZE) {
		    playsound("wall");
		    ball_zm = -ball_zm;
		    ball_z = ball_z + ball_zm;
		}
	    }
	}


	for (pln = 0; pln < num_screens; pln++) {
	    /* Draw screen: */

	    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	    SDL_RenderClear(renderer);


	    /* Draw game arena: */

	    SDL_SetRenderDrawColor(renderer, 255, 255, 255,
				   SDL_ALPHA_OPAQUE);
	    drawline(pln, -X_WIDTH, -Y_HEIGHT, -Z_DEPTH, -X_WIDTH,
		     -Y_HEIGHT, Z_DEPTH );
	    drawline(pln, X_WIDTH, -Y_HEIGHT, -Z_DEPTH, X_WIDTH,
		     -Y_HEIGHT, Z_DEPTH);
	    drawline(pln, -X_WIDTH, Y_HEIGHT, -Z_DEPTH, -X_WIDTH,
		     Y_HEIGHT, Z_DEPTH);
	    drawline(pln, X_WIDTH, Y_HEIGHT, -Z_DEPTH, X_WIDTH,
		     Y_HEIGHT, Z_DEPTH);

	    drawline(pln, -X_WIDTH, -Y_HEIGHT, -Z_DEPTH,
		     X_WIDTH, -Y_HEIGHT, -Z_DEPTH);
	    drawline(pln, X_WIDTH, -Y_HEIGHT, -Z_DEPTH,
		     X_WIDTH, Y_HEIGHT, -Z_DEPTH);
	    drawline(pln, X_WIDTH, Y_HEIGHT, -Z_DEPTH,
		     -X_WIDTH, Y_HEIGHT, -Z_DEPTH);
	    drawline(pln, -X_WIDTH, Y_HEIGHT, -Z_DEPTH,
		     -X_WIDTH, -Y_HEIGHT, -Z_DEPTH);

	    drawline(pln, -X_WIDTH, -Y_HEIGHT, Z_DEPTH,
		     X_WIDTH, -Y_HEIGHT, Z_DEPTH);
	    drawline(pln, X_WIDTH, -Y_HEIGHT, Z_DEPTH,
		     X_WIDTH, Y_HEIGHT, Z_DEPTH);
	    drawline(pln, X_WIDTH, Y_HEIGHT, Z_DEPTH,
		     -X_WIDTH, Y_HEIGHT, Z_DEPTH);
	    drawline(pln, -X_WIDTH, Y_HEIGHT, Z_DEPTH,
		     -X_WIDTH, -Y_HEIGHT, Z_DEPTH);


	    if (game_mode != HANDBALL) {
		/* Draw floor marker: */

		drawline(pln, -X_WIDTH, Y_HEIGHT, 0,
			 X_WIDTH, Y_HEIGHT, 0);


		/* Draw net, if any: */

		if (net != 0.0) {
		    drawline(pln, -X_WIDTH, net_height, 0,
			     X_WIDTH, net_height, 0);
		    drawline(pln, X_WIDTH, Y_HEIGHT, 0,
			     X_WIDTH, net_height, 0);
		    drawline(pln, -X_WIDTH, net_height, 0,
			     -X_WIDTH, Y_HEIGHT, 0);
		}
	    }


	    /* Draw crap on walls: */

	    /* drawline(pln, crap_x, -Y_HEIGHT, crap_z,
	       crap_x, -Y_HEIGHT - 10, crap_z); */


	    SDL_SetRenderDrawColor(renderer, 50, 50, 150,
				   SDL_ALPHA_OPAQUE);
	    if (game_mode != HANDBALL) {
		/* Draw opponent: */

		drawline(pln,
			 -player_x[1 - pln] + PADDLE_WIDTH,
			 player_y[1 - pln] - PADDLE_HEIGHT, Z_DEPTH,
			 -player_x[1 - pln] - PADDLE_WIDTH,
			 player_y[1 - pln] - PADDLE_HEIGHT, Z_DEPTH);

		drawline(pln,
			 -player_x[1 - pln] - PADDLE_WIDTH,
			 player_y[1 - pln] - PADDLE_HEIGHT, Z_DEPTH,
			 -player_x[1 - pln] - PADDLE_WIDTH,
			 player_y[1 - pln] + PADDLE_HEIGHT, Z_DEPTH);
		drawline(pln,
			 -player_x[1 - pln] - PADDLE_WIDTH,
			 player_y[1 - pln] + PADDLE_HEIGHT, Z_DEPTH,
			 -player_x[1 - pln] + PADDLE_WIDTH,
			 player_y[1 - pln] + PADDLE_HEIGHT, Z_DEPTH);

		drawline(pln,
			 -player_x[1 - pln] + PADDLE_WIDTH,
			 player_y[1 - pln] + PADDLE_HEIGHT, Z_DEPTH,
			 -player_x[1 - pln] + PADDLE_WIDTH,
			 player_y[1 - pln] - PADDLE_HEIGHT, Z_DEPTH);


		/* Draw "paddle hit the ball" effect: */

		if (shimmering[1 - pln] != 0) {
		    drawline(pln,
			     -player_x[1 - pln] - PADDLE_WIDTH,
			     player_y[1 - pln] - PADDLE_HEIGHT,
			     Z_DEPTH,
			     -player_x[1 - pln] + PADDLE_WIDTH,
			     player_y[1 - pln] + PADDLE_HEIGHT,
			     Z_DEPTH);

		    drawline(pln,
			     -player_x[1 - pln] + PADDLE_WIDTH,
			     player_y[1 - pln] - PADDLE_HEIGHT,
			     Z_DEPTH,
			     -player_x[1 - pln] - PADDLE_WIDTH,
			     player_y[1 - pln] + PADDLE_HEIGHT,
			     Z_DEPTH);
		}
	    }


	    if (ball_in_play == 1) {
		/* Draw ball: */
		SDL_SetRenderDrawColor(renderer, 10, 200, 10,
				       SDL_ALPHA_OPAQUE);

		if (pln == 0) {
		    x = ball_x;
		    z = ball_z;
		} else {
		    x = -ball_x;
		    z = -ball_z;
		}

		drawline(pln,
			 x - BALL_SIZE, ball_y, z - BALL_SIZE,
			 x + BALL_SIZE, ball_y, z - BALL_SIZE);
		drawline(pln,
			 x + BALL_SIZE, ball_y, z - BALL_SIZE,
			 x + BALL_SIZE, ball_y, z + BALL_SIZE);
		drawline(pln,
			 x + BALL_SIZE, ball_y, z + BALL_SIZE,
			 x - BALL_SIZE, ball_y, z + BALL_SIZE);
		drawline(pln,
			 x - BALL_SIZE, ball_y, z + BALL_SIZE,
			 x - BALL_SIZE, ball_y, z - BALL_SIZE);

		drawline(pln,
			 x - BALL_SIZE, ball_y, z - BALL_SIZE,
			 x, ball_y - BALL_SIZE, z);
		drawline(pln,
			 x + BALL_SIZE, ball_y, z - BALL_SIZE,
			 x, ball_y - BALL_SIZE, z);
		drawline(pln,
			 x + BALL_SIZE, ball_y, z + BALL_SIZE,
			 x, ball_y - BALL_SIZE, z);
		drawline(pln,
			 x - BALL_SIZE, ball_y, z + BALL_SIZE,
			 x, ball_y - BALL_SIZE, z);

		drawline(pln,
			 x - BALL_SIZE, ball_y, z - BALL_SIZE,
			 x, ball_y + BALL_SIZE, z);
		drawline(pln,
			 x + BALL_SIZE, ball_y, z - BALL_SIZE,
			 x, ball_y + BALL_SIZE, z);
		drawline(pln,
			 x + BALL_SIZE, ball_y, z + BALL_SIZE,
			 x, ball_y + BALL_SIZE, z);
		drawline(pln,
			 x - BALL_SIZE, ball_y, z + BALL_SIZE,
			 x, ball_y + BALL_SIZE, z);


		/* Draw ball markers: */
		SDL_SetRenderDrawColor(renderer, 50, 150, 50,
				       SDL_ALPHA_OPAQUE);

		drawline(pln,
			 x - BALL_SIZE, -Y_HEIGHT, z,
			 x + BALL_SIZE, -Y_HEIGHT, z);

		drawline(pln,
			 -X_WIDTH, ball_y - BALL_SIZE, z,
			 -X_WIDTH, ball_y + BALL_SIZE, z);
	    } else {
		/* Ball isn't in play, waiting for someone... */

		if (ball_waiting_for == pln)
		    strcpy(string, "Your serve!");
		else
		    sprintf(string, "Player %d's serve", (1 - pln) + 1);

		sdl_drawtext(renderer, white_color, 50, HEIGHT / 2 - font_height, string);


		/* Show final score (handball): */

		if (game_mode == HANDBALL) {
		    /* Only show it if they've actually played a round yet: */

		    if (final_score != -1) {
			sprintf(string, "Final score: %d", final_score);

			sdl_drawtext(renderer, white_color,
			       	50, HEIGHT/2+font_height,string);
		    }


		    /* Show "got high score" if they got it (handball): */

		    if (got_high_score == 1)
			sdl_drawtext(renderer, white_color,
				50, HEIGHT/2 + font_height * 2,
				"You beat the high score!");
		}
	    }


	    /* Draw you: */
	    SDL_SetRenderDrawColor(renderer, 200, 10, 10,
				   SDL_ALPHA_OPAQUE);

	    drawline(pln,
		     player_x[pln] - PADDLE_WIDTH,
		     player_y[pln] - PADDLE_HEIGHT, -Z_DEPTH,
		     player_x[pln] + PADDLE_WIDTH,
		     player_y[pln] - PADDLE_HEIGHT, -Z_DEPTH);

	    drawline(pln,
		     player_x[pln] + PADDLE_WIDTH,
		     player_y[pln] - PADDLE_HEIGHT, -Z_DEPTH,
		     player_x[pln] + PADDLE_WIDTH,
		     player_y[pln] + PADDLE_HEIGHT, -Z_DEPTH);

	    drawline(pln,
		     player_x[pln] + PADDLE_WIDTH,
		     player_y[pln] + PADDLE_HEIGHT, -Z_DEPTH,
		     player_x[pln] - PADDLE_WIDTH,
		     player_y[pln] + PADDLE_HEIGHT, -Z_DEPTH);

	    drawline(pln,
		     player_x[pln] - PADDLE_WIDTH,
		     player_y[pln] + PADDLE_HEIGHT, -Z_DEPTH,
		     player_x[pln] - PADDLE_WIDTH,
		     player_y[pln] - PADDLE_HEIGHT, -Z_DEPTH);


	    /* Draw "paddle hit the ball" effect: */

	    if (shimmering[pln] != 0) {
		shimmering[pln]--;

		drawline(pln,
			 player_x[pln] - PADDLE_WIDTH,
			 player_y[pln] - PADDLE_HEIGHT, -Z_DEPTH,
			 player_x[pln] + PADDLE_WIDTH,
			 player_y[pln] + PADDLE_HEIGHT, -Z_DEPTH);

		drawline(pln,
			 player_x[pln] + PADDLE_WIDTH,
			 player_y[pln] - PADDLE_HEIGHT, -Z_DEPTH,
			 player_x[pln] - PADDLE_WIDTH,
			 player_y[pln] + PADDLE_HEIGHT, -Z_DEPTH);
	    }


	    /* Draw debris: */

	    for (i = 0; i < NUM_DEBRIS; i++) {
		if (debris[i].exist != 0) {
		    int n = randnum(3);
		    int r = 0, g = 0, b = 0;
		    switch (n) {
		    case 0:
			r = 255;
			break;
		    case 1:
			g = 255;
			break;
		    case 2:
			b = 255;
			break;
		    }
		    SDL_SetRenderDrawColor(renderer, r, g, b,
					   SDL_ALPHA_OPAQUE);
		    drawline(pln, debris[i].x, debris[i].y,
			     debris[i].z, debris[i].x + debris[i].xm,
			     debris[i].y + debris[i].ym,
			     debris[i].z + debris[i].zm);
		}
	    }


	    /* Draw view mode: */

	    sdl_drawtext(renderer, sdl_colors[2], 
		    0, HEIGHT - font_height * 2, viewnames[view[pln]]);

	    if (view[pln] == 3)
	    {
		sdl_drawtext(renderer, sdl_colors[5],
			0, HEIGHT - font_height,
			"Middle-Click and drag to change view");
	    }


	    /* Draw scores: */

	    if (game_mode != HANDBALL) {
		/* Player 1 and 2 scores: */

		for (i = 0; i < 2; i++) {
		    sprintf(string, "Player %d: %d", i + 1, score[i]);

		    sdl_drawtext(renderer, sdl_colors[i],
			    0, font_height * (i), string);
		}
	    } else {
		/* Score and high score for handball: */

		sprintf(string, "Score: %d", score[0]);

		sdl_drawtext(renderer, sdl_colors[0],
			0, 0, string);

		sprintf(string, "High:  %d", high_score);

		sdl_drawtext(renderer, sdl_colors[0],
			0, font_height, string);
	    }


	    /* Update the window: */

	    if (DOUBLE_BUFFER == True) {
	    }

	    SDL_RenderPresent(renderer);
	}


	/* Keep framerate exact: */

	gettimeofday(&now, NULL);

	time_padding =
	    FRAMERATE - ((now.tv_sec - then.tv_sec) * 1000000 +
			 (now.tv_usec - then.tv_usec));

	if (time_padding > 0)
	    usleep(time_padding);
    }
    while (done == False);
}


/* Draw a line in 3D space: */

void drawline(int pln, float x1, float y1, float z1,
	      float x2, float y2, float z2)
{
    int i;
    float sx1, sy1, sx2, sy2, xoff, xx1, yy1, zz1, xx2, yy2, zz2, sinx,
	cosx;
    double anglex;


    for (i = 0; i < glasses[pln] + 1; i++) {
	if (glasses[pln] == 0)
	    xoff = 0;
	else if (glasses[pln] == 1) {
	    xoff = GLASS_OFFSET;

	    if (i == 0)
		xoff = -xoff;

	    if (toggle == 0)
		xoff = -xoff;
	}


	/* Alter perceived x/y/z depending on their view: */

	if (view[pln] == 0) {
	    /* Normal (behind your paddle): */

	    xx1 = x1;
	    yy1 = y1;
	    zz1 = z1;

	    xx2 = x2;
	    yy2 = y2;
	    zz2 = z2;
	} else if (view[pln] == 1) {
	    /* From the side: */

	    xx1 = z1;
	    yy1 = y1;
	    zz1 = x1;

	    xx2 = z2;
	    yy2 = y2;
	    zz2 = x2;
	} else if (view[pln] == 2) {
	    /* From above: */

	    xx1 = x1;
	    yy1 = z1;
	    zz1 = y1;

	    xx2 = x2;
	    yy2 = z2;
	    zz2 = y2;
	} else if (view[pln] == 3) {
	    /* Free view: */

	    xx1 = x1 * cos_anglex[pln] - z1 * sin_anglex[pln];
	    zz1 = x1 * sin_anglex[pln] + z1 * cos_anglex[pln];

	    yy1 = y1 * cos_angley[pln] - zz1 * sin_angley[pln];
	    zz1 = y1 * sin_angley[pln] + zz1 * cos_angley[pln];


	    xx2 = x2 * cos_anglex[pln] - z2 * sin_anglex[pln];
	    zz2 = x2 * sin_anglex[pln] + z2 * cos_anglex[pln];

	    yy2 = y2 * cos_angley[pln] - zz2 * sin_angley[pln];
	    zz2 = y2 * sin_angley[pln] + zz2 * cos_angley[pln];
	} else if (view[pln] == 4) {
	    /* Watch the ball: */

	    anglex = (ball_z - Z_DEPTH) / 10;
	    if (anglex > 90)
		anglex = 90;
	    else if (anglex < -90)
		anglex = -90;

	    anglex = (anglex / 180.0) * M_PI;
	    sinx = sin(anglex);
	    cosx = cos(anglex);

	    xx1 = x1 - (ball_x / 5);
	    yy1 = (z1 - ball_z) * sinx + (y1 - ball_y) * cosx;
	    zz1 = (z1 - ball_z) * cosx - (y1 - ball_y) * sinx + 30;

	    xx2 = x2 - (ball_x / 5);
	    yy2 = (z2 - ball_z) * sinx + (y2 - ball_y) * cosx;
	    zz2 = (z2 - ball_z) * cosx - (y2 - ball_y) * sinx + 30;
	} else if (view[pln] == 5) {
	    /* From your paddle: */

	    xx1 = x1 - player_x[pln];
	    yy1 = y1 - player_y[pln];
	    zz1 = z1;

	    xx2 = x2 - player_x[pln];
	    yy2 = y2 - player_y[pln];
	    zz2 = z2;
	}


	if (zz1 > -DISTANCE && zz2 > -DISTANCE) {
	    /* Convert (x,y,z) into (x,y) with a 3D look: */

	    sx1 = (xx1 + xoff) / ((zz1 + DISTANCE) / ASPECT);
	    sy1 = yy1 / ((zz1 + DISTANCE) / ASPECT);

	    sx2 = (xx2 + xoff) / ((zz2 + DISTANCE) / ASPECT);
	    sy2 = yy2 / ((zz2 + DISTANCE) / ASPECT);


	    /* Transpose (0,0) origin to the center of the window: */

	    sx1 = sx1 + WIDTH / 2;
	    sy1 = sy1 + HEIGHT / 2;

	    sx2 = sx2 + WIDTH / 2;
	    sy2 = sy2 + HEIGHT / 2;


	    /* Draw the line into the window: */

	    if (glasses[pln] == 0) {
		SDL_RenderDrawLineF(renderer, sx1, sy1, sx2, sy2);
	    } else {
		if ((i == 0 && toggle == 1) || (i == 1 && toggle == 0))
		{
		    SDL_RenderDrawLineF(renderer, 
			    sx1, sy1, sx2, sy2);
		}
		else
		{
		    SDL_RenderDrawLineF(renderer,
			    sx1, sy1 + 1, sx2, sy2 + 1);
		}
	    }
	}
    }
}


/* Program set-up (check usage, load data, etc.): */

void setup(int argc, char *argv[])
{
    FILE *fi;
    char temp[512], color[512];
    int which_end;
    float x, y, z;


    /* Get --noclick switches: */

    noclick[1] = 0;

    if (argc > 2) {
	if (strcmp(argv[argc - 1], "--noclick2") == 0 ||
	    strcmp(argv[argc - 1], "-nc2") == 0) {
	    printf("Player 2 set for 'noclick' mode\n");

	    noclick[1] = 1;

	    argc = argc - 1;
	}
    }

    noclick[0] = 0;

    if (argc > 2) {
	if (strcmp(argv[argc - 1], "--noclick1") == 0 ||
	    strcmp(argv[argc - 1], "-nc1") == 0) {
	    printf("Player 1 set for 'noclick' mode\n");

	    noclick[0] = 1;

	    argc = argc - 1;
	}
    }


    /* Get --sound switch: */

    use_sound = 0;

    if (argc > 2) {
	if (strcmp(argv[argc - 1], "--sound") == 0 ||
	    strcmp(argv[argc - 1], "-s") == 0) {
	    use_sound = 1;

	    argc = argc - 1;
	}
    }


    /* Get --spin switch: */

    spin = 0;

    if (argc > 2) {
	if (strcmp(argv[argc - 1], "--spin") == 0) {
	    spin = 1;

	    argc = argc - 1;
	}
    }


    /* Get --net switch: */

    net = 0;
    net_height = 0;

    if (argc > 3) {
	if (strcmp(argv[argc - 2], "--net") == 0 ||
	    strcmp(argv[argc - 2], "-n") == 0) {
	    net = atof(argv[argc - 1]);

	    if (net < 0.0)
		net = -net;

	    if (net > 0.75)
		net = 0.75;

	    argc = argc - 2;
	}

	net_height = Y_HEIGHT * (1.0 - net) / 2;
    }


    /* Get --gravity switch: */

    gravity = 0.0;

    if (argc > 3) {
	if (strcmp(argv[argc - 2], "--gravity") == 0 ||
	    strcmp(argv[argc - 2], "-g") == 0) {
	    gravity = atof(argv[argc - 1]);

	    argc = argc - 2;
	}
    }


    /* Check for "--version": */

    if (argc == 2) {
	if (strcmp(argv[1], "--version") == 0 ||
	    strcmp(argv[1], "-v") == 0) {
	    printf("\n3dpong version 0.5\n\n");
	    printf("by Bill Kendrick\n");
	    printf("bill@newbreedsoftware.com");
	    printf("http://www.newbreedsoftware.com/3dpong/\n\n");
	    exit(0);
	}
    }


    /* Check for "--help": */

    if (argc == 2) {
	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
	    show_usage(argv[0], stdout, 0);
	}
    }


    /* Check proper usage; get program arguments: */

    if (argc == 2) {
	game_mode = HANDBALL;


	/* Keep gravity fair: */

	if (gravity < 0.0)
	    gravity = -gravity;


	/* Make sure there IS gravity: */

	if (gravity < MIN_HANDBALL_GRAVITY)
	    gravity = MIN_HANDBALL_GRAVITY;
    } else if (argc == 3) {

	if (strcmp(argv[2], "computer") == 0 ||
	    strcmp(argv[2], "--computer") == 0) {
	    game_mode = ONE_PLAYER;
	} else {
	    game_mode = TWO_PLAYERS;
	}
    } else {
	show_usage(argv[0], stderr, 1);
    }
}


/* Setup the application: */

void Xsetup(int pln)
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    sdl_font = TTF_OpenFont("fonts/a.ttf",font_height);
    if (sdl_font == NULL)
    {
	printf("TTF_OpenFont Error: %s", SDL_GetError());
    }
    sdl_window = SDL_CreateWindow("3dpong", 100, 100, WIDTH, HEIGHT, 0);
    renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    SDL_RenderPresent(renderer);
}





/* Puts ball in play (direction depends on who launched it): */

void putballinplay(int pln)
{
    /* Remember that the ball is now in play: */

    ball_in_play = 1;


    /* Pick a random starting position: */

    ball_x = player_x[pln];
    ball_y = player_y[pln];
    ball_z = Z_DEPTH / 2;;

    if (pln == 0) {
	ball_x = -ball_x;
	ball_z = -ball_z;
    }


    /* Give it a random speed/direction: */

    ball_speed = BALL_SPEED;
    ball_xm = randnum(ball_speed * 2) - ball_speed;
    ball_ym = randnum(ball_speed * 2) - ball_speed;
    do {
	ball_zm = randnum(BALL_SPEED * 3) / 2;
    }
    while (ball_zm == 0);

    if (pln == 1)
	ball_zm = -ball_zm;
}


/* Recalculate the player's trig. values: */

void recalculatetrig(int i)
{
    cos_anglex[i] = cos(M_PI * anglex[i] / 180.0);
    sin_anglex[i] = sin(M_PI * anglex[i] / 180.0);

    cos_angley[i] = cos(M_PI * angley[i] / 180.0);
    sin_angley[i] = sin(M_PI * angley[i] / 180.0);
}


/* Returns the total of a queue (for spin mode): */

float total(float queue[])
{
    int i;
    float val;

    for (i = 0; i < QUEUE_SIZE; i++)
	val = val + queue[i];

    val = val / QUEUE_SIZE;

    return (val);
}


/* Add sparkly stuff to game (when ball hits, etc.): */

void adddebris(void)
{
    int i, num_to_add;


    /* Pick how many to add: */

    num_to_add = randnum(DEBRIS_MAX - DEBRIS_MIN) + DEBRIS_MIN;

    for (i = 0; i < num_to_add; i++) {
	/* Make it exist: */

	debris[debriscount].exist = 1;
	debris[debriscount].time = randnum(DEBRIS_TIME);


	/* Give it a position: */

	debris[debriscount].x = ball_x + randnum(10) - 5;
	debris[debriscount].y = ball_y + randnum(10) - 5;
	debris[debriscount].z = ball_z + randnum(10) - 5;


	/* Give it a speed/direction: */

	debris[debriscount].xm =
	    (randnum(DEBRIS_SPEED * 2) - DEBRIS_SPEED + ball_xm / 2);
	debris[debriscount].ym =
	    (randnum(DEBRIS_SPEED * 2) - DEBRIS_SPEED + ball_ym / 2);
	debris[debriscount].zm =
	    (randnum(DEBRIS_SPEED * 2) - DEBRIS_SPEED + ball_zm / 2);


	/* Increment the debris counter: */

	debriscount = (debriscount + 1) % NUM_DEBRIS;
    }
}


/* Play a sound (if we can): */

void playsound(char *aufile)
{
    char cmd[1024];

    if (use_sound == 1) {
	sprintf(cmd, "vlc sounds/%s.au &", aufile);

	system(cmd);
    }
}


void show_usage(char *arg, FILE * r, int ext)
{
    fprintf(r, "\n");
    fprintf(r, "Usage: %s ", arg);
    fprintf(r, "display1 [display2 | --computer] [--gravity value]\n");
    fprintf(r, "       [--net value] [--sound] [--nockick1] ");
    fprintf(r, "[--noclick2]\n");
    fprintf(r, "\n");
    fprintf(r, "Controls:\n");
    fprintf(r, "  [V] - Change view\n");
    fprintf(r, "  [3] - Toggle 3D glasses mode\n");
    fprintf(r, "  [C] - Toggle \"noclick\" mode\n");
    fprintf(r, "  [Q] - Quit\n");
    fprintf(r, "\n");

    exit(ext);
}
