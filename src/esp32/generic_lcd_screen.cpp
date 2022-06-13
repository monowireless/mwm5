/* Copyright (C) 2019-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "generic_lcd_common.h"
#include "generic_lcd_screen.hpp"

extern bool g_quit_sdl_loop;

using namespace TWEARD;

void TWEARD::LcdScreen::drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const RGBA c32) {
	if (x1 > x2) {
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

	if (x1 == x2) {
		if (y1 > y2) std::swap(y1, y2);

		// fill vertical line
		for (; y1 <= y2; y1++) {
			ref_pt(x1, y1) = c32;
		}
	}
	else if (y1 == y2) {
		// fill horizontal line
		for (; x1 <= x2; x1++) {
			ref_pt(x1, y1) = c32;
		}
	}
	else {
		// Lind drawing by applying Bresenham's Line Algorithm.
		
		int32_t dx = x2 - x1;
		int32_t dy = y2 - y1;
		int32_t y0 = y1; // start position of Y-axis
		bool b_inv = false;

		// If dy is negative (y1>y2), process with y2 as the mirror position
		if (dy < 0) {
			b_inv = true;
			dy = -dy;
			y2 = y1 + dy;
		}

		// plot starting pixel (x1, y1)
		ref_pt(x1, y1) = c32; 

		// 45 degree diagonal line
		if (dx == dy) {
			for (; x1 <= x2; x1++, y1++) {
				ref_pt(x1, b_inv ? 2 * y0 - y1 : y1) = c32; // 2*y0-y1 is the mirror image position
			}
		}
		// Diagonal line less than 45 degrees (as in textbook)
		else if (dx > dy) {
			int32_t A = 2 * dy;
			int32_t B = A - 2 * dx;
			int32_t P = A - dx;

			while (true) {
				if (x1 >= x2 && y1 >= y2) {
					break;
				}

				if (P < 0) {
					P += A;
					x1++;
				}
				else {
					P += B;
					x1++;
					y1++;
				}

				ref_pt(x1, b_inv ? 2 * y0 - y1 : y1) = c32;
			}
		}
		// Diagonal line more than 45 degrees
		else {
			int32_t A = 2 * dx;
			int32_t B = A - 2 * dy;
			int32_t P = A - dy;

			while (true) {
				if (x1 >= x2 && y1 >= y2) {
					break;
				}

				if (P < 0) {
					P += A;
					y1++;
				}
				else {
					P += B;
					x1++;
					y1++;
				}

				ref_pt(x1, b_inv ? 2 * y0 - y1 : y1) = c32;
			}
		}
	}
}

void TWEARD::LcdScreen::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, const RGBA c32) {
	if (w == 0 || h == 0) return;

	if (w < 0) {
		x = x + w + 1;
		w = -w;
	}
	if (h < 0) {
		y = y + h + 1;
		h = -h;
	}
	
	int32_t xe = x + w - 1;
	int32_t ye = y + h - 1;

	// first line
	for (int32_t xw = x; xw <= xe; xw++) {
		ref_pt(xw, y) = c32;
	}

	for (; y < ye; y++) {
		ref_pt(x, y) = c32;
		ref_pt(xe, y) = c32;
	}
	
	// end line
	for (int32_t xw = x; xw <= xe; xw++) {
		ref_pt(xw, ye) = c32;
	}

}

void TWEARD::LcdScreen::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, const RGBA c32) {
	if (w == 0 || h == 0) return;

	if (w < 0) {
		x = x + w + 1;
		w = -w;
	}
	if (h < 0) {
		y = y + h + 1;
		h = -h;
	}

	int32_t xw = x, yw = y;

	for (; yw < y + h; yw++) {
		for (xw = x; xw < x + w; xw++) {
			ref_pt(xw, yw) = c32;
		}
	}
}

void TWEARD::LcdScreen::drawCircle(int32_t x_c, int32_t y_c, int32_t r, const RGBA c32) {
	int32_t E = -r;
	int32_t X = r;
	int32_t Y = 0;

	ref_pt(x_c + X, y_c) = c32;
	ref_pt(x_c - X, y_c) = c32;
	ref_pt(x_c, y_c + X) = c32;
	ref_pt(x_c, y_c - X) = c32;

	while (Y <= X) {
		E += 2 * Y + 1;
		Y++;
		if (E >= 0) {
			E -= (2 * X - 1);
			X--;
		}

		ref_pt(x_c + X, y_c + Y) = c32;
		ref_pt(x_c - X, y_c + Y) = c32;
		ref_pt(x_c + X, y_c - Y) = c32;
		ref_pt(x_c - X, y_c - Y) = c32;
		ref_pt(x_c + Y, y_c + X) = c32;
		ref_pt(x_c - Y, y_c + X) = c32;
		ref_pt(x_c + Y, y_c - X) = c32;
		ref_pt(x_c - Y, y_c - X) = c32;
	}
}

void TWEARD::LcdScreen::fillCircle(int32_t x_c, int32_t y_c, int32_t r, const RGBA c32) {
	int32_t E = -r;
	int32_t X = r;
	int32_t Y = 0;

	ref_pt(x_c, y_c + X) = c32;
	ref_pt(x_c, y_c - X) = c32;
	
	drawLine(x_c - X, y_c, x_c + X, y_c, c32);

	while (Y <= X) {
		E += 2 * Y + 1;
		Y++;
		if (E >= 0) {
			E -= (2 * X - 1);
			X--;
		}

		drawLine(x_c - X, y_c + Y, x_c + X, y_c + Y, c32);
		drawLine(x_c - X, y_c - Y, x_c + X, y_c - Y, c32);
		drawLine(x_c - Y, y_c + X, x_c + Y, y_c + X, c32);
		drawLine(x_c - Y, y_c - X, x_c + Y, y_c - X, c32);
	}
}