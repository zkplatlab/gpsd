/****************************************************************************

NAME
   libgps_json.c - deserialize gpsd data coming from the server

DESCRIPTION
   This module uses the generic JSON parser to get data from JSON
representations to libgps structures.

***************************************************************************/

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "gpsd_config.h"
#include "gpsd.h"
#include "gps_json.h"

static int json_tpv_read(const char *buf, 
			 struct gps_data_t *gpsdata, const char **endptr)
{
    int status;
    const struct json_attr_t json_attrs_1[] = {
	{"class",  check,   .dflt.check = "TPV"},
	{"device", string,  .addr.string.ptr = gpsdata->gps_device,
			    .addr.string.len = sizeof(gpsdata->gps_device)},
	{"tag",    string,  .addr.string.ptr = gpsdata->tag,
			    .addr.string.len = sizeof(gpsdata->tag)},
	{"time",   real,    .addr.real = &gpsdata->fix.time,
			    .dflt.real = NAN},
	{"ept",    real,    .addr.real = &gpsdata->fix.ept,
			    .dflt.real = NAN},
	{"lon",    real,    .addr.real = &gpsdata->fix.longitude,
			    .dflt.real = NAN},
	{"lat",    real,    .addr.real = &gpsdata->fix.latitude,
			    .dflt.real = NAN},
	{"alt",    real,    .addr.real = &gpsdata->fix.altitude,
			    .dflt.real = NAN},
	{"eph",    real,    .addr.real = &gpsdata->fix.eph,
			    .dflt.real = NAN},
	{"epv",    real,    .addr.real = &gpsdata->fix.epv,
			    .dflt.real = NAN},
	{"track",   real,   .addr.real = &gpsdata->fix.track,
			    .dflt.real = NAN},
	{"speed",   real,   .addr.real = &gpsdata->fix.speed,
			    .dflt.real = NAN},
	{"climb",   real,   .addr.real = &gpsdata->fix.climb,
			    .dflt.real = NAN},
	{"epd",    real,    .addr.real = &gpsdata->fix.epd,
			    .dflt.real = NAN},
	{"eps",    real,    .addr.real = &gpsdata->fix.eps,
			    .dflt.real = NAN},
	{"epc",    real,    .addr.real = &gpsdata->fix.epc,
			    .dflt.real = NAN},
	{"mode",   integer, .addr.integer = &gpsdata->fix.mode,
			    .dflt.integer = MODE_NOT_SEEN},
	{NULL},
    };

    status = json_read_object(buf, json_attrs_1, 0, endptr);

    if (status == 0) {
	gpsdata->status = STATUS_FIX;
	gpsdata->set = STATUS_SET;
	if (isnan(gpsdata->fix.time) == 0)
	    gpsdata->set |= TIME_SET;
	if (isnan(gpsdata->fix.ept) == 0)
	    gpsdata->set |= TIMERR_SET;
	if (isnan(gpsdata->fix.longitude) == 0)
	    gpsdata->set |= LATLON_SET;
	if (isnan(gpsdata->fix.altitude) == 0)
	    gpsdata->set |= ALTITUDE_SET;
	if (isnan(gpsdata->fix.eph)==0)
	    gpsdata->set |= HERR_SET;
	if (isnan(gpsdata->fix.epv)==0)
	    gpsdata->set |= VERR_SET;
	if (isnan(gpsdata->fix.track)==0)
	    gpsdata->set |= TRACK_SET;
	if (isnan(gpsdata->fix.speed)==0)
	    gpsdata->set |= SPEED_SET;
	if (isnan(gpsdata->fix.climb)==0)
	    gpsdata->set |= CLIMB_SET;
	if (isnan(gpsdata->fix.epd)==0)
	    gpsdata->set |= TRACKERR_SET;
	if (isnan(gpsdata->fix.eps)==0)
	    gpsdata->set |= SPEEDERR_SET;
	if (isnan(gpsdata->fix.epc)==0)
	    gpsdata->set |= CLIMBERR_SET;
	if (isnan(gpsdata->fix.epc)==0)
	    gpsdata->set |= CLIMBERR_SET;
	if (gpsdata->fix.mode != MODE_NOT_SEEN)
	    gpsdata->set |= MODE_SET;
    }
    return status;
}

