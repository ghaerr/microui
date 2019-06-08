/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Microui renderer for Nano-X
 */
#include <stdio.h>
#include "microui.h"
#include "renderer.h"
#include "nano-X.h"

static int width  = 800;
static int height = 600;

static GR_WINDOW_ID wid;
static GR_GC_ID gc;
static GR_FONT_ID fontid;
static GR_REGION_ID clip;
static int font_ascent, font_descent, font_height;
static int timeout = 20;	/* initial event wait timeout*/

void r_init(void)
{
	GR_FONT_INFO finfo;

	if (GrOpen() < 0) {
		fprintf(stderr, "Can't open graphics\n");
		exit(1);
	}

	wid = GrNewBufferedWindow(GR_WM_PROPS_APPWINDOW, "Microui", GR_ROOT_WINDOW_ID,
  		0, 0, width, height, 0);

	gc = GrNewGC();
	fontid = GrCreateFontEx(GR_FONT_SYSTEM_VAR, 0, 0, NULL);
	GrSetGCUseBackground(gc, GR_FALSE);
	GrSetGCFont(gc, fontid);

	GrGetFontInfo(fontid, &finfo);
	font_ascent = finfo.baseline;
	font_descent = finfo.descent;
	font_height = finfo.height;

	GrSelectEvents(wid, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_UPDATE |
		GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP |
		GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP |
		GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(wid);
}

static unsigned long nx_color(mu_Color color)
{
    return GR_RGB(color.r, color.g, color.b);
}

void r_draw_rect(mu_Rect rect, mu_Color color) {
    unsigned long c = nx_color(color);
    GrSetGCForeground(gc, c);
	GrFillRect(wid, gc, rect.x, rect.y, rect.w, rect.h);
}


void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color)
{
    int tx, ty;
    unsigned long fg = nx_color(color);

    GrSetGCForeground(gc, fg);
    tx = pos.x;
    ty = pos.y + font_ascent;
    GrSetGCForeground(gc, fg);
    GrText(wid, gc, tx, ty, (void*)text, -1, GR_TFBASELINE);
}


void r_draw_icon(int id, mu_Rect rect, mu_Color color) {
	int c, w, h;
	mu_Vec2 pos;
	char buf[2];
	switch (id) {
	case MU_ICON_CLOSE:		c = 'x'; break;
	case MU_ICON_CHECK:		c = 'X'; break;
	case MU_ICON_COLLAPSED:	c = '>'; break;
	case MU_ICON_EXPANDED:	c = 'v'; break;
	case MU_ICON_RESIZE:	c = '+'; break;
  	}
	buf[0] = c;
	buf[1] = 0;
	w = r_get_text_width(buf, 1);
	h = r_get_text_height();
	pos.x = rect.x + (rect.w - w) / 2;
	pos.y = rect.y + (rect.h - h) / 2;
	r_draw_text(buf, pos, color);
}

int r_get_text_width(const char *text, int len)
{
	int w, h, b;

	GrGetGCTextSize(gc, (void *)text, len, 0, &w, &h, &b);
	return w;
}


int r_get_text_height(void)
{
	return font_height;
}


void r_set_clip_rect(mu_Rect rect)
{
    GR_RECT clip_rect;
    clip_rect.x = rect.x - 1;
    clip_rect.y = rect.y - 1;
    clip_rect.width = rect.w + 2;
    clip_rect.height = rect.h + 2;
	if (clip)
		GrDestroyRegion(clip);
	clip = GrNewRegion();
	GrUnionRectWithRegion(clip, &clip_rect);
	GrSetGCRegion(gc, clip);
}


void r_clear(mu_Color clr)
{
	mu_Rect r = { 0, 0, width, height };
	r_draw_rect(r, clr);
}


void r_present(void)
{
	GrFlushWindow(wid);
	GrFlush();
}

static void r_input_key(mu_Context *ctx, int key, int down)
{
	if (down)
		mu_input_keydown(ctx, key);
	else
		mu_input_keyup(ctx, key);
}

