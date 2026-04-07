int32_t gesture_calculate_swipe_within_degrees(double gestdegrees,
											   double wantdegrees) {
	return (gestdegrees >= wantdegrees - config.touch_degrees_leniency &&
			gestdegrees <= wantdegrees + config.touch_degrees_leniency);
}

uint32_t gesture_calculate_swipe(double x0, double y0, double x1, double y1) {
	double t, degrees, distance;

	t = atan2(x1 - x0, y0 - y1);
	degrees = 57.2957795130823209 * (t < 0 ? t + 6.2831853071795865 : t);
	distance = sqrt(pow(x1 - x0, 2) + pow(y1 - y0, 2));

	wlr_log(WLR_DEBUG, "Swipe distance=[%.2f]; degrees=[%.2f]", distance,
			degrees);

	if (distance < config.touch_distance_threshold)
		return TOUCH_SWIPE_NONE;
	else if (gesture_calculate_swipe_within_degrees(degrees, 0))
		return TOUCH_SWIPE_UP;
	else if (gesture_calculate_swipe_within_degrees(degrees, 45))
		return TOUCH_SWIPE_UP_RIGHT;
	else if (gesture_calculate_swipe_within_degrees(degrees, 90))
		return TOUCH_SWIPE_RIGHT;
	else if (gesture_calculate_swipe_within_degrees(degrees, 135))
		return TOUCH_SWIPE_DOWN_RIGHT;
	else if (gesture_calculate_swipe_within_degrees(degrees, 180))
		return TOUCH_SWIPE_DOWN;
	else if (gesture_calculate_swipe_within_degrees(degrees, 225))
		return TOUCH_SWIPE_DOWN_LEFT;
	else if (gesture_calculate_swipe_within_degrees(degrees, 270))
		return TOUCH_SWIPE_LEFT;
	else if (gesture_calculate_swipe_within_degrees(degrees, 315))
		return TOUCH_SWIPE_UP_LEFT;
	else if (gesture_calculate_swipe_within_degrees(degrees, 360))
		return TOUCH_SWIPE_UP;

	return TOUCH_SWIPE_NONE;
}

uint32_t gesture_calculate_distance(Monitor *m, double x0, double y0, double x1,
									double y1, int32_t swipe) {
	double distance = sqrt(pow(x1 - x0, 2) + pow(y1 - y0, 2));
	double diag = sqrt(pow(m->m.width, 2) + pow(m->m.height, 2));
	switch (swipe) {
	case TOUCH_SWIPE_UP:
	case TOUCH_SWIPE_DOWN:
		if (distance >= m->m.height * 0.66) {
			return DISTANCE_LONG;
		} else if (distance >= m->m.height * 0.33) {
			return DISTANCE_MEDIUM;
		} else {
			return DISTANCE_SHORT;
		}
		break;
	case TOUCH_SWIPE_RIGHT:
	case TOUCH_SWIPE_LEFT:
		if (distance >= m->m.width * 0.66) {
			return DISTANCE_LONG;
		} else if (distance >= m->m.width * 0.33) {
			return DISTANCE_MEDIUM;
		} else {
			return DISTANCE_SHORT;
		}
		break;
	case TOUCH_SWIPE_UP_RIGHT:
	case TOUCH_SWIPE_UP_LEFT:
	case TOUCH_SWIPE_DOWN_RIGHT:
	case TOUCH_SWIPE_DOWN_LEFT:
		if (distance >= diag * 0.66) {
			return DISTANCE_LONG;
		} else if (distance >= diag * 0.33) {
			return DISTANCE_MEDIUM;
		} else {
			return DISTANCE_SHORT;
		}
		break;
	}

	return 0;
}

uint32_t gesture_calculate_edge(Monitor *m, double x0, double y0, double x1,
								double y1) {
	uint32_t horizontal = EDGE_NONE;
	uint32_t vertical = EDGE_NONE;

	if (x0 <= config.touch_edge_size_left) {
		horizontal = EDGE_LEFT;
	} else if (x0 >= m->m.width - config.touch_edge_size_right) {
		horizontal = EDGE_RIGHT;
	} else if (x1 <= config.touch_edge_size_left) {
		horizontal = EDGE_LEFT;
	} else if (x1 >= m->m.width - config.touch_edge_size_right) {
		horizontal = EDGE_RIGHT;
	}
	if (y0 <= config.touch_edge_size_top) {
		vertical = EDGE_TOP;
	} else if (y0 >= m->m.height - config.touch_edge_size_bottom) {
		vertical = EDGE_BOTTOM;
	} else if (y1 <= config.touch_edge_size_top) {
		vertical = EDGE_TOP;
	} else if (y1 >= m->m.height - config.touch_edge_size_bottom) {
		vertical = EDGE_BOTTOM;
	}
	if (horizontal == EDGE_LEFT && vertical == EDGE_TOP) {
		return CORNER_TOP_LEFT;
	} else if (horizontal == EDGE_RIGHT && vertical == EDGE_TOP) {
		return CORNER_TOP_RIGHT;
	} else if (horizontal == EDGE_LEFT && vertical == EDGE_BOTTOM) {
		return CORNER_BOTTOM_LEFT;
	} else if (horizontal == EDGE_RIGHT && vertical == EDGE_BOTTOM) {
		return CORNER_BOTTOM_RIGHT;
	} else if (horizontal != EDGE_NONE) {
		return horizontal;
	} else {
		return vertical;
	}
}

