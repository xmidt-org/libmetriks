/*
 * Copyright 2019 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __METRICS_H__
#define __METRICS_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
struct metrics_config {
    /* The name to prefix all the metrics with. */
    const char *base;

    /* The initial report size in bytes.  0 means use default: 1024 */
    size_t initial_report_size;

    /* The unix time the process starts. */
    time_t unix_time;

    /*------------------------------------------------------------------------*/

    const char *process_name;

    const char *metrics_path;

    /** Only used if ENABLE_PROCFS flag is enabled */
    /* The reporting period in seconds.  0 means use the default: 15s */
    uint32_t report_period_s;
};

typedef void* metrics_t;

/*----------------------------------------------------------------------------*/
/*                               Common Functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  This function initializes the metrics service and returns the object
 *  to reference calls against.
 *
 *  @param c - The structure defining the configuration.
 *
 *  @returns the metrics object to reference
 */
metrics_t metrics_init( const struct metrics_config *c );

/**
 *  Shuts down the metrics service.
 *
 *  @param the metrics object to reference
 */
void metrics_shutdown( metrics_t m );

/**
 *  Used to calculate the metric name based on the associated labels.
 *
 *  This is useful if your code updates metrics very quickly and has several
 *  labels associated with the name because it will greatly speed up the lookup
 *  process for your metric.
 *
 *  @note You will need to free the response if it is not NULL.
 *
 *  @param name        - The base metric name to build onto.
 *  @param label_count - The number of label/value pairs the function expects.
 *                       Example: 1 is the pair of "label", "value".
 *  @param ...         - const char *label, const char *value pairs
 *
 *  @return The complete metric name that can be used for fastest lookup.
 *          The caller is responsible for freeing the returned pointer.
 */
char* metrics_calculate_name( const char *name, size_t label_count, ... );

/**
 *  The varidac version of the metrics_calculate_name() function.
 */
char* metrics_calculate_name_varidac( const char *name, size_t label_count,
                                      va_list args );


/*----------------------------------------------------------------------------*/
/*                              Counter Functions                             */
/*----------------------------------------------------------------------------*/

/*
 *  Counters are designed to monotonically increase forever.  By design, they
 *  never decrease.  Internally they are backed by uint64_t values.  If for
 *  some reason they should overflow, the counter shall be capped to
 *  0xffffffffffffffff.
 *
 *  Examples of counters
 *  ------------------
 *
 *  1. Number of bytes allocated (cumulitive)
 *  2. Number of bytes freed     (cumulitive)
 *  3. TX byte count
 *  4. Number of times a failure has been hit
 *  5. Number of times a success value has been hit
 */

/**
 *  This function increments a metrics counter a specified amount.
 *
 *  @note If you need labels, and have already called metrics_calculate_name(),
 *        you can simply call this function with the name you got back.
 *
 *  @param m    - The metric object to reference.
 *  @param name - The base metric name to increment.
 *  @param inc  - The quantity to increment by.
 */
void metrics_counter_inc(metrics_t m, const char *name, uint32_t inc );

/**
 *  This function increments a metrics counter a specified amount.
 *
 *  @note If you are performing repeated metric updates to the same metric,
 *        this is slower then using metrics_calculate_name() and locally
 *        caching the name.
 *
 *  @param m           - The metric object to reference.
 *  @param name        - The metric name to increment.
 *  @param inc         - The quantity to increment by.
 *  @param label_count - The number of label pairs to associate with this metric.
 *  @param ...         - Label, value pair to associate with the metric.
 */
void metrics_counter_inc_labels(metrics_t m, const char *name, uint32_t inc,
                                size_t label_count, ... );

/*----------------------------------------------------------------------------*/
/*                               Gauge Functions                              */
/*----------------------------------------------------------------------------*/

/*
 *  Gauge functions are designed to track some real value.  This metric may
 *  go up or down.  Internally these are backed by int64_t values.
 *
 *  Examples of gauges
 *  ------------------
 *
 *  1. Temperature in 1000's of degrees C 10.0C = 10000
 *  2. Real time queue depth.
 *  3. Start time of a process in unix time.
 *  4. Number of outstanding allocated bytes.
 *
 */

/**
 *  This function sets a metrics gauge a specified value.
 *
 *  @note If you need labels, and have already called metrics_calculate_name(),
 *        you can simply call this function with the name you got back.
 *
 *  @param m    - The metric object to reference.
 *  @param name  - The metric name to update.
 *  @param value - The value to set the gauge to.
 */
void metrics_gauge_set( metrics_t m, const char *name, int64_t value );

/**
 *  This function updates a metrics gauge to a specified value.
 *
 *  @note If you are performing repeated metric updates to the same metric,
 *        this is slower then using metrics_calculate_name() and locally
 *        caching the name.
 *
 *  @param m           - The metric object to reference.
 *  @param name        - The base metric name to update.
 *  @param value       - The value to set the gauge to.
 *  @param label_count - The number of label pairs to associate with this metric.
 *  @param ...         - Label, value pair to associate with the metric.
 */
void metrics_gauge_set_labels( metrics_t m, const char *name, int64_t value,
                               size_t label_count, ... );

/*----------------------------------------------------------------------------*/
/*                            Histogram Functions                             */
/*----------------------------------------------------------------------------*/
/* TBD */

/*----------------------------------------------------------------------------*/
/*                             Summary Functions                              */
/*----------------------------------------------------------------------------*/
/* TBD */

#endif