static int r_handle_event(mu_Context *ctx, GR_EVENT *evt)
{
	if (evt->type == GR_EVENT_TYPE_NONE)
		return 0;
    if (evt->type == GR_EVENT_TYPE_KEY_DOWN || evt->type == GR_EVENT_TYPE_KEY_UP)
    {
        int down = (evt->type == GR_EVENT_TYPE_KEY_DOWN);
		MWKEY code = evt->keystroke.ch;
        if (code == MWKEY_LSHIFT || code == MWKEY_RSHIFT)
		                                  r_input_key(ctx, MU_KEY_SHIFT, down);
        else if (code == MWKEY_LCTRL || code == MWKEY_RCTRL)
		                                  r_input_key(ctx, MU_KEY_CTRL, down);
        else if (code == MWKEY_LALT || code == MWKEY_RALT)
		                                  r_input_key(ctx, MU_KEY_ALT, down);
        else if (code == MWKEY_ENTER)     r_input_key(ctx, MU_KEY_RETURN, down);
        else if (code == MWKEY_BACKSPACE) r_input_key(ctx, MU_KEY_BACKSPACE, down);
        else if (down && code < 256) {
			char buf[2];
			buf[0] = (char)code;
			buf[1] = 0;
            mu_input_text(ctx, buf);
        }
        return 1;
    }
	if (evt->type == GR_EVENT_TYPE_BUTTON_DOWN || evt->type == GR_EVENT_TYPE_BUTTON_UP)
	{
        int down = (evt->type == GR_EVENT_TYPE_BUTTON_DOWN);
        const int x = evt->button.x, y = evt->button.y;
        if ((evt->button.buttons & GR_BUTTON_SCROLLUP) && down)
            mu_input_scroll(ctx, 0, -30);
        else if ((evt->button.buttons & GR_BUTTON_SCROLLDN) && down)
            mu_input_scroll(ctx, 0, 30);
        else if ((evt->button.buttons & GR_BUTTON_L) ||
		         (evt->button.changebuttons & GR_BUTTON_L))
		{
			if (down)
            	mu_input_mousedown(ctx, x, y, MU_MOUSE_LEFT);
			else
            	mu_input_mouseup(ctx, x, y, MU_MOUSE_LEFT);
        }
		else if ((evt->button.buttons & GR_BUTTON_M) ||
		         (evt->button.changebuttons & GR_BUTTON_M)) {
			if (down)
            	mu_input_mousedown(ctx, x, y, MU_MOUSE_MIDDLE);
			else
            	mu_input_mouseup(ctx, x, y, MU_MOUSE_MIDDLE);
        } else if ((evt->button.buttons & GR_BUTTON_R) ||
		           (evt->button.changebuttons & GR_BUTTON_R)) {
			if (down)
            	mu_input_mousedown(ctx, x, y, MU_MOUSE_RIGHT);
			else
            	mu_input_mouseup(ctx, x, y, MU_MOUSE_RIGHT);
        } else return 0;
        return 1;
    }
	if (evt->type == GR_EVENT_TYPE_MOUSE_POSITION)
	{
        const int x = evt->mouse.x, y = evt->mouse.y;
        mu_input_mousemove(ctx, x, y);
		/* allow ui to run polling to reset scrollbars, then wait*/
		timeout = 0;
        return 1;
    }
	if (evt->type == GR_EVENT_TYPE_UPDATE)
	{
        /* Window resize handler */
		//if (evt->update.wid == wid && evt->update.utype == GR_UPDATE_SIZE)
        	//nk_nxlib_resize_window(evt->update.width, evt->update.height);
        //return 1;
    }

    return 0;
}

void r_handle_input(mu_Context *ctx)
{
    GR_EVENT evt;
	
	do {
		GrGetNextEventTimeout(&evt, timeout);
		r_handle_event(ctx, &evt);
		/*
		 * Break required because below GrPeekEvent generates
		 * another timeout in LINK_APP_INTO_SERVER case
		 */
		if (evt.type == GR_EVENT_TYPE_TIMEOUT)
			break;
		if (evt.type == GR_EVENT_TYPE_CLOSE_REQ) {
			GrClose();
			exit(0);
		}
	} while (GrPeekEvent(&evt));
}
