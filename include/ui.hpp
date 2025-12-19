#pragma once
#include "uix.hpp"
#include "gfx.hpp"
#include "lcd_config.h"
// screen colors
using color_t = gfx::color<LCD_FRAME_ADAPTER::pixel_type>;
// UI colors (UIX)
using ucolor_t = gfx::color<uix::uix_pixel>;
// vector graphics colors
using vcolor_t = gfx::color<gfx::vector_pixel>;

using screen_t = uix::screen_ex<LCD_FRAME_ADAPTER,LCD_X_ALIGN,LCD_Y_ALIGN>;
using surface_t = screen_t::control_surface_type;

extern gfx::const_buffer_stream& text_font;