static void canvas_pan_to_client(Monitor *m, Client *c);

// dir: 1=left, 2=right, 3=up, 4=down
static Client *canvas_chain_end(Monitor *m, uint32_t tag, Client *ref, int dir,
								int gap) {
	Client *c;
	int tolerance = 60;

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || c == ref || c->isunglobal)
			continue;
		if (c->canvas_geom[tag].width == 0 || c->canvas_geom[tag].height == 0)
			continue;

		int cx1 = c->canvas_geom[tag].x;
		int cy1 = c->canvas_geom[tag].y;
		int cx2 = cx1 + c->canvas_geom[tag].width;
		int cy2 = cy1 + c->canvas_geom[tag].height;
		int rx1 = ref->canvas_geom[tag].x;
		int ry1 = ref->canvas_geom[tag].y;
		int rx2 = rx1 + ref->canvas_geom[tag].width;
		int ry2 = ry1 + ref->canvas_geom[tag].height;

		bool overlaps_h = cx1 < rx2 && cx2 > rx1;
		bool overlaps_v = cy1 < ry2 && cy2 > ry1;

		bool adjacent = false;
		switch (dir) {
		case 1:
			adjacent = overlaps_v && abs(cx2 - (rx1 - gap)) < tolerance;
			break;
		case 2:
			adjacent = overlaps_v && abs(cx1 - (rx2 + gap)) < tolerance;
			break;
		case 3:
			adjacent = overlaps_h && abs(cy2 - (ry1 - gap)) < tolerance;
			break;
		case 4:
			adjacent = overlaps_h && abs(cy1 - (ry2 + gap)) < tolerance;
			break;
		}

		if (adjacent)
			return canvas_chain_end(m, tag, c, dir, gap);
	}

	return ref;
}

static void canvas_geom_init(Client *c, Monitor *m, uint32_t tag, float pan_x,
							 float pan_y, int *cascade_idx) {
	wlr_log(WLR_ERROR, "[canvas_debug] canvas_geom_init start: client=%p geom=%dx%d",
			c, c->geom.width, c->geom.height);
	int w = c->geom.width > 100 ? c->geom.width : 640;
	int h = c->geom.height > 100 ? c->geom.height : 480;
	float zoom = m->pertag->canvas_zoom[tag];
	wlr_log(WLR_ERROR, "[canvas_debug] canvas_geom_init: w=%d h=%d zoom=%.3f tiling=%d",
			w, h, zoom, config.canvas_tiling);

	int tiling = config.canvas_tiling;

	if (tiling > 0 && !client_is_float_type(c) && !client_get_parent(c) &&
		!c->is_in_scratchpad && !c->isnamedscratchpad && !c->canvas_notile) {
		Client *focused = NULL, *tmp;
		wl_list_for_each(tmp, &fstack, flink) {
			if (tmp == c || tmp->iskilling || tmp->isunglobal)
				continue;
			if (VISIBLEON(tmp, m)) {
				focused = tmp;
				break;
			}
		}
		float vp_w = m->w.width / zoom;
		float vp_h = m->w.height / zoom;
		bool focused_in_viewport =
			focused && focused->canvas_geom[tag].width > 0 &&
			focused->canvas_geom[tag].height > 0 &&
			focused->canvas_geom[tag].x + focused->canvas_geom[tag].width >
				pan_x &&
			focused->canvas_geom[tag].y + focused->canvas_geom[tag].height >
				pan_y &&
			focused->canvas_geom[tag].x < pan_x + vp_w &&
			focused->canvas_geom[tag].y < pan_y + vp_h;

		if (focused_in_viewport) {

			int dir = tiling;
			int gap = config.canvas_tiling_gap;

			if (tiling == 5) {
				float fx = focused->canvas_geom[tag].x;
				float fy = focused->canvas_geom[tag].y;
				float fw = focused->canvas_geom[tag].width;
				float fh = focused->canvas_geom[tag].height;

				float mouse_cx = pan_x + (cursor->x - m->w.x) / zoom;
				float mouse_cy = pan_y + (cursor->y - m->w.y) / zoom;

				float dl = fabsf(mouse_cx - fx);
				float dr = fabsf(mouse_cx - (fx + fw));
				float du = fabsf(mouse_cy - fy);
				float dd = fabsf(mouse_cy - (fy + fh));

				float min = dl;
				dir = 1;
				if (dr < min) {
					min = dr;
					dir = 2;
				}
				if (du < min) {
					min = du;
					dir = 3;
				}
				if (dd < min) {
					dir = 4;
				}
			}

			Client *end = canvas_chain_end(m, tag, focused, dir, gap);

			switch (dir) {
			case 1:
				c->canvas_geom[tag].x = end->canvas_geom[tag].x - w - gap;
				c->canvas_geom[tag].y = end->canvas_geom[tag].y;
				break;
			case 2:
				c->canvas_geom[tag].x =
					end->canvas_geom[tag].x + end->canvas_geom[tag].width + gap;
				c->canvas_geom[tag].y = end->canvas_geom[tag].y;
				break;
			case 3:
				c->canvas_geom[tag].x = end->canvas_geom[tag].x;
				c->canvas_geom[tag].y = end->canvas_geom[tag].y - h - gap;
				break;
			case 4:
				c->canvas_geom[tag].x = end->canvas_geom[tag].x;
				c->canvas_geom[tag].y = end->canvas_geom[tag].y +
										end->canvas_geom[tag].height + gap;
				break;
			}
			c->canvas_geom[tag].width = w;
			c->canvas_geom[tag].height = h;
			return;
		}
	}

	float cx = pan_x + (m->w.width / 2.0f) / zoom;
	float cy = pan_y + (m->w.height / 2.0f) / zoom;

	if (c->is_pending_open_animation) {
		c->canvas_geom[tag].x = (int32_t)(cx - w / 2.0f);
		c->canvas_geom[tag].y = (int32_t)(cy - h / 2.0f);
	} else {
		c->canvas_geom[tag].x = (int32_t)(cx + (*cascade_idx) * 30 - w / 2.0f);
		c->canvas_geom[tag].y = (int32_t)(cy + (*cascade_idx) * 30 - h / 2.0f);
		(*cascade_idx)++;
	}
	c->canvas_geom[tag].width = w;
	c->canvas_geom[tag].height = h;
	wlr_log(WLR_ERROR, "[canvas_debug] canvas_geom_init done: canvas_geom=%d,%d %dx%d",
			c->canvas_geom[tag].x, c->canvas_geom[tag].y, w, h);
}

