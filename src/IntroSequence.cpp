#include "IntroSequence.h"
#include "Global.h"
#include "MainGame.h"
#include "partypack/bitmap.h"
#include "partypack/gamepad.h"
#include <cstring>

using namespace std;

static const char *NARRATIVE_PASSAGES[IntroSequence::PASSAGE_COUNT] = {
	"No one knows how Endsville, the city of devils, was created, but as soon as it appeared, it started to grow, and slowly eat up the world.",
	"You are an agent of Canti, the God of the Black Flame... and you must stop Endsville from destroying the world.",
	"Your only weapon is flame:  matches, lighters, and fire-bombs. Get the weapons, outsmart the firefighters, hide from the police.",
	"Torch the city, and purge its demons. But you must be careful; you cannot burn everything.",
	"If the entire city burns down, there will be no place to live. Thus, the battle is endless: there is no final victory. The only thing you can do is burn... and burn... and burn...",
	"Your highest goal is to receive the personal blessing of Canti, the God of the Black Flame..."
};

// Reading durations in 100 Hz ticks (8.0s to 12.0s scaled to passage length)
static const int READING_DURATION_TICKS[IntroSequence::PASSAGE_COUNT] = {
	1000, // Screen 1 (27 words): 10.0 seconds
	900,  // Screen 2 (21 words):  9.0 seconds
	900,  // Screen 3 (19 words):  9.0 seconds
	800,  // Screen 4 (16 words):  8.0 seconds
	1200, // Screen 5 (35 words): 12.0 seconds
	850   // Screen 6 (17 words):  8.5 seconds
};

IntroSequence::IntroSequence() {
	backdropWidth = 0;
	backdropHeight = 0;
	resetToTitle();
}

IntroSequence::~IntroSequence() {
}

void IntroSequence::resetToTitle() {
	state = TITLE;
	currentPassage = 0;
	stateTimer = 0;
	if (gSys && gSys->input) {
		wasActionDown = gSys->input->isDown(START) || gSys->input->isDown(A);
		wasSelectDown = gSys->input->isDown(SELECT);
	} else {
		wasActionDown = false;
		wasSelectDown = false;
	}
}

const char *IntroSequence::getPassageText(int passageIndex) {
	if (passageIndex < 0 || passageIndex >= PASSAGE_COUNT) return "";
	return NARRATIVE_PASSAGES[passageIndex];
}

int IntroSequence::getReadingDurationTicks(int passageIndex) const {
	if (passageIndex < 0 || passageIndex >= PASSAGE_COUNT) return 800;
	return READING_DURATION_TICKS[passageIndex];
}

int IntroSequence::measureChar(const FONT *f, char c, int spaceWidth) {
	if (c == ' ') return spaceWidth;
	if (c == ':' || c == ';') return 3;
	char s[2] = { c, 0 };
	return text_length(f, s);
}

int IntroSequence::measureText(const FONT *f, const string &str, int spaceWidth) {
	int w = 0;
	for (size_t i = 0; i < str.size(); i++) {
		w += measureChar(f, str[i], spaceWidth);
	}
	return w;
}

void IntroSequence::drawTextLine(BITMAP *bmp, const FONT *f, const string &str, int x, int y, int color, int spaceWidth) {
	int curX = x;
	for (size_t i = 0; i < str.size(); i++) {
		char c = str[i];
		if (c == ' ') {
			curX += spaceWidth;
		} else if (c == ':') {
			rectfill(bmp, curX, y + 5, curX + 1, y + 5, color);
			rectfill(bmp, curX, y + 9, curX + 1, y + 9, color);
			curX += 3;
		} else if (c == ';') {
			rectfill(bmp, curX, y + 5, curX + 1, y + 5, color);
			rectfill(bmp, curX, y + 9, curX + 1, y + 10, color);
			putpixel(bmp, curX + 1, y + 11, color);
			curX += 3;
		} else {
			char s[2] = { c, 0 };
			textout_ex(bmp, f, s, curX, y, color, -1);
			curX += text_length(f, s);
		}
	}
}

