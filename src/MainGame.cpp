#include <cmath>

#include "Global.h"

#include <iostream>
#include "MainGame.h"
#include "Menu.h"
#include "alBitmap.h"
#include "Random.h"
#include "Audio.h"
#include "partypack/vector2d.h"
#include "partypack/sprite_animator.h"
#include "partypack/bitmap.h"
#include "partypack/gamepad.h"

using namespace std;

static void switch_in() {
	gSys->maingame->paused = false;
}
static void switch_out() {
	gSys->maingame->paused = true;
}

MainGame::MainGame() {
	set_display_switch_callback(SWITCH_IN, switch_in);
	set_display_switch_callback(SWITCH_OUT, switch_out);

	console = gSys->rm.load<Bitmap>("data/interface.png")->getABitmap();
	tiles = gSys->rm.load<Bitmap>("data/roads.png")->getABitmap();

	playerimg = gSys->rm.load<Bitmap>("data/player.png")->getABitmap();
	playerptr = gSys->rm.load<Bitmap>("data/playerptr.png")->getABitmap();

	police = gSys->rm.load<Bitmap>("data/police.png")->getABitmap();
	policeptr = gSys->rm.load<Bitmap>("data/policeptr.png")->getABitmap();

	firetruck = gSys->rm.load<Bitmap>("data/firetruck.png")->getABitmap();
	fireptr = gSys->rm.load<Bitmap>("data/firetruckptr.png")->getABitmap();

	mask = gSys->rm.load<Bitmap>("data/mask.png")->getABitmap();

	heart = gSys->rm.load<Bitmap>("data/heart.png")->getABitmap();
	credits = gSys->rm.load<Bitmap>("data/credits.png")->getABitmap();
	gameover = gSys->rm.load<Bitmap>("data/gameover.png")->getABitmap();

	buildings[0] = gSys->rm.load<Bitmap>("data/rubble.png")->getABitmap();
	buildings[1] = gSys->rm.load<Bitmap>("data/construction0.png")->getABitmap();
	buildings[2] = gSys->rm.load<Bitmap>("data/building0.png")->getABitmap();
	buildings[3] = gSys->rm.load<Bitmap>("data/building1.png")->getABitmap();
	buildings[4] = gSys->rm.load<Bitmap>("data/building2.png")->getABitmap();
	buildings[5] = gSys->rm.load<Bitmap>("data/building3.png")->getABitmap();

	firefx = gSys->rm.load<Audio>("data/fire.wav")->getSAMPLE();
	sirenfx = gSys->rm.load<Audio>("data/siren.wav")->getSAMPLE();

	for (int i=0; i<BUILDING_COUNT; i++) {
		buildings2[i] = buildings[i];
		int color = buildings2[i].getMaskColor();
		for (int y=0; y<buildings2[i].getHeight(); y++)
			for (int x=0; x<buildings2[i].getWidth(); x++)
				if ((x&1)^(y&1))
					buildings2[i].setPixel(x,y,color);
	}

	gameCanvas.create(200,300);
	pausescreen.create(200,300);
	pausescreen.clear(pausescreen.getMaskColor());
	for (int y=0; y<pausescreen.getHeight(); y++)
		for (int x=0; x<pausescreen.getWidth(); x++)
			if ((x&1)^(y&1))
				pausescreen.setPixel(x,y,gSys->colors[4]);

	city = 0;

}
MainGame::~MainGame() {
	delete city;
}

float constrain(float x, float min, float max) {
	if (x<min) return min;
	if (x>max) return max;
	return x;
}
float random( float min, float max ) {
   return min + float(rand())/float(RAND_MAX) * (max-min);
}

void MainGame::setup() {
	srand((unsigned)::time(NULL));
	score = 0;
	time = 0;
	hearts = 3;
	delete city;
	city = new LevelMap(this,50,100);
	canPlay = true;
	paused = false;

	player.game = this;
	player.mx = 5 + rand() % (city->width - 10);
	player.my = 10 + rand() % (city->height - 20);
	player.lastDx = 0;
	player.lastDy = 0;
	player.moveRepeatTimer = 0;
	player.selectAnimTick = 0;
	player.syncPosFromGrid();
	player.startpos = player.pos;

	city->makeCenter(player.mx,player.my);

	viewport.x = constrain(player.pos.x - 100.0f, -5.0f, (float)(city->width * 40 - 195));
	viewport.y = constrain(player.pos.y - 150.0f, -30.0f, (float)(city->height * 10 - 270));

	for (vector<GameObject*>::iterator ii = sprites.begin(); ii != sprites.end(); ii++)
		if ((*ii) != &player)
			delete (*ii);

	sprites.clear();
	sprites.push_back(&player);
	for (int i=0; i<4; i++) {
		sprites.push_back(new Police(this));
		sprites.push_back(new FireTruck(this));
	}
}

