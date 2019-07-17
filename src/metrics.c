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

#include "metrics.h"
#include "trie/trie.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DEFAULT_METRICS_PATH            "/tmp/metrics"
#define DEFAULT_PROCESS_NAME            "example.metrics"

#define DEFAULT_REPORT_PERIOD           900
#define DEFAULT_REPORT_SIZE             1024
#define MAX_LINE_LENGTH_BEFORE_REALLOC  128
#define BUFFER_SIZE_INCREASE            1024

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    MT_COUNTER,
    MT_GAUGE
} metric_type_t;

typedef struct {
    const struct metrics_config *c;

    pthread_mutex_t mutex;

    pthread_t report_thread;
    volatile int keep_running;

    struct trie *counters;
    struct trie *gauges;

    char *label__report_buffer;
} __metrics_t;

struct trie_visitor {
    __metrics_t *m;
    metric_type_t type;
    char *buf;
    size_t len;
    size_t used;
};

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void* __report_loop( void* );
static int __visitor( const char*, void*, void* );

void __generate_report( metrics_t, char**, size_t* );
static void __unsafe_gauge_set( __metrics_t*, const char*, int64_t );
static void __unsafe_counter_inc( __metrics_t*, const char*, uint32_t );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/* See metrics.h for details. */
metrics_t metrics_init( const struct metrics_config *c )
{
    int rv;
    __metrics_t *m;

    m = (__metrics_t*) malloc( sizeof(__metrics_t) );
    m->c = c;

    pthread_mutex_init( &m->mutex, NULL );

    m->counters = trie_create();
    m->gauges = trie_create();

    m->label__report_buffer = metrics_calculate_name( "metrics_report_buffer",
                                                      1, "size", "current" );

    __unsafe_gauge_set( (metrics_t) m, "metrics_boot_time",
                        (uint64_t) c->unix_time );
    m->keep_running = 1;
    rv = pthread_create( &m->report_thread, NULL, __report_loop, m );
    if( 0 != rv ) {
        m = NULL;
    }

    return (metrics_t) m;
}

/* See metrics.h for details. */
void metrics_shutdown( metrics_t __m )
{
    __metrics_t *m = (__metrics_t*) __m;

    if( NULL != m ) {
        m->keep_running = 0;
        pthread_join( m->report_thread, NULL );
        trie_free( m->counters );
        trie_free( m->gauges );
        free( m->label__report_buffer );

        pthread_mutex_lock( &m->mutex );
        pthread_mutex_unlock( &m->mutex );
        pthread_mutex_destroy( &m->mutex );

        free( m );
    }
}

/* See metrics.h for details. */
char* metrics_calculate_name_varidac( const char *name, size_t label_count,
                                      va_list args )
{
    size_t i;
    size_t bytes = 0;
    va_list va_copy;
    char _buf[1024];
    char *buf = &_buf[0];
    char *rv;

    va_copy( va_copy, args );
   
    bytes += strlen( name );
    for( i = 0; i < label_count; i++ ) {
        bytes += strlen( va_arg(va_copy, const char*) );
        bytes += strlen( va_arg(va_copy, const char*) );
    }

    /* TODO: correct the character escaping */

    /* Output format:
     * name{label="value",label2="value"}
     *
     * A bit slopy, but simple way to calulate is the sum of:
     * 1. The lenth of the strings
     * 2. The {} characters = 2
     * 3. The ="", per label pair is 4 - the extra comma accounts for the
     *    trailing '\0' needed. */
    bytes += 2 + label_count * 4;

    if( sizeof(_buf)/sizeof(char) < bytes ) {
        buf = (char*) malloc( bytes * sizeof(char) );
    }

    strcpy( buf, name );
    if( 0 < label_count ) {
        strcat( buf, "{" );

        for( i = 0; i < label_count; i++ ) {
            if( 0 != i ) {
                strcat( buf, "," );
            }
            strcat( buf, va_arg(args, const char*) );
            strcat( buf, "=\"" );
            strcat( buf, va_arg(args, const char*) );
            strcat( buf, "\"" );
        }
        strcat( buf, "}" );
    }

    rv = strdup( buf );

    if( buf != _buf ) {
        free( buf );
    }

    return rv;
}

