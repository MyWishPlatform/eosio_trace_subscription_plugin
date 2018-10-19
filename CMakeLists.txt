file(GLOB HEADERS "include/eosio/trace_subscription_plugin/*.hpp")
add_library( trace_subscription_plugin
             trace_subscription_plugin.cpp
             ${HEADERS} )

target_link_libraries( trace_subscription_plugin chain_plugin tcp_plugin appbase fc )
target_include_directories( trace_subscription_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