bool MainGame::isAtSelectedSquare(int vmx, int vmy, const vector2f &vpos) const {
	if (!city || !canPlay) return false;

	// Direct match: Police or FireTruck is on the exact selected square
	if (vmx == player.mx && vmy == player.my) {
		return true;
	}

	// If the currently selected square is a building/non-road square (which road vehicles
	// cannot drive onto directly), check if the Police or FireTruck has reached the road
	// square directly bordering the selected building square.
	if (city->getCellType(player.mx, player.my) != 1) {
		bool alt = (vmy % 2) == 1;
		bool isAdjacentSquare =
			(player.mx == vmx - (alt ? 0 : 1) && player.my == vmy - 1) ||
			(player.mx == vmx + (alt ? 1 : 0) && player.my == vmy - 1) ||
			(player.mx == vmx - (alt ? 0 : 1) && player.my == vmy + 1) ||
			(player.mx == vmx + (alt ? 1 : 0) && player.my == vmy + 1);

		if (isAdjacentSquare) {
			float selCenterX = (float)(player.mx * 40 + ((player.my % 2) ? 20 : 0) + 19);
			float selCenterY = (float)(player.my * 10 + 10);
			if (fabs(vpos.x - selCenterX) <= 22.0f && fabs(vpos.y - selCenterY) <= 12.0f) {
				return true;
			}
		}
	}

	return false;
}

void MainGame::catchPlayer() {
	if (!canPlay) return;
	hearts = 0;
	canPlay = false;
	time = 0;
	play_sample(sirenfx, 200, 128, 1000, 0);
}

bool MainGame::isSelectedSquareBurning() const {
	if (!city) return false;
	Cell *c = city->getCell(player.mx, player.my);
	return c && (c->fire != 0);
}

void MainGame::moveSelectionToVehicle(int index) {
	if (index >= 1 && index < (int)sprites.size()) {
		sprites[index]->updatePos();
		player.mx = sprites[index]->mx;
		player.my = sprites[index]->my;
		player.syncPosFromGrid();
	}
}


void MainGame::update() {


	if (!paused && canPlay) {

		if (hearts<=0) {
			canPlay = false;
			time = 0;
		}

		city->update();

		for (vector<GameObject*>::iterator ii = sprites.begin(); ii != sprites.end(); ii++) {
			(*ii)->update();
			(*ii)->updatePos();
		}

		viewport.x += (constrain(player.pos.x-100, -5, city->width*40 - 195)-viewport.x)/6.0f;
		viewport.y += (constrain(player.pos.y-150, -30, city->height*10 - 270)-viewport.y)/6.0f;

		time++;

		if (time%1000==0) {
			sprites.push_back(new Police(this));
			sprites.push_back(new FireTruck(this));
		}
	}
	if (!canPlay)
		time++;
	if (canPlay) {
		static int pressp = 0;
		if (gSys->input->isDown(START)) {
			if (!pressp) paused = !paused;
			pressp = 1;
		} else
			pressp = 0;
	} else {
		if (gSys->input->isDown(START))
			gSys->returnToTitle();
	}
	static int presss = 0;
	if (gSys->input->isDown(SELECT)) {
		if (!presss) gSys->switchMusic();
		presss = 1;
	} else
		presss = 0;



}

void MainGame::draw(alBitmap &targetBuffer) {
	alBitmap &buffer = gameCanvas;
	buffer.clear(gSys->colors[5]);

	if (canPlay) {

		city->draw(buffer);

		for (vector<GameObject*>::iterator ii = sprites.begin(); ii != sprites.end(); ii++)
			(*ii)->draw(buffer,true);

		if (paused) {
			buffer.draw(pausescreen);
			city->drawMap(buffer);
		}

		buffer.draw(console);

		for (int i=0; i<hearts; i++) {
			buffer.draw(heart, 24+17*i, 280);
		}

	} else {
		buffer.draw(gameover);
		int y = (time-100)/5;
		if (y>credits->h-190) y = credits->h-190;
		buffer.draw(credits,0,70,0,y,200,190);
	}

	buffer.textprintf(gSys->scorefont, 98,3, gSys->colors[0], -1, "%08i", score);

	if (targetBuffer.getWidth() == buffer.getWidth() && targetBuffer.getHeight() == buffer.getHeight()) {
		buffer.blitTo(targetBuffer, 0, 0);
	} else {
		buffer.stretchBlitTo(targetBuffer, 0, 0, targetBuffer.getWidth(), targetBuffer.getHeight());
	}
}