static void canvas_reposition(Monitor *m) {
	wlr_log(WLR_ERROR, "[canvas_debug] canvas_reposition start");
	Client *c;
	uint32_t tag = m->pertag->curtag;

	float pan_x = m->pertag->canvas_pan_x[tag];
	float pan_y = m->pertag->canvas_pan_y[tag];
	float zoom = m->pertag->canvas_zoom[tag];

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || c->isunglobal)
			continue;
		if (c->isfullscreen || c->ismaximizescreen)
			continue;
		if (c->canvas_geom[tag].width <= 0 || c->canvas_geom[tag].height <= 0)
			continue;

		int new_x =
			m->w.x + (int32_t)roundf((c->canvas_geom[tag].x - pan_x) * zoom);
		int new_y =
			m->w.y + (int32_t)roundf((c->canvas_geom[tag].y - pan_y) * zoom);

		wlr_log(WLR_ERROR, "[canvas_debug] canvas_reposition: client=%p new_x=%d new_y=%d zoom=%.3f",
				c, new_x, new_y, zoom);

		if (c->animation.running) {
			c->animation.running = false;
			c->need_output_flush = false;
		}

		c->geom.x = new_x;
		c->geom.y = new_y;
		c->pending.x = new_x;
		c->pending.y = new_y;
		c->current.x = new_x;
		c->current.y = new_y;
		c->animation.current.x = new_x;
		c->animation.current.y = new_y;
		c->animation.initial.x = new_x;
		c->animation.initial.y = new_y;
		c->animainit_geom.x = new_x;
		c->animainit_geom.y = new_y;

		wlr_log(WLR_ERROR, "[canvas_debug] canvas_reposition: before wlr_scene_node_set_position client=%p", c);
		wlr_scene_node_set_position(&c->scene->node, new_x, new_y);
		wlr_log(WLR_ERROR, "[canvas_debug] canvas_reposition: after wlr_scene_node_set_position client=%p", c);

#ifdef XWAYLAND
		if (client_is_x11(c))
			client_set_size(c, c->geom.width - 2 * c->bw,
							c->geom.height - 2 * c->bw);
#endif

		if (zoom != 1.0f && !c->is_clip_to_hide) {
			wlr_log(WLR_ERROR, "[canvas_debug] canvas_reposition: before apply_canvas_zoom_correct client=%p zoom=%.3f", c, zoom);
			apply_canvas_zoom_correct(c, zoom);
			wlr_log(WLR_ERROR, "[canvas_debug] canvas_reposition: after apply_canvas_zoom_correct client=%p", c);
		}
	}
	wlr_log(WLR_ERROR, "[canvas_debug] canvas_reposition done");
}