/* See metrics.h for details. */
char* metrics_calculate_name( const char *name, size_t label_count, ... )
{
    char *rv;
    va_list args;

    va_start( args, label_count );
    rv = metrics_calculate_name_varidac( name, label_count, args );
    va_end( args );

    return rv;
}


/* See metrics.h for details. */
void metrics_counter_inc( metrics_t __m, const char *name, uint32_t inc )
{
    __metrics_t *m = (__metrics_t*) __m;
    uint64_t *counter;

    counter = (uint64_t*) trie_search( m->counters, name );
    if( NULL != counter ) {
        pthread_mutex_lock( &m->mutex );
        *counter += inc;
        pthread_mutex_unlock( &m->mutex );
        return;
    }

    /* Try again while locking. */
    pthread_mutex_lock( &m->mutex );
    __unsafe_counter_inc( m, name, inc );
    pthread_mutex_unlock( &m->mutex );
}

/* See metrics.h for details. */
void metrics_counter_inc_labels( metrics_t __m, const char *name, uint32_t inc,
                                 size_t label_count, ... )
{
    char *full;
    va_list args;

    va_start( args, label_count );
    full = metrics_calculate_name_varidac( name, label_count, args );
    va_end( args );

    if( NULL == full ) {
        /* Not sure what happened, but we're toast... bail. */
        return;
    }

    metrics_counter_inc( __m, full, inc );

    free( full );
}

/* See metrics.h for details. */
void metrics_gauge_set( metrics_t __m, const char *name, int64_t value )
{
    __metrics_t *m = (__metrics_t*) __m;
    int64_t *gauge;

    gauge = (int64_t*) trie_search( m->gauges, name );
    if( NULL != gauge ) {
        pthread_mutex_lock( &m->mutex );
        *gauge = value;
        pthread_mutex_unlock( &m->mutex );
        return;
    }

    /* Try again while locking. */
    pthread_mutex_lock( &m->mutex );
    __unsafe_gauge_set( m, name, value );
    pthread_mutex_unlock( &m->mutex );
}

/* See metrics.h for details. */
void metrics_gauge_set_labels( metrics_t __m, const char *name, int64_t value,
                               size_t label_count, ... )
{
    char *full;
    va_list args;

    va_start( args, label_count );
    full = metrics_calculate_name_varidac( name, label_count, args );
    va_end( args );

    if( NULL == full ) {
        /* Not sure what happened, but we're toast... bail. */
        return;
    }

    metrics_gauge_set( __m, full, value );

    free( full );
}