void MainGame::GameObject::updatePos() {
	int x = (int)pos.x;
	int y = (int)pos.y;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	mx = (x/40);
	my = (y/20)*2;
	int color = getpixel(game->mask, x%40, y%20);
	if (getr(color)>128) my++;
	if (getg(color)>128) my--;
	if (getb(color)>128) mx--;
	if (game && game->city) {
		if (mx < 0) mx = 0;
		if (my < 0) my = 0;
		if (mx >= game->city->width) mx = game->city->width - 1;
		if (my >= game->city->height) my = game->city->height - 1;
	}
}

MainGame::Player::Player() {
	px = 0;
	py = 0;
	mx = 0;
	my = 0;
	lastDx = 0;
	lastDy = 0;
	moveRepeatTimer = 0;
	selectAnimTick = 0;
	sa = new SpriteAnimator(gSys->rm.load<Sprite>("data/player.nws"));
}
MainGame::Player::~Player() {
	delete sa;
}

void MainGame::Player::syncPosFromGrid() {
	if (game && game->city) {
		if (mx < 0) mx = 0;
		if (my < 0) my = 0;
		if (mx >= game->city->width) mx = game->city->width - 1;
		if (my >= game->city->height) my = game->city->height - 1;
	}
	pos.x = (float)(mx * 40 + ((my % 2) ? 20 : 0) + 19);
	pos.y = (float)(my * 10 + 10);
}

void MainGame::Player::updatePos() {
	syncPosFromGrid();
}

float MainGame::Player::getSelectFade() const {
	// Starts at 0.80f on tick 0 (the instant a square is selected) and smoothly
	// fades in and out between 0.15f and 0.80f over a 48-tick (0.48s) cycle.
	return 0.475f + 0.325f * (float)cos(selectAnimTick * 2.0 * 3.14159265 / 48.0);
}

void MainGame::Player::update() {
	selectAnimTick++;

	int dx = 0;
	int dy = 0;
	if (gSys->input->isDown(LEFT))
		dx--;
	if (gSys->input->isDown(RIGHT))
		dx++;
	if (gSys->input->isDown(UP))
		dy--;
	if (gSys->input->isDown(DOWN))
		dy++;

	if (dx != 0 || dy != 0) {
		// Move immediately on the very first frame a direction is pressed or changed,
		// with no initial pause; when held down, step smoothly every 8 ticks (80 ms).
		if (dx != lastDx || dy != lastDy || moveRepeatTimer <= 0) {
			int gx = mx * 2 + (my & 1);
			int gy = my;
			int parity = (gy & 1);

			if (dx != 0 && dy != 0) {
				gx += dx;
				gy += dy;
			} else if (dy != 0) {
				gy += dy;
				gx += (parity ? -1 : 1);
			} else if (dx != 0) {
				gx += dx;
				gy += (parity ? -1 : 1);
			}

			int newMy = gy;
			int newMx = gx / 2;
			if (newMx < 0) newMx = 0;
			if (newMy < 0) newMy = 0;
			if (newMx >= game->city->width) newMx = game->city->width - 1;
			if (newMy >= game->city->height) newMy = game->city->height - 1;

			mx = newMx;
			my = newMy;
			syncPosFromGrid();
			selectAnimTick = 0;
			moveRepeatTimer = 8;
		} else {
			moveRepeatTimer--;
		}
	} else {
		moveRepeatTimer = 0;
	}

	lastDx = dx;
	lastDy = dy;

	// Pressing A burns the building in the currently selected square
	if (gSys->input->isDown(A)) {
		game->city->burnCell(mx, my, 10);
	}
}

void MainGame::Player::draw(alBitmap &buffer, bool overlay) {
	// No avatar or wireframe square over buildings; the selected building/tile fades in and out in LevelMap::draw()
	(void)buffer;
	(void)overlay;
}


