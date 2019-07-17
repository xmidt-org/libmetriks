/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#include <stdbool.h>

#include <unistd.h>

#include "../src/metrics.h"

void test_counter( void )
{
    struct metrics_config c;
    metrics_t *m;

    memset( &c, 0, sizeof(c) );
    c.base = "simple";
    c.report_period_s = 1;

    m = metrics_init( &c );

    metrics_counter_inc( m, "webpa_process_restart", 1 );
    metrics_counter_inc( m, "webpa_messages_sent", 1 );
    metrics_counter_inc( m, "webpa_process_restart", 1 );
    metrics_counter_inc( m, "webpa_process_restart", 1 );
    metrics_counter_inc( m, "webpa_messages_sent", 1 );
    metrics_counter_inc_labels( m, "webpa_messages_sent", 1, 1, "dest", "wes" );
    metrics_counter_inc_labels( m, "webpa_messages_sent", 1, 1, "dest", "fred" );
    metrics_counter_inc_labels( m, "webpa_messages_sent", 1, 2, "method", "post", "status", "200" );
    metrics_counter_inc_labels( m, "webpa_messages_sent", 1, 2, "method", "post", "status", "400" );
    metrics_counter_inc_labels( m, "webpa_messages_sent", 1, 2, "method", "get", "status", "403" );
    metrics_counter_inc_labels( m, "webpa_messages_sent", 1, 2, "method", "GET", "status", "403" );
    metrics_gauge_set_labels( m, "webpa_gauge", -1023, 1, "dest", "fred" );
    metrics_gauge_set( m, "webpa_cpu", -1023 );
    metrics_gauge_set_labels( m, "cat_dog", -1023, 1, "dest", "fred" );
    metrics_gauge_set_labels( m, "webpa_gauge_3", -1023, 1, "dest", "fred" );
    metrics_gauge_set_labels( m, "webpa_gauge_alfa", -1023, 1, "dest", "fred" );
    metrics_gauge_set_labels( m, "cat_door", -1023, 1, "dest", "fred" );
    metrics_gauge_set_labels( m, "simon", -1023, 1, "dest", "fred" );
    metrics_gauge_set_labels( m, "wtf", -1023, 1, "dest", "fred" );
    metrics_gauge_set_labels( m, "webpa_gauge", 1023, 1, "dest", "fred" );
    //metrics_report( m );
    sleep( 15 );
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "metrics tests", NULL, NULL );
    CU_add_test( *suite, "Test counter", test_counter );
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();
    }

    if( 0 != rv ) {
        return 1;
    }

    return 0;
}