/* See metrics.h for details. */
void metrics_event_record( metrics_t __m, const uint64_t event )
{
    __metrics_t *m = (__metrics_t*) __m;
    (void) m; (void) event;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __unsafe_gauge_set( __metrics_t* m, const char *name, int64_t value )
{
    int64_t *gauge;

    gauge = (int64_t*) trie_search( m->gauges, name );
    if( NULL == gauge ) {
        gauge = (int64_t*) malloc( sizeof(int64_t) );
        *gauge = value;
        trie_insert(m->gauges, name, gauge);
    }

    *gauge = value;
}

static void __unsafe_counter_inc( __metrics_t* m, const char *name, uint32_t inc )
{
    uint64_t *counter;

    counter = (uint64_t*) trie_search( m->counters, name );
    if( NULL == counter ) {
        counter = (uint64_t*) malloc( sizeof(uint64_t) );
        *counter = 0;
        trie_insert(m->counters, name, counter);
    }

    *counter += inc;

}

static uint32_t __get_report_period( __metrics_t *m )
{
    if( 0 < m->c->report_period_s ) {
        return m->c->report_period_s;
    }

    return DEFAULT_REPORT_PERIOD;
}

static void __mkdir( __metrics_t *m )
{
    const char *path = m->c->metrics_path;

    if( NULL == path ) {
        path = DEFAULT_METRICS_PATH;
    }

    mkdir( path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
}

static char* __get_filename( __metrics_t *m )
{
    const char *path, *name;
    char *rv;
    size_t len;

    path = m->c->metrics_path;
    name = m->c->process_name;

    if( NULL == path ) {
        path = DEFAULT_METRICS_PATH;
    }

    if( NULL == name ) {
        name = DEFAULT_PROCESS_NAME;
    }

    len = strlen( path ) + 1 + strlen( name ) + 1;
    rv = (char*) malloc( len * sizeof(char) );
    sprintf( rv, "%s/%s", path, name );

    return rv;
}

static void* __report_loop( void *__m )
{
    __metrics_t *m = (__metrics_t*) __m;
    char *buf;
    size_t len;
    char *filename;

    len = DEFAULT_REPORT_SIZE;
    if( 0 < m->c->initial_report_size ) {
        len = m->c->initial_report_size;
    }
    buf = (char*) malloc( len * sizeof(char) );

    /* Record the report buffer size as metrics */
    metrics_gauge_set_labels( (metrics_t) m, "metrics_report_buffer",
                              len, 1, "size", "default" );
    metrics_gauge_set( (metrics_t) m, m->label__report_buffer, len );

    __mkdir( m );
    filename = __get_filename( m );

    while( 0 != m->keep_running ) {
        int fd;

        sleep( __get_report_period(m) );
        __generate_report( m, &buf, &len );

        fd = open( filename, O_WRONLY | O_CREAT | O_TRUNC, 
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );

        if( -1 != fd ) {
            write( fd, buf, len );
            close( fd );
        }
    }

    free( buf );
    free( filename );

    return NULL;
}

void __generate_report( metrics_t __m, char **buf, size_t *len )
{
    struct trie_visitor d;
    __metrics_t *m = (__metrics_t*) __m;

    (void) buf;
    (void) len;

    __unsafe_counter_inc( (metrics_t) m, "metrics_report_count", 1 );

    d.m = m;
    d.buf = *buf;
    d.len = *len;
    d.used = 0;

    pthread_mutex_lock( &m->mutex );

    d.type = MT_COUNTER;
    trie_visit( m->counters, "", __visitor, &d );

    d.type = MT_GAUGE;
    trie_visit( m->gauges, "", __visitor, &d );

    pthread_mutex_unlock( &m->mutex );

    *buf = d.buf;
    *len = d.len;
}

static int __visitor( const char *key, void *data, void *arg )
{
    struct trie_visitor *tv = (struct trie_visitor*) arg;
    char *p;
    size_t left;
    int written;

    if( tv->len <= (tv->used + MAX_LINE_LENGTH_BEFORE_REALLOC) ) {

        tv->len += BUFFER_SIZE_INCREASE;
        
        tv->buf = (char*) realloc( tv->buf, tv->len * sizeof(char) );
        __unsafe_gauge_set( (metrics_t) tv->m, tv->m->label__report_buffer,
                            tv->len );
    }

    left = tv->len - tv->used - 1; // -1 for ensuring space for the trailing '\0'
    p = &tv->buf[tv->used];

    written = 0;
    switch( tv->type ) {
        case MT_COUNTER:
            written = snprintf( p, left, "%s_%s %"PRIu64"\n", tv->m->c->base,
                                key, *((uint64_t*) data) );
            break;
        case MT_GAUGE:
            written = snprintf( p, left, "%s_%s %"PRId64"\n", tv->m->c->base,
                                key, *((int64_t*) data) );
            break;
        default:
            break;
    }

    p[written] = '\0';
    tv->used += written;

    return 0;
}