MainGame::Police::Police(MainGame *_game) {
	game = _game;

	speed = 0;
	pos.x = random(200,game->city->width*40 - 200)/40*40;
	pos.y = random(200,game->city->height*10 - 200)/20*20 - 5;

	int ddx = (int)(game->player.pos.x-pos.x);
	int ddy = (int)(game->player.pos.y-pos.y);
	if (sqrt(ddx*ddx+ddy*ddy)<140) {
		if (game->player.pos.x<250)
			pos.x+=300;
		else
			pos.x-=300;
	}


	updatePos();
	while (game->city->getCellType(mx,my)!=1 || game->isAtSelectedSquare(mx, my, pos)) {
		pos.y+=20;
		if (pos.y > game->city->height * 10 - 100) pos.y = 100;
		updatePos();
	}

	bool alt = (my%2) == 1;
	dx = 0;
	dy = 0;
	// top left
	if (game->city->getCellType(mx-(alt?0:1),my-1)==1) {
		dx--;dy--;
	}
	// top right
	else if (game->city->getCellType(mx+(alt?1:0),my-1)==1) {
		dx++;dy--;
	}
	// bottom left
	else if (game->city->getCellType(mx-(alt?0:1),my+1)==1) {
		dx--;dy++;
	}
	// bottom right
	else if (game->city->getCellType(mx+(alt?1:0),my+1)==1) {
		dx++;dy++;
	}
}
float sq(float a) {
	return a*a;
}
void MainGame::Police::update() {

	drive();

	updatePos();
	if (game->isAtSelectedSquare(mx, my, pos)) {
		game->catchPlayer();
		return;
	}

	int ddx = (int)(game->player.pos.x-pos.x);
	int ddy = (int)(game->player.pos.y-pos.y);
	if (sqrt(ddx*ddx+ddy*ddy)<100 && abs(ddx)>10 && abs(ddy)>10) {
		if (ddx!=0)
			ddx/=abs(ddx);
		if (ddy!=0)
			ddy/=abs(ddy);

		pos.x += ddx * 10;
		pos.y += ddy * 5;
		updatePos();
		pos.x -= ddx * 10;
		pos.y -= ddy * 5;
		if (game->city->getCellType(mx,my)==1) {
			dx = ddx;
			dy = ddy;
		}
		updatePos();
	}
}
void MainGame::Police::drive() {

	pos.x += dx * 0.5 * speed;
	pos.y += dy * 0.25 * speed;

	if (speed<1)
		speed+=0.01f;

	pos.x += dx * 4;
	pos.y += dy * 2;
	updatePos();
	pos.x -= dx * 4;
	pos.y -= dy * 2;
	if (game->city->getCellType(mx,my)!=1) {
		dx = -dx;
		dy = -dy;
	}

	pos.x -= dx * 8;
	pos.y -= dy * 4;
	updatePos();
	pos.x += dx * 8;
	pos.y += dy * 4;

	if (mx!=lmx || my!=lmy) {

		bool alt = (my%2) == 1;
		bool tl = game->city->getCellType(mx-(alt?0:1),my-1)==1;
		bool tr = game->city->getCellType(mx+(alt?1:0),my-1)==1;
		bool bl = game->city->getCellType(mx-(alt?0:1),my+1)==1;
		bool br = game->city->getCellType(mx+(alt?1:0),my+1)==1;
		if (dx==dy) {
			// if sides are open, 50% of turning
			if ((tr||bl)&&(((rand()%2)==0)||!br||!tl)) {
				// if both ways are open, 50% eliminate one
				if (tr&&bl&&(rand()%2)==0) bl = false;
				if (bl) {
					dx=-1; dy=1;
				} else {
					dx=1;  dy=-1;
				}
				speed = 0.5;
			}
		} else {
			// if sides are open, 50% of turning
			if ((tl||br)&&(((rand()%2)==0)||!bl||!tr)) {
				// if both ways are open, 50% eliminate one
				if (tl&&br&&(rand()%2)==0) br = false;
				if (br) {
					dx=1;  dy=1;
				} else {
					dx=-1; dy=-1;
				}
				speed = 0.5;
			}
		}
		lmx = mx;
		lmy = my;
	}
}
void MainGame::Police::draw(alBitmap &buffer, bool overlay) {
	vector2i p((int)pos.x-(int)game->viewport.x,(int)pos.y-(int)game->viewport.y);
	if (p.x < -24 || p.x > 224 || p.y < -24 || p.y > 324) return;
	if (!overlay) {
		buffer.draw(game->police,p.x-10,p.y-3,
					dx<0 ? 0 : 16, dy>0 ? 0 : 11, 16, 11);
	} else {
		buffer.draw(game->policeptr,p.x-3,p.y-20);
	}
}
MainGame::FireTruck::FireTruck(MainGame *_game) : Police(_game) {
}
void MainGame::FireTruck::update() {
	drive();

	updatePos();
	if (game->isAtSelectedSquare(mx, my, pos)) {
		game->catchPlayer();
		return;
	}

	bool alt = (my%2) == 1;
	restore(mx-(alt?0:1),my-1);
	restore(mx+(alt?1:0),my-1);
	restore(mx-(alt?0:1),my+1);
	restore(mx+(alt?1:0),my+1);
}
void MainGame::FireTruck::restore(int x, int y) {
	Cell *c = game->city->getCell(x,y);
	if (!c) return;
	if (c->fire) {
		delete c->fire;
		c->fire = 0;
		speed = 0;
	}
	if (c->type==100) {
		c->life = c->targetlife = 0;
		c->update();
		speed = 0;
	}
}
void MainGame::FireTruck::draw(alBitmap &buffer, bool overlay) {
	vector2i p((int)pos.x-(int)game->viewport.x,(int)pos.y-(int)game->viewport.y);
	if (p.x < -24 || p.x > 224 || p.y < -24 || p.y > 324) return;
	if (!overlay) {
		buffer.draw(game->firetruck,p.x-8,p.y-3,
					dx<0 ? 0 : 21, dy>0 ? 0 : 16, 21, 16);
	} else {
		buffer.draw(game->fireptr,p.x-3,p.y-20);
	}
}


