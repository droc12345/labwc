// SPDX-License-Identifier: GPL-2.0-only

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#include "common/graphic-helpers.h"
#include "common/list.h"
#include "common/mem.h"
#include "labwc.h"
#include "regions.h"

bool
regions_available(void)
{
	return !wl_list_empty(&rc.regions);
}

void
regions_init(struct server *server, struct seat *seat)
{
	/* To be filled later */
}

struct region *
regions_from_name(const char *region_name, struct output *output)
{
	assert(region_name);
	assert(output);
	struct region *region;
	wl_list_for_each(region, &output->regions, link) {
		if (!strcmp(region->name, region_name)) {
			return region;
		}
	}
	return NULL;
}

struct region *
regions_from_cursor(struct server *server)
{
	assert(server);
	double lx = server->seat.cursor->x;
	double ly = server->seat.cursor->y;

	struct wlr_output *wlr_output = wlr_output_layout_output_at(
		server->output_layout, lx, ly);
	struct output *output = output_from_wlr_output(server, wlr_output);
	if (!output) {
		return NULL;
	}

	double dist;
	double dist_min = DBL_MAX;
	struct region *closest_region = NULL;
	struct region *region;
	wl_list_for_each(region, &output->regions, link) {
		if (wlr_box_contains_point(&region->geo, lx, ly)) {
			/* No need for sqrt((x1 - x2)^2 + (y1 - y2)^2) as we just compare */
			dist = pow(region->center.x - lx, 2) + pow(region->center.y - ly, 2);
			if (dist < dist_min) {
				closest_region = region;
				dist_min = dist;
			}
		}
	}
	return closest_region;
}

void
regions_update(struct output *output)
{
	assert(output);

	struct region *region;
	struct wlr_box usable = output_usable_area_in_layout_coords(output);

	/* Initialize regions */
	if (wl_list_empty(&output->regions)) {
		wl_list_for_each(region, &rc.regions, link) {
			struct region *region_new = znew(*region_new);
			/* Create a copy */
			region_new->name = xstrdup(region->name);
			region_new->percentage = region->percentage;
			wl_list_append(&output->regions, &region_new->link);
		}
	}

	/* Update regions */
	struct wlr_box *perc, *geo;
	wl_list_for_each(region, &output->regions, link) {
		geo = &region->geo;
		perc = &region->percentage;
		geo->x = usable.x + usable.width * perc->x / 100;
		geo->y = usable.y + usable.height * perc->y / 100;
		geo->width = usable.width * perc->width / 100;
		geo->height = usable.height * perc->height / 100;
		region->center.x = geo->x + geo->width / 2;
		region->center.y = geo->y + geo->height / 2;
	}
}

void
regions_destroy(struct wl_list *regions)
{
	assert(regions);
	struct region *region, *region_tmp;
	wl_list_for_each_safe(region, region_tmp, regions, link) {
		wl_list_remove(&region->link);
		zfree(region->name);
		zfree(region);
	}
}

