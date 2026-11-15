/*
SDLPoP, a port/conversion of the DOS game Prince of Persia.
Copyright (C) 2013-2025  Dávid Nagy

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// Post-processing of the final 320x200 frame:
//   1. dynamic torch lighting (flickering, warm colored)
//   2. crossfade between rooms
//   3. Scale2x applied twice (= Scale4x) pixel-art upscaling
//   4. CRT look: scanlines + vignette
// Everything runs on the CPU at present time, the game's own surfaces are never touched.

#include "common.h"
#include <math.h>

#define W 320
#define H 200
#define LIT_H 192 // the status bar below is never lit
#define S 4       // upscale factor

static Uint32 frame[W*H];      // current frame, 0x00RRGGBB
static Uint32 last_frame[W*H]; // last presented frame (after lighting)
static Uint32 fade_from[W*H];
static Uint32 mid[W*2*H*2];
static Uint32 out[W*S*H*S];
static Uint8 crt_mul[W*S*H*S]; // 0..255 brightness factor
static SDL_Texture* fx_texture;
static Uint32 fade_start;
static word last_room;
static int last_crt = -1;
static Uint32 last_present;
static int fading;

#define FADE_MS 220

static int lighting_active(void) {
	return fx_lighting && fx_in_game && !is_cutscene && !is_menu_shown
		&& drawn_room >= 1 && drawn_room <= 24;
}

int fx_enabled(void) {
	return fx_upscale || fx_lighting || fx_crt || fx_transitions;
}

static void fx_load_frame(SDL_Surface* surface) {
	// 24-bit surface: bytes are in Rmsk/Gmsk/Bmsk order (R first on little endian).
	int r_ofs = (Rmsk == 0x000000ff) ? 0 : 2;
	int b_ofs = 2 - r_ofs;
	for (int y = 0; y < H; ++y) {
		const Uint8* p = (const Uint8*)surface->pixels + y * surface->pitch;
		Uint32* d = frame + y * W;
		for (int x = 0; x < W; ++x, p += 3) {
			d[x] = (p[r_ofs] << 16) | (p[1] << 8) | p[b_ofs];
		}
	}
}

static void apply_lighting(void) {
	struct { float x, y, i, r; } torches[30];
	int count = 0;
	float t = SDL_GetTicks() / 1000.0f;
	const byte* tiles = &level.fg[(drawn_room - 1) * 30];
	for (int pos = 0; pos < 30; ++pos) {
		int type = tiles[pos] & 0x1F;
		if (type != tiles_19_torch && type != tiles_30_torch_with_debris) continue;
		float ph = pos * 1.7f + drawn_room * 0.37f;
		// Flicker: a few sines at unrelated frequencies plus a bit of noise.
		float i = 0.85f + 0.09f * sinf(t * 9.1f + ph) + 0.05f * sinf(t * 23.3f + ph * 2.3f)
		          + 0.03f * ((rand() & 255) / 255.0f - 0.5f);
		torches[count].x = (float)((pos % 10) * 32 + 48);
		torches[count].y = (float)((pos / 10) * 63 + 22);
		if (upside_down) torches[count].y = LIT_H - 1 - torches[count].y;
		torches[count].i = i;
		torches[count].r = 95.0f * (0.94f + 0.06f * i);
		++count;
	}
	int amb = fx_ambient * 256 / 100;
	for (int y = 0; y < LIT_H; ++y) {
		Uint32* row = frame + y * W;
		for (int x = 0; x < W; ++x) {
			float light = 0;
			for (int k = 0; k < count; ++k) {
				float dx = x - torches[k].x, dy = (y - torches[k].y) * 1.2f; // pixels are taller than wide
				float q = 1.0f - (dx*dx + dy*dy) / (torches[k].r * torches[k].r);
				if (q > 0) light += torches[k].i * q * q;
			}
			if (light > 1.0f) light = 1.0f;
			// Torch light restores the original brightness plus a small warm boost.
			int l = (int)(light * (256 - amb + 24));
			int mr = amb + l * 9 / 8, mg = amb + l, mb = amb + l * 13 / 16;
			Uint32 c = row[x];
			int r = (((c >> 16) & 255) * mr) >> 8;
			int g = (((c >> 8) & 255) * mg) >> 8;
			int b = ((c & 255) * mb) >> 8;
			row[x] = ((r > 255 ? 255 : r) << 16) | ((g > 255 ? 255 : g) << 8) | (b > 255 ? 255 : b);
		}
	}
}

static void apply_transition(Uint32 now) {
	int in_game = fx_in_game && !is_cutscene && drawn_room >= 1 && drawn_room <= 24;
	if (fx_transitions && in_game && last_room >= 1 && drawn_room != last_room) {
		memcpy(fade_from, last_frame, sizeof(frame));
		fade_start = now;
		fading = 1;
	}
	last_room = in_game ? drawn_room : 0;
	if (fading) {
		Uint32 dt = now - fade_start;
		if (dt >= FADE_MS || !in_game) {
			fading = 0;
		} else {
			int a = (int)(dt * 256 / FADE_MS); // weight of the new frame
			for (int i = 0; i < W * LIT_H; ++i) {
				Uint32 n = frame[i], o = fade_from[i];
				Uint32 rb = ((((n & 0xFF00FF) * a) + ((o & 0xFF00FF) * (256 - a))) >> 8) & 0xFF00FF;
				Uint32 g  = ((((n & 0x00FF00) * a) + ((o & 0x00FF00) * (256 - a))) >> 8) & 0x00FF00;
				frame[i] = rb | g;
			}
		}
	}
	memcpy(last_frame, frame, sizeof(frame));
}

// Scale2x / EPX (Andrea Mazzoleni's AdvMAME2x): keeps hard pixel art edges but rounds off diagonals.
static void scale2x(const Uint32* src, Uint32* dst, int w, int h) {
	for (int y = 0; y < h; ++y) {
		const Uint32* up = src + (y > 0 ? y - 1 : y) * w;
		const Uint32* row = src + y * w;
		const Uint32* dn = src + (y < h - 1 ? y + 1 : y) * w;
		Uint32* d0 = dst + (2 * y) * (2 * w);
		Uint32* d1 = d0 + 2 * w;
		for (int x = 0; x < w; ++x) {
			Uint32 e = row[x];
			Uint32 b = up[x], h_ = dn[x];
			Uint32 d = row[x > 0 ? x - 1 : x], f = row[x < w - 1 ? x + 1 : x];
			Uint32 e0 = e, e1 = e, e2 = e, e3 = e;
			if (b != h_ && d != f) {
				if (d == b) e0 = d;
				if (b == f) e1 = f;
				if (d == h_) e2 = d;
				if (h_ == f) e3 = f;
			}
			d0[2*x] = e0; d0[2*x+1] = e1;
			d1[2*x] = e2; d1[2*x+1] = e3;
		}
	}
}

static void build_crt_table(void) {
	const int ow = W * S, oh = H * S;
	for (int y = 0; y < oh; ++y) {
		float scan = (y % S == S - 1) ? 0.70f : (y % S == 0 ? 0.92f : 1.0f);
		float vy = (y - oh / 2.0f) / (oh / 2.0f);
		for (int x = 0; x < ow; ++x) {
			float vx = (x - ow / 2.0f) / (ow / 2.0f);
			float vig = 1.0f - 0.22f * (vx*vx + vy*vy) * (vx*vx + vy*vy) - 0.10f * (vx*vx + vy*vy);
			crt_mul[y * ow + x] = (Uint8)(255 * scan * vig);
		}
	}
	last_crt = fx_crt;
}

static void apply_crt(Uint32* buf, int n) {
	for (int i = 0; i < n; ++i) {
		Uint32 c = buf[i], m = crt_mul[i] + 1;
		buf[i] = ((((c & 0xFF00FF) * m) >> 8) & 0xFF00FF) | ((((c & 0x00FF00) * m) >> 8) & 0x00FF00);
	}
}

// Replaces the texture upload part of update_screen().
void fx_present(SDL_Surface* surface) {
	if (renderer_ == NULL) return;
	Uint32 now = SDL_GetTicks();
	last_present = now;
	fx_load_frame(surface);
	if (lighting_active()) apply_lighting();
	apply_transition(now);

	const Uint32* final = frame;
	int fw = W, fh = H;
	if (fx_upscale) {
		scale2x(frame, mid, W, H);
		scale2x(mid, out, W * 2, H * 2);
		final = out; fw = W * S; fh = H * S;
	} else if (fx_crt) {
		// CRT needs the big buffer: plain nearest-neighbor upscale.
		for (int y = 0; y < H * S; ++y)
			for (int x = 0; x < W * S; ++x)
				out[y * W * S + x] = frame[(y / S) * W + x / S];
		final = out; fw = W * S; fh = H * S;
	}
	if (fx_crt) {
		if (last_crt != fx_crt) build_crt_table();
		apply_crt(out, W * S * H * S);
	}

	static int tex_w;
	if (fx_texture == NULL || tex_w != fw) {
		if (fx_texture) SDL_DestroyTexture(fx_texture);
		// Linear filtering for the last (non-integer) stretch to the window; the 4x buffer is already sharp.
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, fw > W ? "1" : "0");
		fx_texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, fw, fh);
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
		tex_w = fw;
		if (fx_texture == NULL) {
			sdlperror("fx_present: SDL_CreateTexture");
			quit(1);
		}
	}
	SDL_UpdateTexture(fx_texture, NULL, final, fw * 4);
	SDL_RenderClear(renderer_);
	SDL_RenderCopy(renderer_, fx_texture, NULL, NULL);
	SDL_RenderPresent(renderer_);
}

// Called from the wait loops: keeps flicker and fades smooth (~60 fps) between game ticks.
void fx_idle(void) {
	if (!fx_enabled()) return;
	if (!fading && !lighting_active()) return;
	if (SDL_GetTicks() - last_present < 16) return;
	update_screen();
}