MainGame::LevelMap::LevelMap(MainGame *_game, int _width, int _height) {
	game = _game;
	width = _width;
	height = _height;
	cells = new Cell[width*height];
	for (int y=0; y<height; y++)
		for (int x=0; x<width; x++) {
			setCellType(x,y,(rand()%100)<30 ? 0 : 100+rand()%BUILDING_COUNT);
			Cell *c = getCell(x,y);
			c->x = x;
			c->y = y;
			c->game = game;
		}



	// Draw down right roads
	for (int startx=-width; startx<width; startx++) {

		int x = startx;
		int y = 0;
		while (x<width && y<height) {
			setCellType(x,y,1);
			if ((y%2)==1)
				x++;
			y++;
		}
		startx += rand()%5 + 1;
	}

	// draw down left roads
	for (int startx=rand()%5 ; startx<width*2; startx++) {
		int x = startx;
		int y = 0;
		bool makeroad = true;
		while (x<width*2 && y<height) {
			if (getCellType(x,y)==1)
				makeroad = (rand()%100)<75;
			if (makeroad)
				setCellType(x,y,1);
			if ((y%2)==0)
				x--;
			y++;
		}
		startx += rand()%5 + 1;
	}

}
void MainGame::LevelMap::makeCenter(int cx, int cy) {
	if (getCellType(cx,cy)<=100)
		setCellType(cx,cy,102+rand()%(BUILDING_COUNT-2));
	setCellType(cx,cy+3,-1);
	setCellType(cx,cy-2,1);
	setCellType(cx,cy+2,1);
	setCellType(cx-1,cy,1);
	setCellType(cx+1,cy,1);


	int xx = cy%2;
	setCellType(cx+xx,cy-1,1);
	setCellType(cx+xx,cy+1,1);
	setCellType(cx+xx-1,cy-1,1);
	setCellType(cx+xx-1,cy+1,1);
}

MainGame::LevelMap::~LevelMap() {
	delete [] cells;
}
void MainGame::LevelMap::update() {
	for (int y=0; y<height; y++)
		for (int x=0; x<width; x++)
			getCell(x,y)->update();
}