static void canvas(Monitor *m) {
	wlr_log(WLR_ERROR, "[canvas_debug] canvas layout start");
	Client *c;
	uint32_t tag = m->pertag->curtag;

	float pan_x = m->pertag->canvas_pan_x[tag];
	float pan_y = m->pertag->canvas_pan_y[tag];
	float zoom = m->pertag->canvas_zoom[tag];

	wlr_log(WLR_ERROR, "[canvas_debug] canvas: tag=%u pan=%.1f,%.1f zoom=%.3f", tag, pan_x, pan_y, zoom);

	int cascade_idx = 0;
	Client *newly_tiled = NULL;

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || c->isunglobal)
			continue;

		if (c->isfullscreen || c->ismaximizescreen)
			continue;

		wlr_log(WLR_ERROR, "[canvas_debug] canvas: processing client=%p canvas_geom=%dx%d",
				c, c->canvas_geom[tag].width, c->canvas_geom[tag].height);

		if (c->canvas_geom[tag].width == 0 && c->canvas_geom[tag].height == 0) {
			wlr_log(WLR_ERROR, "[canvas_debug] canvas: calling canvas_geom_init for client=%p", c);
			canvas_geom_init(c, m, tag, pan_x, pan_y, &cascade_idx);
			if (config.canvas_tiling > 0 && !newly_tiled)
				newly_tiled = c;
		}

		int screen_x =
			m->w.x + (int32_t)roundf((c->canvas_geom[tag].x - pan_x) * zoom);
		int screen_y =
			m->w.y + (int32_t)roundf((c->canvas_geom[tag].y - pan_y) * zoom);

		float effective_zoom = zoom;
		int32_t base_w = c->canvas_geom[tag].width;
		int32_t base_h = c->canvas_geom[tag].height;

		if (m->canvas_in_overview && c->canvas_geom_backup[tag].width > 0) {
			base_w = c->canvas_geom_backup[tag].width;
			base_h = c->canvas_geom_backup[tag].height;
			effective_zoom *= (float)c->canvas_geom[tag].width / base_w;
		}

		struct wlr_box client_geom = {
			.x = screen_x,
			.y = screen_y,
			.width = base_w,
			.height = base_h,
		};
		wlr_log(WLR_ERROR, "[canvas_debug] canvas: before resize client=%p geom=%d,%d %dx%d effective_zoom=%.3f",
				c, screen_x, screen_y, base_w, base_h, effective_zoom);
		resize(c, client_geom, 0);
		wlr_log(WLR_ERROR, "[canvas_debug] canvas: after resize client=%p", c);

		if (effective_zoom == 1.0f) {
			wlr_log(WLR_ERROR, "[canvas_debug] canvas: before clear_visual_zoom client=%p", c);
			clear_visual_zoom(c);
			wlr_log(WLR_ERROR, "[canvas_debug] canvas: after clear_visual_zoom client=%p", c);
		} else {
			wlr_log(WLR_ERROR, "[canvas_debug] canvas: before apply_visual_zoom client=%p zoom=%.3f", c, effective_zoom);
			apply_visual_zoom(c, effective_zoom);
			wlr_log(WLR_ERROR, "[canvas_debug] canvas: after apply_visual_zoom client=%p", c);
		}
	}

	if (newly_tiled)
		canvas_pan_to_client(m, newly_tiled);
	wlr_log(WLR_ERROR, "[canvas_debug] canvas layout done");
}

static void canvas_pan_to_client(Monitor *m, Client *c) {
	if (!m || !c || !is_canvas_layout(m))
		return;

	uint32_t tag = m->pertag->curtag;
	if (c->canvas_geom[tag].width <= 0 || c->canvas_geom[tag].height <= 0)
		return;

	float zoom = m->pertag->canvas_zoom[tag];
	float pan_x = m->pertag->canvas_pan_x[tag];
	float pan_y = m->pertag->canvas_pan_y[tag];
	float vp_w = m->w.width / zoom;
	float vp_h = m->w.height / zoom;

	float cx = c->canvas_geom[tag].x;
	float cy = c->canvas_geom[tag].y;
	float cw = c->canvas_geom[tag].width;
	float ch = c->canvas_geom[tag].height;

	if (cx >= pan_x && cy >= pan_y && cx + cw <= pan_x + vp_w &&
		cy + ch <= pan_y + vp_h)
		return;

	m->pertag->canvas_pan_x[tag] = cx + cw / 2.0f - vp_w / 2.0f;
	m->pertag->canvas_pan_y[tag] = cy + ch / 2.0f - vp_h / 2.0f;
	canvas_reposition(m);
}
