/* $Id$ */
/*
 * gpsctrl.c -- tweak the control settings on a GPS
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "gpsd_config.h"
#include "gpsd.h"

static int debuglevel;

void gpsd_report(int errlevel UNUSED, const char *fmt, ... )
/* our version of the logger */
{
    if (errlevel <= debuglevel) {
	va_list ap;
	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
    }
}

static gps_mask_t get_packet(struct gps_device_t *session)
/* try to get a well-formed packet from the GPS */
{
    gps_mask_t fieldmask;

    for (;;) {
	int waiting = 0;
	(void)ioctl(session->gpsdata.gps_fd, FIONREAD, &waiting);
	if (waiting == 0) {
	    usleep(300);
	    continue;
	}
	fieldmask = gpsd_poll(session);
	if ((fieldmask &~ ONLINE_SET)!=0)
	    return fieldmask;
    }
}

int main(int argc, char **argv)
{
    int option, status;
    char *err_str, *device = NULL, *speed = NULL;
    bool to_binary = false, to_nmea = false, lowlevel=false;
    struct gps_data_t *gpsdata;

#define USAGE	"usage: gpsctrl [-b | -n] [-s speed] [-V] <device>\n"
    while ((option = getopt(argc, argv, "bfhns:D:V")) != -1) {
	switch (option) {
	case 'b':
	    to_binary = true;
	    break;
	case 'f':
	    lowlevel = true;	/* force direct access to the deamon */
	    break;
	case 'n':
	    to_nmea = true;
	    break;
	case 's':
	    speed = optarg;
	    break;
	case 'D':
	    debuglevel = atoi(optarg);
	    break;
	case 'V':
	    (void)fprintf(stderr, "gpsctrl at svn revision $Rev$\n");
	    break;
	case 'h':
	default:
	    fprintf(stderr, USAGE);
	    break;
	}
    }

    if (optind < argc)
	device = argv[optind];

    if (!to_nmea && !to_binary && speed==NULL) {
	(void)fprintf(stderr, USAGE);
	exit(0);
    }

    if (to_nmea && to_binary) {
	(void)fprintf(stderr, "gpsctrl: make up your mind, would you?\n");
	exit(0);
    }

    if (speed==NULL && !to_nmea && !to_binary)
	exit(0);

    if (!lowlevel) {
	/* Try to open the stream to gpsd. */
	/*@i@*/gpsdata = gps_open(NULL, NULL);
	if (gpsdata == NULL) {
	    switch (errno) {
	    case NL_NOSERVICE: err_str ="can't get service entry"; break;
	    case NL_NOHOST:    err_str ="can't get host entry"; break;
	    case NL_NOPROTO:   err_str ="can't get protocol entry"; break;
	    case NL_NOSOCK:    err_str ="can't create socket"; break;
	    case NL_NOSOCKOPT: err_str ="error SETSOCKOPT SO_REUSEADDR"; break;
	    case NL_NOCONNECT: err_str ="can't connect to host"; break;
	    default:           err_str ="Unknown"; break;
	    } 
	    (void)fprintf(stderr, 
			  "gpsctrl: no gpsd running or network error: %d, %s\n", 
			  errno, err_str);
	    lowlevel = true;
	}
    }

    if (!lowlevel) {
	/* OK, there's a daemon instance running.  Do things the easy way */
	(void)gps_query(gpsdata, "K\n");
	if (gpsdata->ndevices == 0) {
	    (void)fprintf(stderr, "gpsctrl: no devices connected.\n"); 
	    (void)gps_close(gpsdata);
	    exit(1);
	} else if (gpsdata->ndevices > 1 && device == NULL) {
	    (void)fprintf(stderr, 
			  "gpsctrl: multiple devices and no device specified.\n");
	    (void)gps_close(gpsdata);
	    exit(1);
	}
	gpsd_report(1, "gpsctrl: %d device(s) found.\n", gpsdata->ndevices);

	if (gpsdata->ndevices > 1) {
	    int i;
	    assert(device != NULL);
	    for (i = 0; i < gpsdata->ndevices; i++)
		if (strcmp(device, gpsdata->devicelist[i]) == 0)
		    goto foundit;
	    (void)fprintf(stderr,  "gpsctrl: specified device not found.\n");
	    (void)gps_close(gpsdata);
	    exit(1);
	foundit:
	    (void)gps_query(gpsdata, "F=%s", device);
	}

	status = 0;
	if (to_nmea) {
	    (void)gps_query(gpsdata, "N=0");
	    if (gpsdata->driver_mode != 0) {
		(void)fprintf(stderr, "gpsctrl: mode change failed\n");
		status = 1;
	    } else
		gpsd_report(1, "gpsctrl: mode change on %s succeeded\n", gpsdata->gps_device);
	}
	else if (to_binary) {
	    (void)gps_query(gpsdata, "N=1");
	    if (gpsdata->driver_mode != 1) {
		(void)fprintf( stderr, "gpsctrl: mode change failed\n");
		status = 1;
	    } else
		gpsd_report(1, "gpsctrl: mode change on %s succeeded\n", gpsdata->gps_device);
	}
	if (speed != NULL) {
	    (void)gps_query(gpsdata, "B=%s", speed);
	    if (atoi(speed) != (int)gpsdata->baudrate) {
		(void)fprintf( stderr, "gpsctrl: speed change failed\n");
		status = 1;
	    } else
		gpsd_report(1, "gpsctrl: speed change on %s succeeded\n", gpsdata->gps_device);
	}
	(void)gps_close(gpsdata);
	exit(status);
    } else {
	/* access to the daemon failed, use the low-level facilities */
	static struct gps_context_t	context;	/* start it zeroed */
	struct gps_device_t	session;

	if (device == NULL) {
	    (void)fprintf(stderr,  "gpsctrl: device must be specified for low-level access.\n");
	    exit(1);
	}
	gpsd_init(&session, &context, device);
	gpsd_report(1, "gpsctrl: initialization passed.\n");
	if (gpsd_activate(&session) == -1) {
	    (void)fprintf(stderr, 
			  "gpsd: activation of device %s failed, errno=%d\n",
			  device, errno);
	    exit(2);
	}
	/* hunt for packet type and serial parameters */
	while (session.device_type == NULL) {
	    if (get_packet(&session) == ERROR_SET) {
		(void)fprintf(stderr, "gpsctrl: autodetection failed.\n");
		exit(2);
	    }
	}
	gpsd_report(1, "gpsctrl: %s looks like a %s\n",
		      device, session.device_type->typename);
	/* 
	 * If we've identified this as an NMEA device, we have to eat
	 * packets for a while to see if one of our probes elicits an
	 * ID response telling us that it's really a SiRF or
	 * something.  If so, the libgpsd(3) layer will automatically
	 * redispatch to the correct driver type.
	 */
#define REDIRECT_SNIFF	10
	/*
	 * This is the number of packets we'll look at.  Setting it
	 * lower increases the risk that we'll miss a reply to a probe.
	 * Setting it higher makes this tool slower and more annoying.
	 */
	if (strcmp(session.device_type->typename, "Generic NMEA") == 0) {
	    int dummy;
	    for (dummy = 0; dummy < REDIRECT_SNIFF; dummy++) {
		if ((get_packet(&session) & DEVICEID_SET)!=0)
		    break;
	    }
	}
	gpsd_report(1, "gpsctrl: %s identified as a %s\n",
		      device, session.device_type->typename);
	/* now perform the actual control function */
	status = 0;
	if (to_nmea || to_binary) {
	    if (session.device_type->mode_switcher == NULL) {
		(void)fprintf(stderr, 
			  "gpsctrl: %s devices have no mode switch.\n",
			  session.device_type->typename);
		status = 1;
	    }
	    else if (to_nmea) {
		if (session.gpsdata.driver_mode == 0)
		    (void)fprintf(stderr, "gpsctrl: already in NMEA mode.\n");
		else {
		    session.device_type->mode_switcher(&session, 0);
		    if (session.gpsdata.driver_mode != 0) {
			(void)fprintf(stderr, "gpsctrl: mode change failed\n");
			status = 1;
		    }
		}
	    }
	    else if (to_binary) {
		if (session.gpsdata.driver_mode == 1)
		    (void)fprintf(stderr, "gpsctrl: already in native mode.\n");
		else {
		    session.device_type->mode_switcher(&session, 1);
		    if (session.gpsdata.driver_mode != 1) {
			(void)fprintf(stderr, "gpsctrl: mode change failed\n");
			status = 1;
		    }
		}
	    }
	}
	if (speed) {
	    if (session.device_type->speed_switcher == NULL) {
		(void)fprintf(stderr, 
			      "gpsctrl: %s devices have no speed switch.\n",
			      session.device_type->typename);
		status = 1;
	    }
	    else if (!session.device_type->speed_switcher(&session, 
							  (speed_t)atoi(speed))) {
		(void)fprintf(stderr, "gpsctrl: mode change failed\n");
		status = 1;
	    }
	}

	gpsd_wrap(&session);
	exit(status);
    }
}