static void drawFadedSprite(BITMAP *dst, BITMAP *src, int dx, int dy, float fade) {
	int maskColor = bitmap_mask_color(src);
	int dstW = dst->w;
	int dstH = dst->h;
	int srcW = src->w;
	int srcH = src->h;

	for (int py = 0; py < srcH; py++) {
		int ty = dy + py;
		if (ty < 0 || ty >= dstH) continue;
		for (int px = 0; px < srcW; px++) {
			int tx = dx + px;
			if (tx < 0 || tx >= dstW) continue;
			int c = getpixel(src, px, py);
			if (c == maskColor) continue;
			int bg = getpixel(dst, tx, ty);
			int r = getr(c) + (int)((getr(bg) - getr(c)) * fade);
			int g = getg(c) + (int)((getg(bg) - getg(c)) * fade);
			int b = getb(c) + (int)((getb(bg) - getb(c)) * fade);
			putpixel(dst, tx, ty, makecol(r, g, b));
		}
	}
}

static void drawFadedGroundTile(BITMAP *dst, BITMAP *maskBmp, int dx, int dy, float fade, int fillColor, int borderColor) {
	int fillR = getr(fillColor), fillG = getg(fillColor), fillB = getb(fillColor);
	int bordR = getr(borderColor), bordG = getg(borderColor), bordB = getb(borderColor);
	int dstW = dst->w;
	int dstH = dst->h;

	for (int py = 0; py < 20; py++) {
		int ty = dy + py;
		if (ty < 0 || ty >= dstH) continue;
		for (int px = 0; px < 40; px++) {
			int tx = dx + px;
			if (tx < 0 || tx >= dstW) continue;
			int mc = getpixel(maskBmp, px, py);
			// In data/mask.png, the center isometric diamond of the tile is black (r<128, g<128, b<128)
			if (getr(mc) < 128 && getg(mc) < 128 && getb(mc) < 128) {
				bool isEdge = false;
				if (px == 0 || px == 39 || py == 0 || py == 19) {
					isEdge = true;
				} else {
					int mL = getpixel(maskBmp, px - 1, py);
					int mR = getpixel(maskBmp, px + 1, py);
					int mU = getpixel(maskBmp, px, py - 1);
					int mD = getpixel(maskBmp, px, py + 1);
					if (getr(mL) >= 128 || getg(mL) >= 128 || getb(mL) >= 128 ||
					    getr(mR) >= 128 || getg(mR) >= 128 || getb(mR) >= 128 ||
					    getr(mU) >= 128 || getg(mU) >= 128 || getb(mU) >= 128 ||
					    getr(mD) >= 128 || getg(mD) >= 128 || getb(mD) >= 128) {
						isEdge = true;
					}
				}
				int c = getpixel(dst, tx, ty);
				int tr = isEdge ? bordR : fillR;
				int tg = isEdge ? bordG : fillG;
				int tb = isEdge ? bordB : fillB;
				float f = isEdge ? fade : (fade * 0.65f);
				int r = getr(c) + (int)((tr - getr(c)) * f);
				int g = getg(c) + (int)((tg - getg(c)) * f);
				int b = getb(c) + (int)((tb - getb(c)) * f);
				putpixel(dst, tx, ty, makecol(r, g, b));
			}
		}
	}
}

void MainGame::LevelMap::draw(alBitmap &buffer) {
	int vx = (int)game->viewport.x;
	int vy = (int)game->viewport.y;
	int bufW = buffer.getWidth();
	int bufH = buffer.getHeight();
	float selectFade = game->player.getSelectFade();

	bool alt = false;
	for (int y=0; y<height; y++) {
		int yy = y*10 - vy;
		if (yy <= -20 || yy >= bufH) {
			alt = !alt;
			continue;
		}

		// draw roads/land (culled to visible viewport)
		for (int x=0; x<width; x++) {
			int xx = x*40 + (alt?20:0) - vx;
			if (xx <= -40 || xx >= bufW) continue;

			Cell *cell = getCell(x,y);
			int type = cell->type;
			int roadtype = 0;
			if (type==1) {
				// top left
				if (getCellType(x-(alt?0:1),y-1)==1)
					roadtype |= 8;
				// top right
				if (getCellType(x+(alt?1:0),y-1)==1)
					roadtype |= 4;
				// bottom left
				if (getCellType(x-(alt?0:1),y+1)==1)
					roadtype |= 2;
				// bottom right
				if (getCellType(x+(alt?1:0),y+1)==1)
					roadtype |= 1;
			}
			buffer.draw(game->tiles, xx, yy,
								 roadtype*38, 0, 40, 20);

			// If an empty/road tile is selected (no building on it), fade its ground tile
			// in and out underneath any surrounding buildings.
			if (x == game->player.mx && y == game->player.my && type < 100) {
				drawFadedGroundTile(buffer.getBITMAP(), game->mask, xx, yy, selectFade, gSys->colors[2], gSys->colors[0]);
			}
		}
		alt = !alt;
	}

	alt = false;
	for (int y=0; y<height; y++) {
		int yy = y*10 - vy;
		if (yy <= -20 || yy - 60 >= bufH) {
			alt = !alt;
			continue;
		}

		// draw buildings (culled to visible viewport)
		for (int x=0; x<width; x++) {
			int xx = x*40 + (alt?20:0) - vx;
			if (xx <= -40 || xx >= bufW) continue;

			Cell *cell = getCell(x,y);
			int type = cell->type;
			if (type>=100) {
				BITMAP *img = game->buildings[type-100];
				int drawY = yy - img->h + 20;
				if (x == game->player.mx && y == game->player.my) {
					drawFadedSprite(buffer.getBITMAP(), img, xx, drawY, selectFade);
				} else {
					buffer.draw(img, xx, drawY);
				}
				if (cell->fire)
					cell->fire->draw(buffer.getBITMAP(),vector2i(xx,yy));
			}
		}

		// draw vehicles on row y
		for (vector<GameObject*>::iterator ii = game->sprites.begin(); ii != game->sprites.end(); ii++) {
			GameObject *s = *ii;
			if (s != &(game->player) && s->my==y)
				s->draw(buffer);
		}

		alt = !alt;
	}
}