vector<string> IntroSequence::wrapText(const FONT *f, const char *text, int maxWidth, int spaceWidth) {
	vector<string> tokens;
	vector<int> leadingSpaces;

	int len = (int)strlen(text);
	int i = 0;
	while (i < len) {
		int spaces = 0;
		while (i < len && text[i] == ' ') {
			spaces++;
			i++;
		}
		if (i >= len) break;

		int wordStart = i;
		while (i < len && text[i] != ' ') {
			i++;
		}
		string word(text + wordStart, i - wordStart);

		// Keep explicit multi-space sequences (e.g. "flame:  matches,") joined on the same line
		// so that double spaces are never lost across a line break.
		if (!tokens.empty() && spaces > 1) {
			tokens.back() += string(spaces, ' ') + word;
		} else {
			tokens.push_back(word);
			leadingSpaces.push_back(spaces > 0 ? spaces : 1);
		}
	}

	vector<string> lines;
	string currentLine;
	for (size_t t = 0; t < tokens.size(); t++) {
		if (currentLine.empty()) {
			currentLine = tokens[t];
		} else {
			string candidate = currentLine + string(leadingSpaces[t], ' ') + tokens[t];
			if (measureText(f, candidate, spaceWidth) <= maxWidth) {
				currentLine = candidate;
			} else {
				lines.push_back(currentLine);
				currentLine = tokens[t];
			}
		}
	}
	if (!currentLine.empty()) {
		lines.push_back(currentLine);
	}
	return lines;
}

static void drawScaledText(BITMAP *bmp, const FONT *f, const char *str, int x, int y, int scale, int fgColor, int shadowColor, int spaceWidth) {
	string s(str);
	int baseW = IntroSequence::measureText(f, s, spaceWidth);
	int baseH = text_height(f);
	BITMAP *tmp = create_bitmap(baseW + 2, baseH + 2);
	int mask = bitmap_mask_color(tmp);
	clear_to_color(tmp, mask);
	IntroSequence::drawTextLine(tmp, f, s, 0, 0, fgColor, spaceWidth);

	if (shadowColor >= 0) {
		for (int py = 0; py < baseH; py++) {
			for (int px = 0; px < baseW; px++) {
				if (getpixel(tmp, px, py) != mask) {
					rectfill(bmp,
						x + px * scale + scale,
						y + py * scale + scale,
						x + (px + 1) * scale - 1 + scale,
						y + (py + 1) * scale - 1 + scale,
						shadowColor);
				}
			}
		}
	}
	for (int py = 0; py < baseH; py++) {
		for (int px = 0; px < baseW; px++) {
			if (getpixel(tmp, px, py) != mask) {
				rectfill(bmp,
					x + px * scale,
					y + py * scale,
					x + (px + 1) * scale - 1,
					y + (py + 1) * scale - 1,
					fgColor);
			}
		}
	}
	destroy_bitmap(tmp);
}

