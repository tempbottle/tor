/* Copyright (c) 2010, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file status.c
 * \brief Keep status information and log the heartbeat messages.
 **/

#include "or.h"
#include "config.h"
#include "status.h"
#include "nodelist.h"
#include "router.h"
#include "circuitlist.h"
#include "main.h"

/** Returns the number of open circuits. */
static int
count_circuits(void)
{
  circuit_t *circ;
  int nr=0;

  for (circ = _circuit_get_global_list(); circ; circ = circ->next)
    nr++;

  return nr;
}

/* Takes seconds <b>secs</b> and returns a human-readable uptime string */
static char *
secs_to_uptime(long secs)
{
  long int days = secs / 86400;
  int hours = (secs - (days * 86400)) / 3600;
  int minutes = (secs - (days * 86400) - (hours * 3600)) / 60;
  char *uptime_string = NULL;

  switch (days) {
  case 0:
    tor_asprintf(&uptime_string, "%d:%02d", hours, minutes);
    break;
  case 1:
    tor_asprintf(&uptime_string, "%ld day %d:%02d", days, hours, minutes);
    break;
  default:
    tor_asprintf(&uptime_string, "%ld days %d:%02d", days, hours, minutes);
    break;
  }

  return uptime_string;
}

/* Takes <b>bytes</b> and returns a human-readable bandwidth string. */
static char *
bytes_to_bandwidth(uint64_t bytes)
{
  char *bw_string = NULL;

  if (bytes < (1<<20)) /* Less than a megabyte. */
    tor_asprintf(&bw_string, U64_FORMAT" kB", U64_PRINTF_ARG(bytes>>10));
  else if (bytes < (1<<30)) { /* Megabytes. Let's add some precision. */
    double bw = U64_TO_DBL(bytes);
    tor_asprintf(&bw_string, "%.2f MB", bw/(1<<20));
  }  else { /* Gigabytes. */
    double bw = U64_TO_DBL(bytes);
    tor_asprintf(&bw_string, "%.2f GB", bw/(1<<30));
  }

  return bw_string;
}

/* This function provides the heartbeat log message */
int
log_heartbeat(time_t now)
{
  uint64_t in,out;
  char *bw_sent = NULL;
  char *bw_rcvd = NULL;
  char *uptime = NULL;
  const routerinfo_t *me;
  const node_t *myself;

  or_options_t *options = get_options();
  int is_server = server_mode(options);

  if (is_server) {
    /* Let's check if we are in the current cached consensus. */
    if (!(me = router_get_my_routerinfo()))
      return -1; /* Something stinks, we won't even attempt this. */
    else
      if (!(myself = node_get_by_id(me->cache_info.identity_digest)))
        log_fn(LOG_NOTICE, LD_HEARTBEAT, "Heartbeat: It seems like we are not "
               "in the cached consensus.");
  }

  get_traffic_stats(&in, &out);
  uptime = secs_to_uptime(get_uptime());
  bw_sent = bytes_to_bandwidth(out);
  bw_rcvd = bytes_to_bandwidth(in);

  log_fn(LOG_NOTICE, LD_HEARTBEAT, "Heartbeat: Tor's uptime is %s, with %d "
         "circuits open, I've pushed %s and received %s.",
         uptime, count_circuits(),bw_sent,bw_rcvd);

  tor_free(uptime);
  tor_free(bw_sent);
  tor_free(bw_rcvd);

  return 0;
}