MainGame::Cell *MainGame::LevelMap::getCell(int x, int y) {
	if (x<0 || y<0 || x>=width || y>=height) return 0;
	return &cells[x+y*width];
}
void MainGame::Cell::burn(int _points) {
	if (type<=100 || fire!=0) return;
	fire = new SpriteAnimator(gSys->rm.load<Sprite>("data/fire.nws"));
	life = 0;
	if (abs(x-game->player.mx)<4 && abs(y-game->player.my)<8) {
		stop_sample(game->firefx);
		play_sample(game->firefx, 100, 128, 1000, 0);
	}
	points = _points;
}
void MainGame::Cell::update() {
	life++;
	if (fire) {
		int anim=0;
		if (life>10*1)
			anim++;
		if (life>20*1)
			anim++;
		if (life>30*1)
			anim++;
		if (life>40*1) {
			type = 100;
		}
		if (life>55*1)
			anim--;
		if (life>65*1)
			anim--;
		fire->transition(anim,true);
		fire->update();
		if (life>75*1) {
			delete fire;
			int xx = y%2;
			game->score += points;
			game->city->burnCell(x+xx,y-1,points+5);
			game->city->burnCell(x+xx,y+1,points+5);
			game->city->burnCell(x-1+xx,y-1,points+5);
			game->city->burnCell(x-1+xx,y+1,points+5);
			fire = 0;
			life = 0;
			targetlife = (rand()%8000)+4000;
		}
	} else if (type==101) {
		if (life>targetlife) {
			type = rand()%(BUILDING_COUNT-2)+102;
			life = 0;
		}
	} else if (type==100) {
		if (life>targetlife) {
			type=101;
			life = 0;
			targetlife = (rand()%6000)+6000;
		}
	}
}
void MainGame::LevelMap::drawMap(alBitmap &buffer) {
	bool alt = false;
	for (int y=0; y<height; y++) {
		int yy = y*1 - (int)(game->viewport.y+150)/20+150;

		// draw roads/land
		for (int x=0; x<width; x++) {
			int xx = x*2 + (alt?1:0) - (int)(game->viewport.x+100)/20+100;

			Cell *cell = getCell(x,y);
			int type = cell->type;
			int color = gSys->colors[4];
			if (type==1) color = gSys->colors[2];
			if (type==100) color = gSys->colors[1];
			if (type>100) color = gSys->colors[3];
			buffer.rectFill(xx,yy,xx+1,yy,color);
		}
		alt = !alt;
	}
	for (vector<GameObject*>::iterator ii = game->sprites.begin(); ii != game->sprites.end(); ii++) {
		GameObject *s = *ii;
		s->updatePos();
		int xx = s->mx*2 + ((s->my%2)?1:0) - (int)(game->viewport.x+100)/20+100;
		int yy = s->my - (int)(game->viewport.y+150)/20+150;
		buffer.setPixel(xx,yy,gSys->colors[0]);
		if (s==&(game->player))
			buffer.rect(xx-2,yy-2,xx+2,yy+2,gSys->colors[0]);
	}
}