static int json_sky_read(const char *buf, 
			 struct gps_data_t *gpsdata, const char **endptr)
{
    bool usedflags[MAXCHANNELS];
    const struct json_attr_t json_attrs_2_1[] = {
	{"PRN",	   integer, .addr.integer = gpsdata->PRN},
	{"el",	   integer, .addr.integer = gpsdata->elevation},
	{"az",	   integer, .addr.integer = gpsdata->azimuth},
	{"ss",	   real,    .addr.real = gpsdata->ss},
	{"used",   boolean, .addr.boolean = usedflags},
	{NULL},
    };
    const struct json_attr_t json_attrs_2[] = {
	{"class",      check,   .dflt.check = "SKY"},
	{"device",     string,  .addr.string.ptr  = gpsdata->gps_device,
				.addr.string.len = PATH_MAX},
	{"tag",	       string,  .addr.string.ptr  = gpsdata->tag,
				.addr.string.len = MAXTAGLEN},
	{"time",       real,    .addr.real    = &gpsdata->fix.time},
	{"reported",   integer, .addr.integer = &gpsdata->satellites_used},
	{"satellites", array,   .addr.array.element_type = object,
				.addr.array.arr.subtype = json_attrs_2_1,
				.addr.array.maxlen = MAXCHANNELS},
	{NULL},
    };
    int status, i, j;

    for (i = 0; i < MAXCHANNELS; i++)
	usedflags[i] = false;

    status = json_read_object(buf, json_attrs_2, 0, endptr);
    if (status != 0)
	return status;

    for (i = j = 0; i < MAXCHANNELS; i++) {
	if (usedflags[i]) {
	    gpsdata->used[j++] = gpsdata->PRN[i];
	}
    }

    gpsdata->set |= SATELLITE_SET;
    return 0;
}

static int json_device_read(const char *buf, 
			     struct gps_data_t *gpsdata, const char **endptr)
{
    const struct json_attr_t json_attrs_device[] = {
	{"class",      check,   .dflt.check = "DEVICE"},
	{"path",       string,  .addr.string.ptr  = gpsdata->gps_device,
				.addr.string.len = PATH_MAX},
	{"activated",  real,    .addr.real    = &gpsdata->fix.time},
	// type (list), driver, subtype
	{NULL},
    };
    int status;

    status = json_read_object(buf, json_attrs_device, 0, endptr);
    if (status != 0)
	return status;

    gpsdata->set |= DEVICE_SET;
    return 0;
}

#ifdef __UNUSED_
static int json_devicelist_read(const char *buf, 
			     struct gps_data_t *gpsdata, const char **endptr)
{
    char names[GPS_JSON_DEVICES_MAX][PATH_MAX];
    const struct json_attr_t json_attrs_subdevices[] = {
	// FIXME: Parse device records, too.
	{"name",   string,  .addr.string.ptr = gpsdata->gps_device,
			    .addr.string.len = sizeof(gpsdata->gps_device)},
	{NULL},
    };
    // FIXME: Can we abolish the hard limit on the number of devices? 
    const struct json_attr_t json_attrs_devices[] = {
        {"class",      check,   .dflt.check = "DEVICES"},
	{"devices",    array,   .addr.array.element_type = object,
				.addr.array.arr.subtype = json_attrs_subdevices,
				.addr.array.maxlen = GPS_JSON_DEVICES_MAX},
	{NULL},
    };
    int status;

    status = json_read_object(buf, json_attrs_devices, 0, endptr);
    if (status != 0)
	return status;

    gpsdata->devicelist_time = timestamp();
    gpsdata->set |= DEVICELIST_SET;
    gpsdata->ndevices = *json_attrs_devices[0].addr.array.count;
    return 0;
}
#endif

int libgps_json_unpack(const char *buf, struct gps_data_t *gpsdata)
/* the only entry point - unpack a JSON object into C structs */
{
    if (strstr(buf, "\"class\":\"TPV\"") != 0) {
	return json_tpv_read(buf, gpsdata, NULL);
    } else if (strstr(buf, "\"class\":\"SKY\"") != 0) {
	return json_sky_read(buf, gpsdata, NULL);
    } else if (strstr(buf, "\"class\":\"DEVICE\"") != 0) {
	return json_device_read(buf, gpsdata, NULL);
    } else
	return -1;
}

/* libgps_json.c ends here */