void IntroSequence::buildTitleBackdrop(int width, int height) {
	backdropWidth = width;
	backdropHeight = height;
	titleBackdrop.create(width, height);
	BITMAP *bmp = titleBackdrop.getBITMAP();
	clear_to_color(bmp, gSys->colors[0]);

	BITMAP *tiles = gSys->rm.load<Bitmap>("data/roads.png")->getABitmap();
	BITMAP *buildings[6];
	buildings[0] = gSys->rm.load<Bitmap>("data/rubble.png")->getABitmap();
	buildings[1] = gSys->rm.load<Bitmap>("data/construction0.png")->getABitmap();
	buildings[2] = gSys->rm.load<Bitmap>("data/building0.png")->getABitmap();
	buildings[3] = gSys->rm.load<Bitmap>("data/building1.png")->getABitmap();
	buildings[4] = gSys->rm.load<Bitmap>("data/building2.png")->getABitmap();
	buildings[5] = gSys->rm.load<Bitmap>("data/building3.png")->getABitmap();

	alBitmap buildings2[6];
	for (int i = 0; i < 6; i++) {
		buildings2[i] = buildings[i];
		int mask = buildings2[i].getMaskColor();
		for (int y = 0; y < buildings2[i].getHeight(); y++) {
			for (int x = 0; x < buildings2[i].getWidth(); x++) {
				if ((x & 1) ^ (y & 1))
					buildings2[i].setPixel(x, y, mask);
			}
		}
	}

	const int GW = 12, GH = 44;
	int grid[GH][GW];
	unsigned int seed = 1337;
	for (int y = 0; y < GH; y++) {
		for (int x = 0; x < GW; x++) {
			seed = seed * 1103515245 + 12345;
			int r = (seed >> 16) % 100;
			if ((x + y / 2) % 4 == 1 || (x - y / 2 + 20) % 5 == 2) {
				grid[y][x] = 1;
			} else if (r < 12) {
				grid[y][x] = 100;
			} else if (r < 22) {
				grid[y][x] = 101;
			} else if (r < 42) {
				grid[y][x] = 102;
			} else if (r < 68) {
				grid[y][x] = 103;
			} else if (r < 82) {
				grid[y][x] = 104;
			} else {
				grid[y][x] = 105;
			}
		}
	}

	int viewX = 40;
	int viewY = 20;
	rectfill(bmp, 0, 0, width - 1, height - 1, gSys->colors[5]);

	for (int y = 0; y < GH; y++) {
		bool alt = (y % 2) == 1;
		int yy = y * 10 - viewY;
		for (int x = 0; x < GW; x++) {
			int xx = x * 40 + (alt ? 20 : 0) - viewX;
			int type = grid[y][x];
			int roadtype = 0;
			if (type == 1) {
				int ax0 = x - (alt ? 0 : 1), ax1 = x + (alt ? 1 : 0);
				if (y > 0 && ax0 >= 0 && grid[y - 1][ax0] == 1) roadtype |= 8;
				if (y > 0 && ax1 < GW && grid[y - 1][ax1] == 1) roadtype |= 4;
				if (y + 1 < GH && ax0 >= 0 && grid[y + 1][ax0] == 1) roadtype |= 2;
				if (y + 1 < GH && ax1 < GW && grid[y + 1][ax1] == 1) roadtype |= 1;
			}
			masked_blit(tiles, bmp, roadtype * 38, 0, xx, yy, 40, 20);
		}
	}

	for (int y = 0; y < GH; y++) {
		bool alt = (y % 2) == 1;
		int yy = y * 10 - viewY;
		for (int x = 0; x < GW; x++) {
			int xx = x * 40 + (alt ? 20 : 0) - viewX;
			int type = grid[y][x];
			if (type >= 100) {
				BITMAP *b1 = buildings[type - 100];
				BITMAP *b2 = buildings2[type - 100].getBITMAP();
				draw_sprite(bmp, b1, xx, yy - b1->h + 20);
				draw_sprite(bmp, b2, xx, yy - b2->h + 20);
			}
		}
	}

	int topBarH = (height * 68) / 320;
	if (topBarH < 60) topBarH = 60;
	int botBarH = (height * 24) / 320;
	if (botBarH < 18) botBarH = 18;
	int sideBorder = 6;

	rectfill(bmp, 0, 0, width - 1, topBarH - 1, gSys->colors[0]);
	rectfill(bmp, 0, height - botBarH, width - 1, height - 1, gSys->colors[0]);
	rectfill(bmp, 0, 0, sideBorder - 1, height - 1, gSys->colors[0]);
	rectfill(bmp, width - sideBorder, 0, width - 1, height - 1, gSys->colors[0]);

	hline(bmp, sideBorder, topBarH - 1, width - sideBorder - 1, gSys->colors[1]);
	hline(bmp, sideBorder, height - botBarH, width - sideBorder - 1, gSys->colors[1]);

	const char *titleStr = "FIRE STARTER";
	int titleScale = 2;
	int titleSpace = 6;
	int titleW = measureText(gSys->font, titleStr, titleSpace) * titleScale;
	int titleX = (width - titleW) / 2;
	int titleY = (topBarH >= 64) ? 14 : 10;
	drawScaledText(bmp, gSys->font, titleStr, titleX, titleY, titleScale, gSys->colors[4], gSys->colors[1], titleSpace);

	const char *promptStr = "press start";
	int promptW = measureText(gSys->font, promptStr, 5);
	int promptY = titleY + text_height(gSys->font) * titleScale + 8;
	drawTextLine(bmp, gSys->font, promptStr, (width - promptW) / 2, promptY, gSys->colors[3], 5);
}

int IntroSequence::interpolateGreenColor(float factor) const {
	if (factor <= 0.0f) return gSys->colors[0];
	if (factor >= 1.0f) factor = 1.0f;

	// Target muted green: colors[3] (#4C9D5C)
	int r = (int)(0x4C * factor + 0.5f);
	int g = (int)(0x9D * factor + 0.5f);
	int b = (int)(0x5C * factor + 0.5f);
	return makecol(r, g, b);
}

