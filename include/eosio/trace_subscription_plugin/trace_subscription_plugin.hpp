#pragma once
#include <appbase/application.hpp>

namespace eosio {
	using namespace appbase;

	class trace_subscription_plugin : public appbase::plugin<trace_subscription_plugin> {
	public:
		trace_subscription_plugin();
		virtual ~trace_subscription_plugin();
	 
		APPBASE_PLUGIN_REQUIRES()
		virtual void set_program_options(options_description&, options_description& cfg) override;
	 
		void plugin_initialize(const variables_map& options);
		void plugin_startup();
		void plugin_shutdown();

	private:
		class trace_subscription_plugin_impl* my;
	};
}
