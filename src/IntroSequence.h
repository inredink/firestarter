#ifndef _INTRO_SEQUENCE_H
#define _INTRO_SEQUENCE_H

#include "GameState.h"
#include <vector>
#include <string>

class IntroSequence : public GameState {
	public:
		enum State {
			TITLE = 0,
			INTRO_FADE_IN,
			INTRO_READING,
			INTRO_FADE_OUT,
			GAME_INITIALIZATION,
			GAMEPLAY
		};

		static const int PASSAGE_COUNT = 6;
		static const int FADE_IN_TICKS = 80;   // 800 ms at 100 Hz
		static const int FADE_OUT_TICKS = 80;  // 800 ms at 100 Hz
		static const int GAME_FADE_IN_TICKS = 80; // 800 ms fade into gameplay

		IntroSequence();
		virtual ~IntroSequence();

		void resetToTitle();
		virtual void update();
		virtual void draw(alBitmap &bitmap);

		// State inspection helpers
		State getState() const { return state; }
		int getCurrentPassageIndex() const { return currentPassage; }
		int getStateTimer() const { return stateTimer; }
		int getReadingDurationTicks(int passageIndex) const;
		static const char *getPassageText(int passageIndex);

		// Text layout helpers (exposed for resolution/wrapping verification)
		static int measureChar(const FONT *f, char c, int spaceWidth);
		static int measureText(const FONT *f, const std::string &str, int spaceWidth);
		static void drawTextLine(BITMAP *bmp, const FONT *f, const std::string &str, int x, int y, int color, int spaceWidth);
		static std::vector<std::string> wrapText(const FONT *f, const char *text, int maxWidth, int spaceWidth);

	private:
		void buildTitleBackdrop(int width, int height);
		void drawTitleScreen(alBitmap &bitmap);
		void drawNarrativeScreen(alBitmap &bitmap);
		void drawGameplayWithFade(alBitmap &bitmap);
		int interpolateGreenColor(float factor) const;

		State state;
		int currentPassage;
		int stateTimer;
		bool wasActionDown;
		bool wasSelectDown;

		alBitmap titleBackdrop;
		int backdropWidth;
		int backdropHeight;
};

#endif
