#pragma once
#include "uix.hpp"
#include "gfx.hpp"
#include "panel.h"
#if LCD_COLOR_SPACE == LCD_COLOR_GSC
using screen_pixel_t = gfx::gsc_pixel<LCD_BIT_DEPTH>;
#else
using screen_pixel_t = gfx::rgb_pixel<LCD_BIT_DEPTH>;
#endif
// screen colors
using color_t = gfx::color<screen_pixel_t>;
// UI colors (UIX)
using ucolor_t = gfx::color<uix::uix_pixel>;
// vector graphics colors
using vcolor_t = gfx::color<gfx::vector_pixel>;

using screen_t = uix::screen_ex<gfx::bitmap<screen_pixel_t>,LCD_X_ALIGN,LCD_Y_ALIGN>;
using surface_t = screen_t::control_surface_type;

extern gfx::const_buffer_stream& text_font;