int32_t gesture_execute(int32_t nfingers, uint32_t swipe, uint32_t edge,
						uint32_t distance) {
	int32_t i;
	int32_t handled = 0;

	wlr_log(WLR_DEBUG, "f:%d s:%d e:%d d:%d", nfingers, swipe, edge, distance);

	TouchGestureBinding *g;
	for (i = 0; i < config.touch_gesture_bindings_count; i++) {
		g = &config.touch_gesture_bindings[i];
		if (swipe == g->swipe && nfingers == g->fingers_count &&
			(distance == g->distance || g->distance == DISTANCE_ANY) &&
			(g->edge == EDGE_ANY || edge == g->edge ||
			 ((edge == CORNER_TOP_LEFT || edge == CORNER_TOP_RIGHT) &&
			  g->edge == EDGE_TOP) ||
			 ((edge == CORNER_BOTTOM_LEFT || edge == CORNER_BOTTOM_RIGHT) &&
			  g->edge == EDGE_BOTTOM) ||
			 ((edge == CORNER_TOP_LEFT || edge == CORNER_BOTTOM_LEFT) &&
			  g->edge == EDGE_LEFT) ||
			 ((edge == CORNER_TOP_RIGHT || edge == CORNER_BOTTOM_RIGHT) &&
			  g->edge == EDGE_RIGHT)) &&
			g->func) {
			g->func(&g->arg);
			handled = 1;
		}
	}
	return handled;
}

void gesture_consume(TouchGroup *tg, TouchPoint *t) {
	if (t->consumed_by_gesture)
		return;

	struct timespec now;
	struct wlr_touch_cancel_event *event =
		ecalloc(1, sizeof(struct wlr_touch_cancel_event));

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	event->touch_id = t->touch_id;
	event->touch = tg->touch;
	event->time_msec = now.tv_sec * 1000 + now.tv_nsec / 1000000;

	wlr_log(WLR_DEBUG, "gesture_consume id: %d", t->touch_id);

	t->consumed_by_gesture = true;

	handle_touchcancel(event);

	free(event);
}

void gesture_touch_down(TouchGroup *tg, TouchPoint *t, double x, double y) {
	wlr_log(WLR_DEBUG, "touch_down id: %d", t->touch_id);

	t->end_x = x;
	t->end_y = y;

	if (wl_list_empty(&tg->touch_points))
		clock_gettime(CLOCK_MONOTONIC_RAW, &tg->time_down);
}

void gesture_touch_motion(TouchGroup *tg, TouchPoint *t, double x, double y) {
	t->end_x = x;
	t->end_y = y;
}

void gesture_touch_up(TouchGroup *tg, TouchPoint *t) {
	struct timespec now;

	wlr_log(WLR_DEBUG, "touch_up id: %d", t->touch_id);
	wlr_log(WLR_DEBUG, "len: %d", wl_list_length(&tg->touch_points));

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	uint32_t swipe =
		gesture_calculate_swipe(t->start_x, t->start_y, t->end_x, t->end_y);

	if (swipe == TOUCH_SWIPE_NONE) {
		goto cleanup;
	}

	if (tg->touch_points_pending_swipe == 0) {
		tg->pending_swipe = swipe;
	}
	if (tg->pending_swipe != swipe) {
		goto cleanup;
	}

	tg->touch_points_pending_swipe++;

	if (wl_list_length(&tg->touch_points) == 1) {
		uint32_t edge = gesture_calculate_edge(tg->m, t->start_x, t->start_y,
											   t->end_x, t->end_y);
		uint32_t distance = gesture_calculate_distance(
			tg->m, t->start_x, t->start_y, t->end_x, t->end_y, swipe);

		if (config.touch_timeoutms >
			((now.tv_sec - tg->time_down.tv_sec) * 1000 +
			 (now.tv_nsec - tg->time_down.tv_nsec) / 1000000)) {
			if (gesture_execute(tg->touch_points_pending_swipe,
								tg->pending_swipe, edge, distance))
				gesture_consume(tg, t);

			tg->touch_points_pending_swipe = 0;
		}
		return;
	}

cleanup:
	if (wl_list_length(&tg->touch_points) == 1)
		tg->touch_points_pending_swipe = 0;
}