void IntroSequence::update() {
	bool actionDown = gSys->input->isDown(START) || gSys->input->isDown(A);
	bool actionJustPressed = actionDown && !wasActionDown;
	wasActionDown = actionDown;

	if (state != GAMEPLAY) {
		bool selectDown = gSys->input->isDown(SELECT);
		if (selectDown && !wasSelectDown) {
			gSys->switchMusic();
		}
		wasSelectDown = selectDown;
	}

	switch (state) {
		case TITLE:
			if (actionJustPressed) {
				currentPassage = 0;
				stateTimer = 0;
				state = INTRO_FADE_IN;
			}
			break;

		case INTRO_FADE_IN:
			// Ignore button presses during active fade-in transition
			stateTimer++;
			if (stateTimer >= FADE_IN_TICKS) {
				stateTimer = 0;
				state = INTRO_READING;
			}
			break;

		case INTRO_READING:
			stateTimer++;
			if (actionJustPressed || stateTimer >= getReadingDurationTicks(currentPassage)) {
				stateTimer = 0;
				state = INTRO_FADE_OUT;
			}
			break;

		case INTRO_FADE_OUT:
			// Ignore button presses during active fade-out transition
			stateTimer++;
			if (stateTimer >= FADE_OUT_TICKS) {
				stateTimer = 0;
				currentPassage++;
				if (currentPassage < PASSAGE_COUNT) {
					state = INTRO_FADE_IN;
				} else {
					state = GAME_INITIALIZATION;
				}
			}
			break;

		case GAME_INITIALIZATION:
			if (stateTimer == 0) {
				gSys->maingame->setup();
			}
			stateTimer++;
			if (stateTimer >= GAME_FADE_IN_TICKS) {
				stateTimer = 0;
				state = GAMEPLAY;
			}
			break;

		case GAMEPLAY:
			gSys->maingame->update();
			break;
	}
}

void IntroSequence::drawTitleScreen(alBitmap &bitmap) {
	int w = bitmap.getWidth();
	int h = bitmap.getHeight();
	if (backdropWidth != w || backdropHeight != h || !titleBackdrop.getBITMAP()) {
		buildTitleBackdrop(w, h);
	}
	titleBackdrop.blitTo(bitmap, 0, 0);
}

void IntroSequence::drawNarrativeScreen(alBitmap &bitmap) {
	int w = bitmap.getWidth();
	int h = bitmap.getHeight();
	bitmap.clear(gSys->colors[0]);

	if (currentPassage < 0 || currentPassage >= PASSAGE_COUNT) return;

	float fadeFactor = 1.0f;
	if (state == INTRO_FADE_IN) {
		fadeFactor = (float)stateTimer / (float)FADE_IN_TICKS;
	} else if (state == INTRO_FADE_OUT) {
		fadeFactor = 1.0f - ((float)stateTimer / (float)FADE_OUT_TICKS);
	}

	int textColor = interpolateGreenColor(fadeFactor);
	int spaceWidth = 5;
	int lineHeight = 18;
	int maxWidth = (w * 164) / 240;
	if (maxWidth < 136) maxWidth = w - 16;

	vector<string> lines = wrapText(gSys->font, NARRATIVE_PASSAGES[currentPassage], maxWidth, spaceWidth);
	int fontH = text_height(gSys->font);
	int totalHeight = (int)lines.size() * lineHeight - (lineHeight - fontH);
	int startY = (h - totalHeight) / 2;

	BITMAP *bmp = bitmap.getBITMAP();
	for (size_t i = 0; i < lines.size(); i++) {
		int lineW = measureText(gSys->font, lines[i], spaceWidth);
		int lineX = (w - lineW) / 2;
		int lineY = startY + (int)i * lineHeight;
		drawTextLine(bmp, gSys->font, lines[i], lineX, lineY, textColor, spaceWidth);
	}
}

void IntroSequence::drawGameplayWithFade(alBitmap &bitmap) {
	gSys->maingame->draw(bitmap);

	float factor = (float)stateTimer / (float)GAME_FADE_IN_TICKS;
	if (factor >= 1.0f) return;
	if (factor <= 0.0f) {
		bitmap.clear(gSys->colors[0]);
		return;
	}

	int scale256 = (int)(factor * 256.0f);
	BITMAP *bmp = bitmap.getBITMAP();
	int w = bmp->w;
	int h = bmp->h;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int c = getpixel(bmp, x, y);
			int r = (getr(c) * scale256) >> 8;
			int g = (getg(c) * scale256) >> 8;
			int b = (getb(c) * scale256) >> 8;
			putpixel(bmp, x, y, makecol(r, g, b));
		}
	}
}

void IntroSequence::draw(alBitmap &bitmap) {
	switch (state) {
		case TITLE:
			drawTitleScreen(bitmap);
			break;
		case INTRO_FADE_IN:
		case INTRO_READING:
		case INTRO_FADE_OUT:
			drawNarrativeScreen(bitmap);
			break;
		case GAME_INITIALIZATION:
			drawGameplayWithFade(bitmap);
			break;
		case GAMEPLAY:
			gSys->maingame->draw(bitmap);
			break;
	}
}
