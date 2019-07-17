#   Copyright 2019 Comcast Cable Communications Management, LLC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

set(PROJ_WRPC parodus2metrics)

file(GLOB HEADERS metrics.h)
set(SOURCES metrics.c trie/trie.c)

add_library(${PROJ_WRPC} STATIC ${HEADERS} ${SOURCES})
add_library(${PROJ_WRPC}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJ_WRPC}.shared PROPERTIES OUTPUT_NAME ${PROJ_WRPC})
set_property(TARGET ${PROJ_WRPC} PROPERTY C_STANDARD 99)
set_property(TARGET ${PROJ_WRPC}.shared PROPERTY C_STANDARD 99)

install (TARGETS ${PROJ_WRPC} DESTINATION lib${LIB_SUFFIX})
install (TARGETS ${PROJ_WRPC}.shared DESTINATION lib${LIB_SUFFIX})
install (FILES metrics.h DESTINATION include/${PROJ_WRPC})